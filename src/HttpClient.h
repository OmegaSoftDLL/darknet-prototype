#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// HttpClient — cliente HTTP minimalista sobre WinHTTP (sem dependência externa).
// Usado para falar com a API REST do backend Node.js (loja premium / Stripe).
// Bloqueante: chamar SEMPRE numa thread de fundo (ver StoreClient).
// ─────────────────────────────────────────────────────────────────────────────
#include <string>

struct HttpResponse {
    int         status = 0;     // 0 = falha de rede; senão código HTTP
    std::string body;
};

namespace HttpClient {
    // host ex.: "127.0.0.1", port ex.: 9000, path ex.: "/store".
    // bearer: token JWT (sem "Bearer "); vazio = sem Authorization.
    // useTls=true => WinHTTP fala HTTPS (WINHTTP_FLAG_SECURE, ex.: gateway de
    // produção). Use SOMENTE com URL https:// e certificado válido.
    HttpResponse get (const std::string& host, int port, const std::string& path,
                      const std::string& bearer = "", bool useTls = false);
    HttpResponse post(const std::string& host, int port, const std::string& path,
                      const std::string& jsonBody, const std::string& bearer = "",
                      bool useTls = false);

    // Abre uma URL no navegador padrão do sistema (Stripe Checkout).
    void openBrowser(const std::string& url);
}
