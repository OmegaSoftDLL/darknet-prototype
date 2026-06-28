// Game_Menus.cpp — telas de menu principal, pausa, level-up e evolucao.
// Modularizacao: extraido de Game.cpp (que estava com ~7000 linhas). Mesma classe Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <algorithm>
void Game::drawMainMenu() const {
    BeginTextureMode(gameTarget);
    ClearBackground({2, 4, 8, 255});

    float t = (float)GetTime();
    int   cx = screenWidth  / 2;
    int   cy = screenHeight / 2;

    // ── Animated dark grid ────────────────────────────────────────────────────
    for (int i = 0; i < screenWidth; i += 48) {
        float pulse = 0.06f + 0.04f * std::sin(t * 0.6f + i * 0.01f);
        DrawLine(i, 0, i, screenHeight, ColorAlpha({0,180,255,255}, pulse));
    }
    for (int j = 0; j < screenHeight; j += 32) {
        float pulse = 0.06f + 0.03f * std::sin(t * 0.4f + j * 0.015f);
        DrawLine(0, j, screenWidth, j, ColorAlpha({0,180,255,255}, pulse));
    }

    // ── Horizontal scan line sweeping downward ─────────────────────────────
    int scanY = (int)(std::fmod(t * 180.0f, (float)screenHeight));
    DrawRectangle(0, scanY, screenWidth, 2, ColorAlpha({0,255,220,255}, 0.18f));
    DrawRectangle(0, scanY+2, screenWidth, 8, ColorAlpha({0,255,220,255}, 0.04f));

    // ── IRON-VIII SKULL silhouette (far left, clear of the title) ─────────────
    int sx = cx - 440;
    int sy = cy - 150;
    // Skull outer
    DrawEllipse(sx, sy, 75, 90, {20, 30, 20, 255});
    DrawEllipse(sx, sy, 73, 88, {8, 12, 8, 255});
    // Cheekbones
    DrawEllipse(sx-40, sy+30, 22, 16, {20,30,20,255});
    DrawEllipse(sx+40, sy+30, 22, 16, {20,30,20,255});
    // Jaw
    DrawRectangle(sx-38, sy+50, 76, 40, {10,16,10,255});
    DrawEllipse(sx, sy+90, 30, 15, {10,16,10,255});
    // Teeth lines
    for (int ti = 0; ti < 6; ++ti)
        DrawRectangle(sx-28+ti*10, sy+68, 6, 18, {18,28,18,255});
    // Eye sockets - deep black
    DrawEllipse(sx-24, sy-10, 21, 16, BLACK);
    DrawEllipse(sx+24, sy-10, 21, 16, BLACK);
    // IRON-VIII red eye glow
    float eyeFlicker = 0.75f + 0.25f * std::sin(t * 3.5f);
    float eyeFlicker2 = 0.75f + 0.25f * std::sin(t * 3.5f + 0.8f);
    DrawGlowCircle({(float)(sx-24), (float)(sy-10)}, 12.0f, {220,0,0,255}, 10.0f);
    DrawCircleV({(float)(sx-24),(float)(sy-10)}, 8.0f,
                ColorAlpha({255,30,0,255}, eyeFlicker));
    DrawCircleV({(float)(sx-24),(float)(sy-10)}, 3.5f, WHITE);
    DrawGlowCircle({(float)(sx+24), (float)(sy-10)}, 12.0f, {220,0,0,255}, 10.0f);
    DrawCircleV({(float)(sx+24),(float)(sy-10)}, 8.0f,
                ColorAlpha({255,30,0,255}, eyeFlicker2));
    DrawCircleV({(float)(sx+24),(float)(sy-10)}, 3.5f, WHITE);
    // Neck struts
    DrawRectangle(sx-18, sy+100, 12, 30, {15,25,15,255});
    DrawRectangle(sx+6,  sy+100, 12, 30, {15,25,15,255});
    // Glint on skull
    DrawEllipse(sx-20, sy-40, 8, 5, ColorAlpha({0,200,100,255}, 0.18f));

    // Eye red light cast on nearby area
    float eyeGlow = 0.10f + 0.06f * std::sin(t * 3.5f);
    DrawCircleV({(float)(sx), (float)(sy)}, 120.0f,
                ColorAlpha({180,0,0,255}, eyeGlow * 0.3f));

    // ── TITLE (centralizado com MeasureText — sem sobreposicao) ──────────────
    float titlePulse = 0.85f + 0.15f * std::sin(t * 1.2f);
    int   titleFont  = 88;
    int   titleW     = MeasureText("DARKNET", titleFont);
    int   titleX     = cx - titleW / 2;
    int   titleY     = cy - 205;
    // Glow shadow
    DrawText("DARKNET", titleX + 2, titleY + 2, titleFont,
             ColorAlpha({0,120,200,255}, 0.28f * titlePulse));
    // Main title
    DrawText("DARKNET", titleX, titleY, titleFont,
             ColorAlpha({0,220,255,255}, titlePulse));

    // Subtitle — centralizada abaixo do titulo
    int subFont = 24;
    int subW    = MeasureText("GUERRA CONTRA KRONOS", subFont);
    DrawText("GUERRA CONTRA KRONOS", cx - subW / 2, titleY + titleFont + 8, subFont,
             Color{220,50,50,255});

    int sepY = titleY + titleFont + 44;
    DrawLine(cx - 300, sepY, cx + 300, sepY, ColorAlpha({0,180,255,255}, 0.40f));

    // Tagline — centralizada abaixo do separador
    const char* tagline = "2047 - KRONOS domina. O NEXUS e a ultima esperanca.";
    int tagW = MeasureText(tagline, 17);
    DrawText(tagline, cx - tagW / 2, sepY + 12, 17, ColorAlpha(WHITE, 0.60f));

    // ── MENU BUTTONS (mouse-aware, angular panel style) ──────────────────────
    bool hasSave = SaveManager::exists();
    Vector2 mouse = virtualizeMousePos(GetMousePosition());
    int bw = 360;
    auto isHover = [&](int y) -> bool {
        return mouse.x >= cx-bw/2 && mouse.x <= cx+bw/2 &&
               mouse.y >= y-3     && mouse.y <= y+33;
    };
    auto drawMenuBtn = [&](int y, const char* key, const char* label, bool highlight) {
        int bh = 36;
        int bx = cx - bw/2;
        bool hover = isHover(y);
        Color bg  = (highlight || hover) ? ColorAlpha({0,60,90,255}, 0.90f)
                                         : ColorAlpha({0,20,35,255}, 0.75f);
        Color brd = (highlight || hover) ? Color{0,220,255,255} : Color{0,100,140,255};
        // Hover highlight bar
        if (hover)
            DrawRectangleRec({(float)bx,(float)(y-3),(float)bw,(float)bh},
                             ColorAlpha({0,100,160,255}, 0.25f));
        DrawRectangleRec({(float)bx, (float)(y-3), (float)bw, (float)bh}, bg);
        DrawRectangleLinesEx({(float)bx,(float)(y-3),(float)bw,(float)bh}, 1.5f, brd);
        DrawLine(bx, y+bh-3-8, bx+8, y+bh-3, brd);
        DrawLine(bx+bw, y+bh-3-8, bx+bw-8, y+bh-3, brd);
        // Badge da tecla com largura dinamica (cabe "ENTER" sem transbordar)
        int keyFont  = 15;
        int keyW     = MeasureText(key, keyFont);
        int badgeX   = bx + 8;
        int badgeW   = keyW + 12;
        DrawRectangle(badgeX, y+5, badgeW, 20, ColorAlpha(brd, 0.6f));
        DrawText(key, badgeX + 6, y+7, keyFont, WHITE);
        // Label comeca apos o badge, com folga — sem sobreposicao
        int labelX = badgeX + badgeW + 12;
        DrawText(label, labelX, y+7, 17, (highlight || hover) ? WHITE : LIGHTGRAY);
        // Mouse cursor icon when hovering
        if (hover) DrawText(">", bx+bw-24, y+7, 18, ColorAlpha({0,220,255,255},0.8f));
    };

    if (hasSave) {
        drawMenuBtn(cy - 2,  "ENTER", "Continuar partida salva", true);
        drawMenuBtn(cy + 44, "N",     "Novo jogo",               false);
        drawMenuBtn(cy + 90, "ESC",   "Sair",                    false);
    } else {
        drawMenuBtn(cy + 20,  "ENTER", "Iniciar novo jogo",      true);
        drawMenuBtn(cy + 66,  "ESC",   "Sair",                   false);
    }

    // ── Footer stats (centralizado) ──────────────────────────────────────────
    DrawLine(0, screenHeight - 30, screenWidth, screenHeight - 30,
             ColorAlpha({0,180,255,255}, 0.15f));
    const char* footer = "Mundo Aberto  |  51 Inimigos  |  Construcao RTS  |  Crafting  |  Historia Completa";
    int footW = MeasureText(footer, 14);
    DrawText(footer, cx - footW / 2, screenHeight - 22, 14, ColorAlpha({0,180,255,255}, 0.55f));

    EndTextureMode();
}

