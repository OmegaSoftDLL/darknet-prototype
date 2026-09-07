// NetClient — client WebSocket (RFC 6455) about Winsock, with thread of fundo.
// winsock2/ws2tcpip ANTES of qualquer windows.h.
#include <winsock2.h>
#include <ws2tcpip.h>
#include "NetClient.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <random>
#include <nlohmann/json.hpp>   // vendored in third_party/nlohmann/json.hpp

#pragma comment(lib, "ws2_32.lib")

// ── Parameters ───────────────────────────────────────────────────────────────
static const float PEER_TIMEOUT = 5.0f;   // expira peer without updates ha >5s
static const float SEND_PERIOD  = 0.1f;   // 10x/s

// ── Helpers of reading JSON (nlohmann/json) ──────────────────────────────────
// Same defaults of the parser manual previous: chave ausente (ou with type
// inesperado) returns false and NOT altera `out` — quem calls inicializa with
// 0 / "" before, as before. As mensagens of the server sao objetos planos
// (JSON.stringify), entao read only the chaves of topo and equivalente the busca
// by substring that the parser manual fazia.
static bool jsonNumber(const nlohmann::json& j, const char* key, double& out) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_number()) return false;
    out = it->get<double>();
    return true;
}

static bool jsonString(const nlohmann::json& j, const char* key, std::string& out) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_string()) return false;
    out = it->get<std::string>();
    return true;
}

// ── Base64 (to Sec-WebSocket-Key) ──────────────────────────────────────────
static std::string base64(const unsigned char* data, int len) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int i = 0;
    while (i < len) {
        int b0 = data[i++];
        int b1 = (i < len) ? data[i++] : -1;
        int b2 = (i < len) ? data[i++] : -1;
        out.push_back(tbl[b0 >> 2]);
        out.push_back(tbl[((b0 & 0x03) << 4) | (b1 >= 0 ? (b1 >> 4) : 0)]);
        out.push_back(b1 >= 0 ? tbl[((b1 & 0x0f) << 2) | (b2 >= 0 ? (b2 >> 6) : 0)] : '=');
        out.push_back(b2 >= 0 ? tbl[b2 & 0x3f] : '=');
    }
    return out;
}

// ── SHA1 (RFC 3174) — usado to validate Sec-WebSocket-Accept ────────────────
static void sha1(const unsigned char* msg, size_t len, unsigned char digest[20]) {
    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
    size_t total = ((len + 9 + 63) / 64) * 64;
    std::vector<unsigned char> buf(total);
    std::memcpy(buf.data(), msg, len);
    buf[len] = 0x80;
    uint64_t bits = (uint64_t)len * 8;
    for (int i = 0; i < 8; ++i) buf[total - 1 - i] = (unsigned char)(bits >> (i * 8));
    for (size_t off = 0; off < total; off += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = ((uint32_t)buf[off + i * 4] << 24) |
                   ((uint32_t)buf[off + i * 4 + 1] << 16) |
                   ((uint32_t)buf[off + i * 4 + 2] << 8) |
                   ((uint32_t)buf[off + i * 4 + 3]);
        }
        for (int i = 16; i < 80; ++i) {
            uint32_t x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (x << 1) | (x >> 31);
        }
        uint32_t the = h0, b = h1, c = h2, d = h3, and = h4;
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            uint32_t t = ((the << 5) | (the >> 27)) + f + and + k + w[i];
            and = d; d = c; c = (b << 30) | (b >> 2); b = the; the = t;
        }
        h0 += the; h1 += b; h2 += c; h3 += d; h4 += and;
    }
    auto put = [&](int idx, uint32_t v) {
        digest[idx] = (unsigned char)(v >> 24);
        digest[idx + 1] = (unsigned char)(v >> 16);
        digest[idx + 2] = (unsigned char)(v >> 8);
        digest[idx + 3] = (unsigned char)v;
    };
    put(0, h0); put(4, h1); put(8, h2); put(12, h3); put(16, h4);
}

