// Game_HUD.cpp — HUD and overlays 2D: resources, paineis (character/objetivos/skills),
// quests, minimap and the helpers of painel/barra. Extraido of Game.cpp. Same class Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <string>

// ─── UI helpers ──────────────────────────────────────────────────────────────

void Game::DrawPanel(int x, int y, int w, int h, Color border, float alpha) {
    DrawRectangle(x, y, w, h, ColorAlpha(Color{8,10,18,255}, alpha));
    // Angular corner cuts (cyberpunk style)
    int c = 6;
    DrawLine(x+c, y,   x+w-c, y,   ColorAlpha(border, 0.7f));
    DrawLine(x, y+c,   x, y+h-c,   ColorAlpha(border, 0.5f));
    DrawLine(x+c, y+h, x+w-c, y+h, ColorAlpha(border, 0.7f));
    DrawLine(x+w, y+c, x+w, y+h-c, ColorAlpha(border, 0.5f));
    DrawLine(x, y+c, x+c, y,       ColorAlpha(border, 0.6f));
    DrawLine(x+w-c, y, x+w, y+c,   ColorAlpha(border, 0.6f));
    DrawLine(x, y+h-c, x+c, y+h,   ColorAlpha(border, 0.6f));
    DrawLine(x+w-c, y+h, x+w, y+h-c, ColorAlpha(border, 0.6f));
}

void Game::DrawBarH(int x, int y, int w, int h, float pct, Color fill, Color bg) {
    DrawRectangle(x, y, w, h, bg);
    int filled = (int)(w * std::max(0.0f, std::min(1.0f, pct)));
    if (filled > 0) DrawRectangle(x, y, filled, h, fill);
    // Sheen on top of bar
    DrawRectangle(x, y, filled, h/3, ColorAlpha(WHITE, 0.12f));
    DrawRectangleLinesEx({(float)x,(float)y,(float)w,(float)h}, 1.0f,
                         ColorAlpha(fill, 0.4f));
}

// ─── UI ──────────────────────────────────────────────────────────────────────

void Game::drawHudAndOverlays() {
    // HUD of collected resources (wood/stone/iron/silver/gold)
    if (openWorldMode) drawResourceHUD();

    // ── Evolution Engine HUD: Threat Level + active mutator ─────────────
    {
        int hx = screenWidth - 232, hy = 56;
        int hh = activeMutator == WorldMutator::None ? 24 : 42;
        DrawRectangle(hx, hy, 226, hh, ColorAlpha({8,12,26,255}, 0.86f));
        DrawRectangle(hx, hy, 3, hh, ColorAlpha({255,70,70,255}, 0.9f));
        int ac = 6;
        Color aB = ColorAlpha({255,90,90,255}, 0.65f);
        DrawLine(hx+ac, hy, hx+226-ac, hy, aB);
        DrawLine(hx+ac, hy+hh, hx+226-ac, hy+hh, aB);
        DrawLine(hx, hy+ac, hx+ac, hy, aB);
        DrawLine(hx+226-ac, hy, hx+226, hy+ac, aB);
        DrawLine(hx, hy+hh-ac, hx+ac, hy+hh, aB);
        DrawLine(hx+226-ac, hy+hh, hx+226, hy+hh-ac, aB);
        DrawText(TextFormat("AMEACA  Lv %d", threatLevel), hx + 12, hy + 4, 14,
                 Color{255, 90, 90, 255});
        if (activeMutator != WorldMutator::None) {
            float pulse = 0.6f + 0.4f * std::sin((float)GetTime() * 3.0f);
            DrawText(TextFormat("%s", mutatorName(activeMutator)), hx + 12, hy + 22, 12,
                     ColorAlpha(Color{180, 120, 255, 255}, pulse));
            float frac = 1.0f - (mutatorTimer / mutatorDuration);
            DrawRectangle(hx + 152, hy + 24, (int)(58 * frac), 5, Color{180,120,255,200});
            DrawRectangleLinesEx({(float)(hx+151),(float)(hy+23),60.f,7.f}, 1,
                                 ColorAlpha(Color{180,120,255,255}, 0.4f));
        }
    }

    // SAFE ZONE indicator on the HUD (top-center)
    if (openWorldMode && inSafeZone(player.position)) {
        const char* si = "SAFE ZONE - prepare and build your base";
        int siw = MeasureText(si, 16);
        float pulse = 0.6f + 0.4f * std::sin((float)GetTime() * 3.0f);
        int szx = screenWidth/2 - siw/2 - 14, szy = 30;
        DrawRectangle(szx, szy, siw + 28, 26, ColorAlpha({6,18,28,255}, 0.9f));
        DrawRectangle(szx, szy, 3, 26, ColorAlpha({0,255,180,255}, 0.9f));
        int szc = 6;
        Color szB = ColorAlpha({0,230,160,255}, 0.7f);
        DrawLine(szx+szc, szy, szx+siw+28-szc, szy, szB);
        DrawLine(szx+szc, szy+26, szx+siw+28-szc, szy+26, szB);
        DrawLine(szx, szy+szc, szx+szc, szy, szB);
        DrawLine(szx+siw+28-szc, szy, szx+siw+28, szy+szc, szB);
        DrawLine(szx, szy+26-szc, szx+szc, szy+26, szB);
        DrawLine(szx+siw+28-szc, szy+26, szx+siw+28, szy+26-szc, szB);
        DrawText(si, screenWidth/2 - siw/2, szy + 5, 16, ColorAlpha({0,255,180,255}, pulse));
    }
    // PHASE progress (kills until the portal) — objective always visible.
    // Off-center: dock on the left, under the chapter/zone (in the pill in the middle).
    if (openWorldMode) {
        const char* pt = owPortalOpen
            ? "PORTAL ABERTO - siga the marcador and pressione [E]"
            : TextFormat("PHASE %d  -  kills until the portal: %d/%d",
                         owPhase + 1, owPhaseKills, owPhaseGoal);
        int pw = MeasureText(pt, 13);
        int pxx = 10, pyy = 56;
        DrawRectangle(pxx, pyy, pw + 28, 22, ColorAlpha({8,12,26,255}, 0.86f));
        DrawRectangle(pxx, pyy, 3, 22, ColorAlpha({0,235,255,255}, 0.9f));
        DrawText(pt, pxx + 12, pyy + 4, 13,
                 owPortalOpen ? Color{0,255,180,255} : Color{210,220,235,255});
        DrawLine(pxx, pyy + 22, pxx + pw + 28, pyy + 22, ColorAlpha({0,235,255,255}, 0.35f));
        DrawLine(pxx + 6, pyy + 22, pxx + 14, pyy + 22, ColorAlpha({0,235,255,255}, 0.7f));
        DrawLine(pxx + pw + 14, pyy + 22, pxx + pw + 22, pyy + 22, ColorAlpha({0,235,255,255}, 0.7f));
    }
    drawStoryBanner();
    drawPhaseFade();
    drawPlayerSpeech();
    if (paused) drawPauseMenu();
    if (showLevelUpScreen)   drawLevelUpScreen();
    if (showEvolutionScreen) drawEvolutionScreen();
    if (gameWon)             drawVictoryScreen();

    // Shop overlay (fullscreen, drawn last only it's on top)
    if (shopSystem.open) {
        shopSystem.render(player.credits, screenWidth, screenHeight);
        drawPremiumStore();   // premium tab by up of the shop common (key TAB)
    }

    // Crafting overlay
    if (craftingSystem.open) {
        craftingSystem.render(player.inventory, screenWidth, screenHeight);
    }

    // Group / alliance (always-visible indicator + panel with key O)
    drawPartyPanel();
}

