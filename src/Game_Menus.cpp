// Game_Menus.cpp — telas of menu main, pausa, level-up and evolution.
// Modularizacao: extraido of Game.cpp (that was with ~7000 lines). Same class Game.
#include "Game.h"
#include "Effects.h"
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <algorithm>
void Game::drawMainMenu() const {
    BeginTextureMode(gameTarget.get());
    ClearBackground({2, 4, 8, 255});

    float t = (float)GetTime();
    int   cx = screenWidth  / 2;
    int   cy = screenHeight / 2;

    // ── Fundo: gradiente deep (neon dark) ──────────────────────────────
    for (int i = 0; i < 64; ++i) {
        float v = (float)i / 63.0f;
        Color c = { (unsigned char)(6 + (int)(4.0f * v)),
                    (unsigned char)(7 + (int)(3.0f * v)),
                    (unsigned char)(14 + (int)(12.0f * v)), 255 };
        DrawRectangle(0, (int)(v * (float)screenHeight), screenWidth,
                      screenHeight / 64 + 1, c);
    }

    // ── Moldura of camera (cantos neon) ─────────────────────────────────────
    Color frm = ColorAlpha({0,235,255,255}, 0.16f);
    DrawRectangle(6, 6, 26, 3, frm);  DrawRectangle(6, 6, 3, 26, frm);
    DrawRectangle(screenWidth-32, 6, 26, 3, frm);  DrawRectangle(screenWidth-9, 6, 3, 26, frm);
    DrawRectangle(6, screenHeight-9, 26, 3, frm);  DrawRectangle(6, screenHeight-32, 3, 26, frm);
    DrawRectangle(screenWidth-32, screenHeight-9, 26, 3, frm);
    DrawRectangle(screenWidth-9, screenHeight-32, 3, 26, frm);

    // ── Contadores HUD of fundo (decorativos) ───────────────────────────────
    DrawText("KRN.LOG // 2047", 18, 14, 12, ColorAlpha({120,160,200,255}, 0.45f));
    DrawText("NEXUS ONLINE", screenWidth - MeasureText("NEXUS ONLINE", 12) - 18, 14, 12,
             ColorAlpha({0,235,255,255}, 0.45f + 0.2f * std::sin(t * 1.1f)));

    // ── Grade cibernetica animada (fina, discreta) ──────────────────────────
    for (int i = 0; i < screenWidth; i += 48) {
        float pulse = 0.03f + 0.025f * std::sin(t * 0.6f + i * 0.01f);
        DrawLine(i, 0, i, screenHeight, ColorAlpha({0,200,255,255}, pulse));
    }
    for (int j = 0; j < screenHeight; j += 32) {
        float pulse = 0.03f + 0.02f * std::sin(t * 0.4f + j * 0.015f);
        DrawLine(0, j, screenWidth, j, ColorAlpha({0,200,255,255}, pulse));
    }

    // ── Varredura horizontal descendo (scanline) ────────────────────────────
    int scanY = (int)(std::fmod(t * 160.0f, (float)screenHeight));
    DrawRectangle(0, scanY, screenWidth, 2, ColorAlpha({0,235,255,255}, 0.10f));
    DrawRectangle(0, scanY + 2, screenWidth, 10, ColorAlpha({0,235,255,255}, 0.03f));

    // ── IRON-VIII SKULL (left, outside the title) ──────────────────────────
    int sx = cx - 440;
    int sy = cy - 165;
    DrawEllipse(sx, sy, 75, 90, {18, 22, 36, 255});
    DrawEllipse(sx, sy, 73, 88, {7, 10, 20, 255});
    DrawEllipse(sx-40, sy+30, 22, 16, {18, 22, 36, 255});
    DrawEllipse(sx+40, sy+30, 22, 16, {18, 22, 36, 255});
    DrawRectangle(sx-38, sy+50, 76, 40, {9, 13, 26, 255});
    DrawEllipse(sx, sy+90, 30, 15, {9, 13, 26, 255});
    for (int ti = 0; ti < 6; ++ti)
        DrawRectangle(sx-28+ti*10, sy+68, 6, 18, {16, 22, 40, 255});
    DrawEllipse(sx-24, sy-10, 21, 16, BLACK);
    DrawEllipse(sx+24, sy-10, 21, 16, BLACK);
    float eyeFlicker  = 0.75f + 0.25f * std::sin(t * 3.5f);
    float eyeFlicker2 = 0.75f + 0.25f * std::sin(t * 3.5f + 0.8f);
    DrawGlowCircle({(float)(sx-24), (float)(sy-10)}, 12.0f, {255,40,60,255}, 10.0f);
    DrawCircleV({(float)(sx-24),(float)(sy-10)}, 8.0f, ColorAlpha({255,45,70,255}, eyeFlicker));
    DrawCircleV({(float)(sx-24),(float)(sy-10)}, 3.5f, WHITE);
    DrawGlowCircle({(float)(sx+24), (float)(sy-10)}, 12.0f, {255,40,60,255}, 10.0f);
    DrawCircleV({(float)(sx+24),(float)(sy-10)}, 8.0f, ColorAlpha({255,45,70,255}, eyeFlicker2));
    DrawCircleV({(float)(sx+24),(float)(sy-10)}, 3.5f, WHITE);
    DrawRectangle(sx-18, sy+100, 12, 30, {13, 19, 34, 255});
    DrawRectangle(sx+6,  sy+100, 12, 30, {13, 19, 34, 255});
    DrawEllipse(sx-20, sy-40, 8, 5, ColorAlpha({0,220,255,255}, 0.20f));
    float eyeGlow = 0.08f + 0.05f * std::sin(t * 3.5f);
    DrawCircleV({(float)(sx), (float)(sy)}, 120.0f, ColorAlpha({255,40,60,255}, eyeGlow * 0.25f));

    // ── TITLE: DARKNET with extrusao + aura neon ────────────────────────────
    float titlePulse = 0.86f + 0.14f * std::sin(t * 1.4f);
    int   titleFont  = 92;
    int   titleW     = MeasureText("DARKNET", titleFont);
    int   titleX     = cx - titleW / 2;
    int   titleY     = cy - 216;
    for (int d = 9; d >= 2; d -= 3)
        DrawText("DARKNET", titleX + d, titleY + d, titleFont,
                 ColorAlpha({0,35,80,255}, 0.55f - d * 0.03f));
    DrawText("DARKNET", titleX + 3, titleY + 3, titleFont,
             ColorAlpha({0,140,255,255}, 0.30f * titlePulse));
    DrawText("DARKNET", titleX, titleY, titleFont,
             ColorAlpha({185,245,255,255}, titlePulse));

    // Sub-title in red neon
    int subFont = 26;
    int subW    = MeasureText("GUERRA CONTRA KRONOS", subFont);
    DrawText("GUERRA CONTRA KRONOS", cx - subW/2, titleY + titleFont + 12, subFont,
             ColorAlpha({255,70,90,255}, titlePulse));
    {
        int tinyX = cx - MeasureText("CLASSIFIED // OPERACAO NEXUS", 11)/2;
        DrawText("CLASSIFIED // OPERACAO NEXUS", tinyX, titleY + titleFont + 44, 11,
                 ColorAlpha({140,170,205,255}, 0.55f));
    }

    // Separador with pontas angulares (canto chanfrado)
    int sepY = titleY + titleFont + 64;
    DrawLine(cx - 240, sepY, cx + 240, sepY, ColorAlpha({0,235,255,255}, 0.45f));
    DrawLine(cx - 246, sepY - 5, cx - 246, sepY + 5, ColorAlpha({0,235,255,255}, 0.6f));
    DrawLine(cx + 240, sepY, cx + 246, sepY + 5, ColorAlpha({0,235,255,255}, 0.75f + 0.1f*t));
    DrawLine(cx - 240, sepY, cx - 246, sepY + 5, ColorAlpha({0,235,255,255}, 0.75f + 0.1f*t));
    DrawRectangle(cx - 246, sepY - 5, 2, 10, ColorAlpha({255,180,40,255}, 0.9f));

    const char* tagline = "2047 - KRONOS domina. O NEXUS and the last esperanca.";
    int tagW = MeasureText(tagline, 17);
    DrawText(tagline, cx - tagW / 2, sepY + 12, 17, ColorAlpha({175,195,220,255}, 0.78f));

    // ── BOTOES (painel angular + trilho neon + badge of key) ──────────────
    bool hasSave = SaveManager::exists();
    Vector2 mouse = virtualizeMousePos(GetMousePosition());
    int bw = 392;
    auto isHover = [&](int y) -> bool {
        return mouse.x >= cx-bw/2 && mouse.x <= cx+bw/2 &&
               mouse.y >= y-3     && mouse.y <= y+33;
    };
    auto drawMenuBtn = [&](int y, const char* key, const char* label, bool primary) {
        int bh = 38;
        int bx = cx - bw/2;
        bool hover = isHover(y);
        Color neon  = {0, 235, 255, 255};
        Color amber = {255, 180, 40, 255};
        Color brd = primary && (hover || primary)
                      ? amber
                      : (hover ? neon : ColorAlpha({0, 130, 180, 255}, 0.9f));
        Color bg = (hover || primary) ? ColorAlpha({8, 17, 34, 255}, 0.92f)
                                      : ColorAlpha({6, 12, 24, 255}, 0.82f);
        // Body of the button
        DrawRectangle(bx, y - 3, bw, bh, bg);
        // Trilho neon esquerdo (energy)
        DrawRectangle(bx, y - 3, 3, bh, ColorAlpha(primary ? amber : neon, 0.9f));
        // Cantos chanfrados (estilo DrawPanel)
        int c = 7;
        DrawLine(bx + c, y - 3, bx + bw - c, y - 3, brd);
        DrawLine(bx, y - 3 + c, bx, y + bh - 3 - c, brd);
        DrawLine(bx + c, y + bh - 3, bx + bw - c, y + bh - 3, brd);
        DrawLine(bx + bw, y - 3 + c, bx + bw, y + bh - 3 - c, brd);
        DrawLine(bx, y - 3 + c, bx + c, y - 3, brd);
        DrawLine(bx + bw - c, y - 3, bx + bw, y - 3 + c, brd);
        DrawLine(bx, y + bh - 3 - c, bx + c, y + bh - 3, brd);
        DrawLine(bx + bw - c, y + bh - 3, bx + bw, y + bh - 3 - c, brd);
        // Badge of the key (with recesso chanfrado)
        int keyFont = 15;
        int keyW    = MeasureText(key, keyFont);
        int badgeX  = bx + 12;
        int badgeW  = keyW + 16;
        DrawRectangle(badgeX, y + 5, badgeW, 20, brd);
        DrawLine(badgeX + badgeW, y + 5, badgeX + badgeW + 5, y + 10, ColorAlpha(brd, 0.8f));
        DrawLine(badgeX + badgeW, y + 25, badgeX + badgeW + 5, y + 20, ColorAlpha(brd, 0.8f));
        DrawText(key, badgeX + 8, y + 7, keyFont, Color{4, 8, 16, 255});
        // Rotulo
        int labelX = badgeX + badgeW + 16;
        DrawText(label, labelX, y + 7, 18,
                 (hover || primary) ? Color{235,245,255,255}
                                    : ColorAlpha({155,180,205,255}, 0.9f));
        // Indicador of selecao (seta pulsante)
        if (hover || primary) {
            float px = 0.6f + 0.4f * std::sin(t * 3.0f);
            DrawText("»", bx + bw - 26, y + 6, 20, ColorAlpha(primary ? amber : neon, px));
            DrawRectangle(bx + 8, y + bh - 1, bw - 16, 2,
                          ColorAlpha(primary ? amber : neon, 0.7f + 0.3f * px));
        }
    };

    if (hasSave) {
        drawMenuBtn(cy - 2,  "ENTER", "Continue match saves", true);
        drawMenuBtn(cy + 44, "N",     "Novo game",               false);
        drawMenuBtn(cy + 90, "ESC",   "Leave",                    false);
    } else {
        drawMenuBtn(cy + 20,  "ENTER", "Start new game",      true);
        drawMenuBtn(cy + 66,  "ESC",   "Leave",                   false);
    }

    // ── Rodape: chips of resources (without sobreposicao of width) ─────────────
    DrawLine(0, screenHeight - 32, screenWidth, screenHeight - 32,
             ColorAlpha({0,235,255,255}, 0.10f));
    const char* feat[] = { "OPEN WORLD", "51 INIMIGOS", "CONSTRUCAO RTS",
                           "CRAFTING", "STORY COMPLETA" };
    int fs = 12, sepChip = 34, featW = 0;
    for (int i = 0; i < 5; ++i) featW += MeasureText(feat[i], fs) + sepChip;
    int fx = cx - featW / 2;
    for (int i = 0; i < 5; ++i) {
        DrawText(feat[i], fx, screenHeight - 24, fs, ColorAlpha({0,210,240,255}, 0.55f));
        fx += MeasureText(feat[i], fs) + sepChip - 8;
        if (i < 4) DrawText("//", fx - 4, screenHeight - 27, 10,
                            ColorAlpha({0,235,255,255}, 0.32f));
    }
    DrawText("DARKNET SIMULATOR // RELEASE 0.4", 16, screenHeight - 24, 11,
             ColorAlpha({120,150,190,255}, 0.5f));

    EndTextureMode();
}