// GUID fixed of the RFC 6455 to Sec-WebSocket-Accept.
static const char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// Extrai um header of the resposta HTTP (case-insensitive, without espacos extras).
static std::string getHeader(const std::string& resp, const char* name) {
    std::string needle = "\r\n";
    needle += name;
    needle += ":";
    auto pos = resp.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    while (pos < resp.size() && (resp[pos] == ' ' || resp[pos] == '\t')) ++pos;
    auto end = resp.find("\r\n", pos);
    if (end == std::string::npos) return "";
    return resp.substr(pos, end - pos);
}

// Gerador random safe to mascaras WebSocket and Sec-WebSocket-Key.
static std::mt19937& wsRng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}
static unsigned char randomByte() {
    return static_cast<unsigned char>(std::uniform_int_distribution<int>(0, 255)(wsRng()));
}

// ── Sends all the bytes (read with sends parciais) ───────────────────────────
static bool sendAll(SOCKET s, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(s, data + sent, len - sent, 0);
        if (n == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
            return false;
        }
        sent += n;
    }
    return true;
}

// ── Monta and sends um frame of TEXT mascarado (client -> server) ───────────
static bool wsSendText(SOCKET s, const std::string& payload) {
    std::string frame;
    frame.push_back((char)0x81); // FIN + opcode text
    size_t len = payload.size();
    if (len < 126) {
        frame.push_back((char)(0x80 | len));
    } else if (len < 65536) {
        frame.push_back((char)(0x80 | 126));
        frame.push_back((char)((len >> 8) & 0xff));
        frame.push_back((char)(len & 0xff));
    } else {
        frame.push_back((char)(0x80 | 127));
        for (int i = 7; i >= 0; --i) frame.push_back((char)((len >> (8 * i)) & 0xff));
    }
    unsigned char mask[4] = { randomByte(), randomByte(), randomByte(), randomByte() };
    frame.append((const char*)mask, 4);
    size_t base = frame.size();
    frame.resize(base + len);
    for (size_t i = 0; i < len; ++i)
        frame[base + i] = (char)((unsigned char)payload[i] ^ mask[i % 4]);
    return sendAll(s, frame.data(), (int)frame.size());
}

// ── Sends um frame of controle (pong/close), mascarado and without payload ─────────
static bool wsSendControl(SOCKET s, unsigned char opcode, const std::string& payload) {
    std::string frame;
    frame.push_back((char)(0x80 | opcode));
    frame.push_back((char)(0x80 | (payload.size() & 0x7f)));
    unsigned char mask[4] = { randomByte(), randomByte(), randomByte(), randomByte() };
    frame.append((const char*)mask, 4);
    size_t base = frame.size();
    frame.resize(base + payload.size());
    for (size_t i = 0; i < payload.size(); ++i)
        frame[base + i] = (char)((unsigned char)payload[i] ^ mask[i % 4]);
    return sendAll(s, frame.data(), (int)frame.size());
}

NetClient::~NetClient() { shutdown(); }

// ─── API publica ─────────────────────────────────────────────────────────────

bool NetClient::init(const char* myName, uint32_t myId, const char* wsUrl, const char* authToken) {
    myId_ = myId;
    std::memset(myName_, 0, sizeof(myName_));
    if (myName) std::strncpy(myName_, myName, sizeof(myName_) - 1);
    if (authToken) token_ = authToken;

    // Parse simple of ws://host:port/path
    if (wsUrl && *wsUrl) {
        std::string u = wsUrl;
        // wss:// (TLS in WebSocket) not is suportado here: the NetClient is Winsock
        // puro without TLS. If the URL pedir wss, recusa the init without open thread.
        if (u.rfind("wss://", 0) == 0) {
            enabled = false;
            return false;
        }
        size_t s = u.find("://");
        if (s != std::string::npos) u = u.substr(s + 3);
        size_t slash = u.find('/');
        std::string hostport = (slash == std::string::npos) ? u : u.substr(0, slash);
        path_ = (slash == std::string::npos) ? "/" : u.substr(slash);
        size_t colon = hostport.find(':');
        if (colon == std::string::npos) { host_ = hostport; }
        else { host_ = hostport.substr(0, colon); port_ = std::atoi(hostport.c_str() + colon + 1); }
    }

    running_ = true;
    enabled  = true;
    try {
        thread_ = std::thread(&NetClient::netThreadMain, this);
    } catch (...) {
        running_ = false; enabled = false; return false;
    }
    return true;
}

