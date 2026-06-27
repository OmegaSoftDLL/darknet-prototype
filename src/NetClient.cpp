// NetClient — cliente WebSocket (RFC 6455) sobre Winsock, com thread de fundo.
// winsock2/ws2tcpip ANTES de qualquer windows.h.
#include <winsock2.h>
#include <ws2tcpip.h>
#include "NetClient.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

// ── Parâmetros ───────────────────────────────────────────────────────────────
static const float PEER_TIMEOUT = 5.0f;   // expira peer sem updates há >5s
static const float SEND_PERIOD  = 0.1f;   // 10x/s

// ── Helpers de string/JSON minimalistas (suficientes p/ nosso protocolo) ─────
static bool jsonNumber(const std::string& s, const char* key, double& out) {
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

static bool jsonString(const std::string& s, const char* key, std::string& out) {
    std::string k = std::string("\"") + key + "\"";
    size_t p = s.find(k);
    if (p == std::string::npos) return false;
    p = s.find(':', p + k.size());
    if (p == std::string::npos) return false;
    p = s.find('"', p);
    if (p == std::string::npos) return false;
    p++;
    out.clear();
    while (p < s.size() && s[p] != '"') {
        if (s[p] == '\\' && p + 1 < s.size()) { out.push_back(s[p + 1]); p += 2; }
        else { out.push_back(s[p]); p++; }
    }
    return true;
}

// ── Base64 (para Sec-WebSocket-Key) ──────────────────────────────────────────
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

// ── Envia todos os bytes (lida com sends parciais) ───────────────────────────
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

// ── Monta e envia um frame de TEXTO mascarado (cliente -> servidor) ───────────
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
    unsigned char mask[4] = {
        (unsigned char)(rand() & 0xff), (unsigned char)(rand() & 0xff),
        (unsigned char)(rand() & 0xff), (unsigned char)(rand() & 0xff)
    };
    frame.append((const char*)mask, 4);
    size_t base = frame.size();
    frame.resize(base + len);
    for (size_t i = 0; i < len; ++i)
        frame[base + i] = (char)((unsigned char)payload[i] ^ mask[i % 4]);
    return sendAll(s, frame.data(), (int)frame.size());
}

// ── Envia um frame de controle (pong/close), mascarado e sem payload ─────────
static bool wsSendControl(SOCKET s, unsigned char opcode, const std::string& payload) {
    std::string frame;
    frame.push_back((char)(0x80 | opcode));
    frame.push_back((char)(0x80 | (payload.size() & 0x7f)));
    unsigned char mask[4] = {
        (unsigned char)(rand() & 0xff), (unsigned char)(rand() & 0xff),
        (unsigned char)(rand() & 0xff), (unsigned char)(rand() & 0xff)
    };
    frame.append((const char*)mask, 4);
    size_t base = frame.size();
    frame.resize(base + payload.size());
    for (size_t i = 0; i < payload.size(); ++i)
        frame[base + i] = (char)((unsigned char)payload[i] ^ mask[i % 4]);
    return sendAll(s, frame.data(), (int)frame.size());
}

NetClient::~NetClient() { shutdown(); }

// ─── API pública ─────────────────────────────────────────────────────────────

bool NetClient::init(const char* myName, uint32_t myId, const char* wsUrl) {
    myId_ = myId;
    std::memset(myName_, 0, sizeof(myName_));
    if (myName) std::strncpy(myName_, myName, sizeof(myName_) - 1);

    // Parse simples de ws://host:port/path
    if (wsUrl && *wsUrl) {
        std::string u = wsUrl;
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
    sendAccum_ += SEND_PERIOD; // chamado ~todo frame; aproxima 10x/s via contador
    // throttle real: usa relógio para não depender do dt do raylib
    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - last).count();
    if (elapsed < SEND_PERIOD) return;
    last = now;

    // anim_state compacto: "i"=idle, "wl"=andando p/ esquerda, "wr"=p/ direita
    const char* a = moving ? (facing < 0 ? "wl" : "wr") : "i";
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "{\"t\":\"state\",\"id\":%u,\"x\":%.1f,\"y\":%.1f,\"a\":\"%s\",\"c\":%d,\"n\":\"%s\"}",
        myId_, x, y, a, charClass, myName_);

    std::lock_guard<std::mutex> lk(mtx_);
    if (outQueue_.size() < 32) outQueue_.push_back(buf);
}

void NetClient::joinParty(const std::string& room) {
    if (room.empty()) return;
    std::lock_guard<std::mutex> lk(mtx_);
    room_ = room;
    peersShared_.clear();          // limpa peers da sala anterior
    outQueue_.push_back(std::string("{\"t\":\"join\",\"room\":\"") + room + "\"}");
}

std::string NetClient::currentRoom() const {
    std::lock_guard<std::mutex> lk(const_cast<std::mutex&>(mtx_));
    return room_;
}

