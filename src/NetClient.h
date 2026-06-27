#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// NetClient — multiplayer em tempo real via WebSocket (RFC 6455) sobre Winsock.
//
// Conecta ao backend Node.js (server/game-server) em ws://127.0.0.1:9000/ws
// (ou via gateway nginx em ws://127.0.0.1:8080/ws). Toda a I/O de rede roda numa
// THREAD em segundo plano para nao travar a thread de render da raylib:
//   - O jogo chama sendState(...) -> enfileira um JSON {"t":"state",...}.
//   - A thread de fundo conecta, faz o handshake, envia a fila e recebe frames.
//   - Mensagens {"t":"peer",...} atualizam a lista de peers (protegida por mutex).
//   - poll(dt) (main thread) envelhece/expira peers e publica um snapshot.
// Robusto: se a conexao cair, a thread tenta reconectar com backoff; o jogo
// continua jogavel offline (enabled permanece true mas peers fica vazio).
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>

struct NetPeer {
    uint32_t id        = 0;
    float    x         = 0.0f;
    float    y         = 0.0f;
    int      charClass = 0;   // 0..5
    int      facing    = 1;   // -1/1
    bool     moving    = false;
    char     name[24]  = {0};
    float    lastSeen  = 0.0f; // segundos desde o último pacote recebido
};

class NetClient {
public:
    bool enabled = false;

    // Inicia a thread de rede apontando para wsUrl (default ws://127.0.0.1:9000/ws).
    // Retorna true se a thread subiu (a conexao em si ocorre em segundo plano).
    bool init(const char* myName, uint32_t myId, const char* wsUrl = nullptr);
    void shutdown();

    // Enfileira o estado local. Throttle interno (~10x/s); pode chamar todo frame.
    void sendState(float x, float y, int charClass, int facing, bool moving);

    // Main thread: envelhece e expira peers, publica o snapshot lido por peers().
    void poll(float dt);

    // Grupo/aliança: entra numa sala (party). "lobby" = sala publica.
    void joinParty(const std::string& room);
    std::string currentRoom() const;

    // Jogadores remotos vistos nos últimos ~3s (snapshot, só lido na main thread).
    const std::vector<NetPeer>& peers() const { return snapshot_; }

    bool connected() const { return connected_.load(); }

    ~NetClient();

private:
    // ── Estado compartilhado entre main thread e thread de rede ───────────────
    std::thread              thread_;
    std::mutex               mtx_;
    std::atomic<bool>        running_{false};
    std::atomic<bool>        connected_{false};

    std::vector<NetPeer>     peersShared_;   // atualizado pela thread de rede
    std::vector<NetPeer>     snapshot_;       // só main thread (retornado por peers())
    std::deque<std::string>  outQueue_;       // JSONs a enviar (preenchido na main)

    uint32_t myId_       = 0;
    char     myName_[24] = {0};
    float    sendAccum_  = 0.0f;

    // URL alvo
    std::string host_ = "127.0.0.1";
    int         port_ = 9000;
    std::string path_ = "/ws";
    std::string room_ = "lobby";   // sala/party atual (protegida por mtx_)

    // ── Implementação da thread (NetClient.cpp) ───────────────────────────────
    void netThreadMain();
};