void Game::drawUI() const {
    Color C_cyan  = {0, 210, 255, 255};
    Color C_red   = {220, 30, 30, 255};
    Color C_gold  = {255, 190, 0, 255};
    Color C_green = {0, 210, 80, 255};

    // ── Top bar ───────────────────────────────────────────────────────────────
    DrawRectangle(0, 0, screenWidth, 26, ColorAlpha(BLACK, 0.78f));
    DrawLine(0, 26, screenWidth, 26, ColorAlpha(C_cyan, 0.35f));
    DrawLine(0, 24, 12, 24, ColorAlpha({255,180,40,255}, 0.7f));
    DrawLine(screenWidth - 12, 24, screenWidth, 24, ColorAlpha({0,235,255,255}, 0.7f));
    DrawLine(12, 24, 12, 26, ColorAlpha({255,180,40,255}, 0.7f));
    DrawLine(screenWidth - 12, 24, screenWidth - 12, 26, ColorAlpha({0,235,255,255}, 0.7f));
    DrawRectangle(0, 26, 6, 2, ColorAlpha({0,235,255,255}, 0.8f));
    DrawRectangle(screenWidth - 6, 26, 6, 2, ColorAlpha({0,235,255,255}, 0.8f));

    // Chapter + zone
    ZoneInfo info = getZoneInfo(currentZone);
    DrawText(TextFormat("CAP.%d | %s", storyChapter, info.name.c_str()),
             10, 5, 14, ColorAlpha(C_cyan, 0.9f));

    // Center — kills + credits + gems + time (compact, stays inside top bar)
    DrawText(TextFormat("ABATIDOS: %d   $%d   %.0fs", enemiesKilled, player.credits, sessionTime),
             screenWidth/2 - 175, 5, 13, ColorAlpha(WHITE, 0.55f));
    // Premium gems (backend balance) — highlighted in magenta
    DrawText(TextFormat("GEMS %d", store.gems()),
             screenWidth/2 + 95, 5, 13, Color{225,120,255,255});

    // Right of top bar — compact hint only (does NOT overflow into quest HUD zone x>940)
    DrawText("[Q]Healing [I]Items [B]Construir [C]Forge [E/TAB]Shop [O]Grupo [J]Miss",
             screenWidth - 470, 5, 10, ColorAlpha(C_cyan, 0.55f));

    // ── PAINEL DE CONTROLES (lado esquerdo) — visible when the bot is off ────
    if (!botController.active && !showLevelUpScreen && !showEvolutionScreen &&
        !shopSystem.open && !craftingSystem.open) {
        int cx = 8, cy = 300, cw = 188;
        struct Ctl { const char* key; const char* label; Color col; };
        static const Ctl ctls[] = {
            {"Q",      "Healing (potion + regen)",   {0,255,120,255}},
            {"I",      "Inventory / Equipar",   {0,235,255,255}},
            {"O",      "Grupo / Alianca",        {120,255,160,255}},
            {"B",      "Construir base/tanques", {120,200,255,255}},
            {"C",      "Forge for weapons/armor",  {255,200,100,255}},
            {"TAB/E",  "Item shop",          {255,230,120,255}},
            {"F2 F3 F4","Invocar aliados",        {120,255,160,255}},
            {"SHIFT+arr","Selecionar unidades",   {120,255,160,255}},
            {"Dir.",   "Move unidades selec.",  {120,255,160,255}},
            {"J",      "Quests",                {200,160,255,255}},
            {"L / K",  "Points / Evolution",      {255,215,0,255}},
        };
        int n = (int)(sizeof(ctls)/sizeof(ctls[0]));
        int ch = 18 * n + 24;
        DrawPanel(cx, cy, cw, ch, C_cyan, 0.72f);
        DrawRectangle(cx, cy, 3, ch, ColorAlpha({0,235,255,255}, 0.85f));
        DrawText("CONTROLES", cx + 8, cy + 6, 11, ColorAlpha(C_cyan, 0.9f));
        for (int i = 0; i < n; i++) {
            int ly = cy + 22 + i * 18;
            int kw = MeasureText(ctls[i].key, 11);
            DrawRectangle(cx + 8, ly - 1, kw + 10, 14, ColorAlpha(ctls[i].col, 0.22f));
            DrawRectangleLinesEx({(float)(cx+8), (float)(ly-1), (float)(kw+10), 14}, 1,
                                 ColorAlpha(ctls[i].col, 0.5f));
            DrawText(ctls[i].key, cx + 13, ly, 11, ctls[i].col);
            DrawText(ctls[i].label, cx + 78, ly, 10, ColorAlpha(WHITE, 0.78f));
        }
    }

    // ── CHARACTER PANEL (bottom-left) ─────────────────────────────────────────
    drawCharacterPanel();

    // ── SKILLS BAR (bottom center) — hidden when fullscreen overlay active ─────
    bool anyOverlay = showLevelUpScreen || showEvolutionScreen ||
                      shopSystem.open   || craftingSystem.open;
    if (!anyOverlay) {
        drawSkillsPanel();
    }

    // ── HACK TREE (perk skill tree) ────────────────────────────────────────
    drawSkillTreePanel();

    // ── MINIMAP (bottom-right) — hidden when fullscreen overlay active ─────────
    if (!anyOverlay) {
        drawMinimap();
    }

    // ── Zone transition text ──────────────────────────────────────────────────
    drawZoneInfo();

    // ── Quest HUD (top-right, below top bar — only when in the fullscreen overlay) ─
    if (!anyOverlay) {
        drawQuestHUD();
    }

    // ── Level-up announcement (compact banner, LONG after the zone/phase banner) ──
    // Before it was 420x70 centered at -100 (overlapped the phase name at -40).
    // New: compact pill, smaller font, flush to the top under the HUD bar.
    if (player.leveledUp) {
        float the = std::min(player.levelUpTimer / 2.5f, 1.0f);
        char lvl[48]; snprintf(lvl, sizeof(lvl), "LESPEED %d ALCANCADO!", player.level);
        int fw = MeasureText(lvl, 20);
        int fw2 = player.lastPassive.empty() ? 0 : MeasureText(player.lastPassive.c_str(), 12);
        int bw = std::max(fw, fw2) + 36, bh = player.lastPassive.empty() ? 40 : 52;
        int bx = screenWidth/2 - bw/2, by = 30;
        DrawPanel(bx, by, bw, bh, C_gold, 0.94f);
        DrawText(lvl, bx + bw/2 - fw/2, by + (player.lastPassive.empty() ? 11 : 6), 20,
                 ColorAlpha(C_gold, the));
        if (!player.lastPassive.empty()) {
            DrawText(player.lastPassive.c_str(), bx + bw/2 - fw2/2, by + 32, 12,
                     ColorAlpha({0,255,180,255}, the));
        }
    }

    // ── Open panels (fullscreen) ──────────────────────────────────────────────
    if (showInventory)  player.drawInventory();
    if (showEquipment)  player.drawEquipment();
    if (showQuestLog)   drawQuestLog();

    // ── NPC prompt ────────────────────────────────────────────────────────────
    if (nearNpcIndex >= 0 && !dialogOpen) {
        DrawPanel(screenWidth/2-110, screenHeight-52, 220, 36, C_gold, 0.88f);
        DrawText("[E]  FALAR COM NPC",
                 screenWidth/2 - MeasureText("[E]  FALAR COM NPC",16)/2,
                 screenHeight-44, 16, C_gold);
    }
    if (dialogOpen && nearNpcIndex >= 0 && nearNpcIndex < (int)npcs.size()) {
        npcs[nearNpcIndex].showDialog(dialogLine);   // dialog balloon (works for 3D and 2D)
        DrawText("[E] Continue  [ESC] Close", 10, screenHeight - 36, 14,
                 ColorAlpha(WHITE, 0.7f));
    }

    // ── Chat Input Box ────────────────────────────────────────────────────────
    if (chatActive) {
        int boxW = 500, boxH = 36;
        int boxX = screenWidth/2 - boxW/2;
        int boxY = screenHeight - 120; // above the skill bar
        
        DrawPanel(boxX, boxY, boxW, boxH, C_cyan, 0.85f);
        DrawText("CHAT:", boxX + 12, boxY + 11, 14, C_gold);
        DrawText(chatInput.c_str(), boxX + 65, boxY + 11, 14, WHITE);
        
        // Cursor piscante
        float t = (float)GetTime();
        if (std::fmod(t, 0.8f) < 0.4f) {
            int cursorX = boxX + 65 + MeasureText(chatInput.c_str(), 14);
            DrawRectangle(cursorX + 2, boxY + 10, 2, 16, C_cyan);
        }
    }
}