void NetClient::poll(float dt) {
    std::lock_guard<std::mutex> lk(mtx_);
    // envelhece e expira peers
    for (auto& p : peersShared_) p.lastSeen += dt;
    for (size_t i = 0; i < peersShared_.size();) {
        if (peersShared_[i].lastSeen > PEER_TIMEOUT)
            peersShared_.erase(peersShared_.begin() + i);
        else ++i;
    }
    snapshot_ = peersShared_; // publica copia para a main thread
}

// ─── Thread de rede ──────────────────────────────────────────────────────────

void NetClient::netThreadMain() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { connected_ = false; return; }

    while (running_) {
        // ── Resolve + conecta TCP ─────────────────────────────────────────────
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
        for (int i = 0; i < 16; ++i) keyBytes[i] = (unsigned char)(rand() & 0xff);
        std::string key = base64(keyBytes, 16);
        char req[512];
        std::snprintf(req, sizeof(req),
            "GET %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: %s\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n",
            path_.c_str(), host_.c_str(), port_, key.c_str());
        if (!sendAll(s, req, (int)std::strlen(req))) { closesocket(s); std::this_thread::sleep_for(std::chrono::seconds(2)); continue; }

        // lê resposta até \r\n\r\n
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

        // ── Conectado ─────────────────────────────────────────────────────────
        connected_ = true;
        // socket com timeout de recv curto p/ não bloquear o loop
        DWORD rcvTo = 50; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcvTo, sizeof(rcvTo));
        { std::string r; { std::lock_guard<std::mutex> lk(mtx_); r = room_; }
          wsSendText(s, std::string("{\"t\":\"join\",\"room\":\"") + r + "\"}"); }

        std::string rx;
        // se sobrou corpo após o cabeçalho do handshake, processa
        size_t hdrEnd = resp.find("\r\n\r\n");
        if (hdrEnd != std::string::npos && hdrEnd + 4 < resp.size())
            rx.append(resp.substr(hdrEnd + 4));

        auto lastPing = std::chrono::steady_clock::now();
        bool alive = true;

        while (running_ && alive) {
            // 1) envia mensagens enfileiradas
            {
                std::deque<std::string> out;
                { std::lock_guard<std::mutex> lk(mtx_); out.swap(outQueue_); }
                for (auto& m : out) if (!wsSendText(s, m)) { alive = false; break; }
            }
            if (!alive) break;

            // 2) ping periódico
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration<float>(now - lastPing).count() > 15.0f) {
                lastPing = now;
                if (!wsSendControl(s, 0x9, "")) { alive = false; break; }
            }

            // 3) recebe e processa frames
            char tmp[2048];
            int n = recv(s, tmp, sizeof(tmp), 0);
            if (n == 0) { alive = false; break; }
            if (n == SOCKET_ERROR) {
                int e = WSAGetLastError();
                if (e == WSAETIMEDOUT || e == WSAEWOULDBLOCK) { continue; }
                alive = false; break;
            }
            rx.append(tmp, n);

            // parser de frames
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
                if (rx.size() < pos + len) break; // frame incompleto

                std::string payload = rx.substr(pos, (size_t)len);
                if (masked)
                    for (size_t i = 0; i < payload.size(); ++i)
                        payload[i] = (char)((unsigned char)payload[i] ^ mk[i % 4]);
                rx.erase(0, pos + (size_t)len);

                if (opcode == 0x8) { alive = false; break; }          // close
                else if (opcode == 0x9) { wsSendControl(s, 0xA, payload); } // ping -> pong
                else if (opcode == 0xA) { /* pong */ }
                else if (opcode == 0x1 || opcode == 0x0) {            // texto
                    std::string a; double idd = 0, xd = 0, yd = 0, cd = 0;
                    std::string tt; jsonString(payload, "t", tt);
                    if (tt == "peer" && jsonNumber(payload, "id", idd)) {
                        jsonNumber(payload, "x", xd);
                        jsonNumber(payload, "y", yd);
                        jsonNumber(payload, "c", cd);
                        jsonString(payload, "a", a);
                        std::string nm; jsonString(payload, "n", nm);
                        uint32_t pid = (uint32_t)idd;
                        if (pid != myId_) {
                            std::lock_guard<std::mutex> lk(mtx_);
                            NetPeer* peer = nullptr;
                            for (auto& p : peersShared_) if (p.id == pid) { peer = &p; break; }
                            if (!peer) { peersShared_.push_back(NetPeer{}); peer = &peersShared_.back(); peer->id = pid; }
                            peer->x = (float)xd; peer->y = (float)yd;
                            peer->charClass = (int)cd;
                            peer->moving = (a != "i" && !a.empty());
                            peer->facing = (a == "wl") ? -1 : 1;
                            std::strncpy(peer->name, nm.c_str(), sizeof(peer->name) - 1);
                            peer->name[sizeof(peer->name) - 1] = 0;
                            peer->lastSeen = 0.0f;
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
