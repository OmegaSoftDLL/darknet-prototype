// Game_HUD.cpp — HUD e overlays 2D: recursos, paineis (personagem/objetivos/skills),
// quests, minimapa e os helpers de painel/barra. Extraido de Game.cpp. Mesma classe Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
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
    // HUD de recursos coletados (madeira/pedra/ferro/prata/ouro)
    if (openWorldMode) drawResourceHUD();

    // ── HUD do Motor de Evolucao: Nivel de Ameaca + Mutador ativo ─────────────
    {
        int hx = screenWidth - 224, hy = 56;
        DrawRectangle(hx, hy, 214, activeMutator == WorldMutator::None ? 22 : 40,
                      ColorAlpha(BLACK, 0.55f));
        DrawText(TextFormat("AMEACA  Lv %d", threatLevel), hx + 8, hy + 4, 14,
                 Color{255, 90, 90, 255});
        if (activeMutator != WorldMutator::None) {
            float pulse = 0.6f + 0.4f * std::sin((float)GetTime() * 3.0f);
            DrawText(TextFormat("%s", mutatorName(activeMutator)), hx + 8, hy + 22, 12,
                     ColorAlpha(Color{180, 120, 255, 255}, pulse));
            // barra de tempo restante do mutador
            float frac = 1.0f - (mutatorTimer / mutatorDuration);
            DrawRectangle(hx + 150, hy + 24, (int)(56 * frac), 5, Color{180,120,255,200});
        }
    }

    // Indicador de ZONA SEGURA no HUD (centro-topo)
    if (openWorldMode && inSafeZone(player.position)) {
        const char* si = "ZONA SEGURA - prepare-se e construa sua base";
        int siw = MeasureText(si, 16);
        float pulse = 0.6f + 0.4f * std::sin((float)GetTime() * 3.0f);
        DrawRectangle(screenWidth/2 - siw/2 - 10, 30, siw + 20, 24, ColorAlpha(BLACK, 0.6f));
        DrawRectangleLinesEx({(float)(screenWidth/2 - siw/2 - 10), 30, (float)(siw + 20), 24},
                             1.0f, ColorAlpha(Color{0,230,160,255}, 0.7f));
        DrawText(si, screenWidth/2 - siw/2, 34, 16, ColorAlpha(Color{0,255,180,255}, pulse));
    }
    // Progresso da FASE (abates ate o portal) — objetivo sempre visivel
    if (openWorldMode) {
        const char* pt = owPortalOpen
            ? "PORTAL ABERTO - siga o marcador e pressione [E]"
            : TextFormat("FASE %d  -  abates ate o portal: %d/%d",
                         owPhase + 1, owPhaseKills, owPhaseGoal);
        int pw = MeasureText(pt, 13);
        DrawRectangle(screenWidth/2 - pw/2 - 8, 82, pw + 16, 20, ColorAlpha(BLACK, 0.55f));
        DrawText(pt, screenWidth/2 - pw/2, 85, 13,
                 owPortalOpen ? Color{0,255,180,255} : Color{200,206,220,255});
    }
    drawStoryBanner();
    drawPhaseFade();
    drawPlayerSpeech();
    if (paused) drawPauseMenu();
    if (showLevelUpScreen)   drawLevelUpScreen();
    if (showEvolutionScreen) drawEvolutionScreen();
    if (gameWon)             drawVictoryScreen();

    // Shop overlay (fullscreen, drawn last so it's on top)
    if (shopSystem.open) {
        shopSystem.render(player.credits, screenWidth, screenHeight);
        drawPremiumStore();   // aba premium por cima da loja comum (tecla TAB)
    }

    // Crafting overlay
    if (craftingSystem.open) {
        craftingSystem.render(player.inventory, screenWidth, screenHeight);
    }

    // Grupo / aliança (indicador sempre visível + painel com tecla O)
    drawPartyPanel();
}

