#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// StoreClient — fachada for the shop premium of the backend Node.js.
// Does login (JWT), busca the catalog (/store), saldo of gems (/me), purchase of
// items with gems (/store/buy-item, validated in the server) and opens the Stripe Checkout
// to buy gems (/store/buy-gems). All the chamadas of network rodam in threads
// of fundo; the UI reads the state by getters protegidos by mutex.
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>

struct PremiumItem { std::string id, name, type; int gems = 0; };
struct GemPack     { std::string id; int gems = 0; double priceBRL = 0.0; };

class StoreClient {
public:
    std::string host      = "127.0.0.1";
    int         port      = 9000;
    // Prefixo of route when the API stays behind of the gateway nginx (/api/* -> /api).
    // Empty = connection direta in the game-server (dev local). Configurado pelo game
    // via DARKNET_API_URL (ex.: "https://darknet.seudominio.with" -> /api + TLS).
    std::string apiPrefix = "";
    bool        useTls    = false;   // HTTPS (WinHTTP) in the API configured

    ~StoreClient();   // espera threads of network in voo (evita use-after-free in the shutdown)

    // Token JWT obtido in the login (usado also pelo NetClient in the handshake WS).
    std::string token() const;

    // All assincronas (disparam thread of fundo, retornam imediatamente).
    void loginAsync(const std::string& email, const std::string& password); // POST /auth/login -> token+id
    void fetchStoreAsync();                      // GET  /store      -> catalog
    void refreshAsync();                         // GET  /me         -> gems/inventory
    void buyItemAsync(const std::string& itemId);// POST /store/buy-item (server valid)
    void buyGemsAsync(const std::string& packId);// POST /store/buy-gems -> opens navegador

    // ── Getters of UI (thread-safe) ───────────────────────────────────────────
    bool        loggedIn() const { return logged_.load(); }
    bool        busy()     const { return busy_.load(); }
    int         gems()     const;
    std::vector<PremiumItem> items() const;
    std::vector<GemPack>     packs() const;
    std::string lastMessage() const;
    bool        ownsItem(const std::string& id) const;

private:
    mutable std::mutex        mtx_;
    std::string               token_;
    std::string               playerId_;
    std::atomic<bool>         logged_{false};
    std::atomic<bool>         busy_{false};
    std::atomic<int>          activeThreads_{0};   // threads of network in voo
    std::vector<std::thread>  threads_;            // threads ativas (join in the destrutor)
    int                       gems_ = 0;
    std::vector<PremiumItem>  items_;
    std::vector<GemPack>      packs_;
    std::vector<std::string>  inventory_;
    std::string               msg_;

    void setMsg(const std::string& m);
    void startThread(std::thread&& t);             // helper: guard and limpa threads finalizadas
};
