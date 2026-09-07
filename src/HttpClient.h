#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// HttpClient — client HTTP minimalista about WinHTTP (without dependency externa).
// Usado to falar with the API REST of the backend Node.js (shop premium / Stripe).
// Bloqueante: call SEMPRE numa thread of fundo (see StoreClient).
// ─────────────────────────────────────────────────────────────────────────────
#include <string>

struct HttpResponse {
    int         status = 0;     // 0 = failure of network; otherwise codigo HTTP
    std::string body;
};

namespace HttpClient {
    // host ex.: "127.0.0.1", port ex.: 9000, path ex.: "/store".
    // bearer: token JWT (without "Bearer "); empty = without Authorization.
    // useTls=true => WinHTTP fala HTTPS (WINHTTP_FLAG_SECURE, ex.: gateway of
    // production). Use SOMENTE with URL https:// and certificate valid.
    HttpResponse get (const std::string& host, int port, const std::string& path,
                      const std::string& bearer = "", bool useTls = false);
    HttpResponse post(const std::string& host, int port, const std::string& path,
                      const std::string& jsonBody, const std::string& bearer = "",
                      bool useTls = false);

    // Opens uma URL in the navegador padrao of the system (Stripe Checkout).
    void openBrowser(const std::string& url);
}