void Game::drawPauseMenu() const {
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.82f));

    int cx  = screenWidth / 2;
    float tp = 0.9f + 0.1f * std::sin((float)GetTime() * 1.6f);

    // Titulo embracado ([ PAUSADO ]) with sublinha neon
    const char* title = "PAUSADO";
    int tFont = 44;
    int tw = MeasureText(title, tFont);
    int txc = cx - tw / 2;
    int ty0 = screenHeight / 2 - 214;
    DrawText(title, txc + 3, ty0 + 3, tFont, ColorAlpha({0,140,255,255}, 0.25f));
    DrawText(title, txc,     ty0,     tFont, ColorAlpha({0,235,255,255}, tp));
    int bl = tw / 2 + 16;
    DrawLine(cx - bl, ty0 - 8, cx + bl, ty0 - 8, ColorAlpha({0,235,255,255}, 0.3f));
    DrawLine(cx - bl - 8, ty0 - 14, cx - bl, ty0 - 6, ColorAlpha({0,235,255,255}, 0.6f));
    DrawLine(cx + bl + 8, ty0 - 14, cx + bl, ty0 - 6, ColorAlpha({0,235,255,255}, 0.6f));

    int pby = screenHeight / 2 - 156;
    int pbw = 340, pbh = 32, pgap = 6;
    int pnlTop = pby - 14, pnlH = 9 * (pbh + pgap) + 36;

    // Painel acolchoa the list
    DrawPanel(cx - pbw/2 - 18, pnlTop, pbw + 36, pnlH, {0,235,255,255}, 0.72f);

    auto onoff = [](bool b){ return b ? "ON" : "OFF"; };
    struct Opt { const char* label; Color col; };
    const Opt opts[9] = {
        {"Continue",        {0,235,255,255}},
        {"Save  [F5]",     {120,220,140,255}},
        {TextFormat("Dificuldade: %s", getDifficulty().name), {255,180,40,255}},
        {TextFormat("Trilha sonora: %s", onoff(audio.musicEnabled)), {120,200,255,255}},
        {TextFormat("All the sounds: %s", onoff(audio.allSoundOn)),   {120,200,255,255}},
        {TextFormat("Vozes/characters: %s", onoff(audio.voiceEnabled)), {120,200,255,255}},
        {"Restart match",{255,200,80,255}},
        {"Return to the menu",   {200,180,255,255}},
        {"Leave of the game",     {255,110,110,255}},
    };

    for (int i = 0; i < 9; ++i) {
        int y = pby + i * (pbh + pgap);
        bool hov = (pauseHovered == i);
        Color bg  = hov ? ColorAlpha({8,24,44,255}, 0.96f) : ColorAlpha({6,12,26,255}, 0.88f);
        Color brd = hov ? opts[i].col : ColorAlpha(opts[i].col, 0.35f);
        DrawRectangle(cx - pbw/2, y, pbw, pbh, bg);
        if (hov) DrawRectangle(cx - pbw/2, y, 3, pbh, brd);   // trilho neon
        int c = 5;
        if (hov) {  // cantos chanfrados in the item focado
            DrawLine(cx-pbw/2 + c, y,     cx+pbw/2 - c, y,     brd);
            DrawLine(cx-pbw/2,     y + c, cx-pbw/2,     y+pbh-c, brd);
            DrawLine(cx-pbw/2 + c, y+pbh, cx+pbw/2 - c, y+pbh, brd);
            DrawLine(cx+pbw/2,     y + c, cx+pbw/2,     y+pbh-c, brd);
            DrawLine(cx-pbw/2,     y + c, cx-pbw/2 + c, y, brd);
            DrawLine(cx+pbw/2 - c, y,     cx+pbw/2,     y + c, brd);
            DrawLine(cx-pbw/2,     y+pbh-c, cx-pbw/2 + c, y+pbh, brd);
            DrawLine(cx+pbw/2 - c, y+pbh, cx+pbw/2,     y+pbh-c, brd);
        } else {
            DrawLine(cx-pbw/2, y+pbh, cx+pbw/2, y+pbh, ColorAlpha(brd, 0.5f));
        }
        int lw = MeasureText(opts[i].label, 18);
        DrawText(opts[i].label, cx - lw/2, y + 7, 18, hov ? WHITE : opts[i].col);
        if (hov) DrawText("»", cx - pbw/2 + 12, y + 5, 19, opts[i].col);
    }

    DrawText("Setas/Mouse  -  ENTER/Click confirma  -  ESC continuous",
             cx - 210, pnlTop + pnlH + 8, 13, ColorAlpha(WHITE, 0.5f));
}