void Game::drawPauseMenu() const {
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.78f));

    int cx  = screenWidth / 2;
    // Titulo
    const char* title = "PAUSADO";
    int tFont = 46;
    int tw = MeasureText(title, tFont);
    DrawText(title, cx - tw/2 + 2, screenHeight/2 - 206, tFont, ColorAlpha(BLACK, 0.6f));
    DrawText(title, cx - tw/2,     screenHeight/2 - 208, tFont, Color{0,220,255,255});

    int pby = screenHeight / 2 - 150;
    int pbw = 340, pbh = 32, pgap = 6;

    auto onoff = [](bool b){ return b ? "ON" : "OFF"; };
    struct Opt { const char* label; Color col; };
    const Opt opts[9] = {
        {"Continuar",        {0,220,255,255}},
        {"Salvar  [F5]",     {120,220,140,255}},
        {TextFormat("Dificuldade: %s", getDifficulty().name), {255,160,40,255}},
        {TextFormat("Trilha sonora: %s", onoff(audio.musicEnabled)), {120,200,255,255}},
        {TextFormat("Todos os sons: %s", onoff(audio.allSoundOn)),   {120,200,255,255}},
        {TextFormat("Vozes/personagens: %s", onoff(audio.voiceEnabled)), {120,200,255,255}},
        {"Reiniciar partida",{255,200,80,255}},
        {"Voltar ao menu",   {200,180,255,255}},
        {"Sair do jogo",     {255,110,110,255}},
    };

    for (int i = 0; i < 9; ++i) {
        int y = pby + i * (pbh + pgap);
        bool hov = (pauseHovered == i);
        Color bg  = hov ? ColorAlpha({0,60,90,255}, 0.95f) : ColorAlpha({0,18,30,255}, 0.85f);
        Color brd = hov ? opts[i].col : ColorAlpha(opts[i].col, 0.5f);
        DrawRectangle(cx - pbw/2, y, pbw, pbh, bg);
        DrawRectangleLinesEx({(float)(cx - pbw/2), (float)y, (float)pbw, (float)pbh},
                             hov ? 2.0f : 1.0f, brd);
        int lw = MeasureText(opts[i].label, 18);
        DrawText(opts[i].label, cx - lw/2, y + 7, 18, hov ? WHITE : opts[i].col);
        if (hov) DrawText(">", cx - pbw/2 + 10, y + 7, 18, opts[i].col);
    }

    DrawText("Setas/Mouse  -  ENTER/Clique confirma  -  ESC continua",
             cx - 210, pby + 9*(pbh+pgap) + 10, 13, ColorAlpha(WHITE, 0.5f));
}