void NetClient::shutdown() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
    enabled   = false;
    connected_ = false;
    std::lock_guard<std::mutex> lk(mtx_);
    peersShared_.clear();
    snapshot_.clear();
    outQueue_.clear();
}

void NetClient::sendState(float x, float y, int charClass, int facing, bool moving) {
    if (!enabled) return;
    // throttle real: usa relogio to not depender of the dt of the raylib
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastSend_).count();
    if (elapsed < SEND_PERIOD) return;
    lastSend_ = now;

    // anim_state compacto: "i"=idle, "wl"=andando p/ left, "wr"=p/ right
    const char* the = moving ? (facing < 0 ? "wl" : "wr") : "i";
    nlohmann::json j = {
        {"t", "state"},
        {"id", myId_},
        {"x", std::round(x * 10.0f) / 10.0f},
        {"y", std::round(y * 10.0f) / 10.0f},
        {"the", the},
        {"c", charClass},
        {"n", myName_}
    };

    std::lock_guard<std::mutex> lk(mtx_);
    if (outQueue_.size() < 32) outQueue_.push_back(j.dump());
}

void NetClient::joinParty(const std::string& room) {
    if (room.empty()) return;
    std::lock_guard<std::mutex> lk(mtx_);
    room_ = room;
    peersShared_.clear();          // limpa peers of the room previous
    outQueue_.push_back(nlohmann::json{{"t", "join"}, {"room", room}}.dump());
}

std::string NetClient::currentRoom() const {
    std::lock_guard<std::mutex> lk(const_cast<std::mutex&>(mtx_));
    return room_;
}

// ── Chat ─────────────────────────────────────────────────────────────────────
void NetClient::sendChat(const std::string& text) {
    if (!enabled || text.empty()) return;
    nlohmann::json j = {
        {"t", "chat"},
        {"id", myId_},
        {"text", text.substr(0, 200)}
    };
    std::lock_guard<std::mutex> lk(mtx_);
    if (outQueue_.size() < 32) outQueue_.push_back(j.dump());
}

std::vector<std::pair<uint32_t,std::string>> NetClient::drainChats() {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<std::pair<uint32_t,std::string>> out(chatIn_.begin(), chatIn_.end());
    chatIn_.clear();
    return out;
}

void NetClient::sendEnemyDeath(uint32_t enemyId) {
    if (!enabled) return;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "{\"t\":\"edeath\",\"id\":%u}", enemyId);
    std::lock_guard<std::mutex> lk(mtx_);
    if (outQueue_.size() < 64) outQueue_.push_back(buf);
}

std::vector<uint32_t> NetClient::drainEnemyDeaths() {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<uint32_t> out(enemyDeathIn_.begin(), enemyDeathIn_.end());
    enemyDeathIn_.clear();
    return out;
}

void NetClient::poll(float dt) {
    std::lock_guard<std::mutex> lk(mtx_);
    // envelhece and expira peers
    for (auto& p : peersShared_) p.lastSeen += dt;
    for (size_t i = 0; i < peersShared_.size();) {
        if (peersShared_[i].lastSeen > PEER_TIMEOUT)
            peersShared_.erase(peersShared_.begin() + i);
        else ++i;
    }
    snapshot_ = peersShared_; // publica copia for the main thread
}

// ─── Thread of network ──────────────────────────────────────────────────────────