// ─── Level Up / Evolution System ─────────────────────────────────────────────

void Game::generateLevelUpChoices() {
    struct CT { const char* title; const char* desc; int stat; float amt; };
    static const CT pool[] = {
        { "+30 HP Maximum",     "Blindagem reforcada",           0, 30.0f },
        { "+10 Damage",          "Core of combat expandido",   1, 10.0f },
        { "+15 Speed",    "Implant motor enabled",        2, 15.0f },
        { "+8% Armor",      "Placa defensiva installed",     3,  8.0f },
        { "+20 Range",       "Amplificador of range",       4, 20.0f },
        { "+50 HP Maximum",     "Blindagem pesada installed",    0, 50.0f },
        { "+18 Damage",          "Protocolo of attack avancado",  1, 18.0f },
        { "+25 Speed",    "Motores of combat ativados",   2, 25.0f },
        { "+12% Armor",     "Blindagem ceramica implantada", 3, 12.0f },
        { "+35 Range",       "Mira laser estendida",          4, 35.0f },
        { "+20 HP + 8 Damage",   "Upgrade hibrido of combat",   0, 20.0f },
    };
    const int poolSize = 11;
    int picked[3] = {-1,-1,-1};
    for (int i = 0; i < 3; ++i) {
        for (int attempts = 0; attempts < 30; ++attempts) {
            int r = GetRandomValue(0, poolSize-1);
            bool dup = false;
            for (int j = 0; j < i; ++j) if (picked[j]==r) { dup=true; break; }
            if (!dup) {
                bool sameStat = false;
                for (int j = 0; j < i; ++j)
                    if (pool[picked[j]].stat==pool[r].stat) { sameStat=true; break; }
                if (!sameStat || attempts>10) { picked[i]=r; break; }
            }
        }
        if (picked[i]<0) picked[i]=i;
        levelUpOptions[i].title       = pool[picked[i]].title;
        levelUpOptions[i].description = pool[picked[i]].desc;
        levelUpOptions[i].statType    = pool[picked[i]].stat;
        levelUpOptions[i].bonusAmount = pool[picked[i]].amt;
    }
}

