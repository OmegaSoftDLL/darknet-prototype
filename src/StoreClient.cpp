#include "StoreClient.h"
#include "HttpClient.h"
#include <thread>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <nlohmann/json.hpp>   // vendored em third_party/nlohmann/json.hpp

// ── Parsing JSON (nlohmann/json) ─────────────────────────────────────────────
// Mesmos defaults do parser manual anterior: chave ausente ou com tipo
// inesperado vira "" / 0. Respostas invalidas (parse falho) viram um json
// "discarded" sem chaves — o efeito e o mesmo de nao achar as chaves no corpo.
namespace {

nlohmann::json jParse(const std::string& body) {
    return nlohmann::json::parse(body, nullptr, false);
}

std::string jStr(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
}

double jNum(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    return (it != j.end() && it->is_number()) ? it->get<double>() : 0.0;
}

// Extrai um array de strings (ex.: "inventory") — ignora elementos nao-string.
std::vector<std::string> jStrArray(const nlohmann::json& j, const char* key) {
    std::vector<std::string> out;
    auto it = j.find(key);
    if (it != j.end() && it->is_array())
        for (const auto& e : *it) if (e.is_string()) out.push_back(e.get<std::string>());
    return out;
}

} // namespace

// ── RAII helpers ─────────────────────────────────────────────────────────────

// Garante que busy_ volte a false mesmo se a lambda lançar exceção.
struct BusyGuard {
    std::atomic<bool>* flag;
    explicit BusyGuard(std::atomic<bool>* f) : flag(f) {}
    ~BusyGuard() { if (flag) flag->store(false); }
};

// ─────────────────────────────────────────────────────────────────────────────

void StoreClient::startThread(std::thread&& t) {
    std::lock_guard<std::mutex> lk(mtx_);
    // Tenta dar join em threads que já terminaram para não acumular handles.
    for (auto& th : threads_) {
        if (th.joinable()) {
            // Não podemos verificar se terminou sem C++20; como as operações
            // são curtas (timeout 5s), fazemos join não-bloqueante não é padrão.
            // Optamos por manter join no destrutor e, aqui, apenas garantir
            // que não re-adicionamos sem necessidade. Nenhuma ação extra.
        }
    }
    threads_.emplace_back(std::move(t));
}

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
std::string StoreClient::token() const {
    std::lock_guard<std::mutex> lk(mtx_); return token_;
}

// ── login: POST /auth/login {name} -> {token,id} ─────────────────────────────
void StoreClient::loginAsync(const std::string& name) {
    std::string h = host; int p = port; std::string nm = name;
    bool tls = useTls; std::string pre = apiPrefix;
    activeThreads_.fetch_add(1);
    startThread(std::thread([this, h, p, tls, pre, nm]() {
        std::string body = nlohmann::json{{"name", nm}}.dump();
        HttpResponse r = HttpClient::post(h, p, pre + "/auth/login", body, "", tls);
        if (r.status == 200) {
            nlohmann::json j = jParse(r.body);
            std::string tok = jStr(j, "token"), id = jStr(j, "id");
            { std::lock_guard<std::mutex> lk(mtx_); token_ = tok; playerId_ = id; }
            logged_ = true;
            setMsg("Conectado a Cyber Station");
            // Logo apos o login, busca o saldo de gems / inventario.
            HttpResponse me = HttpClient::get(h, p, pre + "/me", tok, tls);
            if (me.status == 200) {
                double g = jNum(jParse(me.body), "gems");
                std::lock_guard<std::mutex> lk(mtx_); gems_ = (int)g;
            }
        } else {
            setMsg(r.status == 0 ? "Backend offline (loja premium indisponivel)"
                                 : "Falha no login da loja");
        }
        activeThreads_.fetch_sub(1);
    }));
}

// ── catálogo: GET /store -> {gemPacks:[...], items:[...]} ─────────────────────
void StoreClient::fetchStoreAsync() {
    std::string h = host; int p = port;
    bool tls = useTls; std::string pre = apiPrefix;
    activeThreads_.fetch_add(1);
    startThread(std::thread([this, h, p, tls, pre]() {
        HttpResponse r = HttpClient::get(h, p, pre + "/store", "", tls);
        if (r.status == 200) {
            std::vector<PremiumItem> its;
            std::vector<GemPack>     pks;
            nlohmann::json j = jParse(r.body);
            auto itemsArr = j.find("items");
            if (itemsArr != j.end() && itemsArr->is_array()) {
                for (const auto& o : *itemsArr) {
                    if (!o.is_object()) continue;
                    PremiumItem it;
                    it.id = jStr(o, "id"); it.name = jStr(o, "name");
                    it.type = jStr(o, "type"); it.gems = (int)jNum(o, "gems");
                    if (!it.id.empty()) its.push_back(it);
                }
            }
            auto packsArr = j.find("gemPacks");
            if (packsArr != j.end() && packsArr->is_array()) {
                for (const auto& o : *packsArr) {
                    if (!o.is_object()) continue;
                    GemPack gp;
                    gp.id = jStr(o, "id"); gp.gems = (int)jNum(o, "gems");
                    gp.priceBRL = jNum(o, "priceBRL");
                    if (!gp.id.empty()) pks.push_back(gp);
                }
            }
            { std::lock_guard<std::mutex> lk(mtx_); items_ = its; packs_ = pks; }
            setMsg("Catalogo da loja carregado");
        } else {
            setMsg(r.status == 0 ? "Backend offline" : "Falha ao carregar a loja");
        }
        activeThreads_.fetch_sub(1);
    }));
}