void Game::drawCharacterPanel() const {
    Color C_cyan  = {0,235,255,255};
    Color C_green = {0,210,80, 255};
    Color C_gold  = {255,190,0,255};

    // Painel reorganizado — lines well separadas, SEM sobreposicao.
    bool hasPts = (pendingLevelUps > 0 || pendingEvolutions > 0 || player.skillPoints > 0);
    int panX = 8, panW = 322;
    int panH = hasPts ? 196 : 172;
    int panY = screenHeight - panH - 8;
    DrawPanel(panX, panY, panW, panH, C_cyan);

    int y = panY + 8;
    // ── Line 1: LESPEED + difficulty + credits ──
    DrawText(TextFormat("LESPEED %d", player.level), panX+8, y, 15, C_gold);
    {
        const DifficultySettings& diff = getDifficulty();
        int bx = panX + 8 + MeasureText("LESPEED 00", 15) + 10;
        int bw = MeasureText(diff.name, 11) + 10;
        DrawRectangle(bx, y-1, bw, 17, ColorAlpha(diff.labelColor, 0.20f));
        DrawText(diff.name, bx+5, y+1, 11, ColorAlpha(diff.labelColor, 0.95f));
    }
    DrawText(TextFormat("$ %d", player.credits), panX+panW - MeasureText(TextFormat("$ %d", player.credits),15) - 10, y, 15, C_gold);

    // ── Line 2: DMG / DEF / SPEED (line own) ──
    y += 22;
    DrawText(TextFormat("DAMAGE %.0f    DEF %.0f%%    SPEED %.0f",
             player.getEffectiveDamage(), player.defense, player.speed),
             panX+8, y, 13, ColorAlpha(WHITE, 0.85f));

    // ── Line 3: HP ──
    y += 22;
    {
        float pct = player.health / player.maxHealth;
        Color col = pct > 0.5f ? C_green : pct > 0.25f ? Color{255,180,0,255} : Color{220,30,30,255};
        DrawText("HP", panX+8, y, 13, ColorAlpha(col, 0.9f));
        DrawBarH(panX+34, y, 280, 15, pct, col, ColorAlpha(BLACK, 0.5f));
        DrawText(TextFormat("%d/%d", (int)player.health, (int)player.maxHealth),
                 panX+40, y+2, 12, ColorAlpha(WHITE, 0.95f));
        // Shield/Overload the the short label at the end of the HP bar
        if (player.isShielded())
            DrawText("[BARREIRA]", panX+232, y+2, 11, Color{0,235,255,255});
        else if (player.isOverloaded())
            DrawText("[SOBRECGA]", panX+232, y+2, 11, Color{255,150,0,255});
    }

    // ── Line 4: XP ──
    y += 22;
    {
        float pct = (float)player.xp / player.xpToNextLevel;
        DrawText("XP", panX+8, y, 13, Color{60,140,255,255});
        DrawBarH(panX+34, y, 280, 13, pct, Color{60,140,255,255}, ColorAlpha(BLACK,0.5f));
        DrawText(TextFormat("%d / %d", player.xp, player.xpToNextLevel),
                 panX+40, y+1, 11, ColorAlpha(WHITE,0.85f));
    }

    // ── Line 5 (optional): points acumulados ──
    if (hasPts) {
        y += 22;
        float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 6.0f);
        std::string txt;
        if (pendingLevelUps > 0)  txt += TextFormat("[L] %d point(s)  ", pendingLevelUps);
        if (pendingEvolutions > 0) txt += TextFormat("[K] %d evolution", pendingEvolutions);
        if (player.skillPoints > 0) txt += TextFormat("[X] %d hack", player.skillPoints);
        DrawRectangle(panX+8, y-1, panW-16, 17, ColorAlpha(Color{90,60,0,255}, 0.5f * pulse));
        DrawText(txt.c_str(), panX+12, y+1, 12, ColorAlpha(Color{255,215,0,255}, 0.6f+0.4f*pulse));
    }

    // ── Separador + equipment ──
    y += 24;
    DrawLine(panX+8, y, panX+panW-8, y, ColorAlpha(C_cyan, 0.25f));
    y += 4;
    if (!player.equippedWeapon.isEmpty())
        DrawText(TextFormat("Weapon: %s", player.equippedWeapon.name.c_str()),
                 panX+8, y, 12, player.equippedWeapon.color);
    if (!player.equippedArmor.isEmpty())
        DrawText(TextFormat("Armor: %s", player.equippedArmor.name.c_str()),
                 panX+8, y+15, 12, player.equippedArmor.color);

    // ── Proxima passiva (rodape) ──
    const char* nextPassive = "";
    if      (player.level < 3)  nextPassive = "Prox: Lv3 Blindagem";
    else if (player.level < 5)  nextPassive = "Next: Lv5 Combat Core";
    else if (player.level < 7)  nextPassive = "Prox: Lv7 Amplificador";
    else if (player.level < 10) nextPassive = "Prox: Lv10 Protocolo IRON-VIII";
    else if (player.level < 15) nextPassive = "Prox: Lv15 Executor Legendary";
    if (nextPassive[0] != '\0')
        DrawText(nextPassive, panX+8, panY+panH-17, 11, ColorAlpha(C_gold, 0.7f));
}

void Game::drawObjectivesPanel() const {
    Color C_cyan = {0,235,255,255};
    Color C_gold = {255,190,0,255};
    Color C_red  = {220, 30, 30,255};

    int panX = screenWidth - 310, panY = 34, panW = 302, panH = 0;

    // Count active quests
    int activeCount = 0;
    for (const auto& q : quests) if (!q.completed) activeCount++;
    panH = 38 + activeCount * 50 + 30;
    if (panH < 80) panH = 80;

    DrawPanel(panX, panY, panW, panH, C_cyan);

    // Header
    DrawText(TextFormat("OBJETIVOS  [CAP.%d]", storyChapter),
             panX+10, panY+6, 13, C_gold);
    DrawLine(panX+8, panY+22, panX+panW-8, panY+22, ColorAlpha(C_cyan, 0.3f));

    int dy = panY + 26;
    int shown = 0;
    for (const auto& q : quests) {
        if (q.completed) continue;
        // Quest title
        Color col = q.active ? Color{255,200,80,255} : ColorAlpha(WHITE, 0.5f);
        DrawText(TextFormat("[%s] %s", q.active ? "ATIVO" : "INATIVO", q.title.c_str()),
                 panX+10, dy, 13, col);
        // Description
        DrawText(q.description.c_str(), panX+14, dy+16, 11, ColorAlpha(WHITE, 0.6f));
        // Progress bar
        float pct = q.target > 0 ? (float)q.current / q.target : 0.0f;
        DrawBarH(panX+10, dy+30, panW-20, 8, pct,
                 {0,200,100,255}, ColorAlpha(BLACK,0.5f));
        DrawText(q.getProgressText().c_str(), panX+14, dy+30, 10,
                 ColorAlpha(WHITE, 0.75f));
        dy += 52;
        if (++shown >= 4) break;
    }

    // Kills until boss
    if (!bossSpawned) {
        int killsNeeded = bossSpawnThreshold - enemiesKilled;
        if (killsNeeded > 0) {
            DrawLine(panX+8, dy, panX+panW-8, dy, ColorAlpha(C_red, 0.3f));
            DrawText(TextFormat("BOSS in %d kills", killsNeeded),
                     panX+10, dy+4, 12, ColorAlpha(C_red, 0.85f));
        }
    } else {
        DrawText(">> BOSS ATIVO! <<", panX+10, dy+4, 14, C_red);
    }
}