void Game::applyLevelUpChoice(int idx) {
    if (idx<0||idx>2) return;
    const LevelUpChoice& c = levelUpOptions[idx];
    switch (c.statType) {
        case 0: player.increaseBaseMaxHP(c.bonusAmount); player.health = player.maxHealth; break;
        case 1: player.increaseBaseAttackDamage(c.bonusAmount); break;
        case 2: player.increaseBaseSpeed(c.bonusAmount); break;
        case 3: player.increaseBaseDefense(c.bonusAmount); break;
        case 4: player.increaseBaseAttackRange(c.bonusAmount); break;
    }
    particles.spawnLevelUp(player.position);
}

void Game::applyEvolutionPath(int pathIdx) {
    audio.playEvolve();
    if (pathIdx<0||pathIdx>2) pathIdx=1;
    player.evolutionPath = static_cast<EvolutionPath>(pathIdx+1);
    player.evolutionTier++;
    switch (pathIdx) {
        case 0:
            player.attackDamage *= 1.30f;
            showStoryBanner("CYBORG SOLDIER","Implantes of combat ativados. +30% Damage.",4.0f);
            triggerPlayerSpeech("Implantes instalados. Damage aumentado.",4.0f);
            break;
        case 1:
            player.speed *= 1.40f;
            showStoryBanner("HACKER GHOST","Protocolos of infiltracao ativados. +40% Speed.",4.0f);
            triggerPlayerSpeech("Modo ghost enabled. Sou more fast.",4.0f);
            break;
        case 2:
            player.maxHealth *= 1.50f;
            player.health = player.maxHealth;
            showStoryBanner("EXECUTOR OMEGA","Blindagem maxima installed. +50% HP.",4.0f);
            triggerPlayerSpeech("Armor omega. Sou imparavel.",4.0f);
            break;
    }
    particles.spawnLevelUp(player.position);
    audio.playLevelUp();
}