void NetClient::netThreadMain() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { connected_ = false; return; }

    while (running_) {
        // ── Resolve + connects TCP ─────────────────────────────────────────────
        char portStr[16]; std::snprintf(portStr, sizeof(portStr), "%d", port_);
        addrinfo hints{}; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
        addrinfo* res = nullptr;
        if (getaddrinfo(host_.c_str(), portStr, &hints, &res) != 0 || !res) {
            std::this_thread::sleep_for(std::chrono::seconds(2)); continue;
        }
        SOCKET s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s == INVALID_SOCKET) { freeaddrinfo(res); std::this_thread::sleep_for(std::chrono::seconds(2)); continue; }
        if (connect(s, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
            closesocket(s); freeaddrinfo(res);
            std::this_thread::sleep_for(std::chrono::seconds(2)); continue;
        }
        freeaddrinfo(res);

        // ── Handshake WebSocket ───────────────────────────────────────────────
        unsigned char keyBytes[16];
        for (int i = 0; i < 16; ++i) keyBytes[i] = randomByte();
        std::string key = base64(keyBytes, 16);
        // JWT in the header Authorization: the server valid in the UPGRADE and recusa
        // with 401 (without token valid the socket nem opens). Nunca in the body JSON.
        std::string authHeader;
        if (!token_.empty()) authHeader = "Authorization: Bearer " + token_ + "\r\n";
        // Buffer dynamic: cabecalho fixed ~170 bytes + strings variaveis.
        int baseSize = 256 + (int)path_.size() + (int)host_.size() + (int)authHeader.size() + (int)key.size();
        std::vector<char> req(baseSize);
        int reqLen = std::snprintf(req.data(), req.size(),
            "GET %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "%s"
            "Sec-WebSocket-Key: %s\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n",
            path_.c_str(), host_.c_str(), port_, authHeader.c_str(), key.c_str());
        if (reqLen <= 0 || reqLen >= (int)req.size()) { closesocket(s); std::this_thread::sleep_for(std::chrono::seconds(2)); continue; }
        if (!sendAll(s, req.data(), reqLen)) { closesocket(s); std::this_thread::sleep_for(std::chrono::seconds(2)); continue; }

        // reads resposta until \r\n\r\n
        std::string resp;
        bool ok = false;
        {
            char tmp[1024];
            for (int tries = 0; tries < 100 && running_; ++tries) {
                int n = recv(s, tmp, sizeof(tmp), 0);
                if (n <= 0) break;
                resp.append(tmp, n);
                if (resp.find("\r\n\r\n") != std::string::npos) { ok = true; break; }
            }
        }
        if (!ok || resp.find(" 101") == std::string::npos) {
            closesocket(s); std::this_thread::sleep_for(std::chrono::seconds(2)); continue;
        }

        // Validate Sec-WebSocket-Accept conforme RFC 6455
        {
            std::string accept = getHeader(resp, "Sec-WebSocket-Accept");
            std::string concat = key + WS_GUID;
            unsigned char digest[20];
            sha1((const unsigned char*)concat.data(), concat.size(), digest);
            std::string expected = base64(digest, 20);
            if (accept != expected) {
                closesocket(s); std::this_thread::sleep_for(std::chrono::seconds(2)); continue;
            }
        }

        // ── Conectado ─────────────────────────────────────────────────────────
        connected_ = true;
        // socket with timeout of recv short p/ not bloquear the loop
        DWORD rcvTo = 50; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcvTo, sizeof(rcvTo));
        { std::string r; { std::lock_guard<std::mutex> lk(mtx_); r = room_; }
          wsSendText(s, nlohmann::json{{"t", "join"}, {"room", r}}.dump()); }

        std::string rx;
        // if sobrou body after the cabecalho of the handshake, processes
        size_t hdrEnd = resp.find("\r\n\r\n");
        if (hdrEnd != std::string::npos && hdrEnd + 4 < resp.size())
            rx.append(resp.substr(hdrEnd + 4));

        auto lastPing = std::chrono::steady_clock::now();
        bool alive = true;

        while (running_ && alive) {
            // 1) sends mensagens enfileiradas
            {
                std::deque<std::string> out;
                { std::lock_guard<std::mutex> lk(mtx_); out.swap(outQueue_); }
                for (auto& m : out) if (!wsSendText(s, m)) { alive = false; break; }
            }
            if (!alive) break;

            // 2) ping periodico
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration<float>(now - lastPing).count() > 15.0f) {
                lastPing = now;
                if (!wsSendControl(s, 0x9, "")) { alive = false; break; }
            }

            // 3) receives and processes frames
            char tmp[2048];
            int n = recv(s, tmp, sizeof(tmp), 0);
            if (n == 0) { alive = false; break; }
            if (n == SOCKET_ERROR) {
                int and = WSAGetLastError();
                if (and == WSAETIMEDOUT || and == WSAEWOULDBLOCK) { continue; }
                alive = false; break;
            }
            rx.append(tmp, n);
            // Protecao contra peer malicioso/server with failure.
            static constexpr size_t MAX_RX = 8 * 1024 * 1024;
            if (rx.size() > MAX_RX) { alive = false; break; }

            // parser of frames
            for (;;) {
                if (rx.size() < 2) break;
                unsigned char b0 = (unsigned char)rx[0];
                unsigned char b1 = (unsigned char)rx[1];
                unsigned char opcode = b0 & 0x0f;
                bool masked = (b1 & 0x80) != 0;
                uint64_t len = b1 & 0x7f;
                size_t pos = 2;
                if (len == 126) {
                    if (rx.size() < 4) break;
                    len = ((unsigned char)rx[2] << 8) | (unsigned char)rx[3];
                    pos = 4;
                } else if (len == 127) {
                    if (rx.size() < 10) break;
                    len = 0;
                    for (int i = 0; i < 8; ++i) len = (len << 8) | (unsigned char)rx[2 + i];
                    pos = 10;
                }
                unsigned char mk[4] = {0,0,0,0};
                if (masked) {
                    if (rx.size() < pos + 4) break;
                    for (int i = 0; i < 4; ++i) mk[i] = (unsigned char)rx[pos + i];
                    pos += 4;
                }
                // Protecao contra overflow aritmetico: len is uint64_t, pos is size_t.
                if (len > rx.size() - pos) break; // frame incompleto ou len malicioso

                std::string payload = rx.substr(pos, (size_t)len);
                if (masked)
                    for (size_t i = 0; i < payload.size(); ++i)
                        payload[i] = (char)((unsigned char)payload[i] ^ mk[i % 4]);
                rx.erase(0, pos + (size_t)len);

                if (opcode == 0x8) { alive = false; break; }          // close
                else if (opcode == 0x9) { wsSendControl(s, 0xA, payload); } // ping -> pong
                else if (opcode == 0xA) { /* pong */ }
                else if (opcode == 0x1 || opcode == 0x0) {            // text
                    // Parse tolerante the failures: payload invalid and ignorado,
                    // as the parser manual (that simplesmente not achava "t").
                    nlohmann::json j = nlohmann::json::parse(payload, nullptr, false);
                    if (!j.is_discarded() && j.is_object()) {
                    std::string the; double idd = 0, xd = 0, yd = 0, cd = 0;
                    std::string tt; jsonString(j, "t", tt);
                    if (tt == "peer" && jsonNumber(j, "id", idd)) {
                        jsonNumber(j, "x", xd);
                        jsonNumber(j, "y", yd);
                        jsonNumber(j, "c", cd);
                        jsonString(j, "the", the);
                        std::string nm; jsonString(j, "n", nm);
                        uint32_t pid = (uint32_t)idd;
                        if (pid != myId_) {
                            std::lock_guard<std::mutex> lk(mtx_);
                            NetPeer* peer = nullptr;
                            for (auto& p : peersShared_) if (p.id == pid) { peer = &p; break; }
                            if (!peer) { peersShared_.push_back(NetPeer{}); peer = &peersShared_.back(); peer->id = pid; }
                            peer->x = (float)xd; peer->y = (float)yd;
                            peer->charClass = (int)cd;
                            peer->moving = (the != "i" && !the.empty());
                            peer->facing = (the == "wl") ? -1 : 1;
                            std::strncpy(peer->name, nm.c_str(), sizeof(peer->name) - 1);
                            peer->name[sizeof(peer->name) - 1] = 0;
                            peer->lastSeen = 0.0f;
                        }
                    }
                    else if (tt == "chat" && jsonNumber(j, "id", idd)) {
                        uint32_t pid = (uint32_t)idd;
                        std::string txt; jsonString(j, "text", txt);
                        if (pid != myId_ && !txt.empty()) {
                            std::lock_guard<std::mutex> lk(mtx_);
                            chatIn_.push_back({ pid, txt });
                            if (chatIn_.size() > 32) chatIn_.pop_front();
                        }
                    }
                    else if (tt == "edeath" && jsonNumber(j, "id", idd)) {
                        std::lock_guard<std::mutex> lk(mtx_);
                        enemyDeathIn_.push_back((uint32_t)idd);
                        if (enemyDeathIn_.size() > 256) enemyDeathIn_.pop_front();
                    }
                    }
                }
            }
        }

        connected_ = false;
        closesocket(s);
        if (running_) std::this_thread::sleep_for(std::chrono::seconds(2)); // backoff p/ reconectar
    }

    WSACleanup();
}