void Game::drawUI() const {
    Color C_cyan  = {0, 210, 255, 255};
    Color C_red   = {220, 30, 30, 255};
    Color C_gold  = {255, 190, 0, 255};
    Color C_green = {0, 210, 80, 255};

    // ── Top bar ───────────────────────────────────────────────────────────────
    DrawRectangle(0, 0, screenWidth, 26, ColorAlpha(BLACK, 0.75f));
    DrawLine(0, 26, screenWidth, 26, ColorAlpha(C_cyan, 0.3f));

    // Chapter + zone
    ZoneInfo info = getZoneInfo(currentZone);
    DrawText(TextFormat("CAP.%d | %s", storyChapter, info.name.c_str()),
             10, 5, 14, ColorAlpha(C_cyan, 0.9f));

    // Center — kills + credits + gems + time (compact, stays inside top bar)
    DrawText(TextFormat("ABATIDOS: %d   $%d   %.0fs", enemiesKilled, player.credits, sessionTime),
             screenWidth/2 - 175, 5, 13, ColorAlpha(WHITE, 0.55f));
    // Gems premium (saldo do backend) — destaque em magenta
    DrawText(TextFormat("GEMS %d", store.gems()),
             screenWidth/2 + 95, 5, 13, Color{225,120,255,255});

    // Right of top bar — compact hint only (does NOT overflow into quest HUD zone x>940)
    DrawText("[Q]Cura [I]Itens [B]Construir [C]Forja [E/TAB]Loja [O]Grupo [J]Miss",
             screenWidth - 470, 5, 10, ColorAlpha(C_cyan, 0.55f));

    // ── PAINEL DE CONTROLES (lado esquerdo) — visivel quando o bot esta off ────
    if (!botController.active && !showLevelUpScreen && !showEvolutionScreen &&
        !shopSystem.open && !craftingSystem.open) {
        int cx = 8, cy = 300, cw = 188;
        struct Ctl { const char* key; const char* label; Color col; };
        static const Ctl ctls[] = {
            {"Q",      "Cura (pocao + regen)",   {0,255,120,255}},
            {"I",      "Inventario / Equipar",   {0,230,255,255}},
            {"O",      "Grupo / Aliança",        {120,255,160,255}},
            {"B",      "Construir base/tanques", {120,200,255,255}},
            {"C",      "Forja de armas/armad.",  {255,200,100,255}},
            {"TAB/E",  "Loja de itens",          {255,230,120,255}},
            {"F2 F3 F4","Invocar aliados",        {120,255,160,255}},
            {"SHIFT+arr","Selecionar unidades",   {120,255,160,255}},
            {"Dir.",   "Mover unidades selec.",  {120,255,160,255}},
            {"J",      "Missoes",                {200,160,255,255}},
            {"L / K",  "Pontos / Evolucao",      {255,215,0,255}},
        };
        int n = (int)(sizeof(ctls)/sizeof(ctls[0]));
        int ch = 18 * n + 24;
        DrawRectangle(cx, cy, cw, ch, ColorAlpha(BLACK, 0.62f));
        DrawRectangleLinesEx({(float)cx,(float)cy,(float)cw,(float)ch}, 1.0f,
                             ColorAlpha(C_cyan, 0.5f));
        DrawText("CONTROLES", cx + 8, cy + 6, 11, ColorAlpha(C_cyan, 0.9f));
        for (int i = 0; i < n; i++) {
            int ly = cy + 22 + i * 18;
            DrawText(ctls[i].key, cx + 8, ly, 11, ctls[i].col);
            DrawText(ctls[i].label, cx + 74, ly, 10, ColorAlpha(WHITE, 0.78f));
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

    // ── MINIMAP (bottom-right) — hidden when fullscreen overlay active ─────────
    if (!anyOverlay) {
        drawMinimap();
    }

    // ── Zone transition text ──────────────────────────────────────────────────
    drawZoneInfo();

    // ── Quest HUD (top-right, below top bar — only when no fullscreen overlay) ─
    if (!anyOverlay) {
        drawQuestHUD();
    }

    // ── Level Up announcement ─────────────────────────────────────────────────
    if (player.leveledUp) {
        float a = std::min(player.levelUpTimer / 2.5f, 1.0f);
        // Main banner
        int bw = 420, bh = 70;
        int bx = screenWidth/2 - bw/2, by = screenHeight/2 - 100;
        DrawPanel(bx, by, bw, bh, C_gold, 0.92f);
        DrawText(TextFormat("NIVEL %d ALCANCADO!", player.level),
                 bx + bw/2 - MeasureText(TextFormat("NIVEL %d ALCANCADO!", player.level), 28)/2,
                 by + 8, 28, ColorAlpha(C_gold, a));
        if (!player.lastPassive.empty()) {
            DrawText(player.lastPassive.c_str(),
                     bx + bw/2 - MeasureText(player.lastPassive.c_str(), 14)/2,
                     by + 44, 14, ColorAlpha({0,255,180,255}, a));
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
        npcs[nearNpcIndex].showDialog(dialogLine);   // balão do diálogo (vale p/ 3D e 2D)
        DrawText("[E] Continuar  [ESC] Fechar", 10, screenHeight - 36, 14,
                 ColorAlpha(WHITE, 0.7f));
    }

    // ── Chat Input Box ────────────────────────────────────────────────────────
    if (chatActive) {
        int boxW = 500, boxH = 36;
        int boxX = screenWidth/2 - boxW/2;
        int boxY = screenHeight - 120; // acima da barra de habilidades
        
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
    Color C_cyan  = {0,210,255,255};
    Color C_green = {0,210,80, 255};
    Color C_gold  = {255,190,0,255};

    // Painel reorganizado — linhas bem separadas, SEM sobreposicao.
    bool hasPts = (pendingLevelUps > 0 || pendingEvolutions > 0);
    int panX = 8, panW = 322;
    int panH = hasPts ? 196 : 172;
    int panY = screenHeight - panH - 8;
    DrawPanel(panX, panY, panW, panH, C_cyan);

    int y = panY + 8;
    // ── Linha 1: NIVEL + dificuldade + creditos ──
    DrawText(TextFormat("NIVEL %d", player.level), panX+8, y, 15, C_gold);
    {
        const DifficultySettings& diff = getDifficulty();
        int bx = panX + 8 + MeasureText("NIVEL 00", 15) + 10;
        int bw = MeasureText(diff.name, 11) + 10;
        DrawRectangle(bx, y-1, bw, 17, ColorAlpha(diff.labelColor, 0.20f));
        DrawText(diff.name, bx+5, y+1, 11, ColorAlpha(diff.labelColor, 0.95f));
    }
    DrawText(TextFormat("$ %d", player.credits), panX+panW - MeasureText(TextFormat("$ %d", player.credits),15) - 10, y, 15, C_gold);

    // ── Linha 2: DMG / DEF / VEL (linha propria) ──
    y += 22;
    DrawText(TextFormat("DANO %.0f    DEF %.0f%%    VEL %.0f",
             player.getEffectiveDamage(), player.defense, player.speed),
             panX+8, y, 13, ColorAlpha(WHITE, 0.85f));

    // ── Linha 3: HP ──
    y += 22;
    {
        float pct = player.health / player.maxHealth;
        Color col = pct > 0.5f ? C_green : pct > 0.25f ? Color{255,180,0,255} : Color{220,30,30,255};
        DrawText("HP", panX+8, y, 13, ColorAlpha(col, 0.9f));
        DrawBarH(panX+34, y, 280, 15, pct, col, ColorAlpha(BLACK, 0.5f));
        DrawText(TextFormat("%d/%d", (int)player.health, (int)player.maxHealth),
                 panX+40, y+2, 12, ColorAlpha(WHITE, 0.95f));
        // Escudo/Sobrecarga como rotulo curto no fim da barra de HP
        if (player.isShielded())
            DrawText("[BARREIRA]", panX+232, y+2, 11, Color{0,210,255,255});
        else if (player.isOverloaded())
            DrawText("[SOBRECGA]", panX+232, y+2, 11, Color{255,150,0,255});
    }

    // ── Linha 4: XP ──
    y += 22;
    {
        float pct = (float)player.xp / player.xpToNextLevel;
        DrawText("XP", panX+8, y, 13, Color{60,140,255,255});
        DrawBarH(panX+34, y, 280, 13, pct, Color{60,140,255,255}, ColorAlpha(BLACK,0.5f));
        DrawText(TextFormat("%d / %d", player.xp, player.xpToNextLevel),
                 panX+40, y+1, 11, ColorAlpha(WHITE,0.85f));
    }

    // ── Linha 5 (opcional): pontos acumulados ──
    if (hasPts) {
        y += 22;
        float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 6.0f);
        std::string txt;
        if (pendingLevelUps > 0)  txt += TextFormat("[L] %d ponto(s)  ", pendingLevelUps);
        if (pendingEvolutions > 0) txt += TextFormat("[K] %d evolucao", pendingEvolutions);
        DrawRectangle(panX+8, y-1, panW-16, 17, ColorAlpha(Color{90,60,0,255}, 0.5f * pulse));
        DrawText(txt.c_str(), panX+12, y+1, 12, ColorAlpha(Color{255,215,0,255}, 0.6f+0.4f*pulse));
    }

    // ── Separador + equipamento ──
    y += 24;
    DrawLine(panX+8, y, panX+panW-8, y, ColorAlpha(C_cyan, 0.25f));
    y += 4;
    if (!player.equippedWeapon.isEmpty())
        DrawText(TextFormat("Arma: %s", player.equippedWeapon.name.c_str()),
                 panX+8, y, 12, player.equippedWeapon.color);
    if (!player.equippedArmor.isEmpty())
        DrawText(TextFormat("Armadura: %s", player.equippedArmor.name.c_str()),
                 panX+8, y+15, 12, player.equippedArmor.color);

    // ── Proxima passiva (rodape) ──
    const char* nextPassive = "";
    if      (player.level < 3)  nextPassive = "Prox: Lv3 Blindagem";
    else if (player.level < 5)  nextPassive = "Prox: Lv5 Nucleo de Combate";
    else if (player.level < 7)  nextPassive = "Prox: Lv7 Amplificador";
    else if (player.level < 10) nextPassive = "Prox: Lv10 Protocolo IRON-VIII";
    else if (player.level < 15) nextPassive = "Prox: Lv15 Executor Lendario";
    if (nextPassive[0] != '\0')
        DrawText(nextPassive, panX+8, panY+panH-17, 11, ColorAlpha(C_gold, 0.7f));
}

void Game::drawObjectivesPanel() const {
    Color C_cyan = {0,210,255,255};
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
            DrawText(TextFormat("BOSS em %d abates", killsNeeded),
                     panX+10, dy+4, 12, ColorAlpha(C_red, 0.85f));
        }
    } else {
        DrawText(">> BOSS ATIVO! <<", panX+10, dy+4, 14, C_red);
    }
}

void Game::drawSkillsPanel() const {
    Color C_cyan  = {0, 210, 255, 255};
    Color C_gold  = {255, 190, 0,  255};
    Color C_green = {0,  210, 80,  255};

    int count  = (int)player.skills.size();
    int slotW  = 90, slotH = 84;
    int totalW = count * slotW;
    int baseX  = screenWidth/2 - totalW/2;
    int baseY  = screenHeight - slotH - 8;

    // Background bar
    DrawRectangle(baseX-8, baseY-4, totalW+16, slotH+12, ColorAlpha(BLACK, 0.65f));
    DrawLine(baseX-8, baseY-4, baseX+totalW+8, baseY-4, ColorAlpha(C_cyan, 0.3f));

    for (int i = 0; i < count; ++i) {
        const Skill& s = player.skills[i];
        int sx = baseX + i * slotW;

        // Slot background
        bool ready = s.isReady();
        Color border = ready ? C_green : ColorAlpha(WHITE, 0.2f);
        DrawPanel(sx, baseY, slotW-4, slotH, border, 0.75f);

        // Key number
        DrawText(TextFormat("%d", i+1), sx+4, baseY+4, 15, C_gold);

        // Skill name (wrapped)
        DrawText(s.name.c_str(), sx+4, baseY+22, 12, ColorAlpha(WHITE, 0.9f));

        // Ready / cooldown
        if (ready) {
            DrawText("PRONTO", sx+4, baseY+40, 11, C_green);
            // Subtle glow
            DrawRectangle(sx, baseY+slotH-6, slotW-4, 6, ColorAlpha(C_green, 0.35f));
        } else {
            float pct = 1.0f - s.cooldownPercent();
            DrawBarH(sx, baseY+slotH-6, slotW-4, 6, pct,
                     {60,140,255,255}, ColorAlpha(BLACK, 0.5f));
            // Cooldown overlay
            DrawRectangle(sx, baseY, slotW-4,
                          (int)((slotH) * s.cooldownPercent()),
                          ColorAlpha(BLACK, 0.55f));
            DrawText(TextFormat("%.1fs", s.currentCooldown),
                     sx+4, baseY+40, 13, ColorAlpha({255,140,0,255}, 0.9f));
        }

        // Damage hint
        if (s.damage > 0)
            DrawText(TextFormat("DMG:%.0f", s.damage), sx+4, baseY+58, 10,
                     ColorAlpha({255,80,80,255}, 0.7f));
    }
}

void Game::drawZoneInfo() const {
    ZoneInfo info = getZoneInfo(currentZone);
    // Zone name is shown in the top-bar left ("CAP.X | ZoneName") — no duplicate here.

    if (zoneNameTimer > 0.0f) {
        float alpha = std::min(zoneNameTimer, 1.0f);
        Color c = ColorAlpha(info.portalColor, alpha);
        DrawText(info.name.c_str(), screenWidth/2 - MeasureText(info.name.c_str(), 40)/2,
                 screenHeight/2 - 40, 40, c);
        DrawText(info.description.c_str(),
                 screenWidth/2 - MeasureText(info.description.c_str(), 20)/2,
                 screenHeight/2 + 10, 20, ColorAlpha(WHITE, alpha));
    }
}

void Game::drawQuestHUD() const {
    // Always-visible active mission panel — top-right corner
    Color C_gold  = {255,200,0,255};
    Color C_cyan  = {0,210,255,255};
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
    // py=70: starts below the top bar (0–26) + gap — no overlap with hint text
    // py=102: abaixo do HUD de AMEACA/mutador (y=56, ate 40px de altura). Com 70
    // o painel nascia POR BAIXO dele e os dois textos se sobrepunham na tela.
    int px = screenWidth - panW - 8, py = 102;

    // Panel background
    DrawRectangle(px, py, panW, panH, ColorAlpha(BLACK, 0.82f));
    DrawRectangleLinesEx({(float)px,(float)py,(float)panW,(float)panH}, 1.5f,
                         ColorAlpha(C_gold, 0.75f));

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
        // por VALOR: getProgressText() devolve std::string, entao o .c_str() de um
        // temporario ja estava liberado quando o DrawText lia (texto virava lixo).
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
    DrawRectangle(x - 10, y - 10, 380, 40 + rows * 55, ColorAlpha(BLACK, 0.9f));
    DrawRectangleLinesEx({(float)x-10, (float)y-10, 380, (float)(40+rows*55)}, 1, DARKGRAY);
    DrawText("DIARIO DE MISSOES (J):", x, y, 18, GOLD);

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

void Game::drawSkills() const {
    int baseX = screenWidth - 530;
    int baseY = screenHeight - 92;

    for (int i = 0; i < (int)player.skills.size(); ++i) {
        const Skill& s = player.skills[i];
        int x = baseX + i * 88;
        Rectangle rect = {(float)x, (float)baseY, 80.0f, 80.0f};

        DrawRectangleRec(rect, ColorAlpha(DARKGRAY, 0.8f));
        DrawRectangleLinesEx(rect, 2, s.isReady() ? GREEN : Color{100,100,100,255});

        DrawText(TextFormat("%d", i + 1), x + 4, baseY + 4, 16, WHITE);
        DrawText(s.name.c_str(), x + 3, baseY + 42, 11, WHITE);

        if (!s.isReady()) {
            float pct = s.cooldownPercent();
            DrawRectangle(x, (int)(baseY + 80 * (1.0f - pct)), 80, (int)(80 * pct),
                          ColorAlpha(BLACK, 0.65f));
            DrawText(TextFormat("%.1fs", s.currentCooldown), x + 22, baseY + 30, 14, ORANGE);
        } else {
            DrawText("PRONTO", x + 12, baseY + 28, 12, GREEN);
        }
    }
}

void Game::drawMinimap() const {
    // ── Minimap — bottom-right, above skills bar ──────────────────────────────
    //   Skills bar is at screenHeight - 92 → minimap sits just above it
    int mapW = 160;
    int mapH = 130;
    int mapX = screenWidth - mapW - 8;
    int mapY = screenHeight - mapH - 100;  // above skills panel

    float worldW = (float)(tilemap.width  * Tilemap::tileSize);
    float worldH = (float)(tilemap.height * Tilemap::tileSize);
    float scaleX = mapW / worldW;
    float scaleY = mapH / worldH;

    // Panel background with cyberpunk border
    DrawRectangle(mapX - 2, mapY - 14, mapW + 4, mapH + 16, ColorAlpha(BLACK, 0.82f));
    DrawRectangleLines(mapX - 2, mapY - 14, mapW + 4, mapH + 16,
                       ColorAlpha({0,210,255,255}, 0.55f));
    // Title
    DrawText("RADAR", mapX, mapY - 12, 10, ColorAlpha({0,210,255,255}, 0.75f));

    // Clipping region background
    DrawRectangle(mapX, mapY, mapW, mapH, ColorAlpha({5,10,20,255}, 0.9f));

    // Open world region grid overlay
    if (openWorldMode && !worldRegions.empty()) {
        float rScale = (float)mapW / worldW;
        // Draw each region
        for (const auto& r : worldRegions) {
            int rx = mapX + (int)(r.bounds.x * rScale);
            int ry = mapY + (int)(r.bounds.y * rScale);
            int rw = std::max(1, (int)(r.bounds.width  * rScale));
            int rh = std::max(1, (int)(r.bounds.height * rScale));
            Color col = r.discovered ? r.mapColor : Color{25,25,30,255};
            DrawRectangle(rx, ry, rw, rh, ColorAlpha(col, r.discovered ? 0.45f : 0.25f));
            DrawRectangleLinesEx({(float)rx,(float)ry,(float)rw,(float)rh}, 0.8f,
                                  ColorAlpha(WHITE, 0.15f));
            if (r.discovered) {
                int tw = MeasureText(r.name.c_str(), 6);
                // Clamp text inside minimap
                int tx2 = rx + rw/2 - tw/2;
                int ty2 = ry + rh/2 - 3;
                if (tx2 >= mapX && tx2 + tw <= mapX + mapW && ty2 >= mapY && ty2 + 6 <= mapY + mapH)
                    DrawText(r.name.c_str(), tx2, ty2, 6, ColorAlpha(WHITE, 0.7f));
            }
        }
        // Region borders
        float szPx = worldRegions[0].bounds.width * rScale;
        for (int c = 1; c < Tilemap::OW_COLS; ++c)
            DrawLine(mapX + (int)(c * szPx), mapY, mapX + (int)(c * szPx), mapY + mapH,
                     ColorAlpha({0,210,255,255}, 0.3f));
        for (int r2 = 1; r2 < Tilemap::OW_ROWS; ++r2)
            DrawLine(mapX, mapY + (int)(r2 * szPx), mapX + mapW, mapY + (int)(r2 * szPx),
                     ColorAlpha({0,210,255,255}, 0.3f));
        DrawText("MAPA", mapX + 2, mapY - 12, 10, ColorAlpha({0,210,255,255}, 0.75f));
    }

    // Grid lines (faint)
    for (int gx = 0; gx <= 4; ++gx) {
        int lx = mapX + gx * mapW / 4;
        DrawLine(lx, mapY, lx, mapY + mapH, ColorAlpha({0,210,255,255}, 0.08f));
    }
    for (int gy = 0; gy <= 4; ++gy) {
        int ly = mapY + gy * mapH / 4;
        DrawLine(mapX, ly, mapX + mapW, ly, ColorAlpha({0,210,255,255}, 0.08f));
    }

    // Portals on minimap
    for (const auto& portal : tilemap.portals) {
        int px = mapX + (int)(portal.position.x * scaleX);
        int py = mapY + (int)(portal.position.y * scaleY);
        DrawCircle(px, py, 4, portal.color);
        DrawCircleLines(px, py, 6, ColorAlpha(portal.color, 0.4f));
    }

    // Portal de FASE aberto: blip pulsante no radar. O HUD manda "seguir o
    // marcador" — sem isto nao existia marcador nenhum.
    if (openWorldMode && owPortalOpen) {
        int px = mapX + (int)(owPortalPos.x * scaleX);
        int py = mapY + (int)(owPortalPos.y * scaleY);
        float pulse = 0.55f + 0.45f * std::sin((float)GetTime() * 5.0f);
        DrawCircle(px, py, 5, ColorAlpha(Color{0,255,180,255}, pulse));
        DrawCircleLines(px, py, 8, ColorAlpha(Color{0,255,180,255}, 0.5f));
    }

    // NPCs
    for (const auto& npc : npcs) {
        DrawCircle(mapX + (int)(npc.position.x * scaleX),
                   mapY + (int)(npc.position.y * scaleY), 3, BLUE);
    }

    // Construcoes do jogador (quadrados coloridos por tipo — saber onde estao)
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
        int bx = mapX + (int)(b.position.x * scaleX);
        int by = mapY + (int)(b.position.y * scaleY);
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
        int ex = mapX + (int)(enemy.position.x * scaleX);
        int ey = mapY + (int)(enemy.position.y * scaleY);
        DrawCircle(ex, ey, enemy.isElite ? 4.0f : 3.0f, col);
    }

    // Viewport rectangle (what the camera sees)
    {
        float vw = (float)screenWidth  / camera.zoom / worldW * mapW;
        float vh = (float)screenHeight / camera.zoom / worldH * mapH;
        float vx = mapX + (camera.target.x - screenWidth /(2.0f*camera.zoom)) * scaleX;
        float vy = mapY + (camera.target.y - screenHeight/(2.0f*camera.zoom)) * scaleY;
        DrawRectangleLines((int)vx, (int)vy, (int)vw, (int)vh,
                           ColorAlpha({0,255,150,255}, 0.35f));
    }

    // Player (bright green, 5px)
    {
        int ppx = mapX + (int)(player.position.x * scaleX);
        int ppy = mapY + (int)(player.position.y * scaleY);
        DrawCircle(ppx, ppy, 5, GREEN);
        DrawCircleLines(ppx, ppy, 8, ColorAlpha(GREEN, 0.4f));
    }

    DrawRectangleLines(mapX, mapY, mapW, mapH, ColorAlpha({0,210,255,255}, 0.4f));

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

        // Draw a small arrow triangle
        float aLen = 12.0f, aW = 7.0f;
        Vector2 tip  = {tx + dx*aLen, ty + dy*aLen};
        Vector2 perp = {-dy, dx};
        Vector2 b1   = {tx + perp.x*aW, ty + perp.y*aW};
        Vector2 b2   = {tx - perp.x*aW, ty - perp.y*aW};
        DrawTriangle(tip, b2, b1, ColorAlpha(arrowCol, 0.85f));
        DrawTriangleLines(tip, b2, b1, ColorAlpha(WHITE, 0.35f));
    }

    // ── Marcador do portal de FASE: seta na borda apontando para owPortalPos ──
    // (mesmo esquema dos indicadores de inimigo fora da tela; dentro da tela,
    //  um anel pulsante sobre o portal). E o "marcador" que o HUD manda seguir.
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
                // Rotulo inconfundivel: nao se confunde com os indicadores de inimigo.
                int lw = MeasureText("PORTAL", 11);
                float lx = tx - dx * (aLen + 6.0f) - lw * 0.5f;
                float ly = ty - dy * (aLen + 6.0f) - 6.0f;
                DrawText("PORTAL", (int)lx, (int)ly, 11, ColorAlpha(portalCol, pulse));
            }
        } else {
            // portal visivel: anel pulsante sobre ele (nao depende de zoom)
            DrawCircleLines((int)screenPos.x, (int)screenPos.y, 16.0f + pulse * 4.0f,
                            ColorAlpha(portalCol, 0.9f));
            DrawCircle((int)screenPos.x, (int)screenPos.y, 4, ColorAlpha(portalCol, pulse));
            DrawText("PORTAL", (int)screenPos.x - MeasureText("PORTAL", 11) / 2,
                     (int)screenPos.y - 30, 11, ColorAlpha(portalCol, pulse));
        }
    }
}