void Game::drawSkillsPanel() const {
    Color C_cyan  = {0, 210, 255, 255};

    const int count = (int)player.skills.size();
    if (count <= 0) return;

    // Cada power has IDENTIDADE own: color, icone procedural, state vivo.
    static const Color POW_COLORS[6] = {
        {  0, 235, 255, 255 },   // 1 Laser       - cyan
        {255, 215,  60, 255 },   // 2 EMP         - gold
        {255, 140,  30, 255 },   // 3 Grenade     - orange
        {255,  90,  50, 255 },   // 4 Overload  - red
        { 70, 170, 255, 255 },   // 5 Barrier    - blue
        {  0, 220, 110, 255 }    // 6 Burst      - green
    };

    const int slotW = 92, slotH = 106, gap = 4;
    const int totalW = count * slotW + (count - 1) * gap;
    const int baseX  = screenWidth/2 - totalW/2;   // stays BELOW, centered
    const int baseY  = screenHeight - slotH - 8;
    const float t    = (float)GetTime();

    // Bar background — more present angular panel
    DrawRectangle(baseX-12, baseY-8, totalW+24, slotH+18, ColorAlpha({7,12,26,255}, 0.86f));
    DrawRectangle(baseX-12, baseY-8, 4, slotH+18, ColorAlpha(C_cyan, 0.85f));
    {
        int bb = 9;
        DrawLine(baseX-12+bb, baseY-8, baseX+totalW+12-bb, baseY-8, ColorAlpha(C_cyan, 0.4f));
        DrawLine(baseX-12+bb, baseY+slotH+10, baseX+totalW+12-bb, baseY+slotH+10, ColorAlpha(C_cyan, 0.4f));
        DrawLine(baseX-12, baseY-8+bb, baseX-12+bb, baseY-8, ColorAlpha(C_cyan, 0.7f));
        DrawLine(baseX+totalW+12-bb, baseY-8, baseX+totalW+12, baseY-8+bb, ColorAlpha(C_cyan, 0.7f));
        DrawLine(baseX-12, baseY+slotH+10-bb, baseX-12+bb, baseY+slotH+10, ColorAlpha(C_cyan, 0.6f));
        DrawLine(baseX+totalW+12-bb, baseY+slotH+10, baseX+totalW+12, baseY+slotH+10-bb, ColorAlpha(C_cyan, 0.5f));
    }

    for (int i = 0; i < count; ++i) {
        const Skill& s = player.skills[i];
        Color col = POW_COLORS[i % 6];
        const int sx = baseX + i * (slotW + gap);
        const bool ready = s.isReady();
        const float pul = 0.55f + 0.45f * sinf(t * 3.0f + i * 1.3f);

        // Slot body
        DrawRectangle(sx, baseY, slotW, slotH, Color{11,18,34,255});
        DrawRectangle(sx+2, baseY+2, slotW-4, slotH-4, ColorAlpha(col, ready ? 0.05f : 0.015f));
        Color border = ready ? ColorAlpha(col, 0.55f + 0.45f * pul)
                             : ColorAlpha(WHITE, 0.18f);
        DrawRectangleLinesEx({(float)sx,(float)baseY,(float)slotW,(float)slotH}, 2, border);
        if (ready) {
            DrawLine(sx, baseY, sx+slotW, baseY, ColorAlpha(col, 0.9f * pul));
            DrawLine(sx, baseY+slotH, sx+slotW, baseY+slotH, ColorAlpha(col, 0.25f));
        }

        // Key (chip in the corner)
        DrawRectangle(sx+4, baseY+4, 20, 16, ColorAlpha(col, 0.92f));
        DrawText(TextFormat("%d", i+1), sx+9, baseY+5, 12, Color{6,10,20,255});

        // ── Icone PROCEDURAL (without assets) ─────
        const float cx = sx + slotW/2.0f, cy = baseY + 34.0f;
        switch (i) {
        case 0: {   // LASER — energy projectile
            DrawTriangle({cx, cy-11},{cx-7, cy+9},{cx+7, cy+9}, col);
            DrawTriangle({cx-2.5f, cy-2},{cx-5.5f, cy+6},{cx+5.5f, cy+6}, ColorAlpha(WHITE, 0.55f));
            DrawLineV({cx, cy+11},{cx, cy+19}, ColorAlpha(col, 0.5f));
        } break;
        case 1: {   // EMP — pulso concentrico
            DrawRing({cx, cy}, 11, 14, 0, 360, 30, ColorAlpha(col, 0.85f));
            DrawCircle((int)cx, (int)cy, 4, ColorAlpha(col, 0.9f));
            DrawRing({cx, cy}, 4, 6, 0, 360, 20, ColorAlpha(WHITE, 0.45f));
        } break;
        case 2: {   // GRANADA — sphere with lit fuse
            DrawCircle((int)(cx), (int)(cy+1), 10, col);
            DrawCircleSector({cx, cy+1}, 10, 30, 90, 14, ColorAlpha(WHITE, 0.35f));
            DrawLineV({cx, cy-9},{cx+7, cy-14},{255,230,150,255});
            DrawCircle((int)(cx+8), (int)(cy-15), 2.3f, {255,245,200,255});
        } break;
        case 3: {   // SOBRECARGA — zig-zagging radius
            DrawTriangle({cx-5, cy-12},{cx+3, cy-12},{cx-2, cy+2}, col);
            DrawTriangle({cx+6, cy-3},{cx-2, cy+2},{cx+3, cy+12}, ColorAlpha({255,200,140,255}, 0.95f));
            DrawLineV({cx-8, cy},{cx+8, cy}, ColorAlpha({255,220,160,255}, 0.6f));
        } break;
        case 4: {   // BARREIRA — shield hexagonal
            DrawPoly({cx, cy}, 6, 13, 90.0f, ColorAlpha(col, 0.35f));
            DrawPolyLines({cx, cy}, 6, 13, 90.0f, ColorAlpha(col, 0.95f));
            DrawTriangle({cx-6, cy+7},{cx+6, cy+7},{cx, cy-6}, col);
        } break;
        case 5: {   // RAJADA — fan of projectiles
            for (int k = -1; k <= 1; ++k) {
                float the = -PI/2.0f + k * 0.34f;
                Vector2 tip = { cx + cosf(the)*15, cy + sinf(the)*15 };
                Vector2 p1  = { cx + cosf(the-0.14f)*7, cy + sinf(the-0.14f)*7 };
                Vector2 p2  = { cx + cosf(the+0.14f)*7, cy + sinf(the+0.14f)*7 };
                DrawTriangle(tip, p1, p2, k == 0 ? col : ColorAlpha(col, 0.6f));
            }
        } break;
        default: break;
        }

        // Nome
        int nw = MeasureText(s.name.c_str(), 11);
        DrawText(s.name.c_str(), sx + slotW/2 - nw/2, baseY + 58, 11,
                 ready ? ColorAlpha(WHITE, 0.95f) : ColorAlpha(WHITE, 0.65f));

        // State
        if (ready) {
            int tw = MeasureText("PRONTO", 10);
            DrawText("PRONTO", sx + slotW/2 - tw/2, baseY + 74, 10, ColorAlpha(col, 0.9f));
            DrawRectangle(sx+2, baseY+slotH-4, slotW-4, 4, ColorAlpha(col, 0.35f + 0.4f*pul));
        } else {
            // Overlay descending (reads the "loading") ANTES of the textos
            float cd = s.cooldownPercent();
            DrawRectangle(sx, baseY, slotW, (int)(slotH * cd), ColorAlpha(BLACK, 0.58f));
            Color warm = ColorAlpha({255,150,40,255}, 0.9f);
            std::string cds = TextFormat("%.1fs", s.currentCooldown);
            int tw = MeasureText(cds.c_str(), 10);
            DrawText(cds.c_str(), sx + slotW/2 - tw/2, baseY + 74, 10, warm);
            DrawBarH(sx+2, baseY+slotH-6, slotW-4, 4, 1.0f - cd,
                     {80,150,255,255}, ColorAlpha(BLACK, 0.6f));
        }

        // Damage (when relevante)
        if (s.damage > 0)
            DrawText(TextFormat("DMG %.0f", s.damage), sx+4, baseY + slotH - 17, 9,
                     ColorAlpha({255,120,120,255}, 0.75f));
    }
}

