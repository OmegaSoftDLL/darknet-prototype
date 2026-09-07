#include "StoreClient.h"
#include "HttpClient.h"
#include <thread>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <nlohmann/json.hpp>   // vendored in third_party/nlohmann/json.hpp

// ── Parsing JSON (nlohmann/json) ─────────────────────────────────────────────
// Same defaults of the parser manual previous: chave ausente ou with type
// inesperado vira "" / 0. Respostas invalidas (parse falho) viram um json
// "discarded" without chaves — the effect and the same of not find the chaves in the body.
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

// Extrai um array of strings (ex.: "inventory") — ignora elementos not-string.
std::vector<std::string> jStrArray(const nlohmann::json& j, const char* key) {
    std::vector<std::string> out;
    auto it = j.find(key);
    if (it != j.end() && it->is_array())
        for (const auto& and : *it) if (and.is_string()) out.push_back(and.get<std::string>());
    return out;
}

} // namespace

// ── RAII helpers ─────────────────────────────────────────────────────────────

// Ensures that busy_ volte the false same if the lambda throw exception.
struct BusyGuard {
    std::atomic<bool>* flag;
    explicit BusyGuard(std::atomic<bool>* f) : flag(f) {}
    ~BusyGuard() { if (flag) flag->store(false); }
};

// ─────────────────────────────────────────────────────────────────────────────

void StoreClient::startThread(std::thread&& t) {
    std::lock_guard<std::mutex> lk(mtx_);
    // Tenta dar join in threads that already terminaram to not acumular handles.
    for (auto& th : threads_) {
        if (th.joinable()) {
            // Not podemos check if ended without C++20; as the operacoes
            // are curtas (timeout 5s), fazemos join not-bloqueante not is padrao.
            // Optamos by manter join in the destrutor and, here, only ensure
            // that not re-adicionamos without necessidade. Nenhuma acao extra.
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

// ── login: POST /auth/login {email,password} -> {token,id,name} ───────────────
void StoreClient::loginAsync(const std::string& email, const std::string& password) {
    std::string h = host; int p = port;
    std::string in = email; std::string pw = password;
    bool tls = useTls; std::string pre = apiPrefix;
    activeThreads_.fetch_add(1);
    startThread(std::thread([this, h, p, tls, pre, in, pw]() {
        std::string body = nlohmann::json{{"email", in}, {"password", pw}}.dump();
        HttpResponse r = HttpClient::post(h, p, pre + "/auth/login", body, "", tls);
        if (r.status == 200) {
            nlohmann::json j = jParse(r.body);
            std::string tok = jStr(j, "token"), id = jStr(j, "id");
            { std::lock_guard<std::mutex> lk(mtx_); token_ = tok; playerId_ = id; }
            logged_ = true;
            setMsg("Conectado the Cyber Station");
            // Logo after the login, busca the saldo of gems / inventory.
            HttpResponse me = HttpClient::get(h, p, pre + "/me", tok, tls);
            if (me.status == 200) {
                double g = jNum(jParse(me.body), "gems");
                std::lock_guard<std::mutex> lk(mtx_); gems_ = (int)g;
            }
        } else {
            setMsg(r.status == 0 ? "Backend offline (shop premium unavailable)"
                                 : "Credenciais invalidas (registre-if in the server)");
        }
        activeThreads_.fetch_sub(1);
    }));
}

// ── catalog: GET /store -> {gemPacks:[...], items:[...]} ─────────────────────
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
                for (const auto& the : *itemsArr) {
                    if (!the.is_object()) continue;
                    PremiumItem it;
                    it.id = jStr(the, "id"); it.name = jStr(the, "name");
                    it.type = jStr(the, "type"); it.gems = (int)jNum(the, "gems");
                    if (!it.id.empty()) its.push_back(it);
                }
            }
            auto packsArr = j.find("gemPacks");
            if (packsArr != j.end() && packsArr->is_array()) {
                for (const auto& the : *packsArr) {
                    if (!the.is_object()) continue;
                    GemPack gp;
                    gp.id = jStr(the, "id"); gp.gems = (int)jNum(the, "gems");
                    gp.priceBRL = jNum(the, "priceBRL");
                    if (!gp.id.empty()) pks.push_back(gp);
                }
            }
            { std::lock_guard<std::mutex> lk(mtx_); items_ = its; packs_ = pks; }
            setMsg("Catalogo of the shop loaded");
        } else {
            setMsg(r.status == 0 ? "Backend offline" : "Failure to the load the shop");
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

// ── purchase with gems: POST /store/buy-item {itemId} (server valid saldo) ────
void StoreClient::buyItemAsync(const std::string& itemId) {
    if (!logged_.load()) { setMsg("Faca login in the shop first"); return; }
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
            setMsg(r.status == 0 ? "Backend offline" : "Failure in the purchase");
        }
        activeThreads_.fetch_sub(1);
    }));
}

// ── buy gems: POST /store/buy-gems {packId} -> opens Stripe Checkout ───────
void StoreClient::buyGemsAsync(const std::string& packId) {
    if (!logged_.load()) { setMsg("Faca login in the shop first"); return; }
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
                setMsg("Abrindo payment safe (Stripe)...");
            } else {
                setMsg("Configure STRIPE_SECRET_KEY in the server p/ payment real");
            }
        } else {
            setMsg(r.status == 0 ? "Backend offline" : "Failure to the start payment");
        }
        activeThreads_.fetch_sub(1);
    }));
}


StoreClient::~StoreClient() {
    // Aguarda the termino of all the threads of network before destroy membros
    // compartilhados (mutex/strings) — evita use-after-free in the encerramento.
    std::vector<std::thread> toJoin;
    { std::lock_guard<std::mutex> lk(mtx_); toJoin.swap(threads_); }
    for (auto& th : toJoin) if (th.joinable()) th.join();
}