void Game::drawLevelUpScreen() const {
    DrawRectangle(0,0,screenWidth,screenHeight,ColorAlpha(BLACK,0.75f));
    for (int i=0;i<20;++i) {
        float gx=(float)((i*157+43)%screenWidth);
        float gy=std::fmod((float)(i*83)+levelUpAnimTimer*80.0f,(float)screenHeight);
        DrawCircleV({gx,gy},3.0f,ColorAlpha({255,210,0,255},0.4f+0.4f*std::sin(levelUpAnimTimer*3.0f+i)));
    }
    float sc=levelUpAnimTimer<0.3f?levelUpAnimTimer/0.3f:1.0f;
    float off=(1.0f-sc)*80.0f;
    Color Cg={255,210,0,255}; Color Cc={0,220,255,255};
    int ty=(int)(screenHeight/2-160+off);
    const char* ttl=TextFormat("LESPEED %d ATINGIDO!",player.level);
    DrawText(ttl,screenWidth/2-MeasureText(ttl,36)/2,ty,36,ColorAlpha(Cg,sc));
    const char* sub="Escolha um upgrade (1 / 2 / 3):";
    DrawText(sub,screenWidth/2-MeasureText(sub,15)/2,ty+44,15,ColorAlpha(WHITE,0.7f*sc));
    int cW=240,cH=160,gap=20;
    int totW=cW*3+gap*2;
    int sX=screenWidth/2-totW/2,cY=(int)(screenHeight/2-60+off);
    Vector2 mouse2=virtualizeMousePos(GetMousePosition());
    for (int i=0;i<3;++i) {
        int cx=sX+i*(cW+gap),cy2=cY;
        bool sel=(i==levelUpChoice);
        Color bc=sel?Color{255,210,0,255}:Color{60,60,80,255};
        Rectangle card={(float)cx,(float)cy2,(float)cW,(float)cH};
        DrawRectangleRec(card,ColorAlpha({14,16,28,255},0.88f*sc));
        if (sel) DrawRectangle(cx,cy2,cW,3,ColorAlpha(bc,0.9f*sc));
        int lu = 6;
        DrawLine(cx+lu,cy2,     cx+cW-lu,cy2,     ColorAlpha(bc,0.85f));
        DrawLine(cx,cy2+lu,     cx,cy2+cH-lu,     ColorAlpha(bc,0.7f));
        DrawLine(cx+lu,cy2+cH,  cx+cW-lu,cy2+cH,  ColorAlpha(bc,0.85f));
        DrawLine(cx+cW,cy2+lu,  cx+cW,cy2+cH-lu,  ColorAlpha(bc,0.7f));
        DrawLine(cx,cy2+lu,     cx+lu,cy2,        ColorAlpha(bc,0.9f));
        DrawLine(cx+cW-lu,cy2,  cx+cW,cy2+lu,     ColorAlpha(bc,0.9f));
        DrawLine(cx,cy2+cH-lu,  cx+lu,cy2+cH,     ColorAlpha(bc,0.9f));
        DrawLine(cx+cW-lu,cy2+cH,cx+cW,cy2+cH-lu, ColorAlpha(bc,0.9f));
        DrawText(levelUpOptions[i].title.c_str(),cx+12,cy2+14,16,ColorAlpha(bc,sc));
        DrawText(levelUpOptions[i].description.c_str(),cx+12,cy2+40,11,ColorAlpha(WHITE,0.75f*sc));
        const char* k=i==0?"[1]":i==1?"[2]":"[3]";
        DrawText(k,cx+cW-24,cy2+cH-20,13,ColorAlpha(bc,0.8f*sc));
        if (sel) {
            DrawRectangle(cx,cy2,cW,4,Color{255,210,0,200});
        }
    }
    const char* hint="Mouse ou 1/2/3 to select   ENTER to confirm";
    DrawText(hint,screenWidth/2-MeasureText(hint,12)/2,(int)(screenHeight/2+125+off),12,ColorAlpha(GRAY,0.7f*sc));
}

