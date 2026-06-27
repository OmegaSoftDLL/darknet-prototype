#include "StoreClient.h"
#include "HttpClient.h"
#include <thread>
#include <cstdlib>
#include <cstring>

// ── Parsing JSON minimalista (suficiente para as respostas do backend) ───────
namespace {

bool jStr(const std::string& s, const char* key, std::string& out) {
    std::string k = std::string("\"") + key + "\"";
    size_t p = s.find(k);
    if (p == std::string::npos) return false;
    p = s.find(':', p + k.size());
    if (p == std::string::npos) return false;
    p = s.find('"', p);
    if (p == std::string::npos) return false;
    p++; out.clear();
    while (p < s.size() && s[p] != '"') {
        if (s[p] == '\\' && p + 1 < s.size()) { out.push_back(s[p + 1]); p += 2; }
        else { out.push_back(s[p]); p++; }
    }
    return true;
}

bool jNum(const std::string& s, const char* key, double& out) {
    std::string k = std::string("\"") + key + "\"";
    size_t p = s.find(k);
    if (p == std::string::npos) return false;
    p = s.find(':', p + k.size());
    if (p == std::string::npos) return false;
    p++;
    while (p < s.size() && (s[p] == ' ' || s[p] == '\t')) p++;
    char* end = nullptr;
    out = std::strtod(s.c_str() + p, &end);
    return end != s.c_str() + p;
}

// Extrai o conteúdo do array "key":[ ... ] (entre colchetes balanceados).
bool jArray(const std::string& s, const char* key, std::string& out) {
    std::string k = std::string("\"") + key + "\"";
    size_t p = s.find(k);
    if (p == std::string::npos) return false;
    p = s.find('[', p);
    if (p == std::string::npos) return false;
    int depth = 0; size_t start = p;
    for (; p < s.size(); ++p) {
        if (s[p] == '[') depth++;
        else if (s[p] == ']') { depth--; if (depth == 0) { out = s.substr(start, p - start + 1); return true; } }
    }
    return false;
}

// Quebra um array "[ {..}, {..} ]" em objetos "{..}" individuais.
std::vector<std::string> splitObjects(const std::string& arr) {
    std::vector<std::string> objs;
    int depth = 0; size_t start = std::string::npos;
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i] == '{') { if (depth == 0) start = i; depth++; }
        else if (arr[i] == '}') { depth--; if (depth == 0 && start != std::string::npos) { objs.push_back(arr.substr(start, i - start + 1)); start = std::string::npos; } }
    }
    return objs;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────

void StoreClient::setMsg(const std::string& m) {
    std::lock_guard<std::mutex> lk(mtx_);
    msg_ = m;
}

int StoreClient::gems() const {
    std::lock_guard<std::mutex> lk(mtx_); return gems_;
}
std::vector<PremiumItem> StoreClient::items() const {
    std::lock_guard<std::mutex> lk(mtx_); return items_;
}
std::vector<GemPack> StoreClient::packs() const {
    std::lock_guard<std::mutex> lk(mtx_); return packs_;
}
std::string StoreClient::lastMessage() const {
    std::lock_guard<std::mutex> lk(mtx_); return msg_;
}
bool StoreClient::ownsItem(const std::string& id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    for (const auto& i : inventory_) if (i == id) return true;
    return false;
}

// ── login: POST /auth/login {name} -> {token,id} ─────────────────────────────
void StoreClient::loginAsync(const std::string& name) {
    std::string h = host; int p = port; std::string nm = name;
    std::thread([this, h, p, nm]() {
        std::string body = std::string("{\"name\":\"") + nm + "\"}";
        HttpResponse r = HttpClient::post(h, p, "/auth/login", body);
        if (r.status == 200) {
            std::string tok, id;
            jStr(r.body, "token", tok);
            jStr(r.body, "id", id);
            { std::lock_guard<std::mutex> lk(mtx_); token_ = tok; playerId_ = id; }
            logged_ = true;
            setMsg("Conectado a Cyber Station");
            // Logo apos o login, busca o saldo de gems / inventario.
            HttpResponse me = HttpClient::get(h, p, "/me", tok);
            if (me.status == 200) {
                double g = 0; jNum(me.body, "gems", g);
                std::lock_guard<std::mutex> lk(mtx_); gems_ = (int)g;
            }
        } else {
            setMsg(r.status == 0 ? "Backend offline (loja premium indisponivel)"
                                 : "Falha no login da loja");
        }
    }).detach();
}