void Game::drawZoneInfo() const {
    ZoneInfo info = getZoneInfo(currentZone);
    // Zone name is shown in the top-bar left ("CAP.X | ZoneName") — in the duplicate here.

    if (zoneNameTimer > 0.0f) {
        float alpha = std::min(zoneNameTimer, 1.0f);
        Color c = ColorAlpha(info.portalColor, alpha);
        DrawText(info.name.c_str(), screenWidth/2 - MeasureText(info.name.c_str(), 40)/2 + 2,
                 screenHeight/2 - 38, 40, ColorAlpha(BLACK, alpha * 0.7f));
        DrawText(info.name.c_str(), screenWidth/2 - MeasureText(info.name.c_str(), 40)/2,
                 screenHeight/2 - 40, 40, c);
        DrawText(info.description.c_str(),
                 screenWidth/2 - MeasureText(info.description.c_str(), 20)/2 + 1,
                 screenHeight/2 + 11, 20, ColorAlpha(BLACK, alpha * 0.7f));
        DrawText(info.description.c_str(),
                 screenWidth/2 - MeasureText(info.description.c_str(), 20)/2,
                 screenHeight/2 + 10, 20, ColorAlpha(WHITE, alpha));
    }
}

void Game::drawQuestHUD() const {
    // Always-visible active mission panel — top-right corner
    Color C_gold  = {255,200,0,255};
    Color C_cyan  = {0,235,255,255};
    Color C_green = {0,220,100,255};

    // Count active non-completed quests
    int panW = 260, rowH = 52;
    int count = 0;
    for (const auto& q : quests) if (q.active && !q.completed) ++count;
    if (count == 0) {
        // Nothing active: tiny label below top bar
        int lx = screenWidth - 120, ly = 32;
        DrawRectangle(lx, ly, 112, 18, ColorAlpha(BLACK, 0.7f));
        DrawText("[J] MISSOES", lx + 6, ly + 2, 12, ColorAlpha(C_gold, 0.7f));
        return;
    }
    int show = std::min(count, 3);
    int panH = 22 + show * rowH + 6;
    // py=70: starts below the top bar (0–26) + gap — in the overlap with hint text
    // py=102: below the HUD of AMEACA/mutador (y=56, until 40px of height). Com 70
    // the painel nascia POR BAIXO dele and the dois textos if sobrepunham in the screen.
    int px = screenWidth - panW - 8, py = 102;

    // Panel background (angular)
    DrawPanel(px, py, panW, panH, C_gold, 0.82f);
    DrawRectangle(px, py, 3, panH, ColorAlpha(C_gold, 0.85f));

    // Header
    DrawText("MISSOES ATIVAS [J]", px + 8, py + 4, 12, ColorAlpha(C_gold, 0.95f));
    DrawLine(px + 4, py + 18, px + panW - 4, py + 18, ColorAlpha(C_gold, 0.3f));

    int ry = py + 22;
    int drawn = 0;
    for (const auto& q : quests) {
        if (!q.active || q.completed) continue;
        if (drawn >= 3) break;

        // Title
        DrawText(q.title.c_str(), px + 8, ry, 13, C_cyan);

        // Progress bar
        float pct = (q.target > 0) ? std::min(1.0f, (float)q.current / q.target) : 0.0f;
        DrawRectangle(px + 8, ry + 16, panW - 16, 8, ColorAlpha(BLACK, 0.6f));
        DrawRectangle(px + 8, ry + 16, (int)((panW - 16) * pct), 8,
                      pct >= 1.0f ? C_green : C_gold);
        DrawRectangleLinesEx({(float)(px+8),(float)(ry+16),(float)(panW-16),8}, 1,
                             ColorAlpha(WHITE, 0.2f));

        // Progress text + NPC
        // by VALOR: getProgressText() devolve std::string, entao the .c_str() of um
        // temporary already was liberado when the DrawText lia (text virava lixo).
        std::string prog = q.getProgressText();
        DrawText(prog.c_str(), px + 8,  ry + 27, 11, ColorAlpha(WHITE, 0.85f));
        DrawText(q.npcOwner.c_str(), px + panW - MeasureText(q.npcOwner.c_str(),10) - 6,
                 ry + 27, 10, ColorAlpha(C_cyan, 0.7f));

        // Reward hint
        DrawText(TextFormat("+%dHP  +%dXP", (int)q.rewardHP, q.rewardXP),
                 px + 8, ry + 39, 10, ColorAlpha(C_green, 0.75f));

        ry += rowH;
        ++drawn;
    }
}

void Game::drawQuestLog() const {
    int x = 340, y = 160;
    int rows = (int)quests.size();
    DrawPanel(x - 10, y - 10, 380, 40 + rows * 55, {255,200,0,255}, 0.90f);
    DrawRectangle(x - 10, y - 10, 3, 40 + rows * 55, ColorAlpha({255,200,0,255}, 0.85f));
    DrawText("DIARIO DE MISSOES (J):", x, y, 18, {255,215,40,255});

    int dy = 30;
    for (const auto& q : quests) {
        Color col = q.completed ? GREEN : (q.active ? YELLOW : DARKGRAY);
        const char* status = q.completed ? "[CONCLUIDA]" : "[ATIVA]";
        DrawText(TextFormat("%s %s", status, q.title.c_str()), x, y + dy, 15, col);
        DrawText(TextFormat("  %s  %s", q.description.c_str(), q.getProgressText().c_str()),
                 x, y + dy + 18, 13, WHITE);
        if (!q.rewardEquip.isEmpty()) {
            DrawText(TextFormat("  Recomp: %s +%dHP +%dXP",
                     q.rewardEquip.name.c_str(), (int)q.rewardHP, q.rewardXP),
                     x, y + dy + 34, 12, q.rewardEquip.color);
        }
        dy += 55;
    }
}