void Game::drawEvolutionScreen() const {
    DrawRectangle(0,0,screenWidth,screenHeight,ColorAlpha(BLACK,0.80f));
    float sc=levelUpAnimTimer<0.3f?levelUpAnimTimer/0.3f:1.0f;
    Color Cp={180,0,255,255};
    const char* ttl="PONTO DE EVOLUTION";
    DrawText(ttl,screenWidth/2-MeasureText(ttl,32)/2,screenHeight/2-170,32,ColorAlpha(Cp,sc));
    struct EP { const char* name; const char* desc; Color col; } paths[3]={
        {"SOLDADO CYBORG",   "+HP +Defense +Armor",   {0,200,255,255}},
        {"HACKER GHOST",  "+Vel +Damage +Range",     {0,255,120,255}},
        {"EXECUTOR OMEGA","MAXIMO HP",               {255,80,0,255}},
    };
    const char* keys[3]={"[A]","[S]","[D]"};
    int cW=210,cH=140,gap=24;
    int totW=cW*3+gap*2,sX=screenWidth/2-totW/2,cY=screenHeight/2-50;
    for (int i=0;i<3;++i) {
        int cx=sX+i*(cW+gap);
        bool sel=(i==evolutionChoice);
        Color bc=sel?paths[i].col:Color{60,50,70,255};
        Rectangle card={(float)cx,(float)cY,(float)cW,(float)cH};
        DrawRectangleRec(card,ColorAlpha({16,12,26,255},0.88f*sc));
        if (sel) DrawRectangle(cx,cY,cW,3,ColorAlpha(bc,0.9f*sc));
        int eu = 6;
        DrawLine(cx+eu,cY,     cx+cW-eu,cY,     ColorAlpha(bc,0.85f));
        DrawLine(cx,cY+eu,     cx,cY+cH-eu,     ColorAlpha(bc,0.7f));
        DrawLine(cx+eu,cY+cH,  cx+cW-eu,cY+cH,  ColorAlpha(bc,0.85f));
        DrawLine(cx+cW,cY+eu,  cx+cW,cY+cH-eu,  ColorAlpha(bc,0.7f));
        DrawLine(cx,cY+eu,     cx+eu,cY,        ColorAlpha(bc,0.9f));
        DrawLine(cx+cW-eu,cY,  cx+cW,cY+eu,     ColorAlpha(bc,0.9f));
        DrawLine(cx,cY+cH-eu,  cx+eu,cY+cH,     ColorAlpha(bc,0.9f));
        DrawLine(cx+cW-eu,cY+cH,cx+cW,cY+cH-eu, ColorAlpha(bc,0.9f));
        DrawText(paths[i].name,cx+10,cY+14,14,ColorAlpha(bc,sc));
        DrawText(paths[i].desc,cx+10,cY+38,11,ColorAlpha(WHITE,0.75f*sc));
        DrawText(keys[i],cx+cW-28,cY+cH-20,14,ColorAlpha(bc,0.8f*sc));
    }
    const char* hint2="A/S/D ou arrows to select   ENTER to confirm";
    DrawText(hint2,screenWidth/2-MeasureText(hint2,12)/2,screenHeight/2+110,12,ColorAlpha(GRAY,0.7f*sc));
}