// ── catálogo: GET /store -> {gemPacks:[...], items:[...]} ─────────────────────
void StoreClient::fetchStoreAsync() {
    std::string h = host; int p = port;
    std::thread([this, h, p]() {
        HttpResponse r = HttpClient::get(h, p, "/store");
        if (r.status == 200) {
            std::vector<PremiumItem> its;
            std::vector<GemPack>     pks;
            std::string arr;
            if (jArray(r.body, "items", arr)) {
                for (auto& o : splitObjects(arr)) {
                    PremiumItem it; double g = 0;
                    jStr(o, "id", it.id); jStr(o, "name", it.name);
                    jStr(o, "type", it.type); jNum(o, "gems", g); it.gems = (int)g;
                    if (!it.id.empty()) its.push_back(it);
                }
            }
            if (jArray(r.body, "gemPacks", arr)) {
                for (auto& o : splitObjects(arr)) {
                    GemPack gp; double g = 0, pr = 0;
                    jStr(o, "id", gp.id); jNum(o, "gems", g); jNum(o, "priceBRL", pr);
                    gp.gems = (int)g; gp.priceBRL = pr;
                    if (!gp.id.empty()) pks.push_back(gp);
                }
            }
            { std::lock_guard<std::mutex> lk(mtx_); items_ = its; packs_ = pks; }
            setMsg("Catalogo da loja carregado");
        } else {
            setMsg(r.status == 0 ? "Backend offline" : "Falha ao carregar a loja");
        }
    }).detach();
}

// ── saldo: GET /me -> {gems, inventory:[...]} ────────────────────────────────
void StoreClient::refreshAsync() {
    if (!logged_.load()) return;
    std::string h = host; int p = port; std::string tok;
    { std::lock_guard<std::mutex> lk(mtx_); tok = token_; }
    std::thread([this, h, p, tok]() {
        HttpResponse r = HttpClient::get(h, p, "/me", tok);
        if (r.status == 200) {
            double g = 0; jNum(r.body, "gems", g);
            std::vector<std::string> inv;
            std::string arr;
            if (jArray(r.body, "inventory", arr)) {
                // inventory é um array de strings — extrai cada "..."
                size_t i = 0;
                while ((i = arr.find('"', i)) != std::string::npos) {
                    size_t j = arr.find('"', i + 1);
                    if (j == std::string::npos) break;
                    inv.push_back(arr.substr(i + 1, j - i - 1));
                    i = j + 1;
                }
            }
            { std::lock_guard<std::mutex> lk(mtx_); gems_ = (int)g; inventory_ = inv; }
        }
    }).detach();
}

// ── compra com gems: POST /store/buy-item {itemId} (servidor valida saldo) ────
void StoreClient::buyItemAsync(const std::string& itemId) {
    if (!logged_.load()) { setMsg("Faca login na loja primeiro"); return; }
    if (busy_.exchange(true)) return;
    std::string h = host; int p = port; std::string tok, id = itemId;
    { std::lock_guard<std::mutex> lk(mtx_); tok = token_; }
    std::thread([this, h, p, tok, id]() {
        std::string body = std::string("{\"itemId\":\"") + id + "\"}";
        HttpResponse r = HttpClient::post(h, p, "/store/buy-item", body, tok);
        if (r.status == 200) {
            double g = 0; jNum(r.body, "gems", g);
            std::vector<std::string> inv; std::string arr;
            if (jArray(r.body, "inventory", arr)) {
                size_t i = 0;
                while ((i = arr.find('"', i)) != std::string::npos) {
                    size_t j = arr.find('"', i + 1);
                    if (j == std::string::npos) break;
                    inv.push_back(arr.substr(i + 1, j - i - 1));
                    i = j + 1;
                }
            }
            { std::lock_guard<std::mutex> lk(mtx_); gems_ = (int)g; if (!inv.empty()) inventory_ = inv; }
            setMsg("Item comprado!");
        } else if (r.status == 402) {
            setMsg("Gems insuficientes");
        } else {
            setMsg(r.status == 0 ? "Backend offline" : "Falha na compra");
        }
        busy_ = false;
    }).detach();
}

// ── comprar gems: POST /store/buy-gems {packId} -> abre Stripe Checkout ───────
void StoreClient::buyGemsAsync(const std::string& packId) {
    if (!logged_.load()) { setMsg("Faca login na loja primeiro"); return; }
    if (busy_.exchange(true)) return;
    std::string h = host; int p = port; std::string tok, id = packId;
    { std::lock_guard<std::mutex> lk(mtx_); tok = token_; }
    std::thread([this, h, p, tok, id]() {
        std::string body = std::string("{\"packId\":\"") + id + "\"}";
        HttpResponse r = HttpClient::post(h, p, "/store/buy-gems", body, tok);
        if (r.status == 200) {
            std::string url;
            if (jStr(r.body, "url", url) && !url.empty()) {
                HttpClient::openBrowser(url);
                setMsg("Abrindo pagamento seguro (Stripe)...");
            } else {
                setMsg("Configure STRIPE_SECRET_KEY no servidor p/ pagamento real");
            }
        } else {
            setMsg(r.status == 0 ? "Backend offline" : "Falha ao iniciar pagamento");
        }
        busy_ = false;
    }).detach();
}
