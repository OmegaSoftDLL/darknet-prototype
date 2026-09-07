// Game_Network.cpp — LAN multiplayer (NetClient), remote players and party/alliance.
// Extracted from Game.cpp. Same class Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdlib>

// ─── Multiplayer LAN (NetClient) ─────────────────────────────────────────────

// WebSocket URL: defaults directly to the local game server (dev). In production
// point DARKNET_WS_URL to the gateway (and.g. ws://darknet.yourdomain.with/ws).
static std::string wsUrl() {
    const char* u = getenv("DARKNET_WS_URL");
    return (u && *u) ? u : "ws://127.0.0.1:9000/ws";
}

void Game::startNetwork() {
    if (netActive || netPending_) return;
    netId  = (uint32_t)GetRandomValue(1, 2000000000);
    // The current server requires the valid JWT in the WS handshake. The token
    // comes from the shop login (StoreClient). If not logged in yet, it queues
    // the start — pollStartNetwork() fires the soon the login completes (via updateParty).
    if (store.loggedIn()) {
        const std::string u = wsUrl();
        netActive = net.init(Player::className(player.charClass), netId,
                             u.c_str(), store.token().c_str());
    } else {
        startStore();          // ensures the ongoing login
        netPending_ = true;
    }
}

void Game::pollStartNetwork() {
    if (netActive || !netPending_ || !store.loggedIn()) return;
    netPending_ = false;
    const std::string u = wsUrl();
    netActive = net.init(Player::className(player.charClass), netId,
                         u.c_str(), store.token().c_str());
}

void Game::renderRemotePlayers() const {
    if (!net.enabled) return;
    Vector2 cam = camera.target;
    static const Color cols[6] = {
        {60,120,220,255},{220,80,140,255},{150,160,175,255},
        {120,80,220,255},{180,120,255,255},{200,130,60,255}
    };
    for (const auto& p : net.peers()) {
        if (std::fabs(p.x - cam.x) > 1100 || std::fabs(p.y - cam.y) > 700) continue;
        Color c = cols[(p.charClass >= 0 && p.charClass < 6) ? p.charClass : 0];
        DrawEllipse((int)p.x, (int)(p.y + 18), 14.0f, 5.0f, ColorAlpha(BLACK, 0.4f));
        DrawRectangle((int)p.x - 9, (int)p.y - 14, 18, 28, c);
        DrawCircle((int)p.x, (int)(p.y - 20), 9.0f, c);
        DrawCircleLines((int)p.x, (int)(p.y - 20), 9.0f, ColorAlpha(WHITE, 0.4f));
        int w = MeasureText(p.name, 11);
        DrawRectangle((int)p.x - w/2 - 3, (int)p.y - 42, w + 6, 14, ColorAlpha(BLACK, 0.6f));
        DrawText(p.name, (int)p.x - w/2, (int)p.y - 40, 11, ColorAlpha(WHITE, 0.95f));
        DrawCircle((int)p.x + 11, (int)(p.y - 30), 3.0f, Color{0,255,80,255}); // online
    }
}

// ─── Grupo / Alianca (party multiplayer) ─────────────────────────────────────

void Game::updateParty() {
    pollStartNetwork();   // starts the WS the soon the the login (JWT) completes
    if (chatActive) return;
    if (IsKeyPressed(KEY_O)) { partyPanel = !partyPanel; partyInput.clear(); }
    if (!partyPanel) return;

    // Type the numeric group code
    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= '0' && ch <= '9' && partyInput.size() < 6) partyInput.push_back((char)ch);
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !partyInput.empty()) partyInput.pop_back();

    // ENTER joins the typed group; L = public room; C = creates the group (code from netId)
    if (IsKeyPressed(KEY_ENTER) && !partyInput.empty()) {
        net.joinParty("party_" + partyInput);
        triggerPlayerSpeech("Joined group " + partyInput, 2.5f);
        partyPanel = false;
    }
    if (IsKeyPressed(KEY_L)) {
        net.joinParty("lobby");
        triggerPlayerSpeech("Joined the public room.", 2.0f);
        partyPanel = false;
    }
    if (IsKeyPressed(KEY_C)) {
        std::string code = std::to_string(1000 + (int)(netId % 9000));
        partyInput = code;
        net.joinParty("party_" + code);
        triggerPlayerSpeech("Group created! Code: " + code, 4.0f);
        partyPanel = false;
    }
}

void Game::drawPartyPanel() const {
    // Permanent indicator: current room + number of online allies
    Color C_cyan = {0,235,255,255};
    std::string room = net.currentRoom();
    bool isParty = (room.rfind("party_", 0) == 0);
    int allies = (int)net.peers().size();
    const char* roomLbl = isParty ? room.c_str() + 6 : "PUBLIC";
    DrawText(TextFormat("GROUP: %s  |  Allies online: %d  [O]",
             isParty ? roomLbl : "PUBLIC", allies),
             14, 30, 11, ColorAlpha(C_cyan, 0.6f));

    if (!partyPanel) return;

    int pw = 460, ph = 250;
    int px = screenWidth/2 - pw/2, py = screenHeight/2 - ph/2;
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.55f));
    DrawPanel(px, py, pw, ph, C_cyan, 0.95f);
    DrawText("GROUP / ALLIANCE", px + 18, py + 14, 22, C_cyan);
    DrawText("Play together with friends in the same room in real time.",
             px + 18, py + 44, 12, ColorAlpha(WHITE, 0.6f));

    int y = py + 78;
    DrawText(TextFormat("Current room: %s", isParty ? roomLbl : "PUBLIC (lobby)"),
             px + 18, y, 14, C_cyan); y += 26;
    DrawText(TextFormat("Connected allies: %d", allies), px + 18, y, 14,
             Color{0,230,120,255}); y += 30;

    DrawText("Type the code and press ENTER to join the group:", px + 18, y, 12,
             ColorAlpha(WHITE, 0.7f)); y += 20;
    DrawRectangle(px + 18, y, 200, 26, ColorAlpha(BLACK, 0.5f));
    DrawRectangleLinesEx({(float)(px+18),(float)y,200,26}, 1.5f, C_cyan);
    DrawText(partyInput.empty() ? "_" : partyInput.c_str(), px + 26, y + 5, 18, WHITE);
    y += 38;

    DrawText("[C] Create private group   [L] Public room   [O] Close",
             px + 18, y, 12, ColorAlpha(C_cyan, 0.8f));
    // List of allies in the room
    y += 26;
    int shown = 0;
    for (const auto& p : net.peers()) {
        if (shown >= 4) break;
        DrawText(TextFormat("- %s", p.name[0] ? p.name : "Operator"),
                 px + 26, y, 12, ColorAlpha(WHITE, 0.75f));
        y += 16; shown++;
    }
}