// ─── Level Up / Evolution System ─────────────────────────────────────────────

void Game::generateLevelUpChoices() {
    struct CT { const char* title; const char* desc; int stat; float amt; };
    static const CT pool[] = {
        { "+30 HP Maximo",     "Blindagem reforcada",           0, 30.0f },
        { "+10 Dano",          "Nucleo de combate expandido",   1, 10.0f },
        { "+15 Velocidade",    "Implante motor ativado",        2, 15.0f },
        { "+8% Armadura",      "Placa defensiva instalada",     3,  8.0f },
        { "+20 Alcance",       "Amplificador de alcance",       4, 20.0f },
        { "+50 HP Maximo",     "Blindagem pesada instalada",    0, 50.0f },
        { "+18 Dano",          "Protocolo de ataque avancado",  1, 18.0f },
        { "+25 Velocidade",    "Motores de combate ativados",   2, 25.0f },
        { "+12% Armadura",     "Blindagem ceramica implantada", 3, 12.0f },
        { "+35 Alcance",       "Mira laser estendida",          4, 35.0f },
        { "+20 HP + 8 Dano",   "Upgrade hibrido de combate",   0, 20.0f },
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
        case 0: player.maxHealth    += c.bonusAmount; player.health = player.maxHealth; break;
        case 1: player.attackDamage += c.bonusAmount; break;
        case 2: player.speed        += c.bonusAmount; break;
        case 3: player.defense      += c.bonusAmount; break;
        case 4: player.attackRange  += c.bonusAmount; break;
    }
    particles.spawnLevelUp(player.position);
}