void Game::drawMinimap() const {
    // ── Minimap — bottom-right, above skills bar ──────────────────────────────
    //   Skills bar is at screenHeight - 92 → minimap sits just above it
    int mapW = 160;
    int mapH = 130;
    int mapX = screenWidth - mapW - 8;
    int mapY = screenHeight - mapH - 100;  // above skills panel

    // Radar window in WORLD UNITS. Em world open the minimap shows SO
    // the PHASE ATUAL (the circle of owPhaseRadius around the base center), never
    // the map global integer — before the scale usava the world all (OW_ZONE) and the
    // blips espremiam num canto, deixando the radar useless.
    float worldW = (float)(tilemap.width  * Tilemap::tileSize);
    float worldH = (float)(tilemap.height * Tilemap::tileSize);
    float vx0, vy0, vx1, vy1;
    bool  owFocus = openWorldMode;
    if (owFocus) {
        // World open and um DISCO of owPhaseRadius around of the base; the window of the
        // radar and esse disco with folga for the portal. Sem clamp in worldW/H: the scenario
        // and infinito and the phase now cresce (radius 5200..7800) to alem of the 7680u of
        // antigamente — the central text/ellipse traveled outside the panel.
        float half = owPhaseRadius * 1.06f + 20.0f;   // margin for the portal at the edge
        vx0 = safeZoneCenter.x - half; vx1 = safeZoneCenter.x + half;
        vy0 = safeZoneCenter.y - half; vy1 = safeZoneCenter.y + half;
    } else {
        vx0 = 0; vy0 = 0; vx1 = worldW; vy1 = worldH;
    }
    float scaleX = mapW / (vx1 - vx0);
    float scaleY = mapH / (vy1 - vy0);
    auto mx = [&](float p) { return (int)((p - vx0) * scaleX); };   // world -> px (relativo the mapX)
    auto my = [&](float p) { return (int)((p - vy0) * scaleY); };

    // Panel background with cyberpunk border
    DrawPanel(mapX - 2, mapY - 14, mapW + 4, mapH + 16, {0,235,255,255}, 0.85f);
    DrawRectangle(mapX - 2, mapY - 14, mapW + 4, 2, ColorAlpha({0,235,255,255}, 0.7f));
    // Title chip
    DrawRectangle(mapX, mapY - 12, 54, 13, ColorAlpha({0,235,255,255}, 0.25f));
    DrawRectangleLinesEx({(float)mapX, (float)(mapY-12), 54.f, 13.f}, 1,
                         ColorAlpha({0,235,255,255}, 0.6f));
    DrawText("RADAR", mapX + 4, mapY - 11, 10, ColorAlpha({0,235,255,255}, 0.95f));

    // Clipping region background
    DrawRectangle(mapX, mapY, mapW, mapH, ColorAlpha({5,10,20,255}, 0.9f));

    // Em world open: highlight of the playable area = the disco of the PHASE ATUAL in the center
    // of the radar, with the safe zone marked inside.
    if (owFocus) {
        DrawRectangle(mapX, mapY, mapW, mapH, ColorAlpha({0,140,220,255}, 0.16f));
        DrawEllipseLines(mapX + mapW / 2, mapY + mapH / 2, mapW * 0.5f, mapH * 0.5f,
                         ColorAlpha({0,235,255,255}, 0.55f));
        DrawEllipseLines(mapX + mapW / 2, mapY + mapH / 2, mapW * 0.5f - 2, mapH * 0.5f - 2,
                         ColorAlpha({255,200,90,255}, 0.18f));
        float zrX = safeZoneRadius * scaleX, zrY = safeZoneRadius * scaleY;
        if (zrX > 4.0f && zrY > 4.0f)
            DrawEllipseLines(mapX + mx(safeZoneCenter.x), mapY + my(safeZoneCenter.y),
                             zrX, zrY, ColorAlpha(GREEN, 0.35f));
    }

    // Region borders that cross the phase window (orientation in the world);
    // in closed map there are in the regions, only the overlay only exists in open world.
    if (owFocus) {
        // REGION borders (dynamic grid centered on the base) that cruzam the
        // window of the phase — in the longer the old tilemap fixed 3x3 grid.
        for (const auto& r : worldRegions) {
            float lx = r.bounds.x;
            if (lx > vx0 && lx < vx1) {
                int ll = mapX + mx(lx);
                DrawLine(ll, mapY, ll, mapY + mapH, ColorAlpha({0,235,255,255}, 0.10f));
            }
            float ly = r.bounds.y;
            if (ly > vy0 && ly < vy1) {
                int l2 = mapY + my(ly);
                DrawLine(mapX, l2, mapX + mapW, l2, ColorAlpha({0,235,255,255}, 0.10f));
            }
        }
        DrawText(TextFormat("PHASE %d", owPhase + 1), mapX + 60, mapY - 11, 10,
                 ColorAlpha({0,235,255,255}, 0.9f));
    }

    // Grid lines (faint)
    for (int gx = 0; gx <= 4; ++gx) {
        int lx = mapX + gx * mapW / 4;
        DrawLine(lx, mapY, lx, mapY + mapH, ColorAlpha({0,235,255,255}, 0.08f));
    }
    for (int gy = 0; gy <= 4; ++gy) {
        int ly = mapY + gy * mapH / 4;
        DrawLine(mapX, ly, mapX + mapW, ly, ColorAlpha({0,235,255,255}, 0.08f));
    }

    // Portals on minimap
    for (const auto& portal : tilemap.portals) {
        int px = mapX + mx(portal.position.x);
        int py = mapY + my(portal.position.y);
        DrawCircle(px, py, 4, portal.color);
        DrawCircleLines(px, py, 6, ColorAlpha(portal.color, 0.4f));
    }

    // PHASE portal open: pulsing blip on the radar. O HUD sends "seguir the
    // marcador" — without isto not existia marcador none.
    if (openWorldMode && owPortalOpen) {
        int px = mapX + mx(owPortalPos.x);
        int py = mapY + my(owPortalPos.y);
        float pulse = 0.55f + 0.45f * std::sin((float)GetTime() * 5.0f);
        DrawCircle(px, py, 5, ColorAlpha(Color{0,255,180,255}, pulse));
        DrawCircleLines(px, py, 8, ColorAlpha(Color{0,255,180,255}, 0.5f));
    }

    // NPCs
    for (const auto& npc : npcs) {
        DrawCircle(mapX + mx(npc.position.x),
                   mapY + my(npc.position.y), 3, BLUE);
    }

    // Structures of the player (squares coloridos by type — saber where are)
    for (const auto& b : buildingSystem.buildings) {
        if (!b.active) continue;
        Color bc;
        switch (b.type) {
            case BuildingType::Ark:          bc = Color{0,255,200,255};  break;
            case BuildingType::House:        bc = Color{255,210,120,255};break;
            case BuildingType::Barracks:     bc = Color{120,200,255,255};break;
            case BuildingType::TankFactory:  bc = Color{255,140,0,255};  break;
            case BuildingType::Turret:       bc = Color{255,70,70,255};  break;
            case BuildingType::ResourceNode: bc = Color{180,255,120,255};break;
            case BuildingType::MedBay:       bc = Color{255,120,200,255};break;
            default:                         bc = Color{200,200,200,255};break;
        }
        int bx = mapX + mx(b.position.x);
        int by = mapY + my(b.position.y);
        DrawRectangle(bx - 2, by - 2, 5, 5, bc);
        DrawRectangleLines(bx - 2, by - 2, 5, 5, ColorAlpha(WHITE, 0.5f));
    }

    // Enemies (color-coded by type)
    for (const auto& enemy : enemies) {
        Color col = (enemy.type == EnemyType::Boss)    ? ORANGE   :
                    (enemy.isElite)                    ? YELLOW   :
                    (enemy.type == EnemyType::MorphX)   ? Color{0,255,255,255} :
                    (enemy.type == EnemyType::HunterDrone) ? SKYBLUE  :
                    (enemy.type == EnemyType::Kamikaze)? Color{255,80,0,255} : RED;
        int ex = mapX + mx(enemy.position.x);
        int ey = mapY + my(enemy.position.y);
        DrawCircle(ex, ey, enemy.isElite ? 4.0f : 3.0f, col);
    }

    // Viewport rectangle (what the camera sees)
    {
        float vw = (float)screenWidth  / camera.zoom * scaleX;
        float vh = (float)screenHeight / camera.zoom * scaleY;
        float vx = mapX + mx(camera.target.x) - (float)screenWidth  / (2.0f * camera.zoom) * scaleX;
        float vy = mapY + my(camera.target.y) - (float)screenHeight / (2.0f * camera.zoom) * scaleY;
        DrawRectangleLines((int)vx, (int)vy, (int)vw, (int)vh,
                           ColorAlpha({0,255,150,255}, 0.35f));
    }

    // Player (bright green, 5px)
    {
        int ppx = mapX + mx(player.position.x);
        int ppy = mapY + my(player.position.y);
        DrawCircle(ppx, ppy, 5, GREEN);
        DrawCircleLines(ppx, ppy, 8, ColorAlpha(GREEN, 0.4f));
    }

    DrawRectangleLines(mapX, mapY, mapW, mapH, ColorAlpha({0,235,255,255}, 0.4f));
    {
        Color ct = ColorAlpha({0,235,255,255}, 0.9f);
        DrawLine(mapX, mapY+4, mapX, mapY+8, ct);
        DrawLine(mapX, mapY, mapX+4, mapY, ct);
        DrawLine(mapX+mapW, mapY+4, mapX+mapW, mapY+8, ct);
        DrawLine(mapX+mapW-4, mapY, mapX+mapW, mapY, ct);
        DrawLine(mapX, mapY+mapH-4, mapX, mapY+mapH-8, ct);
        DrawLine(mapX, mapY+mapH, mapX+4, mapY+mapH, ct);
        DrawLine(mapX+mapW, mapY+mapH-4, mapX+mapW, mapY+mapH-8, ct);
        DrawLine(mapX+mapW-4, mapY+mapH, mapX+mapW, mapY+mapH, ct);
    }

    // ── Screen-edge indicators for off-screen enemies ──────────────────────
    float margin = 28.0f;
    float sw = (float)screenWidth;
    float sh = (float)screenHeight;
    for (const auto& enemy : enemies) {
        // Convert world pos to screen space
        Vector2 screenPos = GetWorldToScreen2D(enemy.position, camera);
        bool offScreen = (screenPos.x < -20 || screenPos.x > sw + 20 ||
                          screenPos.y < -20 || screenPos.y > sh + 20);
        if (!offScreen) continue;

        // Direction from screen center to enemy screen pos
        float dx = screenPos.x - sw * 0.5f;
        float dy = screenPos.y - sh * 0.5f;
        float len = std::sqrt(dx*dx + dy*dy);
        if (len < 0.001f) continue;
        dx /= len; dy /= len;

        // Clamp arrow to screen edge
        float tx = sw * 0.5f + dx * (sw * 0.5f - margin);
        float ty = sh * 0.5f + dy * (sh * 0.5f - margin);

        // Clamp to bounds
        if (tx < margin)      tx = margin;
        if (tx > sw - margin) tx = sw - margin;
        if (ty < margin)      ty = margin;
        if (ty > sh - margin) ty = sh - margin;

        Color arrowCol = (enemy.type == EnemyType::Boss)   ? ORANGE :
                         (enemy.isElite)                   ? YELLOW :
                         (enemy.type == EnemyType::Kamikaze) ? Color{255,80,0,255} :
                                                             Color{220,50,50,255};

        // Draw the small arrow triangle
        float aLen = 12.0f, aW = 7.0f;
        Vector2 tip  = {tx + dx*aLen, ty + dy*aLen};
        Vector2 perp = {-dy, dx};
        Vector2 b1   = {tx + perp.x*aW, ty + perp.y*aW};
        Vector2 b2   = {tx - perp.x*aW, ty - perp.y*aW};
        DrawTriangle(tip, b2, b1, ColorAlpha(arrowCol, 0.85f));
        DrawTriangleLines(tip, b2, b1, ColorAlpha(WHITE, 0.35f));
    }

    // ── Marcador of the portal of PHASE: seta in the edge apontando to owPortalPos ──
    // (same esquema of the indicadores of enemy outside the screen; inside the screen,
    //  um anel pulsante about the portal). E the "marcador" that the HUD sends seguir.
    if (openWorldMode && owPortalOpen) {
        Vector2 screenPos = GetWorldToScreen2D(owPortalPos, camera);
        bool offScreen = (screenPos.x < -20 || screenPos.x > sw + 20 ||
                          screenPos.y < -20 || screenPos.y > sh + 20);
        float pulse = 0.55f + 0.45f * std::sin((float)GetTime() * 5.0f);
        Color portalCol = Color{0,255,180,255};
        if (offScreen) {
            float dx = screenPos.x - sw * 0.5f;
            float dy = screenPos.y - sh * 0.5f;
            float len = std::sqrt(dx*dx + dy*dy);
            if (len > 0.001f) {
                dx /= len; dy /= len;
                float tx = sw * 0.5f + dx * (sw * 0.5f - margin);
                float ty = sh * 0.5f + dy * (sh * 0.5f - margin);
                if (tx < margin)      tx = margin;
                if (tx > sw - margin) tx = sw - margin;
                if (ty < margin)      ty = margin;
                if (ty > sh - margin) ty = sh - margin;
                float aLen = 14.0f, aW = 9.0f;
                Vector2 tip  = {tx + dx*aLen, ty + dy*aLen};
                Vector2 perp = {-dy, dx};
                Vector2 b1   = {tx + perp.x*aW, ty + perp.y*aW};
                Vector2 b2   = {tx - perp.x*aW, ty - perp.y*aW};
                DrawTriangle(tip, b2, b1, ColorAlpha(portalCol, pulse));
                DrawTriangleLines(tip, b2, b1, ColorAlpha(WHITE, 0.5f));
                // Rotulo inconfundivel: not if confunde with the indicadores of enemy.
                int lw = MeasureText("PORTAL", 11);
                float lx = tx - dx * (aLen + 6.0f) - lw * 0.5f;
                float ly = ty - dy * (aLen + 6.0f) - 6.0f;
                DrawText("PORTAL", (int)lx, (int)ly, 11, ColorAlpha(portalCol, pulse));
            }
        } else {
            // portal visible: anel pulsante about ele (not depende of zoom)
            DrawCircleLines((int)screenPos.x, (int)screenPos.y, 16.0f + pulse * 4.0f,
                            ColorAlpha(portalCol, 0.9f));
            DrawCircle((int)screenPos.x, (int)screenPos.y, 4, ColorAlpha(portalCol, pulse));
            DrawText("PORTAL", (int)screenPos.x - MeasureText("PORTAL", 11) / 2,
                     (int)screenPos.y - 30, 11, ColorAlpha(portalCol, pulse));
        }
    }

    // ── Barras of HP of the enemies (2D about the projecao 3D, type ARPG) ─────────
    for (const auto& and : enemies) {
        if (and.isDead()) continue;
        bool always = and.isBoss() || and.isElite;
        if (!always && and.health >= and.maxHealth) continue;
        if (Vector2Distance(and.position, camera.target) > 900.0f) continue;
        float hgt = always ? 88.0f : 58.0f;   // above the head of the voxel
        Vector2 sp = GetWorldToScreenEx({and.position.x, hgt, and.position.y},
                                        camera3D, screenWidth, screenHeight);
        if (sp.x < -70 || sp.x > screenWidth + 70 ||
            sp.y < -70 || sp.y > screenHeight + 70) continue;
        float pct = and.maxHealth > 0.0f
            ? std::max(0.0f, std::min(1.0f, and.health / and.maxHealth)) : 0.0f;
        int bw = always ? 50 : 40;
        int bx = (int)sp.x - bw/2, by = (int)sp.y - (always ? 9 : 7);
        Color c = pct > 0.5f ? Color{0, 220, 90, 255}
                : pct > 0.25f ? Color{255, 190, 60, 255}
                              : Color{235, 60, 40, 255};
        DrawRectangle(bx, by, bw, 6, ColorAlpha(BLACK, 0.62f));
        DrawRectangle(bx + 1, by + 1, (int)((bw - 2) * pct), 4, c);
        if (and.hitFlashTimer > 0.0f)
            DrawRectangle(bx + 1, by + 1, (int)((bw - 2) * pct), 4,
                          ColorAlpha(WHITE, and.hitFlashTimer));
        DrawRectangleLinesEx({(float)bx, (float)by, (float)bw, 6.0f}, 1,
                             ColorAlpha(WHITE, always ? 0.5f : 0.28f));
    }

    // ── Reticulo of mira + LOCK of the aim assist ────────────────────────────────
    //    4 ticks around of the cursor (without esconder the cursor of the system). With the
    //    enemy grudado pela assistencia, the cursor stays red, um anel of
    //    lock pulsa about the alvo and uma line fina liga um to the other.
    {
        Vector2 ms = GetMousePosition();
        float t3 = (float)GetTime();
        bool locked = (hudAimLock.x >= 0.0f);
        Color rcl = locked ? Color{255, 90, 70, 255} : Color{0,235,255,255};
        const int ofs = 10;
        float ta = locked ? 0.9f : 0.36f;
        DrawLine((int)ms.x - ofs, (int)ms.y, (int)ms.x - 4, (int)ms.y, ColorAlpha(rcl, ta));
        DrawLine((int)ms.x + 4, (int)ms.y, (int)ms.x + ofs, (int)ms.y, ColorAlpha(rcl, ta));
        DrawLine((int)ms.x, (int)ms.y - ofs, (int)ms.x, (int)ms.y - 4, ColorAlpha(rcl, ta));
        DrawLine((int)ms.x, (int)ms.y + 4, (int)ms.x, (int)ms.y + ofs, ColorAlpha(rcl, ta));
        if (locked) {
            Vector2 lk = GetWorldToScreenEx({hudAimLock.x, 40.0f, hudAimLock.y},
                                            camera3D, screenWidth, screenHeight);
            if (lk.x > -60 && lk.x < screenWidth + 60 && lk.y > -60 && lk.y < screenHeight + 60) {
                float pulse = 0.6f + 0.4f * std::sin(t3 * 9.0f);
                float rad = 15.0f + 3.0f * pulse;
                DrawLineEx(ms, lk, 1.2f, ColorAlpha({255,180,60,255}, 0.30f));
                DrawCircleLines((int)lk.x, (int)lk.y, rad, ColorAlpha(rcl, 0.55f + 0.45f * pulse));
                DrawCircleLines((int)lk.x, (int)lk.y, rad * 0.55f, ColorAlpha(rcl, 0.30f));
                for (int k4 = 0; k4 < 4; ++k4) {
                    float the = t3 * 3.0f + k4 * 1.5708f;
                    float ex = rad + 7.0f * pulse + 2.0f;
                    DrawLineEx({ lk.x + cosf(the) * rad, lk.y + sinf(the) * rad },
                               { lk.x + cosf(the) * ex,  lk.y + sinf(the) * ex },
                               2.0f, ColorAlpha(rcl, 0.9f));
                }
            }
        }
    }
    const Enemy* boss = nullptr;
    for (const auto& and : enemies) {
        if ((and.isBoss() || and.isFinalBoss) && !and.isDead()) { boss = &and; break; }
    }
    if (boss) {
        auto bossName = [](EnemyType t) -> const char* {
            switch (t) {
                case EnemyType::Boss:            return "COMANDANTE KRONOS";
                case EnemyType::AlienBoss:       return "MATRIARCA XENON";
                case EnemyType::OmegaBoss:       return "ALFA-OMEGA";
                case EnemyType::PoltergeistBoss: return "ESPECTRO POLTER";
                case EnemyType::ZombieLord:      return "SENHOR ZOMBIE";
                case EnemyType::VoidColossus:    return "COLOSSO DO VAZIO";
                case EnemyType::FrostWyrm:       return "VERME GLACIAL";
                case EnemyType::InfernoHerald:   return "ARAUTO DO INFERNO";
                case EnemyType::VolcanicTitan:   return "TITAN VULCANICO";
                case EnemyType::Leviathan:       return "NUCLEO KRONOS";
                default:                         return "COMANDANTE";
            }
        };

        int bw = 560, bh = 16;
        int bx = screenWidth/2 - bw/2, by = 112;
        float hpPct = boss->maxHealth > 0.0f ? boss->health / boss->maxHealth : 0.0f;
        hpPct = std::max(0.0f, std::min(1.0f, hpPct));
        float pulse = (hpPct < 0.30f)
            ? 0.55f + 0.45f * std::sin((float)GetTime() * 8.0f)
            : 1.0f;
        Color barFill = hpPct  > 0.55f ? Color{255, 70, 70, 255} :
                        hpPct  > 0.25f ? Color{255, 190, 60, 255} :
                                         Color{255, 250, 60, 255};

        DrawPanel(bx - 8, by - 22, bw + 16, bh + 28, ColorAlpha(barFill, 0.65f), 0.94f);
        DrawRectangle(bx, by, bw, bh, ColorAlpha({4,6,14,255}, 0.92f));
        DrawRectangle(bx, by, (int)(bw * hpPct), bh, ColorAlpha(barFill, 0.95f));
        DrawRectangle(bx, by, (int)(bw * hpPct), bh/3, ColorAlpha(WHITE, 0.12f));
        // ticks the cada 10%
        for (int k = 1; k < 10; ++k)
            DrawLine(bx + bw * k / 10, by, bx + bw * k / 10, by + bh,
                     ColorAlpha({0,0,0,255}, 0.55f));
        DrawRectangleLinesEx({(float)bx,(float)by,(float)bw,(float)bh}, 1.5f,
                             ColorAlpha(barFill, pulse * 0.9f));
        const char* bn = bossName(boss->type);
        int nw = MeasureText(bn, 16);
        DrawText(bn, screenWidth/2 - nw/2, by - 19, 16,
                 ColorAlpha(Color{255, 230, 230, 255}, hpPct < 0.30f ? pulse : 1.0f));
        DrawText(TextFormat("%d / %d  HP  (%.1f%%)", (int)boss->health,
                 (int)boss->maxHealth, hpPct * 100.0f),
                 screenWidth/2 - bw/2 + 4, by + bh + 4, 12, ColorAlpha(WHITE, 0.75f));
    }

    // ── Indicador direcional of DAMAGE (aponta to quem feriu the player) ───────
    if (hurtDirTimer > 0.0f) {
        float t = std::min(1.0f, hurtDirTimer / 1.15f);
        float the = 0.15f + 0.85f * t;
        Vector2 v = Vector2Normalize(hurtDir);
        float ang = ::atan2f(v.y, v.x);
        float radx = (sw * 0.5f) / std::max(0.01f, std::fabs(std::cos(ang)));
        float rady = (sh * 0.5f) / std::max(0.01f, std::fabs(std::sin(ang)));
        float radius = std::min(radx, rady);
        float px = sw * 0.5f + std::cos(ang) * (radius - 30.0f);
        float py = sh * 0.5f + std::sin(ang) * (radius - 30.0f);

        // trail of chevrons acelerando in direction to the fonte of the damage
        for (int i = 3; i >= 0; --i) {
            float back = (float)(i + 1) * 20.0f * t;
            float bx = px - std::cos(ang) * back;
            float by = py - std::sin(ang) * back;
            float bs = 4.0f + (float)i * 1.6f;
            float ba = the * (0.20f + 0.14f * (float)(3 - i));
            float perpx = -std::sin(ang), perpy = std::cos(ang);
            Vector2 tip = {bx + std::cos(ang) * bs * 2.0f, by + std::sin(ang) * bs * 2.0f};
            Vector2 mid = {bx + std::cos(ang) * bs,       by + std::sin(ang) * bs};
            Vector2 c1  = {bx + perpx * bs,               by + perpy * bs};
            Vector2 c2  = {bx - perpx * bs,               by - perpy * bs};
            DrawTriangle(tip, mid, c1, ColorAlpha(Color{255, 60, 40, 255}, ba));
            DrawTriangle(tip, mid, c2, ColorAlpha(Color{255, 60, 40, 255}, ba));
        }
        float pulse = 0.7f + 0.3f * std::sin((float)GetTime() * 12.0f);
        float size = 14.0f;
        float perpx = -std::sin(ang), perpy = std::cos(ang);
        Vector2 tip = {px + std::cos(ang) * size, py + std::sin(ang) * size};
        Vector2 w1  = {px + perpx * size * 0.7f,  py + perpy * size * 0.7f};
        Vector2 w2  = {px - perpx * size * 0.7f,  py - perpy * size * 0.7f};
        DrawTriangle(tip, w1, w2, ColorAlpha(Color{255, 90, 60, 255}, the * pulse));
        DrawTriangleLines(tip, w1, w2, ColorAlpha(WHITE, the * 0.6f));
    }

    // ── Vinheta of HP BAIXO of the player (visao vermelha in the bordas) ────────────
    if (player.health > 0.0f && player.health < player.maxHealth * 0.30f) {
        float the = (player.maxHealth * 0.30f - player.health) / (player.maxHealth * 0.30f);
        float pulse = 0.7f + 0.3f * std::sin((float)GetTime() * 5.0f);
        float edgeA = std::min(0.36f, 0.08f + the * 0.34f * pulse);
        int bandV = (int)(screenHeight * 0.11f * (0.35f + the));
        int bandH = (int)(screenWidth  * 0.11f * (0.35f + the));
        Color r0 = ColorAlpha(Color{210, 8, 18, 255}, edgeA * 0.45f);
        Color r1 = ColorAlpha(Color{210, 8, 18, 255}, edgeA);
        DrawRectangleGradientV(0, 0, screenWidth, bandV, r1, r0);
        DrawRectangleGradientV(0, screenHeight - bandV, screenWidth, bandV, r0, r1);
        DrawRectangleGradientH(0, 0, bandH, screenHeight, r1, r0);
        DrawRectangleGradientH(screenWidth - bandH, 0, bandH, screenHeight, r0, r1);
    }

    // ── Flash red of damage (screen all) — era setado mas NUNCA drawn ───
    if (hitFlashTimer > 0.0f) {
        float alpha = std::min(0.45f, hitFlashTimer * 1.5f);
        DrawRectangle(0, 0, screenWidth, screenHeight,
                      ColorAlpha(Color{255, 20, 20, 255}, alpha));
    }
    // ── Flash BRANCO of damage heavy (elite/boss): cegante short, by up ────
    if (eliteFlashTimer > 0.0f) {
        float alpha = std::min(0.55f, eliteFlashTimer * 2.2f);
        DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(WHITE, alpha));
    }

    // Tutorial and achievement overlays
    tutorial.render(screenWidth, screenHeight);
    achievements.renderPopup(screenWidth, screenHeight);
}
