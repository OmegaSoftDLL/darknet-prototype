// Game_PremiumStore.cpp — loja premium (gems / Stripe via backend Node).
// Extraido de Game.cpp. Mesma classe Game.
#include "Game.h"
#include <raylib.h>
#include <string>

// ─── Loja Premium (Gems / Stripe via backend Node) ───────────────────────────

void Game::startStore() {
    if (storeStarted) return;
    storeStarted = true;
    store.host = "127.0.0.1";
    store.port = 9000;
    store.loginAsync(Player::className(player.charClass)); // login -> token + saldo
    store.fetchStoreAsync();                                // catalogo de itens/packs
}

void Game::updatePremiumStore(float dt) {
    // Atualiza o saldo de gems periodicamente quando logado.
    storeRefreshT -= dt;
    if (store.loggedIn() && storeRefreshT <= 0.0f) {
        storeRefreshT = 8.0f;
        store.refreshAsync();
    }

    // A aba premium só faz sentido com a loja do NPC aberta.
    if (!shopSystem.open) { premiumView = false; return; }

    if (IsKeyPressed(KEY_P)) premiumView = !premiumView;   // P = alterna aba premium
    if (!premiumView) return;

    auto items = store.items();
    int n = (int)items.size();
    if (n > 0) {
        if (IsKeyPressed(KEY_DOWN)) premiumSel = (premiumSel + 1) % n;
        if (IsKeyPressed(KEY_UP))   premiumSel = (premiumSel - 1 + n) % n;
        premiumSel = (premiumSel % (n > 0 ? n : 1));
        if (IsKeyPressed(KEY_ENTER) && premiumSel < n)
            store.buyItemAsync(items[premiumSel].id);  // servidor valida saldo
    }

    // Comprar gems (abre Stripe Checkout no navegador). Teclas 1..4 = packs.
    auto packs = store.packs();
    for (int i = 0; i < (int)packs.size() && i < 4; ++i)
        if (IsKeyPressed(KEY_ONE + i)) store.buyGemsAsync(packs[i].id);
}

void Game::drawPremiumStore() const {
    if (!shopSystem.open) return;

    // Dica para abrir a aba premium quando a loja comum está aberta.
    if (!premiumView) {
        const char* hint = "[P]  LOJA PREMIUM (Gems)";
        int w = MeasureText(hint, 16);
        DrawRectangle(screenWidth/2 - w/2 - 10, 34, w + 20, 24, ColorAlpha(BLACK, 0.7f));
        DrawText(hint, screenWidth/2 - w/2, 38, 16, Color{225,120,255,255});
        return;
    }

    Color C_mag  = {225,120,255,255};
    Color C_cyan = {0,210,255,255};
    int pw = 520, ph = 420;
    int px = screenWidth/2 - pw/2, py = screenHeight/2 - ph/2;
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.55f));
    DrawPanel(px, py, pw, ph, C_mag, 0.95f);

    DrawText("LOJA PREMIUM", px + 18, py + 14, 24, C_mag);
    DrawText(TextFormat("GEMS: %d", store.gems()),
             px + pw - MeasureText(TextFormat("GEMS: %d", store.gems()), 18) - 18,
             py + 18, 18, C_mag);
    DrawText(store.loggedIn() ? "[P] voltar  [SETAS] escolher  [ENTER] comprar  [1-4] comprar gems"
                              : "Conectando ao servidor da loja...",
             px + 18, py + 46, 11, ColorAlpha(WHITE, 0.6f));

    // Itens premium
    auto items = store.items();
    int y = py + 78;
    for (int i = 0; i < (int)items.size(); ++i) {
        bool sel   = (i == premiumSel);
        bool owned = store.ownsItem(items[i].id);
        if (sel) DrawRectangle(px + 12, y - 2, pw - 24, 26, ColorAlpha(C_mag, 0.18f));
        DrawText(items[i].name.c_str(), px + 20, y, 16,
                 owned ? ColorAlpha(WHITE, 0.4f) : WHITE);
        const char* tag = owned ? "ADQUIRIDO" : TextFormat("%d gems", items[i].gems);
        DrawText(tag, px + pw - MeasureText(tag, 14) - 20, y + 1, 14,
                 owned ? C_cyan : C_mag);
        y += 28;
    }

    // Packs de gems
    y += 10;
    DrawText("COMPRAR GEMS (pagamento seguro - Stripe):", px + 18, y, 13, C_cyan);
    y += 22;
    auto packs = store.packs();
    for (int i = 0; i < (int)packs.size() && i < 4; ++i) {
        DrawText(TextFormat("[%d] %d gems  -  R$ %.2f", i + 1, packs[i].gems, packs[i].priceBRL),
                 px + 24, y, 14, ColorAlpha(WHITE, 0.9f));
        y += 22;
    }

    // Mensagem de feedback do backend
    std::string msg = store.lastMessage();
    if (!msg.empty())
        DrawText(msg.c_str(), px + 18, py + ph - 26, 12, C_cyan);
}