void Game::applyEvolutionPath(int pathIdx) {
    if (pathIdx<0||pathIdx>2) pathIdx=1;
    player.evolutionPath = static_cast<EvolutionPath>(pathIdx+1);
    player.evolutionTier++;
    switch (pathIdx) {
        case 0:
            player.attackDamage *= 1.30f;
            showStoryBanner("CYBORG SOLDIER","Implantes de combate ativados. +30% Dano.",4.0f);
            triggerPlayerSpeech("Implantes instalados. Dano aumentado.",4.0f);
            break;
        case 1:
            player.speed *= 1.40f;
            showStoryBanner("HACKER FANTASMA","Protocolos de infiltracao ativados. +40% Velocidade.",4.0f);
            triggerPlayerSpeech("Modo fantasma ativado. Sou mais rapido.",4.0f);
            break;
        case 2:
            player.maxHealth *= 1.50f;
            player.health = player.maxHealth;
            showStoryBanner("EXECUTOR OMEGA","Blindagem maxima instalada. +50% HP.",4.0f);
            triggerPlayerSpeech("Armadura omega. Sou imparavel.",4.0f);
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
    const char* ttl=TextFormat("NIVEL %d ATINGIDO!",player.level);
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
        DrawRectangleRec(card,ColorAlpha({20,20,30,255},0.85f*sc));
        DrawRectangleLinesEx(card,sel?2.5f:1.5f,ColorAlpha(bc,0.9f*sc));
        DrawText(levelUpOptions[i].title.c_str(),cx+12,cy2+14,16,ColorAlpha(bc,sc));
        DrawText(levelUpOptions[i].description.c_str(),cx+12,cy2+40,11,ColorAlpha(WHITE,0.75f*sc));
        const char* k=i==0?"[1]":i==1?"[2]":"[3]";
        DrawText(k,cx+cW-24,cy2+cH-20,13,ColorAlpha(bc,0.8f*sc));
        if (sel) {
            DrawRectangle(cx,cy2,cW,4,Color{255,210,0,200});
        }
    }
    const char* hint="Mouse ou 1/2/3 para selecionar   ENTER para confirmar";
    DrawText(hint,screenWidth/2-MeasureText(hint,12)/2,(int)(screenHeight/2+125+off),12,ColorAlpha(GRAY,0.7f*sc));
}

void Game::drawEvolutionScreen() const {
    DrawRectangle(0,0,screenWidth,screenHeight,ColorAlpha(BLACK,0.80f));
    float sc=levelUpAnimTimer<0.3f?levelUpAnimTimer/0.3f:1.0f;
    Color Cp={180,0,255,255};
    const char* ttl="PONTO DE EVOLUCAO";
    DrawText(ttl,screenWidth/2-MeasureText(ttl,32)/2,screenHeight/2-170,32,ColorAlpha(Cp,sc));
    struct EP { const char* name; const char* desc; Color col; } paths[3]={
        {"SOLDADO CYBORG",   "+HP +Defesa +Armadura",   {0,200,255,255}},
        {"HACKER FANTASMA",  "+Vel +Dano +Alcance",     {0,255,120,255}},
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
        DrawRectangleRec(card,ColorAlpha({18,12,22,255},0.85f*sc));
        DrawRectangleLinesEx(card,sel?2.5f:1.5f,ColorAlpha(bc,0.9f*sc));
        DrawText(paths[i].name,cx+10,cY+14,14,ColorAlpha(bc,sc));
        DrawText(paths[i].desc,cx+10,cY+38,11,ColorAlpha(WHITE,0.75f*sc));
        DrawText(keys[i],cx+cW-28,cY+cH-20,14,ColorAlpha(bc,0.8f*sc));
    }
    const char* hint2="A/S/D ou setas para selecionar   ENTER para confirmar";
    DrawText(hint2,screenWidth/2-MeasureText(hint2,12)/2,screenHeight/2+110,12,ColorAlpha(GRAY,0.7f*sc));
}