// ── saldo: GET /me -> {gems, inventory:[...]} ────────────────────────────────
void StoreClient::refreshAsync() {
    if (!logged_.load()) return;
    std::string h = host; int p = port; std::string tok;
    { std::lock_guard<std::mutex> lk(mtx_); tok = token_; }
    bool tls = useTls; std::string pre = apiPrefix;
    activeThreads_.fetch_add(1);
    startThread(std::thread([this, h, p, tls, pre, tok]() {
        HttpResponse r = HttpClient::get(h, p, pre + "/me", tok, tls);
        if (r.status == 200) {
            nlohmann::json j = jParse(r.body);
            double g = jNum(j, "gems");
            std::vector<std::string> inv = jStrArray(j, "inventory");
            { std::lock_guard<std::mutex> lk(mtx_); gems_ = (int)g; inventory_ = inv; }
        }
        activeThreads_.fetch_sub(1);
    }));
}

// ── compra com gems: POST /store/buy-item {itemId} (servidor valida saldo) ────
void StoreClient::buyItemAsync(const std::string& itemId) {
    if (!logged_.load()) { setMsg("Faca login na loja primeiro"); return; }
    if (busy_.exchange(true)) return;
    std::string h = host; int p = port; std::string tok, id = itemId;
    { std::lock_guard<std::mutex> lk(mtx_); tok = token_; }
    bool tls = useTls; std::string pre = apiPrefix;
    activeThreads_.fetch_add(1);
    startThread(std::thread([this, h, p, tls, pre, tok, id]() {
        BusyGuard bg(&busy_);
        std::string body = nlohmann::json{{"itemId", id}}.dump();
        HttpResponse r = HttpClient::post(h, p, pre + "/store/buy-item", body, tok, tls);
        if (r.status == 200) {
            nlohmann::json j = jParse(r.body);
            double g = jNum(j, "gems");
            std::vector<std::string> inv = jStrArray(j, "inventory");
            { std::lock_guard<std::mutex> lk(mtx_); gems_ = (int)g; if (!inv.empty()) inventory_ = inv; }
            setMsg("Item comprado!");
        } else if (r.status == 402) {
            setMsg("Gems insuficientes");
        } else {
            setMsg(r.status == 0 ? "Backend offline" : "Falha na compra");
        }
        activeThreads_.fetch_sub(1);
    }));
}

// ── comprar gems: POST /store/buy-gems {packId} -> abre Stripe Checkout ───────
void StoreClient::buyGemsAsync(const std::string& packId) {
    if (!logged_.load()) { setMsg("Faca login na loja primeiro"); return; }
    if (busy_.exchange(true)) return;
    std::string h = host; int p = port; std::string tok, id = packId;
    { std::lock_guard<std::mutex> lk(mtx_); tok = token_; }
    bool tls = useTls; std::string pre = apiPrefix;
    activeThreads_.fetch_add(1);
    startThread(std::thread([this, h, p, tls, pre, tok, id]() {
        BusyGuard bg(&busy_);
        std::string body = nlohmann::json{{"packId", id}}.dump();
        HttpResponse r = HttpClient::post(h, p, pre + "/store/buy-gems", body, tok, tls);
        if (r.status == 200) {
            std::string url = jStr(jParse(r.body), "url");
            if (!url.empty()) {
                HttpClient::openBrowser(url);
                setMsg("Abrindo pagamento seguro (Stripe)...");
            } else {
                setMsg("Configure STRIPE_SECRET_KEY no servidor p/ pagamento real");
            }
        } else {
            setMsg(r.status == 0 ? "Backend offline" : "Falha ao iniciar pagamento");
        }
        activeThreads_.fetch_sub(1);
    }));
}


StoreClient::~StoreClient() {
    // Aguarda o término de todas as threads de rede antes de destruir membros
    // compartilhados (mutex/strings) — evita use-after-free no encerramento.
    std::vector<std::thread> toJoin;
    { std::lock_guard<std::mutex> lk(mtx_); toJoin.swap(threads_); }
    for (auto& th : toJoin) if (th.joinable()) th.join();
}