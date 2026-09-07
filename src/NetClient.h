#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// NetClient — multiplayer in time real via WebSocket (RFC 6455) about Winsock.
//
// Connects to the backend Node.js (server/game-server) in ws://127.0.0.1:9000/ws
// (ou via gateway nginx in ws://127.0.0.1:8080/ws). All the I/O of network roda numa
// THREAD in second plano to not travar the thread of render of the raylib:
//   - O game calls sendState(...) -> enfileira um JSON {"t":"state",...}.
//   - A thread of fundo connects, does the handshake, sends the queue and receives frames.
//   - Mensagens {"t":"peer",...} atualizam the list of peers (protegida by mutex).
//   - poll(dt) (main thread) envelhece/expira peers and publica um snapshot.
// Robusto: if the connection fall, the thread tenta reconectar with backoff; the game
// continuous playable offline (enabled permanece true mas peers stays empty).
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <chrono>

struct NetPeer {
    uint32_t id        = 0;
    float    x         = 0.0f;
    float    y         = 0.0f;
    int      charClass = 0;   // 0..5
    int      facing    = 1;   // -1/1
    bool     moving    = false;
    char     name[24]  = {0};
    float    lastSeen  = 0.0f; // seconds since the last packet received
};

class NetClient {
public:
    std::atomic<bool> enabled{false};

    // Inicia the thread of network apontando to wsUrl (default ws://127.0.0.1:9000/ws).
    // authToken: JWT sent in the header Authorization of the handshake (the server
    // recusa the upgrade with 401 without token valid). optional only p/ tests off.
    // Returns true if the thread went up (the connection in si ocorre in second plano).
    bool init(const char* myName, uint32_t myId, const char* wsUrl = nullptr,
              const char* authToken = nullptr);
    void shutdown();

    // Enfileira the state local. Throttle internal (~10x/s); can call all frame.
    void sendState(float x, float y, int charClass, int facing, bool moving);

    // Main thread: envelhece and expira peers, publica the snapshot read by peers().
    void poll(float dt);

    // Grupo/alianca: enters numa room (party). "lobby" = room publica.
    void joinParty(const std::string& room);
    std::string currentRoom() const;

    // ── Chat of room ───────────────────────────────────────────────────────────
    void sendChat(const std::string& text);          // sends {"t":"chat",...}
    // Drena the mensagens of chat recebidas since the last chamada (id != the meu).
    std::vector<std::pair<uint32_t,std::string>> drainChats();

    // ── Synchronization of enemies ──────────────────────────────────────────────
    void sendEnemyDeath(uint32_t enemyId);           // sends {"t":"edeath","id":...}
    // Drena the IDs of enemies abatidos by others players (evita "fantasmas").
    std::vector<uint32_t> drainEnemyDeaths();

    // Players remotos vistos in the ultimos ~3s (snapshot, only read in the main thread).
    const std::vector<NetPeer>& peers() const { return snapshot_; }

    bool connected() const { return connected_.load(); }

    ~NetClient();

private:
    // ── State compartilhado between main thread and thread of network ───────────────
    std::thread              thread_;
    std::mutex               mtx_;
    std::atomic<bool>        running_{false};
    std::atomic<bool>        connected_{false};

    std::vector<NetPeer>     peersShared_;   // updated pela thread of network
    std::vector<NetPeer>     snapshot_;       // only main thread (retornado by peers())
    std::deque<std::string>  outQueue_;       // JSONs the send (preenchido in the main)
    std::deque<std::pair<uint32_t,std::string>> chatIn_;     // chats recebidos
    std::deque<uint32_t>     enemyDeathIn_;   // deaths of enemies recebidas

    uint32_t myId_       = 0;
    char     myName_[24] = {0};
    // Throttle of the sendState (era um static of function — compartilhado between
    // instancias and without reset in the shutdown/init).
    std::chrono::steady_clock::time_point lastSend_ = std::chrono::steady_clock::now();

    // URL alvo
    std::string host_ = "127.0.0.1";
    int         port_ = 9000;
    std::string path_ = "/ws";
    std::string room_ = "lobby";   // room/party current (protegida by mtx_)
    std::string token_;            // JWT p/ header Authorization in the handshake

    // ── Implementation of the thread (NetClient.cpp) ───────────────────────────────
    void netThreadMain();
};
