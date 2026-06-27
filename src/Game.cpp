#include "Game.h"
#include "SpriteGen.h"
#include "SpriteExtrude.h"
bool g_renderPass3D = false;
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <chrono>

static void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    rlSetTexture(texture.id);

    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);

        // Front Face
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);

        // Back Face
        rlNormal3f(0.0f, 0.0f, -1.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);

        // Top Face
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);

        // Bottom Face
        rlNormal3f(0.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);

        // Right face
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);

        // Left Face
        rlNormal3f(-1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
    rlEnd();

    rlSetTexture(0);
}

// ─── Constructor / Destructor ────────────────────────────────────────────────

Game::Game() {
    // Janela redimensionavel — o conteudo (1280x720) e escalado com letterbox em
    // presentFrame(), entao nunca corta. F11 alterna tela cheia.
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "DARKNET - ARPG Futurista | Guerra contra KRONOS");
    SetExitKey(KEY_NULL);   // ESC NAO fecha o jogo — abre o menu de pause
    SetTargetFPS(60);

    // Inicia em JANELA que cabe na area util do monitor (evita ficar maior que a
    // tela e cortar). Reduz mantendo proporcao se o monitor for pequeno.
    {
        // Abre no monitor MAIS A DIREITA (onde o Antigravity fica). Acha o monitor
        // com maior X virtual.
        int mc = GetMonitorCount();
        int mon = 0; float bestX = -1e9f;
        for (int i = 0; i < mc; ++i) {
            Vector2 mp = GetMonitorPosition(i);
            if (mp.x > bestX) { bestX = mp.x; mon = i; }
        }
        Vector2 mpos = GetMonitorPosition(mon);
        int mw  = GetMonitorWidth(mon);
        int mh  = GetMonitorHeight(mon);
        if (mw > 0 && mh > 0) {
            float maxW = mw * 0.90f, maxH = mh * 0.90f;
            float s = std::min(maxW / screenWidth, maxH / screenHeight);
            if (s > 1.0f) s = 1.0f;
            int winW = (int)(screenWidth  * s);
            int winH = (int)(screenHeight * s);
            SetWindowSize(winW, winH);
            SetWindowPosition((int)mpos.x + (mw - winW) / 2,
                              (int)mpos.y + std::max(0, (mh - winH) / 2 - 16));
        }
    }
    gameTarget = LoadRenderTexture(screenWidth, screenHeight);
    // POINT (nearest) deixa o texto NITIDO ao escalar para tela cheia (BILINEAR borrava).
    SetTextureFilter(gameTarget.texture, TEXTURE_FILTER_POINT);
    tempEntityTarget = LoadRenderTexture(128, 128);
    SetTextureFilter(tempEntityTarget.texture, TEXTURE_FILTER_POINT);
    lightSystem.init(screenWidth, screenHeight);

    SpriteBank::get().init();   // gera os sprites pixel-art (precisa de contexto GL)

    // Carrega modelos 3D para graficos reais
    if (FileExists("resources/models/house.obj")) {
        m_houseModel = LoadModel("resources/models/house.obj");
        m_houseTex = LoadTexture("resources/models/house_diffuse.png");
        m_houseModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_houseTex;
    }
    if (FileExists("resources/models/turret.obj")) {
        m_turretModel = LoadModel("resources/models/turret.obj");
        m_turretTex = LoadTexture("resources/models/turret_diffuse.png");
        m_turretModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_turretTex;
    }
    if (FileExists("resources/models/barracks.obj")) {
        m_barracksModel = LoadModel("resources/models/barracks.obj");
        m_barracksTex = LoadTexture("resources/models/barracks_diffuse.png");
        m_barracksModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_barracksTex;
    }
    if (FileExists("resources/models/castle.obj")) {
        m_castleModel = LoadModel("resources/models/castle.obj");
        m_castleTex = LoadTexture("resources/models/castle_diffuse.png");
        m_castleModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_castleTex;
    }
    if (FileExists("resources/models/market.obj")) {
        m_marketModel = LoadModel("resources/models/market.obj");
        m_marketTex = LoadTexture("resources/models/market_diffuse.png");
        m_marketModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_marketTex;
    }
    if (FileExists("resources/models/well.obj")) {
        m_wellModel = LoadModel("resources/models/well.obj");
        m_wellTex = LoadTexture("resources/models/well_diffuse.png");
        m_wellModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_wellTex;
    }
    if (FileExists("resources/models/old_car_new.glb")) {
        m_carModel = LoadModel("resources/models/old_car_new.glb");
    }
    {
        auto _fit = [](Model m, float target)->float {
            if (m.meshCount <= 0) return 1.0f;
            BoundingBox bb = GetModelBoundingBox(m);
            float d = fmaxf(bb.max.y - bb.min.y, fmaxf(bb.max.x - bb.min.x, bb.max.z - bb.min.z));
            return (d > 0.001f) ? target / d : 1.0f;
        };
        m_houseScale    = _fit(m_houseModel, 110.0f);
        m_barracksScale = _fit(m_barracksModel, 90.0f);
        m_castleScale   = _fit(m_castleModel, 155.0f);
        m_turretScale   = _fit(m_turretModel, 75.0f);
        m_marketScale   = _fit(m_marketModel, 130.0f);
        m_wellScale     = _fit(m_wellModel, 55.0f);
        m_carScale      = _fit(m_carModel, 40.0f);
    }
    m_modelsLoaded = true;

    audio.init();
    buildQuests();
    buildNPCs();
    craftingSystem.buildRecipes();
    achievements.init();

    // Open world — generate unified 9-region map
    openWorldMode = true;
    tilemap.generateOpenWorld();
    setupWorldRegions();
    buildOpenWorldScenery();

    // Player starts in center of first region (LARuins) = ZONA SEGURA
    float cx = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f;
    float cy = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
    player.position  = {cx, cy};
    safeZoneCenter   = {cx, cy};   // refugio fica no centro da regiao inicial

    camera.offset   = {screenWidth / 2.0f, screenHeight / 2.0f};
    camera.target   = player.position;
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    // Câmera 3D (2.5D) — valores iniciais válidos antes do primeiro update.
    updateCamera3D();

    spawnInterval = getZoneInfo(currentZone).spawnInterval;
    background.generate(currentZone, tilemap.width, tilemap.height, Tilemap::tileSize);
}

Game::~Game() {
    UnloadRenderTexture(gameTarget);
    UnloadRenderTexture(tempEntityTarget);
    lightSystem.shutdown();
    SpriteBank::get().shutdown();
    audio.shutdown();

    // Desaloca modelos 3D e texturas correspondentes
    if (m_modelsLoaded) {
        if (m_houseModel.meshCount > 0) {
            UnloadModel(m_houseModel);
            UnloadTexture(m_houseTex);
        }
        if (m_turretModel.meshCount > 0) {
            UnloadModel(m_turretModel);
            UnloadTexture(m_turretTex);
        }
        if (m_barracksModel.meshCount > 0) {
            UnloadModel(m_barracksModel);
            UnloadTexture(m_barracksTex);
        }
        if (m_castleModel.meshCount > 0) {
            UnloadModel(m_castleModel);
            UnloadTexture(m_castleTex);
        }
        if (m_marketModel.meshCount > 0) {
            UnloadModel(m_marketModel);
            UnloadTexture(m_marketTex);
        }
        if (m_wellModel.meshCount > 0) {
            UnloadModel(m_wellModel);
            UnloadTexture(m_wellTex);
        }
    }

    CloseWindow();
}

// ── RenderTexture helpers ─────────────────────────────────────────────────────

void Game::presentFrame() const {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float scaleX = (float)sw / screenWidth;
    float scaleY = (float)sh / screenHeight;
    float scale  = scaleX < scaleY ? scaleX : scaleY;
    float drawW  = screenWidth  * scale;
    float drawH  = screenHeight * scale;
    float drawX  = (sw - drawW) * 0.5f;
    float drawY  = (sh - drawH) * 0.5f;
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(gameTarget.texture,
        {0.f, 0.f, (float)screenWidth, -(float)screenHeight},
        {drawX, drawY, drawW, drawH},
        {0, 0}, 0.0f, WHITE);
    EndDrawing();
}

Vector2 Game::virtualizeMousePos(Vector2 m) const {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float scaleX = (float)sw / screenWidth;
    float scaleY = (float)sh / screenHeight;
    float scale  = scaleX < scaleY ? scaleX : scaleY;
    float drawX  = (sw - screenWidth  * scale) * 0.5f;
    float drawY  = (sh - screenHeight * scale) * 0.5f;
    return {(m.x - drawX) / scale, (m.y - drawY) / scale};
}

// ─── Difficulty System ───────────────────────────────────────────────────────

const DifficultySettings& Game::getDifficulty() const {
    return DIFFICULTY_TABLE[(int)difficulty];
}

void Game::drawDifficultyScreen() const {
    BeginTextureMode(gameTarget);  // overlay on top of menu (no ClearBackground)

    float t = (float)GetTime();

    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.88f));

    // Title
    const char* title = "SELECIONE A DIFICULDADE";
    int titleW = MeasureText(title, 30);
    DrawText(title, screenWidth/2 - titleW/2, 98, 30, Color{0,210,255,255});

    // Decorative lines
    DrawLine(screenWidth/2 - 340, 135, screenWidth/2 + 340, 135,
             ColorAlpha(Color{0,210,255,255}, 0.45f));
    DrawLine(screenWidth/2 - 340, 138, screenWidth/2 + 340, 138,
             ColorAlpha(Color{0,210,255,255}, 0.18f));

    // 5 cards layout
    const int cardW   = 196;
    const int cardH   = 268;
    const int cardGap = 10;
    const int totalW  = 5 * cardW + 4 * cardGap;
    const int startX  = (screenWidth - totalW) / 2;
    const int startY  = 150;

    for (int i = 0; i < 5; ++i) {
        const DifficultySettings& ds = DIFFICULTY_TABLE[i];
        bool isHov = (difficultyHovered == i);
        bool isSel = ((int)difficulty == i);
        float alpha = isHov ? 1.0f : 0.75f;
        float pulse = 0.5f + 0.5f * std::sin(t * 3.0f);

        int cx = startX + i * (cardW + cardGap);
        int cy = startY;

        // Glow halo behind hovered card
        if (isHov) {
            DrawRectangle(cx - 5, cy - 5, cardW + 10, cardH + 10,
                         ColorAlpha(ds.labelColor, 0.12f * pulse));
        }

        // Card background
        {
            unsigned char cr = isHov ? 18 : 8;
            unsigned char cg = isHov ? 28 : 12;
            unsigned char cb = isHov ? 50 : 20;
            DrawRectangle(cx, cy, cardW, cardH,
                         ColorAlpha(Color{cr, cg, cb, 255}, 0.96f));
        }

        // Card border
        float bw = isHov ? 2.5f : 1.0f;
        DrawRectangleLinesEx({(float)cx,(float)cy,(float)cardW,(float)cardH}, bw,
                             ColorAlpha(ds.labelColor, isHov ? 1.0f : 0.40f));

        // Cyberpunk corner cuts
        int cc = 9;
        DrawLine(cx, cy+cc, cx+cc, cy, ColorAlpha(ds.labelColor, isHov ? 0.9f : 0.35f));
        DrawLine(cx+cardW-cc, cy, cx+cardW, cy+cc, ColorAlpha(ds.labelColor, isHov ? 0.9f : 0.35f));
        DrawLine(cx, cy+cardH-cc, cx+cc, cy+cardH, ColorAlpha(ds.labelColor, isHov ? 0.7f : 0.25f));
        DrawLine(cx+cardW-cc, cy+cardH, cx+cardW, cy+cardH-cc, ColorAlpha(ds.labelColor, isHov ? 0.7f : 0.25f));

        // Pulsing top glow on hovered
        if (isHov) {
            DrawRectangle(cx, cy, cardW, 3, ColorAlpha(ds.labelColor, 0.75f * pulse));
        }

        // Number badge
        DrawRectangle(cx+cardW-26, cy+4, 22, 18, ColorAlpha(ds.labelColor, 0.20f));
        DrawText(TextFormat("%d", i+1), cx+cardW-21, cy+6, 13, ColorAlpha(ds.labelColor, 0.85f));

        // Difficulty name
        int nameW = MeasureText(ds.name, 15);
        DrawText(ds.name, cx + cardW/2 - nameW/2, cy + 10, 15,
                 ColorAlpha(ds.labelColor, alpha));

        // Separator
        DrawLine(cx+8, cy+32, cx+cardW-8, cy+32,
                 ColorAlpha(ds.labelColor, isHov ? 0.45f : 0.22f));

        // Description (clipped to card)
        BeginScissorMode(cx+4, cy+36, cardW-8, 22);
        DrawText(ds.description, cx+8, cy+36, 10, ColorAlpha(WHITE, alpha * 0.75f));
        EndScissorMode();

        // ── Bars ─────────────────────────────────────────────────────────────
        struct BarDef { const char* label; float value; float maxVal; Color col; };
        BarDef bars[3] = {
            { "INIMIGOS",   ds.enemyHPMult,   3.5f, {220,60, 60,255} },
            { "VELOCIDADE", ds.spawnRateMult,  2.5f, {255,160,0, 255} },
            { "RECOMPENSA", ds.dropChanceMult, 2.5f, {0, 200,100,255} },
        };

        int barX = cx + 8;
        int barInnerW = cardW - 16;
        int barH2 = 9;
        int barSpacing = 38;
        int barsStartY = cy + 64;

        for (int b = 0; b < 3; ++b) {
            int by = barsStartY + b * barSpacing;
            float pct = std::min(bars[b].value / bars[b].maxVal, 1.0f);
            float barAlpha = isHov ? 1.0f : 0.65f;

            // Label
            DrawText(bars[b].label, barX, by, 9, ColorAlpha(WHITE, alpha * 0.65f));

            // Track
            DrawRectangle(barX, by + 13, barInnerW, barH2, ColorAlpha(BLACK, 0.55f));

            // Fill
            int filled = (int)(barInnerW * pct);
            if (filled > 0) {
                DrawRectangle(barX, by+13, filled, barH2, ColorAlpha(bars[b].col, barAlpha));
                DrawRectangle(barX, by+13, filled, barH2/3, ColorAlpha(WHITE, 0.12f * barAlpha));
            }
            DrawRectangleLinesEx({(float)barX,(float)(by+13),(float)barInnerW,(float)barH2},
                                 1.0f, ColorAlpha(bars[b].col, 0.3f * barAlpha));

            // Multiplier value
            DrawText(TextFormat("x%.1f", bars[b].value),
                     cx + cardW - 34, by + 13, 8, ColorAlpha(bars[b].col, barAlpha * 0.9f));
        }

        // ── Info line ─────────────────────────────────────────────────────────
        int infoY = barsStartY + 3 * barSpacing + 2;
        DrawLine(cx+4, infoY, cx+cardW-4, infoY, ColorAlpha(ds.labelColor, 0.20f));
        DrawText(TextFormat("XP:%.1fx  $:%.1fx",  ds.xpMult, ds.creditMult),
                 cx+8, infoY+5, 9, ColorAlpha(Color{160,220,255,255}, alpha * 0.75f));
        DrawText(TextFormat("Boss HP: x%.1f",     ds.bossHPMult),
                 cx+8, infoY+18, 9, ColorAlpha(Color{255,160,100,255}, alpha * 0.65f));

        // ── "SELECIONADO" badge at bottom ──────────────────────────────────
        if (isSel) {
            DrawRectangle(cx+6, cy+cardH-22, cardW-12, 18,
                         ColorAlpha(ds.labelColor, 0.25f));
            const char* selTxt = "SELECIONADO";
            DrawText(selTxt, cx + cardW/2 - MeasureText(selTxt,11)/2,
                     cy+cardH-20, 11, ds.labelColor);
        }
    }

    // ── Instructions ───────────────────────────────────────────────────────────
    int hy = startY + cardH + 18;
    const char* h1 = "< Setas/Mouse: navegar >";
    const char* h2 = "ENTER ou clique: confirmar";
    const char* h3 = "ESC: voltar";
    DrawText(h1, screenWidth/2 - MeasureText(h1,13)/2, hy,    13, ColorAlpha(WHITE, 0.55f));
    DrawText(h2, screenWidth/2 - MeasureText(h2,14)/2, hy+20, 14, ColorAlpha(Color{0,220,255,255}, 0.85f));
    DrawText(h3, screenWidth/2 - MeasureText(h3,12)/2, hy+42, 12, ColorAlpha(WHITE, 0.38f));

    EndTextureMode();
}

// ─── Quests & NPCs Setup ─────────────────────────────────────────────────────

void Game::buildQuests() {
    quests.clear();

    Quest q1("q_limpar", "Limpeza de Ruinas",
             "Elimine 5 patrulhas KRONOS", "VANCE RIOS", QuestType::Kill, 5);
    q1.rewardHP = 50.0f; q1.rewardXP = 100;
    q1.rewardEquip = EDB::pistolaPlas();
    quests.push_back(q1);

    Quest q2("q_suprimentos", "Suprimentos Criticos",
             "Colete 3 Energy Cores para o NEXUS", "MARCO VEIL", QuestType::Collect, 3);
    q2.rewardHP = 30.0f; q2.rewardXP = 80;
    q2.rewardEquip = EDB::coleteMilitar();
    quests.push_back(q2);

    Quest q3("q_executor", "O Executor",
             "Destrua o IRON-VIII Boss da Forja KRONOS", "COMANDANTE LYRA", QuestType::KillBoss, 1);
    q3.rewardHP = 100.0f; q3.rewardXP = 300;
    q3.rewardEquip = EDB::rifleEnergia();
    quests.push_back(q3);

    Quest q4("q_morphx", "Metal Liquido",
             "Destrua 3 MORPH-X (metal liquido)", "DR. CHEN", QuestType::Kill, 3);
    q4.rewardHP = 0.0f; q4.rewardXP = 200;
    q4.rewardEquip = EDB::armaduraAvan();
    quests.push_back(q4);

    Quest q5("q_drones", "Caca-Drones",
             "Abata 5 Hunter Drones do KRONOS", "MARCO VEIL", QuestType::Kill, 5);
    q5.rewardHP = 50.0f; q5.rewardXP = 250;
    q5.rewardEquip = EDB::neuralLink();
    quests.push_back(q5);

    // ── 20 Novas Quests ──────────────────────────────────────────────────────

    { Quest q("q_zergling",  "Praga Alienigena",
              "Elimine 15 Zerglings — eles se multiplicam rapido!", "DR. CHEN",
              QuestType::Kill, 15);
      q.rewardXP = 400; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_hydra",     "Acido na Veia",
              "Destrua 8 Hydras — cuidado com os projéteis acidos", "COMANDANTE LYRA",
              QuestType::Kill, 8);
      q.rewardXP = 500; q.rewardHP = 80.0f; quests.push_back(q); }

    { Quest q("q_brood",     "Mae dos Monstros",
              "Mate a Broodmother antes que invoque mais zerglings", "DR. CHEN",
              QuestType::KillBoss, 1);
      q.rewardXP = 700; q.rewardHP = 100.0f; quests.push_back(q); }

    { Quest q("q_fantasmas", "Assombracoes",
              "Elimine 10 fantasmas nas zonas sombrias", "VANCE RIOS",
              QuestType::Kill, 10);
      q.rewardXP = 600; q.rewardHP = 70.0f; quests.push_back(q); }

    { Quest q("q_zumbis",    "Apocalipse Zumbi",
              "Elimine 20 zumbis — eles estao se espalhando", "MARCO VEIL",
              QuestType::Kill, 20);
      q.rewardXP = 500; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_portais",   "Fechar os Rifts",
              "Feche 3 portais de anomalia antes que mais inimigos entrem", "VANCE RIOS",
              QuestType::Kill, 3);
      q.rewardXP = 800; q.rewardHP = 120.0f; quests.push_back(q); }

    { Quest q("q_orc",       "Orc Cibernetico",
              "Derrote 5 Orcs Ciberneticos — implantes KRONOS os fortaleceram", "DR. CHEN",
              QuestType::Kill, 5);
      q.rewardXP = 450; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_paladin",   "Paladin Corrompido",
              "Destrua 5 Paladins Corrompidos pelos nanobots KRONOS", "COMANDANTE LYRA",
              QuestType::Kill, 5);
      q.rewardXP = 450; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_omega_boss","Encontro com OMEGA",
              "Sobreviva e destrua o OmegaBoss — ele aparece apos 50 kills", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 2000; q.rewardHP = 200.0f; quests.push_back(q); }

    { Quest q("q_cemiterio", "Zona do Cemiterio",
              "Explore e sobreviva ao Cemiterio Abandonado", "COMANDANTE LYRA",
              QuestType::Zone, 4);
      q.rewardXP = 300; q.rewardHP = 40.0f; quests.push_back(q); }

    { Quest q("q_inferno",   "Descida ao Inferno",
              "Alcance e sobreviva na Zona Inferno", "DR. CHEN",
              QuestType::Zone, 10);
      q.rewardXP = 1000; q.rewardHP = 150.0f; quests.push_back(q); }

    { Quest q("q_loot",      "Colecionador",
              "Colete 10 itens durante sua missao", "MARCO VEIL",
              QuestType::Collect, 10);
      q.rewardXP = 400; q.rewardHP = 0.0f; quests.push_back(q); }

    { Quest q("q_kamikaze",  "Bombas Vivas",
              "Derrote 8 Kamikazes antes que explodam perto de voce", "DR. CHEN",
              QuestType::Kill, 8);
      q.rewardXP = 350; q.rewardHP = 40.0f; quests.push_back(q); }

    { Quest q("q_sniper",    "Atiradores de Elite",
              "Elimine 6 Snipers inimigos — eles atiram de longe", "MARCO VEIL",
              QuestType::Kill, 6);
      q.rewardXP = 400; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_alien_boss","Chefe Alienigena",
              "Enfrente e destrua o AlienBoss — ameaca maxima", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 1500; q.rewardHP = 180.0f; quests.push_back(q); }

    { Quest q("q_sombra",    "Wraith das Sombras",
              "Elimine 8 Shadow Wraiths — rapidos e letais", "COMANDANTE LYRA",
              QuestType::Kill, 8);
      q.rewardXP = 600; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_banshee",   "Grito da Banshee",
              "Destrua 5 Banshee Howlers antes que paralisem sua equipe", "DR. CHEN",
              QuestType::Kill, 5);
      q.rewardXP = 500; q.rewardHP = 55.0f; quests.push_back(q); }

    { Quest q("q_zombie_lord","Senhor dos Zumbis",
              "Venca o ZombieLord — ele invoca hordas interminaveis", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 1800; q.rewardHP = 200.0f; quests.push_back(q); }

    { Quest q("q_coleta_raro","Tesouros Raros",
              "Colete 5 itens Raros ou superiores", "MARCO VEIL",
              QuestType::Collect, 5);
      q.rewardXP = 700; q.rewardHP = 0.0f; quests.push_back(q); }

    { Quest q("q_nocturna",  "Missao Noturna",
              "Elimine 30 inimigos nas zonas sombrias", "COMANDANTE LYRA",
              QuestType::Kill, 30);
      q.rewardXP = 900; q.rewardHP = 100.0f; quests.push_back(q); }
}

void Game::buildNPCs() {
    npcs.clear();

    float cx = static_cast<float>(tilemap.width  * Tilemap::tileSize) / 2.0f;
    float cy = static_cast<float>(tilemap.height * Tilemap::tileSize) / 2.0f;

    // VANCE RIOS - lider do NEXUS
    npcs.emplace_back(
        Vector2{cx - 120, cy - 80}, "VANCE RIOS", NPCRole::Leader,
        std::vector<std::string>{
            "KRONOS enviou maquinas para nos destruir. Lute!",
            "Elimine as patrulhas KRONOS para liberar a area.",
            "O futuro da humanidade esta nas suas maos."
        }, "q_limpar");

    // MARCO VEIL
    npcs.emplace_back(
        Vector2{cx + 150, cy - 60}, "MARCO VEIL", NPCRole::Soldier,
        std::vector<std::string>{
            "Especialista em explosivos do NEXUS.",
            "Colete os Energy Cores - precisamos de energia.",
            "Cuidado com os Hunter Drones. Eles nos rastreiam.",
            "Destrua-os antes que reportem nossa posicao!"
        }, "q_suprimentos");

    // COMANDANTE LYRA
    npcs.emplace_back(
        Vector2{cx - 60, cy + 130}, "COMANDANTE LYRA", NPCRole::Soldier,
        std::vector<std::string>{
            "O IRON-VIII nao tem emocoes, nao para, nao negocia.",
            "Voce precisa destruir o Boss da Forja KRONOS.",
            "Use todo o seu arsenal. Nao hesite."
        }, "q_executor");

    // DR. CHEN - cientista dos implantes
    npcs.emplace_back(
        Vector2{cx + 80, cy + 100}, "DR. CHEN", NPCRole::Scientist,
        std::vector<std::string>{
            "Desenvolvi seus implantes de combate, VANCE.",
            "O MORPH-X e minha maior preocupacao. Metal liquido.",
            "Sua unica fraqueza: temperatura extrema e forca bruta.",
            "Ataque sem parar - ele regenera HP rapidamente!"
        }, "q_morphx");
}

// NPCs de servico ancorados na ZONA SEGURA — lider (quests), armeiro, ferreiro,
// mercador e cientista. O jogador equipa, compra e pega missoes na base.
void Game::setupBaseNPCs() {
    float bx = safeZoneCenter.x, by = safeZoneCenter.y;

    // VANCE RIOS — fundador da resistencia NEXUS
    npcs.emplace_back(
        Vector2{bx - 120, by - 90}, "VANCE RIOS", NPCRole::Leader,
        std::vector<std::string>{
            "Eu sou VANCE RIOS, fundador do NEXUS — a ultima resistencia humana.",
            "Quando o KRONOS despertou, perdi minha cidade e minha familia numa noite.",
            "Reuni os sobreviventes nesta base. E tudo que resta de nos.",
            "Meu objetivo e simples: destruir o nucleo do KRONOS e libertar a humanidade.",
            "Meu desejo? Ver o sol nascer sem maquinas patrulhando o ceu.",
            "Eu te ajudo com MISSOES e estrategia. Cumpra-as e ficamos mais fortes.",
            "Comece limpando as patrulhas la fora. Confio em voce, soldado."
        }, "q_limpar");

    // Zara — engenheira de armas
    npcs.emplace_back(
        Vector2{bx + 150, by - 70}, "Zara", NPCRole::WeaponDealer,
        std::vector<std::string>{
            "Me chamo Zara. Eu projetava armas para o exercito... antes do colapso.",
            "Escapei da primeira purga do KRONOS com uma caixa de ferramentas e raiva.",
            "Hoje forjo e vendo armas aqui na base para quem luta de verdade.",
            "Meu objetivo e armar a resistencia ate os dentes.",
            "Desejo vingar cada pessoa que aquelas maquinas tiraram de mim.",
            "Eu te ajudo te dando PODER DE FOGO. Aperte [TAB] e veja meu arsenal."
        }, "");

    // FERREIRO KANE — armeiro/ferreiro
    npcs.emplace_back(
        Vector2{bx + 180, by + 100}, "FERREIRO KANE", NPCRole::ArmorSmith,
        std::vector<std::string>{
            "KANE. Fui ferreiro militar nas trincheiras antes do KRONOS dominar tudo.",
            "Cheguei aqui carregando minha bigorna nas costas por 200 km.",
            "Forjo blindagem e implantes — o que te mantem inteiro la fora.",
            "Meu objetivo e que nenhum soldado do NEXUS caia por falta de protecao.",
            "Desejo, mesmo, e forjar a armadura que vai derrubar o nucleo KRONOS.",
            "Eu te ajudo com DEFESA. Traga materiais e [TAB] para ver minhas forjas."
        }, "");

    // LUNA — mercadora/sucateira
    npcs.emplace_back(
        Vector2{bx - 180, by + 110}, "LUNA", NPCRole::Merchant,
        std::vector<std::string>{
            "Oi. Sou a LUNA. Vasculho as ruinas atras de qualquer coisa util.",
            "Sobrevivi sozinha 3 anos nas cidades mortas antes de achar o NEXUS.",
            "Troco suprimentos: pocoes, kits, energia — o que te mantem vivo.",
            "Meu objetivo e que ninguem aqui morra por falta de um remedio.",
            "Meu desejo e um lugar onde eu nao precise mais catar lixo pra viver.",
            "Eu te ajudo com CONSUMIVEIS. Aperte [TAB] e reabasteca antes de sair."
        }, "");

    // DR. CHEN — cientista dos implantes
    npcs.emplace_back(
        Vector2{bx - 30, by + 150}, "DR. CHEN", NPCRole::Scientist,
        std::vector<std::string>{
            "Dr. Chen. Fui eu quem projetou os implantes de combate no seu corpo.",
            "Trabalhei para o KRONOS antes de entender o que ele planejava. Desertei.",
            "Trouxe comigo a tecnologia que hoje te torna mais que humano.",
            "Meu objetivo e evoluir voce ate poder enfrentar o nucleo de igual pra igual.",
            "Meu desejo e reparar o erro de ter ajudado a criar aquela IA.",
            "Eu te ajudo com EVOLUCAO: suba de nivel e use [K] para evoluir, [L] p/ pontos."
        }, "q_morphx");
}

void Game::setupZoneNPCs(ZoneID zone) {
    npcs.clear();
    float cx = static_cast<float>(tilemap.width  * Tilemap::tileSize) / 2.0f;
    float cy = static_cast<float>(tilemap.height * Tilemap::tileSize) / 2.0f;

    switch (zone) {
        case ZoneID::LARuins:
            // Regiao inicial = base. Em mundo aberto, NPCs ficam na ZONA SEGURA.
            if (openWorldMode) {
                setupBaseNPCs();
            } else {
                buildNPCs();
                npcs.emplace_back(
                    Vector2{cx + 200, cy + 160}, "Zara", NPCRole::WeaponDealer,
                    std::vector<std::string>{
                        "Tenho armas da resistencia. [E] Abrir loja",
                        "Compre logo — KRONOS ta chegando.",
                        "Melhores armas do NEXUS!"
                    }, "");
            }
            break;
        case ZoneID::Bunker:
            npcs.emplace_back(
                Vector2{cx, cy - 100}, "VANCE RIOS", NPCRole::Leader,
                std::vector<std::string>{
                    "Bem-vindo ao Bunker NEXUS!",
                    "Aqui voce pode recuperar forcas.",
                    "A Forja KRONOS fica ao leste. Seja cuidadoso."
                }, "");
            npcs.emplace_back(
                Vector2{cx - 150, cy}, "MARCO VEIL", NPCRole::Soldier,
                std::vector<std::string>{
                    "Abata os drones que patrulham o perimetro.",
                    "Cinco Hunter Drones destruidos e nossa rota fica livre."
                }, "q_drones");
            // Armeiro no bunker — WeaponDealer
            npcs.emplace_back(
                Vector2{cx + 180, cy + 80}, "Rex", NPCRole::WeaponDealer,
                std::vector<std::string>{
                    "Tenho itens para vender. [E] Abrir loja",
                    "Armas e implantes de ponta. Preco justo.",
                    "Cada credito importa contra KRONOS."
                }, "");
            // Mercador de suprimentos militares
            npcs.emplace_back(
                Vector2{cx - 200, cy + 120}, "LUNA", NPCRole::Merchant,
                std::vector<std::string>{
                    "Suprimentos militares disponíveis. [E] Abrir loja",
                    "Tudo que sobrou das bases NEXUS destruidas.",
                    "Pague e sobreviva — nessa ordem."
                }, "");
            break;
        case ZoneID::KronosForge:
            npcs.emplace_back(
                Vector2{cx, cy + 120}, "COMANDANTE LYRA", NPCRole::Soldier,
                std::vector<std::string>{
                    "Esta forja produz IRON-VIII a cada hora.",
                    "Destrua o IRON-VIII Boss no centro da forja!",
                    "O implante neural ajuda a esquivar dos projetos."
                }, "q_executor");
            npcs.emplace_back(
                Vector2{cx + 130, cy - 80}, "DR. CHEN", NPCRole::Scientist,
                std::vector<std::string>{
                    "Esta e minha chance de corrigir o que KRONOS corrompeu.",
                    "Os MORPH-X patrulham os andares superiores.",
                    "Use a Barreira de Escudo contra o MORPH-X!"
                }, "q_morphx");
            // Ferreiro de armaduras na forja
            npcs.emplace_back(
                Vector2{cx - 160, cy + 50}, "KOBA-7", NPCRole::ArmorSmith,
                std::vector<std::string>{
                    "Tenho itens para vender. [E] Abrir loja",
                    "Armaduras forjadas com metal KRONOS capturado.",
                    "So os mais fortes sobrevivem aqui dentro."
                }, "");
            break;
        case ZoneID::KronosNexus:
            npcs.emplace_back(
                Vector2{cx, cy - 150}, "VANCE RIOS", NPCRole::Leader,
                std::vector<std::string>{
                    "Este e o coracao de KRONOS. Tudo termina aqui.",
                    "Destrua o Nucleo Principal para libertar a humanidade!",
                    "Forca, soldado. O futuro e nosso."
                }, "");
            // Ultimo vendedor antes do final
            npcs.emplace_back(
                Vector2{cx + 140, cy - 180}, "Nyx", NPCRole::Merchant,
                std::vector<std::string>{
                    "Tenho itens para vender. [E] Abrir loja",
                    "Ultima chance antes do nucleo. Equipese bem.",
                    "Se voce falhar, todos morremos. Sem desconto."
                }, "");
            break;
    }
}

// ─── Open World ──────────────────────────────────────────────────────────────

void Game::setupWorldRegions() {
    worldRegions.clear();
    // Each region = OW_ZONE_W * tileSize pixels wide/tall
    float sz = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize); // 2560

    auto add = [&](int col, int row, ZoneID z, const char* n, Color c) {
        WorldRegion r;
        r.bounds     = {col * sz, row * sz, sz, sz};
        r.zoneType   = z;
        r.name       = n;
        r.discovered = false;
        r.mapColor   = c;
        worldRegions.push_back(r);
    };

    // Must match Tilemap::generateOpenWorld() owLayout
    add(0, 0, ZoneID::LARuins,       "Ruinas de Avalon",    {80,120,80,255});
    add(1, 0, ZoneID::Bunker,         "Bunker NEXUS",        {60,80,120,255});
    add(2, 0, ZoneID::DarkForest,     "Floresta Negra",      {20,50,20,255});
    add(0, 1, ZoneID::CursedFarm,     "Fazenda Maldita",     {100,80,40,255});
    add(1, 1, ZoneID::Cemetery,       "Cemiterio",           {60,60,80,255});
    add(2, 1, ZoneID::GhostCity,      "Cidade Fantasma",     {40,60,80,255});
    add(0, 2, ZoneID::KronosForge,    "Kronos Forge",        {120,40,20,255});
    add(1, 2, ZoneID::AbandonedManor, "Mansao das Sombras",  {80,40,80,255});
    add(2, 2, ZoneID::KronosNexus,    "Nucleo KRONOS",       {80,0,120,255});

    // First region starts discovered
    if (!worldRegions.empty()) worldRegions[0].discovered = true;
}

ZoneID Game::getRegionAt(Vector2 pos) const {
    for (const auto& r : worldRegions) {
        if (CheckCollisionPointRec(pos, r.bounds)) return r.zoneType;
    }
    return ZoneID::LARuins;
}

// Popula TODAS as regioes do mundo aberto com cenario denso, espalhado por toda
// a area de cada regiao. Chamado uma vez ao iniciar o mundo aberto.
void Game::buildOpenWorldScenery() {
    owDecor.scenery.clear();

    // Gerador pseudo-aleatorio deterministico (nao usa Math.random)
    unsigned int rng = 0x1234abcd;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (float)((rng >> 8) & 0xFFFF) / 65535.0f; // 0..1
    };

    auto place = [&](Rectangle b, int type, int count,
                     float minScale, float maxScale, float margin) {
        for (int i = 0; i < count; ++i) {
            SceneryObject o;
            o.type     = type;
            o.position = {
                b.x + margin + rnd() * (b.width  - 2 * margin),
                b.y + margin + rnd() * (b.height - 2 * margin)
            };
            o.rotation = rnd() * 3.14159f;
            o.scale    = minScale + rnd() * (maxScale - minScale);
            // tint.r > 128 acende luzes (janelas/postes)
            o.tint     = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
            owDecor.scenery.push_back(o);
        }
    };

    for (const auto& r : worldRegions) {
        Rectangle b = r.bounds;
        switch (r.zoneType) {
            case ZoneID::LARuins: // Ruinas tecnologicas
                place(b, 7, 14, 0.8f, 1.6f, 200);  // predios em ruina
                place(b, 6, 10, 1.0f, 1.0f, 150);  // carros abandonados
                place(b, 5, 12, 1.0f, 1.0f, 150);  // postes
                place(b, 2,  6, 0.8f, 1.2f, 150);  // arvores mortas
                break;
            case ZoneID::Bunker: // Bunker militar
                place(b, 7, 10, 0.7f, 1.3f, 200);  // estruturas
                place(b, 8,  6, 1.0f, 1.6f, 200);  // silos/tanques
                place(b, 5,  8, 1.0f, 1.0f, 150);  // postes
                place(b, 4, 14, 1.0f, 1.4f, 120);  // cercas
                break;
            case ZoneID::DarkForest: // Floresta densa
                place(b, 2, 60, 0.9f, 1.8f, 80);   // MUITAS arvores mortas
                place(b, 4, 10, 1.0f, 1.3f, 120);  // cercas
                place(b, 3,  8, 0.7f, 1.0f, 120);  // lapides perdidas
                break;
            case ZoneID::CursedFarm: // Fazenda
                place(b, 0,  6, 1.0f, 1.6f, 250);  // casas
                place(b, 1,  4, 1.0f, 1.5f, 300);  // celeiros
                place(b, 8,  5, 1.0f, 1.4f, 250);  // silos
                place(b, 4, 26, 1.0f, 1.4f, 100);  // muitas cercas
                place(b, 2, 10, 0.8f, 1.3f, 120);  // arvores
                break;
            case ZoneID::Cemetery: // Cemiterio
                place(b, 3, 55, 0.7f, 1.2f, 80);   // MUITAS lapides
                place(b, 2, 14, 0.9f, 1.5f, 120);  // arvores mortas
                place(b, 4, 16, 1.0f, 1.3f, 100);  // cercas
                place(b, 9,  6, 1.0f, 1.0f, 200);  // arcos de catacumba
                place(b,10,  4, 1.0f, 1.4f, 250);  // estatuas
                break;
            case ZoneID::GhostCity: // Cidade fantasma
                place(b, 7, 22, 1.0f, 2.2f, 180);  // MUITOS predios
                place(b, 5, 16, 1.0f, 1.0f, 120);  // postes
                place(b, 6, 10, 1.0f, 1.0f, 150);  // carros
                break;
            case ZoneID::KronosForge: // Forja de lava industrial
                place(b, 8,  8, 1.0f, 1.8f, 220);  // silos/tanques
                place(b, 7, 10, 0.8f, 1.5f, 220);  // estruturas
                place(b,10,  5, 1.0f, 1.5f, 250);  // estatuas
                place(b, 2,  6, 0.7f, 1.0f, 150);  // arvores carbonizadas
                break;
            case ZoneID::AbandonedManor: // Mansao
                place(b, 0,  3, 1.6f, 2.4f, 350);  // casarões grandes
                place(b, 2, 16, 0.9f, 1.6f, 120);  // arvores
                place(b, 3, 12, 0.7f, 1.1f, 120);  // lapides
                place(b,10,  8, 1.0f, 1.5f, 200);  // estatuas
                place(b, 4, 20, 1.0f, 1.3f, 100);  // cercas
                break;
            case ZoneID::KronosNexus: // Nucleo final
                place(b,10, 10, 1.2f, 2.0f, 200);  // estatuas imponentes
                place(b, 7,  8, 1.0f, 1.8f, 220);  // estruturas void
                break;
            default:
                place(b, 2, 10, 0.8f, 1.4f, 150);
                break;
        }
        // Vegetação rasteira densa em TODA zona — vida no chão (grama + detritos)
        place(b, 11, 130, 0.6f, 1.7f, 20);  // grama
        place(b, 12,  36, 0.6f, 1.2f, 40);  // pedras/detritos
    }

    // ── Colisao de cenario: estruturas grandes bloqueiam passagem (nao andar em
    //    cima de casas/predios/carros/silos/estatuas). Tipos: 0 casa, 1 celeiro,
    //    6 carro, 7 predio, 8 silo, 9 catacumba, 10 estatua. Arvores/cercas/postes
    //    ficam atravessaveis para nao criar labirintos que prendem o jogador.
    tilemap.clearSolidFlags();
    for (const auto& o : owDecor.scenery) {
        // Pegada de colisao por tipo (predios/casas grandes bloqueiam mais area).
        float rad = 0.0f;
        switch (o.type) {
            case 0: case 1: case 7: rad = 70.0f + 30.0f * o.scale; break; // casa/celeiro/predio
            case 8: case 9:         rad = 55.0f + 20.0f * o.scale; break; // silo/catacumba
            case 6: case 10:        rad = 40.0f + 16.0f * o.scale; break; // carro/estatua
            default: continue;                                            // arvores/cercas/postes: atravessavel
        }
        tilemap.markSolidAt(o.position, rad);
    }

    owDecorBuilt       = true;
    setupResourceNodes();   // nós de coleta (madeira/pedra/ferro/prata/ouro)
    setupAnimals();         // vida selvagem (veado/coelho/javali/lobo/passaro)
}

// Mundo INFINITO: gera cenário em CHUNKS ao redor do player conforme explora e
// descarrega chunks distantes. Determinístico por chunk (auto-construção).
void Game::updateSceneryChunks(Vector2 playerPos) {
    if (!openWorldMode) return;
    const float CH   = 1280.0f;                 // tamanho do chunk (~20 tiles)
    const int   RAD  = 2;                        // raio em chunks (5x5 carregados)
    const float ORIG = (float)(Tilemap::OW_COLS * Tilemap::OW_ZONE_W * 64); // região fixa original
    int pcx = (int)floorf(playerPos.x / CH);
    int pcy = (int)floorf(playerPos.y / CH);
    if (pcx == m_lastChunkX && pcy == m_lastChunkY) return;  // só recalcula ao cruzar chunk
    m_lastChunkX = pcx; m_lastChunkY = pcy;

    auto keyOf = [](int cx, int cy) -> long long {
        return ((long long)(cx + 100000) << 21) | (long long)(cy + 100000);
    };
    std::set<long long> want;
    for (int cy = pcy - RAD; cy <= pcy + RAD; ++cy)
        for (int cx = pcx - RAD; cx <= pcx + RAD; ++cx) want.insert(keyOf(cx, cy));

    // Descarrega cenário de chunks fora do raio (mantém o fixo chunk == -1)
    owDecor.scenery.erase(std::remove_if(owDecor.scenery.begin(), owDecor.scenery.end(),
        [&](const SceneryObject& o){ return o.chunk != -1 && want.find(o.chunk) == want.end(); }),
        owDecor.scenery.end());
    for (auto it = m_sceneryChunks.begin(); it != m_sceneryChunks.end();)
        it = (want.find(*it) == want.end()) ? m_sceneryChunks.erase(it) : std::next(it);

    // Gera chunks novos
    for (int cy = pcy - RAD; cy <= pcy + RAD; ++cy)
        for (int cx = pcx - RAD; cx <= pcx + RAD; ++cx) {
            long long k = keyOf(cx, cy);
            if (m_sceneryChunks.count(k)) continue;
            m_sceneryChunks.insert(k);
            float ox = cx * CH, oy = cy * CH;
            if (ox >= 0 && oy >= 0 && ox < ORIG && oy < ORIG) continue;  // região fixa já populada
            unsigned int rng = (unsigned int)(cx * 73856093) ^ (unsigned int)(cy * 19349663) ^ 0x5151u;
            auto rnd = [&]() { rng = rng * 1664525u + 1013904223u; return (float)((rng >> 8) & 0xFFFF) / 65535.0f; };
            auto add = [&](int type, int count, float mn, float mx) {
                for (int i = 0; i < count; ++i) {
                    SceneryObject o;
                    o.type = type;
                    o.position = { ox + rnd() * CH, oy + rnd() * CH };
                    o.rotation = rnd() * 3.14159f;
                    o.scale = mn + rnd() * (mx - mn);
                    o.tint = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
                    o.chunk = k;
                    owDecor.scenery.push_back(o);
                }
            };
            add(11, 70, 0.6f, 1.7f);    // grama
            add(2,  10, 0.8f, 1.7f);    // árvores
            add(12, 16, 0.6f, 1.2f);    // pedras/detritos
            add(5,   4, 1.0f, 1.0f);    // postes
            if (rnd() > 0.55f) add(7, 1, 1.0f, 1.7f);   // prédio ocasional
            if (rnd() > 0.70f) add(3, 6, 0.7f, 1.1f);   // lápides ocasionais
        }
}

// ─── Coleta de Recursos Naturais ─────────────────────────────────────────────

const char* Game::resourceName(ResourceType t) {
    switch (t) {
        case ResourceType::Wood:   return "Madeira";
        case ResourceType::Stone:  return "Pedra";
        case ResourceType::Iron:   return "Ferro";
        case ResourceType::Silver: return "Prata";
        case ResourceType::Gold:   return "Ouro";
        default: return "";
    }
}

Color Game::resourceColor(ResourceType t) {
    switch (t) {
        case ResourceType::Wood:   return {120, 80, 40, 255};
        case ResourceType::Stone:  return {150, 150, 160, 255};
        case ResourceType::Iron:   return {110, 90, 80, 255};
        case ResourceType::Silver: return {210, 220, 235, 255};
        case ResourceType::Gold:   return {255, 200, 40, 255};
        default: return GRAY;
    }
}

void Game::setupResourceNodes() {
    resourceNodes.clear();
    if (!openWorldMode) return;
    unsigned int rng = 0x5EED1234u;
    auto rnd = [&]() { rng = rng*1664525u+1013904223u; return (rng>>8) & 0x7FFF; };

    auto addNode = [&](Vector2 pos, ResourceType t, int amt) {
        ResourceNode n;
        n.position = pos; n.type = t; n.amount = amt; n.maxAmount = amt;
        n.harvestProg = 0.0f; n.respawnTimer = 0.0f; n.depleted = false; n.shake = 0.0f;
        resourceNodes.push_back(n);
    };

    // Distribui nós por região conforme o bioma
    for (const auto& r : worldRegions) {
        Rectangle b = r.bounds;
        auto randPos = [&](float margin) -> Vector2 {
            return { b.x + margin + (float)(rnd() % (int)(b.width  - 2*margin)),
                     b.y + margin + (float)(rnd() % (int)(b.height - 2*margin)) };
        };
        // contagens por bioma
        int wood=8, stone=6, iron=3, silver=1, gold=1;
        switch (r.zoneType) {
            case ZoneID::DarkForest:   wood=26; stone=6;  iron=2; silver=1; gold=0; break;
            case ZoneID::CursedFarm:   wood=18; stone=8;  iron=3; silver=1; gold=0; break;
            case ZoneID::Cemetery:     wood=10; stone=14; iron=4; silver=2; gold=1; break;
            case ZoneID::KronosForge:  wood=2;  stone=16; iron=10;silver=4; gold=3; break; // rica em metal
            case ZoneID::LARuins:      wood=6;  stone=12; iron=6; silver=2; gold=1; break;
            case ZoneID::Bunker:       wood=4;  stone=10; iron=8; silver=3; gold=2; break;
            case ZoneID::GhostCity:    wood=4;  stone=14; iron=5; silver=2; gold=1; break;
            case ZoneID::AbandonedManor:wood=10;stone=10; iron=4; silver=3; gold=2; break;
            case ZoneID::KronosNexus:  wood=2;  stone=8;  iron=6; silver=4; gold=4; break;
            default: break;
        }
        for (int i=0;i<wood;i++)   addNode(randPos(120), ResourceType::Wood,   GetRandomValue(3,6));
        for (int i=0;i<stone;i++)  addNode(randPos(120), ResourceType::Stone,  GetRandomValue(3,6));
        for (int i=0;i<iron;i++)   addNode(randPos(140), ResourceType::Iron,   GetRandomValue(2,4));
        for (int i=0;i<silver;i++) addNode(randPos(160), ResourceType::Silver, GetRandomValue(1,3));
        for (int i=0;i<gold;i++)   addNode(randPos(160), ResourceType::Gold,   GetRandomValue(1,2));
    }
}

void Game::updateResourceGathering(float dt) {
    nearResourceIdx = -1;
    float bestDist = 64.0f;
    for (int i = 0; i < (int)resourceNodes.size(); ++i) {
        auto& n = resourceNodes[i];
        if (n.shake > 0.0f) n.shake -= dt * 6.0f;
        if (n.depleted) {
            n.respawnTimer -= dt;
            if (n.respawnTimer <= 0.0f) {       // ressurge cheio
                n.depleted = false; n.amount = n.maxAmount; n.harvestProg = 0.0f;
            }
            continue;
        }
        float d = Vector2Distance(player.position, n.position);
        if (d < bestDist) { bestDist = d; nearResourceIdx = i; }
    }

    // Coletar segurando H no nó mais próximo
    if (nearResourceIdx >= 0 && IsKeyDown(KEY_H)) {
        auto& n = resourceNodes[nearResourceIdx];
        // ferramentas: metais demoram mais que madeira
        float speed = (n.type == ResourceType::Wood)  ? 1.4f :
                      (n.type == ResourceType::Stone) ? 1.0f : 0.7f;
        n.harvestProg += dt * speed;
        n.shake = 1.0f;
        if (n.harvestProg >= 1.0f) {
            n.harvestProg = 0.0f;
            int got = 1;
            playerResources[(int)n.type] += got;
            n.amount -= got;
            damageNumbers.push_back({n.position, (float)got, resourceColor(n.type), 1.0f,
                                     TextFormat("%s+", resourceName(n.type))});
            particles.spawnHit(n.position, resourceColor(n.type), 8);
            audio.playPickup();
            if (n.amount <= 0) {                // esgotou — ressurge depois
                n.depleted = true;
                n.respawnTimer = 35.0f;
                n.harvestProg = 0.0f;
            }
        }
    }
}

void Game::renderResourceNodes() const {
    Vector2 cam = camera.target;
    for (int i = 0; i < (int)resourceNodes.size(); ++i) {
        const auto& n = resourceNodes[i];
        if (n.depleted) continue;
        // culling
        if (std::fabs(n.position.x - cam.x) > 1100 || std::fabs(n.position.y - cam.y) > 700) continue;
        float sx = (n.shake > 0.0f) ? std::sin(n.shake * 30.0f) * 2.0f : 0.0f;
        float x = n.position.x + sx, y = n.position.y;
        Color c = resourceColor(n.type);
        // sombra
        DrawEllipse((int)n.position.x, (int)(y + 14), 16.0f, 5.0f, ColorAlpha(BLACK, 0.35f));
        switch (n.type) {
            case ResourceType::Wood: { // arvore
                DrawRectangle((int)(x-4), (int)(y-6), 8, 22, Color{90,60,30,255});
                DrawCircleV({x, y-22}, 18.0f, Color{30,90,40,255});
                DrawCircleV({x-10, y-14}, 12.0f, Color{36,100,46,255});
                DrawCircleV({x+10, y-14}, 12.0f, Color{28,84,38,255});
                break;
            }
            case ResourceType::Stone: { // pedra
                DrawCircleV({x, y}, 15.0f, Color{120,120,128,255});
                DrawCircleV({x-6, y+2}, 9.0f, Color{145,145,155,255});
                DrawCircleV({x+7, y-1}, 8.0f, Color{100,100,110,255});
                break;
            }
            default: { // minério (ferro/prata/ouro) — rocha com veios coloridos
                DrawCircleV({x, y}, 15.0f, Color{80,72,66,255});
                DrawCircleV({x-5, y+2}, 8.0f, Color{96,88,80,255});
                for (int v = 0; v < 5; ++v) {
                    float a = v * 1.2f + i;
                    DrawCircleV({x + std::cos(a)*7.0f, y + std::sin(a)*7.0f}, 2.6f, c);
                }
                break;
            }
        }
        // Indicador quando é o nó coletável mais próximo
        if (i == nearResourceIdx) {
            DrawCircleLines((int)x, (int)y, 22.0f, ColorAlpha(c, 0.8f));
            const char* lbl = TextFormat("[H] Coletar %s (%d)", resourceName(n.type), n.amount);
            int w = MeasureText(lbl, 11);
            DrawRectangle((int)x - w/2 - 4, (int)y - 46, w + 8, 16, ColorAlpha(BLACK, 0.7f));
            DrawText(lbl, (int)x - w/2, (int)y - 44, 11, c);
            if (n.harvestProg > 0.0f) {
                DrawRectangle((int)x - 20, (int)y - 28, 40, 5, ColorAlpha(BLACK, 0.6f));
                DrawRectangle((int)x - 20, (int)y - 28, (int)(40 * n.harvestProg), 5, c);
            }
        }
    }
}

void Game::drawResourceHUD() const {
    // Painel compacto de recursos (topo, à direita do centro)
    int n = (int)ResourceType::COUNT;
    int pw = 70 * n + 12;
    int px = screenWidth/2 - pw/2;
    int py = 56;
    DrawRectangle(px, py, pw, 22, ColorAlpha(BLACK, 0.5f));
    for (int i = 0; i < n; ++i) {
        ResourceType t = (ResourceType)i;
        Color c = resourceColor(t);
        int ix = px + 8 + i * 70;
        DrawRectangle(ix, py + 6, 10, 10, c);
        DrawText(TextFormat("%d", playerResources[i]), ix + 14, py + 5, 13,
                 ColorAlpha(WHITE, 0.9f));
        DrawText(resourceName(t), ix, py + 24, 8, ColorAlpha(c, 0.0f)); // tooltip oculto
    }
}

// ─── Animais / Vida Selvagem ─────────────────────────────────────────────────

void Game::setupAnimals() {
    animals.clear();
    if (!openWorldMode) return;
    unsigned int rng = 0xA417BEEFu;
    auto rnd = [&]() { rng = rng*1664525u+1013904223u; return (rng>>8) & 0x7FFF; };

    for (const auto& r : worldRegions) {
        Rectangle b = r.bounds;
        // bioma define que animais aparecem
        struct Spawn { AnimalType t; int count; };
        std::vector<Spawn> spawns;
        switch (r.zoneType) {
            case ZoneID::DarkForest:
                spawns = {{AnimalType::Deer,8},{AnimalType::Rabbit,10},{AnimalType::Boar,4},{AnimalType::Wolf,5},{AnimalType::Bird,8}}; break;
            case ZoneID::CursedFarm:
                spawns = {{AnimalType::Deer,6},{AnimalType::Rabbit,8},{AnimalType::Boar,5},{AnimalType::Bird,6}}; break;
            case ZoneID::LARuins: case ZoneID::Bunker:
                spawns = {{AnimalType::Rabbit,5},{AnimalType::Bird,5},{AnimalType::Wolf,2}}; break;
            case ZoneID::Cemetery: case ZoneID::AbandonedManor:
                spawns = {{AnimalType::Wolf,4},{AnimalType::Bird,4},{AnimalType::Rabbit,3}}; break;
            case ZoneID::GhostCity:
                spawns = {{AnimalType::Rabbit,3},{AnimalType::Wolf,3},{AnimalType::Bird,3}}; break;
            default:
                spawns = {{AnimalType::Rabbit,3},{AnimalType::Bird,3}}; break;
        }
        for (auto& sp : spawns) {
            for (int i = 0; i < sp.count; ++i) {
                Animal a{};
                a.position = { b.x + 100 + (float)(rnd() % (int)(b.width  - 200)),
                               b.y + 100 + (float)(rnd() % (int)(b.height - 200)) };
                a.type = sp.t;
                a.hostile = (sp.t == AnimalType::Wolf);
                float hp = (sp.t == AnimalType::Boar) ? 60.f : (sp.t == AnimalType::Wolf) ? 45.f :
                           (sp.t == AnimalType::Deer) ? 35.f : 15.f;
                a.health = a.maxHealth = hp;
                a.wanderDir = { (float)(rnd()%100-50)/50.f, (float)(rnd()%100-50)/50.f };
                a.wanderTimer = (float)(rnd()%300)/100.f;
                a.dead = false; a.fleeing = false; a.animTimer = (float)(rnd()%628)/100.f; a.attackCD = 0.f;
                animals.push_back(a);
            }
        }
    }
}

void Game::updateAnimals(float dt) {
    if (!openWorldMode) return;
    for (auto it = animals.begin(); it != animals.end();) {
        Animal& a = *it;
        a.animTimer += dt;
        if (a.attackCD > 0.f) a.attackCD -= dt;

        float distToPlayer = Vector2Distance(a.position, player.position);

        // Dano de projéteis do player
        for (auto& p : projectiles) {
            if (Vector2Distance(p.position, a.position) < 18.0f) {
                a.health -= 25.0f; a.fleeing = true;
                particles.spawnHit(a.position, Color{200,60,40,255}, 6);
            }
        }
        // Dano melee (clique direito perto)
        if (botMeleeRequest || (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !dialogOpen)) {
            if (distToPlayer < player.attackRange + 20.0f && meleeCooldown <= 0.05f) {
                a.health -= 30.0f; a.fleeing = true;
            }
        }

        if (a.health <= 0.0f) {
            // Caça recompensa: créditos + XP + "carne"
            int cred = (a.type == AnimalType::Boar) ? GetRandomValue(20,40) :
                       (a.type == AnimalType::Deer) ? GetRandomValue(15,30) :
                       (a.type == AnimalType::Wolf) ? GetRandomValue(12,24) : GetRandomValue(4,10);
            player.credits += cred;
            xpOrbs.emplace_back(a.position, cred / 2 + 5);
            damageNumbers.push_back({a.position, (float)cred, Color{255,200,0,255}, 1.0f, "$"});
            particles.spawnHit(a.position, Color{180,40,30,255}, 14);
            audio.playEnemyDeath(false);
            it = animals.erase(it);
            continue;
        }

        // Movimento
        Vector2 move{0,0};
        if (a.hostile && distToPlayer < 320.0f && distToPlayer > 28.0f && !a.fleeing) {
            // lobo persegue
            move = Vector2Normalize(Vector2Subtract(player.position, a.position));
            if (distToPlayer < 40.0f && a.attackCD <= 0.f && !player.isShielded()) {
                player.takeDamage(8.0f); hitFlashTimer = 0.2f; a.attackCD = 1.2f;
            }
        } else if (a.fleeing || (!a.hostile && distToPlayer < 160.0f)) {
            // foge do player
            a.fleeing = (distToPlayer < 280.0f);
            move = Vector2Normalize(Vector2Subtract(a.position, player.position));
        } else {
            // vagueia
            a.wanderTimer -= dt;
            if (a.wanderTimer <= 0.f) {
                a.wanderDir = { (float)(GetRandomValue(-100,100))/100.f,
                                (float)(GetRandomValue(-100,100))/100.f };
                a.wanderTimer = (float)GetRandomValue(2,5);
            }
            move = a.wanderDir;
        }
        float spd = (a.type==AnimalType::Rabbit||a.type==AnimalType::Bird) ? 140.f :
                    (a.type==AnimalType::Wolf) ? 165.f :
                    a.fleeing ? 180.f : 55.f;
        a.position.x += move.x * spd * dt;
        a.position.y += move.y * spd * dt;
        ++it;
    }
}

void Game::renderAnimals() const {
    Vector2 cam = camera.target;
    for (const auto& a : animals) {
        if (std::fabs(a.position.x - cam.x) > 1100 || std::fabs(a.position.y - cam.y) > 700) continue;
        float x = a.position.x, y = a.position.y;
        float bob = std::sin(a.animTimer * 8.0f) * 1.5f;
        DrawEllipse((int)x, (int)(y+8), 12.0f, 4.0f, ColorAlpha(BLACK, 0.3f));
        switch (a.type) {
            case AnimalType::Deer: {
                Color body={150,110,70,255};
                DrawEllipse((int)x,(int)(y+bob),12.0f,8.0f,body);
                DrawCircleV({x+9,y-6+bob},5.0f,body);             // cabeça
                DrawLine((int)x+9,(int)(y-10+bob),(int)x+6,(int)(y-16+bob),Color{90,60,30,255}); // chifre
                DrawLine((int)x+11,(int)(y-10+bob),(int)x+14,(int)(y-16+bob),Color{90,60,30,255});
                DrawRectangle((int)x-8,(int)(y+6),2,8,body); DrawRectangle((int)x+6,(int)(y+6),2,8,body);
                break;
            }
            case AnimalType::Rabbit: {
                Color body={210,200,190,255};
                DrawCircleV({x,y+bob},6.0f,body);
                DrawEllipse((int)(x-2),(int)(y-8+bob),2.0f,5.0f,body); // orelhas
                DrawEllipse((int)(x+2),(int)(y-8+bob),2.0f,5.0f,body);
                break;
            }
            case AnimalType::Boar: {
                Color body={90,70,60,255};
                DrawEllipse((int)x,(int)(y+bob),13.0f,8.0f,body);
                DrawCircleV({x+10,y+bob},5.0f,body);
                DrawCircleV({x+13,y+bob},2.0f,Color{40,30,25,255}); // focinho
                break;
            }
            case AnimalType::Wolf: {
                Color body=a.fleeing?Color{120,120,130,255}:Color{90,95,105,255};
                DrawEllipse((int)x,(int)(y+bob),12.0f,7.0f,body);
                DrawCircleV({x+9,y-3+bob},5.0f,body);
                DrawLine((int)x+7,(int)(y-7+bob),(int)x+6,(int)(y-11+bob),body); // orelha
                DrawLine((int)x+11,(int)(y-7+bob),(int)x+12,(int)(y-11+bob),body);
                DrawCircleV({x+11,y-3+bob},1.5f,Color{255,200,0,255}); // olho
                break;
            }
            default: { // Bird
                float fl = std::sin(a.animTimer*12.0f)*4.0f;
                Color body={60,60,70,255};
                DrawCircleV({x,y-20+bob*2},3.0f,body);
                DrawLine((int)x,(int)(y-20+bob*2),(int)(x-6),(int)(y-20-fl+bob*2),body);
                DrawLine((int)x,(int)(y-20+bob*2),(int)(x+6),(int)(y-20-fl+bob*2),body);
                break;
            }
        }
    }
}

// ─── Juice de combate: decalques de chão (sangue/queimado) ───────────────────
void Game::addDecal(Vector2 p, Color c, int type, float size) {
    if (decals.size() > 120) decals.erase(decals.begin());  // teto p/ perf
    decals.push_back({ p, c, 10.0f, 10.0f, size, type });
}

void Game::renderDecals() const {
    Vector2 cam = camera.target;
    for (const auto& d : decals) {
        if (std::fabs(d.pos.x - cam.x) > 1000 || std::fabs(d.pos.y - cam.y) > 650) continue;
        float a = (d.life / d.maxLife);   // some aos poucos
        if (d.type == 0) { // mancha de sangue — manchas irregulares
            DrawEllipse((int)d.pos.x, (int)d.pos.y, d.size, d.size*0.6f, ColorAlpha(d.color, 0.45f*a));
            DrawCircleV({d.pos.x - d.size*0.4f, d.pos.y + 2}, d.size*0.35f, ColorAlpha(d.color, 0.4f*a));
            DrawCircleV({d.pos.x + d.size*0.5f, d.pos.y - 1}, d.size*0.3f,  ColorAlpha(d.color, 0.35f*a));
        } else {           // marca de queimado/faísca — escuro com brasa
            DrawCircleV(d.pos, d.size*0.7f, ColorAlpha(Color{20,18,16,255}, 0.5f*a));
            DrawCircleLines((int)d.pos.x, (int)d.pos.y, d.size*0.7f, ColorAlpha(Color{255,120,30,255}, 0.3f*a));
        }
    }
}

// ─── Pathfinding BFS (grade de tiles transitaveis) ───────────────────────────
// Acha um caminho de 'from' ate 'to' contornando paredes. Usado quando o bot
// trava em cantos concavos (o sensor de 8 direcoes oscila sem sair).
bool Game::botFindPath(Vector2 from, Vector2 to, std::vector<Vector2>& outPath) const {
    outPath.clear();
    const int W = tilemap.width, H = tilemap.height;
    int ts = Tilemap::tileSize;
    int sx = (int)(from.x / ts), sy = (int)(from.y / ts);
    int gx = (int)(to.x   / ts), gy = (int)(to.y   / ts);
    if (sx < 0 || sy < 0 || sx >= W || sy >= H) return false;
    if (gx < 0) gx = 0; if (gy < 0) gy = 0;
    if (gx >= W) gx = W-1; if (gy >= H) gy = H-1;
    auto walkable = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= W || y >= H) return false;
        return tilemap.tiles[y][x].type != TileType::Wall;
    };
    if (!walkable(gx, gy)) {
        // procura tile transitavel proximo do alvo
        bool found = false;
        for (int r = 1; r < 8 && !found; ++r)
            for (int dy=-r; dy<=r && !found; ++dy)
                for (int dx=-r; dx<=r && !found; ++dx)
                    if (walkable(gx+dx, gy+dy)) { gx+=dx; gy+=dy; found=true; }
        if (!found) return false;
    }
    // BFS
    std::vector<int> prev(W*H, -1);
    std::vector<char> seen(W*H, 0);
    std::vector<int> q; q.reserve(1024);
    int start = sy*W+sx, goal = gy*W+gx;
    q.push_back(start); seen[start] = 1;
    const int dx4[4]={1,-1,0,0}, dy4[4]={0,0,1,-1};
    size_t head = 0; bool reached = false;
    int budget = 9000; // limite de nos para nao custar caro
    while (head < q.size() && budget-- > 0) {
        int cur = q[head++];
        if (cur == goal) { reached = true; break; }
        int cx = cur % W, cy = cur / W;
        for (int d=0; d<4; ++d) {
            int nx=cx+dx4[d], ny=cy+dy4[d];
            if (!walkable(nx,ny)) continue;
            int ni = ny*W+nx;
            if (seen[ni]) continue;
            seen[ni]=1; prev[ni]=cur; q.push_back(ni);
        }
    }
    if (!reached) return false;
    // reconstroi e converte para centros de tile (do inicio ao fim)
    std::vector<Vector2> rev;
    for (int c = goal; c != -1; c = prev[c]) {
        int cx = c % W, cy = c / W;
        rev.push_back({ (cx + 0.5f) * ts, (cy + 0.5f) * ts });
        if (c == start) break;
    }
    for (int i = (int)rev.size()-1; i >= 0; --i) outPath.push_back(rev[i]);
    return outPath.size() > 1;
}

// ─── Multiplayer LAN (NetClient) ─────────────────────────────────────────────

void Game::startNetwork() {
    if (netActive) return;
    netId     = (uint32_t)GetRandomValue(1, 2000000000);
    netActive = net.init(Player::className(player.charClass), netId);
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

// ─── Grupo / Aliança (party multiplayer) ─────────────────────────────────────

void Game::updateParty() {
    if (chatActive) return;
    if (IsKeyPressed(KEY_O)) { partyPanel = !partyPanel; partyInput.clear(); }
    if (!partyPanel) return;

    // Digita um codigo numerico de grupo
    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= '0' && ch <= '9' && partyInput.size() < 6) partyInput.push_back((char)ch);
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !partyInput.empty()) partyInput.pop_back();

    // ENTER entra no grupo digitado; L = sala publica; C = cria grupo (codigo do netId)
    if (IsKeyPressed(KEY_ENTER) && !partyInput.empty()) {
        net.joinParty("party_" + partyInput);
        triggerPlayerSpeech("Entrou no grupo " + partyInput, 2.5f);
        partyPanel = false;
    }
    if (IsKeyPressed(KEY_L)) {
        net.joinParty("lobby");
        triggerPlayerSpeech("Entrou na sala publica.", 2.0f);
        partyPanel = false;
    }
    if (IsKeyPressed(KEY_C)) {
        std::string code = std::to_string(1000 + (int)(netId % 9000));
        partyInput = code;
        net.joinParty("party_" + code);
        triggerPlayerSpeech("Grupo criado! Codigo: " + code, 4.0f);
        partyPanel = false;
    }
}

void Game::drawPartyPanel() const {
    // Indicador permanente: sala atual + nº de aliados online
    Color C_cyan = {0,210,255,255};
    std::string room = net.currentRoom();
    bool isParty = (room.rfind("party_", 0) == 0);
    int allies = (int)net.peers().size();
    const char* roomLbl = isParty ? room.c_str() + 6 : "PUBLICO";
    DrawText(TextFormat("GRUPO: %s  |  Aliados online: %d  [O]",
             isParty ? roomLbl : "PUBLICO", allies),
             14, 30, 11, ColorAlpha(C_cyan, 0.6f));

    if (!partyPanel) return;

    int pw = 460, ph = 250;
    int px = screenWidth/2 - pw/2, py = screenHeight/2 - ph/2;
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.55f));
    DrawPanel(px, py, pw, ph, C_cyan, 0.95f);
    DrawText("GRUPO / ALIANCA", px + 18, py + 14, 22, C_cyan);
    DrawText("Jogue junto com amigos na mesma sala em tempo real.",
             px + 18, py + 44, 12, ColorAlpha(WHITE, 0.6f));

    int y = py + 78;
    DrawText(TextFormat("Sala atual: %s", isParty ? roomLbl : "PUBLICO (lobby)"),
             px + 18, y, 14, C_cyan); y += 26;
    DrawText(TextFormat("Aliados conectados: %d", allies), px + 18, y, 14,
             Color{0,230,120,255}); y += 30;

    DrawText("Digite um codigo e ENTER para entrar num grupo:", px + 18, y, 12,
             ColorAlpha(WHITE, 0.7f)); y += 20;
    DrawRectangle(px + 18, y, 200, 26, ColorAlpha(BLACK, 0.5f));
    DrawRectangleLinesEx({(float)(px+18),(float)y,200,26}, 1.5f, C_cyan);
    DrawText(partyInput.empty() ? "_" : partyInput.c_str(), px + 26, y + 5, 18, WHITE);
    y += 38;

    DrawText("[C] Criar grupo privado   [L] Sala publica   [O] Fechar",
             px + 18, y, 12, ColorAlpha(C_cyan, 0.8f));
    // Lista de aliados na sala
    y += 26;
    int shown = 0;
    for (const auto& p : net.peers()) {
        if (shown >= 4) break;
        DrawText(TextFormat("- %s", p.name[0] ? p.name : "Operador"),
                 px + 26, y, 12, ColorAlpha(WHITE, 0.75f));
        y += 16; shown++;
    }
}

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

// ─── Run / Update ────────────────────────────────────────────────────────────

void Game::runAutoTest(bool autoTest) {
    if (autoTest) {
        // Skip menu, start game immediately with bot active
        buildQuests();
        if (openWorldMode) { tilemap.generateOpenWorld(); setupWorldRegions(); buildOpenWorldScenery(); currentRegion = currentZone; }
        else { tilemap.generate(currentZone); }
        setupZoneNPCs(currentZone);
        inMainMenu = false;
        render3D   = true;   // autoteste no modo 3D/2.5D isométrico completo
        botController.active   = true;
        botController.autoTest = true;
        botController.testDuration = 7200.0f; // 2 horas max
        botController.addLog("=== AUTO-BOT TEST MODE ATIVADO ===");
        botController.addLog("Duracao maxima: 7200s (2h)");
        startNetwork();   // testa o cliente WebSocket (multiplayer)
        startStore();     // testa login + catalogo da loja premium
    }
    run();
    // After run() exits, write report if bot was active
    if (botController.active || autoTest) {
        botController.writeReport("C:\\Users\\ricar\\darknet-prototype\\bot_report.txt");
    }
}

void Game::run() {
    bool menuMusicStarted = false;
    while (!WindowShouldClose() && !quitRequested) {
        float dt = GetFrameTime();

        if (inMainMenu) {
            // Start menu music once
            if (!menuMusicStarted) { audio.playMenuMusic(); menuMusicStarted = true; }
            audio.updateMusic();

            bool hasSave = SaveManager::exists();
            Vector2 mouse = virtualizeMousePos(GetMousePosition());
            int cx = screenWidth/2;
            int cy = screenHeight/2;
            int bw = 360;
            // Button Y positions matching drawMainMenu
            int btn0y = hasSave ? cy - 2  : cy + 20;
            int btn1y = hasSave ? cy + 44 : cy + 66;
            int btn2y = hasSave ? cy + 90 : -999;

            auto hitBtn = [&](int y) {
                return mouse.x >= cx-bw/2 && mouse.x <= cx+bw/2 &&
                       mouse.y >= y-3     && mouse.y <= y+33;
            };

            // Hover tracking
            menuHoveredBtn = -1;
            if (hitBtn(btn0y)) menuHoveredBtn = 0;
            else if (hitBtn(btn1y)) menuHoveredBtn = 1;
            else if (btn2y > 0 && hitBtn(btn2y)) menuHoveredBtn = 2;

            bool mouseClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

            // ── Difficulty overlay ────────────────────────────────────────────
            if (selectingDifficulty) {
                const int dCW = 196, dCG = 10;
                const int dSX = (screenWidth - (5*dCW + 4*dCG)) / 2;
                const int dSY = 150, dCH = 268;
                for (int i = 0; i < 5; ++i) {
                    int bx = dSX + i*(dCW+dCG);
                    if (mouse.x >= bx && mouse.x <= bx+dCW &&
                        mouse.y >= dSY && mouse.y <= dSY+dCH)
                        difficultyHovered = i;
                }
                if (IsKeyPressed(KEY_LEFT)  && difficultyHovered > 0) difficultyHovered--;
                if (IsKeyPressed(KEY_RIGHT) && difficultyHovered < 4) difficultyHovered++;
                bool dConf = false;
                if (mouseClicked) {
                    for (int i = 0; i < 5; ++i) {
                        int bx = dSX + i*(dCW+dCG);
                        if (mouse.x >= bx && mouse.x <= bx+dCW &&
                            mouse.y >= dSY && mouse.y <= dSY+dCH)
                        { difficultyHovered = i; dConf = true; break; }
                    }
                }
                if (IsKeyPressed(KEY_ENTER)) dConf = true;
                if (dConf) {
                    difficulty = (DifficultyLevel)difficultyHovered;
                    selectingDifficulty = false;
                    if (pendingNewGame) {
                        // Novo jogo: escolher PERSONAGEM antes de comecar
                        selectingCharacter = true;
                        characterHovered   = 0;
                        drawMainMenu(); drawCharacterSelectScreen(); presentFrame(); continue;
                    } else {
                        if (hasSave) SaveManager::load(player, quests, currentZone);
                        if (openWorldMode) {
                            tilemap.generateOpenWorld();
                            setupWorldRegions();
                            buildOpenWorldScenery();
                            currentRegion = currentZone;
                        } else {
                            tilemap.generate(currentZone);
                        }
                        setupZoneNPCs(currentZone);
                        spawnInterval = getZoneInfo(currentZone).spawnInterval / getDifficulty().spawnRateMult;
                        inMainMenu = false;
                        audio.stopMenuMusic(); audio.setZone(currentZone);
                        triggerPlayerSpeech("Missao iniciada. Eliminando ameacas KRONOS.", 4.0f);
                    }
                    drawMainMenu(); presentFrame(); continue;
                }
                if (IsKeyPressed(KEY_ESCAPE)) selectingDifficulty = false;
                drawMainMenu(); drawDifficultyScreen(); presentFrame(); continue;
            }

            // ── Selecao de PERSONAGEM (apos a dificuldade, em novo jogo) ──────
            if (selectingCharacter) {
                const int total = (int)CharacterClass::COUNT; // 6
                const int cardW = 188, cardG = 10;
                const int totalW = total*cardW + (total-1)*cardG;
                const int csx = (screenWidth - totalW) / 2;
                const int csy = 150, cardH = 300;
                for (int i = 0; i < total; ++i) {
                    int bx = csx + i*(cardW+cardG);
                    if (mouse.x >= bx && mouse.x <= bx+cardW &&
                        mouse.y >= csy && mouse.y <= csy+cardH)
                        characterHovered = i;
                }
                if (IsKeyPressed(KEY_LEFT)  && characterHovered > 0)         characterHovered--;
                if (IsKeyPressed(KEY_RIGHT) && characterHovered < total-1)   characterHovered++;
                bool cConf = false;
                if (mouseClicked) {
                    for (int i = 0; i < total; ++i) {
                        int bx = csx + i*(cardW+cardG);
                        if (mouse.x >= bx && mouse.x <= bx+cardW &&
                            mouse.y >= csy && mouse.y <= csy+cardH)
                        { characterHovered = i; cConf = true; break; }
                    }
                }
                if (IsKeyPressed(KEY_ENTER)) cConf = true;
                if (IsKeyPressed(KEY_ESCAPE)) { selectingCharacter = false; selectingDifficulty = true; }
                if (cConf) {
                    player.applyClass((CharacterClass)characterHovered);
                    selectingCharacter = false;
                    startNewGame();
                    drawMainMenu(); presentFrame(); continue;
                }
                drawMainMenu(); drawCharacterSelectScreen(); presentFrame(); continue;
            }

            // Botao 0 / ENTER:
            //  - com save  = CONTINUAR (carrega direto, SEM tela de dificuldade)
            //  - sem save  = NOVO JOGO (mostra dificuldade)
            if (IsKeyPressed(KEY_ENTER) || (mouseClicked && menuHoveredBtn == 0)) {
                if (hasSave) {
                    startLoadedGame();
                    presentFrame();
                    continue;
                } else {
                    pendingNewGame = true; selectingDifficulty = true;
                }
            }
            // Novo jogo (tecla N ou botao 1, so existe quando ha save) — mostra dificuldade
            if (IsKeyPressed(KEY_N) || (mouseClicked && hasSave && menuHoveredBtn == 1)) {
                pendingNewGame = true; selectingDifficulty = true;
            }
            if (IsKeyPressed(KEY_ESCAPE) || (mouseClicked && menuHoveredBtn == (hasSave ? 2 : 1))) {
                break;
            }
            drawMainMenu();
            if (selectingDifficulty) drawDifficultyScreen();
            presentFrame();
            continue;
        }

        // F11 — toggle fullscreen (RenderTexture handles scaling)
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (shopSystem.open) { shopSystem.close(); continue; }
            if (craftingSystem.open) { craftingSystem.open = false; continue; }
            if (dialogOpen) { dialogOpen = false; continue; }
            paused = !paused;
            // Renderiza e PULA o processamento de input deste frame, senao o
            // mesmo ESC seria lido pelo menu de pause e fecharia na hora.
            render();
            presentFrame();
            continue;
        }

        if (paused) {
            // Menu de pause (9 opcoes): Continuar/Salvar/Dificuldade/Trilha/Efeitos/
            //                            Vozes/Reiniciar/Menu/Sair
            const int PAUSE_OPTS = 9;
            Vector2 pm = virtualizeMousePos(GetMousePosition());
            int pcx = screenWidth / 2;
            int pby = screenHeight / 2 - 150;  // mesma base do drawPauseMenu
            int pbw = 340, pbh = 32, pgap = 6;
            pauseHovered = -1;
            for (int i = 0; i < PAUSE_OPTS; ++i) {
                int y = pby + i * (pbh + pgap);
                if (pm.x >= pcx - pbw/2 && pm.x <= pcx + pbw/2 &&
                    pm.y >= y && pm.y <= y + pbh) pauseHovered = i;
            }
            // Navegacao por teclado
            if (IsKeyPressed(KEY_DOWN)) pauseHovered = (pauseHovered + 1 + PAUSE_OPTS) % PAUSE_OPTS;
            if (IsKeyPressed(KEY_UP))   pauseHovered = (pauseHovered - 1 + PAUSE_OPTS) % PAUSE_OPTS;

            int chosen = -1;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && pauseHovered >= 0) chosen = pauseHovered;
            if (IsKeyPressed(KEY_ENTER) && pauseHovered >= 0) chosen = pauseHovered;
            // Atalhos diretos
            if (IsKeyPressed(KEY_ESCAPE)) chosen = 0;   // ESC continua
            if (IsKeyPressed(KEY_F5))     chosen = 1;   // F5 salva

            if (chosen == 0) {            // Continuar
                paused = false;
            } else if (chosen == 1) {     // Salvar
                autoSave();
                showStoryBanner("JOGO SALVO", "Progresso gravado com sucesso.", 2.0f);
            } else if (chosen == 2) {     // Dificuldade — cicla e aplica (continua pausado)
                difficulty = (DifficultyLevel)(((int)difficulty + 1) % 5);
                spawnInterval = getZoneInfo(currentZone).spawnInterval / getDifficulty().spawnRateMult;
            } else if (chosen == 3) {     // Trilha sonora ON/OFF
                audio.setMusicEnabled(!audio.musicEnabled);
            } else if (chosen == 4) {     // Todos os sons ON/OFF (master)
                audio.setAllSoundOn(!audio.allSoundOn);
            } else if (chosen == 5) {     // Vozes/personagens ON/OFF
                audio.setVoiceEnabled(!audio.voiceEnabled);
            } else if (chosen == 6) {     // Reiniciar partida
                paused = false;
                restartRun();
            } else if (chosen == 7) {     // Voltar ao menu principal
                paused = false;
                inMainMenu = true;
            } else if (chosen == 8) {     // Sair do jogo
                quitRequested = true;
            }

            render();       // drawPauseMenu() is called inside render() when paused
            presentFrame();
            continue;
        }

        auto _t0 = std::chrono::high_resolution_clock::now();
        update(dt);
        if (WindowShouldClose()) break;  // bot may have called CloseWindow()
        auto _t1 = std::chrono::high_resolution_clock::now();
        render();
        auto _t2 = std::chrono::high_resolution_clock::now();
        if (botController.active) {
            float um = std::chrono::duration<float, std::milli>(_t1 - _t0).count();
            float rm = std::chrono::duration<float, std::milli>(_t2 - _t1).count();
            if (um > botController.peakUpdateMs) botController.peakUpdateMs = um;
            if (rm > botController.peakRenderMs) botController.peakRenderMs = rm;
        }
        presentFrame();
    }
}

void Game::startNewGame() {
    // Inicia a partida do zero apos escolher dificuldade e personagem.
    buildQuests();
    currentZone   = ZoneID::LARuins;
    currentRegion = ZoneID::LARuins;
    if (openWorldMode) {
        tilemap.generateOpenWorld();
        setupWorldRegions();
        buildOpenWorldScenery();
        float ox = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f;
        float oy = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
        player.position = {ox, oy};
        safeZoneCenter  = {ox, oy};
    } else {
        tilemap.generate(currentZone);
    }
    setupZoneNPCs(currentZone);
    spawnInterval = getZoneInfo(currentZone).spawnInterval / getDifficulty().spawnRateMult;
    inMainMenu = false; storyChapter = 1;
    // Reset do motor de evolucao para a nova partida
    threatLevel = 1; threatTimer = 0.0f; threatKillMark = 0;
    activeMutator = WorldMutator::None; mutatorTimer = 0.0f;
    totalKills = 0; enemiesKilled = 0; sessionTime = 0.0f;
    audio.stopMenuMusic(); audio.setZone(currentZone);
    showStoryBanner("CAPITULO 1: O JULGAMENTO",
        "2047 - KRONOS domina. O NEXUS e a ultima esperanca da humanidade.", 5.0f);
    triggerPlayerSpeech(TextFormat("%s pronto para o combate.",
                        Player::className(player.charClass)), 4.0f);
    startNetwork();   // multiplayer em tempo real (mostra outros jogadores)
    startStore();     // loja premium (login + catalogo de gems)
}

void Game::drawCharacterSelectScreen() const {
    BeginTextureMode(gameTarget);  // overlay sobre o menu
    float t = (float)GetTime();
    int cx = screenWidth / 2;

    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.82f));
    const char* title = "ESCOLHA SEU PERSONAGEM";
    int tw = MeasureText(title, 34);
    DrawText(title, cx - tw/2, 80, 34, Color{0,220,255,255});
    const char* sub = "Cada classe tem visual, stats e estilo proprios.";
    int sw = MeasureText(sub, 16);
    DrawText(sub, cx - sw/2, 120, 16, ColorAlpha(WHITE, 0.6f));

    const int total = (int)CharacterClass::COUNT;
    const int cardW = 188, cardG = 10;
    const int totalW = total*cardW + (total-1)*cardG;
    const int csx = (screenWidth - totalW) / 2;
    const int csy = 150, cardH = 300;

    // Cores por classe (combinam com o visual do Player)
    const Color cardCols[6] = {
        {60,90,150,255},   // Soldado
        {180,70,120,255},  // Guerreira
        {120,130,150,255}, // Robo
        {90,60,160,255},   // Mago
        {140,60,170,255},  // Bruxa
        {150,90,40,255},   // HomemFera
    };

    for (int i = 0; i < total; ++i) {
        int bx = csx + i*(cardW+cardG);
        bool sel = (i == characterHovered);
        Color col = cardCols[i];
        // Card
        DrawRectangle(bx, csy, cardW, cardH, ColorAlpha(sel ? col : Color{20,24,34,255},
                      sel ? 0.55f : 0.85f));
        DrawRectangleLinesEx({(float)bx,(float)csy,(float)cardW,(float)cardH},
                             sel ? 3.0f : 1.5f,
                             sel ? Color{0,230,255,255} : ColorAlpha(col, 0.7f));
        if (sel) { // brilho pulsante
            float p = 0.5f + 0.5f * std::sin(t*4.0f);
            DrawRectangleLinesEx({(float)bx-2,(float)csy-2,(float)cardW+4,(float)cardH+4},
                                 1.5f, ColorAlpha(Color{0,230,255,255}, p));
        }
        // Nome
        CharacterClass cc = (CharacterClass)i;
        const char* nm = Player::className(cc);
        int nw = MeasureText(nm, 20);
        DrawText(nm, bx + cardW/2 - nw/2, csy + 12, 20, sel ? WHITE : col);

        // Avatar (retrato pixel-art da classe)
        int ax = bx + cardW/2, ay = csy + 108;
        SpriteBank& sb = SpriteBank::get();
        if (sb.ready && i < SpriteBank::NUM_CHAR_AVATARS) {
            Texture2D av = sb.charAvatar[i];
            float scale = 1.0f;
            float aw = av.width * scale, ah = av.height * scale;
            // halo da cor da classe atras
            DrawCircle(ax, ay, 50, ColorAlpha(col, 0.18f));
            DrawTexturePro(av, {0,0,(float)av.width,(float)av.height},
                           {ax - aw/2, ay - ah/2, aw, ah}, {0,0}, 0.0f, WHITE);
        } else {
            DrawCircle(ax, ay, 42, ColorAlpha(col, 0.25f));
            DrawCircleLines(ax, ay, 42.0f, ColorAlpha(col, 0.8f));
        }

        // Descricao + sabor
        const char* desc = Player::classDescription(cc);
        std::string d = desc ? desc : "";
        // quebra simples
        int ty = csy + 170, lineMax = 22;
        std::string word, line;
        std::istringstream iss(d);
        while (iss >> word) {
            std::string test = line.empty() ? word : line + " " + word;
            if ((int)test.size() > lineMax) {
                DrawText(line.c_str(), bx + 10, ty, 11, ColorAlpha(WHITE, 0.8f));
                ty += 15; line = word;
            } else line = test;
        }
        if (!line.empty()) DrawText(line.c_str(), bx + 10, ty, 11, ColorAlpha(WHITE, 0.8f));

        const char* fant = Player::classFantasy(cc);
        if (fant) {
            std::string f = fant; std::string l2, w2;
            int fy = csy + cardH - 64; std::istringstream iss2(f);
            while (iss2 >> w2) {
                std::string test = l2.empty() ? w2 : l2 + " " + w2;
                if ((int)test.size() > 24) {
                    DrawText(l2.c_str(), bx + 10, fy, 10, ColorAlpha(col, 0.9f));
                    fy += 13; l2 = w2;
                } else l2 = test;
            }
            if (!l2.empty()) DrawText(l2.c_str(), bx + 10, fy, 10, ColorAlpha(col, 0.9f));
        }
    }

    DrawText("Setas/Mouse para escolher  -  ENTER/Clique para confirmar  -  ESC volta",
             cx - 250, csy + cardH + 24, 14, ColorAlpha(WHITE, 0.55f));
    EndTextureMode();
}

void Game::startLoadedGame() {
    // Carrega o save e entra direto no jogo — SEM tela de dificuldade.
    // A dificuldade salva e mantida (so muda em Novo Jogo ou pelo menu de pause).
    if (SaveManager::exists()) SaveManager::load(player, quests, currentZone);
    if (openWorldMode) {
        tilemap.generateOpenWorld();
        setupWorldRegions();
        buildOpenWorldScenery();
        currentRegion = currentZone;
    } else {
        tilemap.generate(currentZone);
    }
    setupZoneNPCs(currentZone);
    spawnInterval = getZoneInfo(currentZone).spawnInterval / getDifficulty().spawnRateMult;
    inMainMenu = false;
    selectingDifficulty = false;
    audio.stopMenuMusic(); audio.setZone(currentZone);
    triggerPlayerSpeech("Partida carregada. Retomando a missao.", 4.0f);
    startNetwork();   // multiplayer em tempo real
    startStore();     // loja premium (login + catalogo de gems)
}

void Game::restartRun() {
    // Reset do jogador (o construtor reconfigura skills e stats base)
    player = Player();

    // Limpa todas as entidades em jogo
    enemies.clear();
    items.clear();
    projectiles.clear();
    enemyProjectiles.clear();
    xpOrbs.clear();
    groundEquips.clear();
    companions.clear();
    damageNumbers.clear();
    particles.particles.clear();
    buildingSystem.buildings.clear();
    buildingSystem.tanks.clear();
    buildingSystem.soldiers.clear();
    buildingSystem.buildModeActive = false;

    // Reset de progressao e flags
    enemiesKilled       = 0;
    totalKills          = 0;
    sessionTime         = 0.0f;
    storyChapter        = 1;
    omegaKillThreshold  = 50;
    threatLevel         = 1;
    threatTimer         = 0.0f;
    threatKillMark      = 0;
    activeMutator       = WorldMutator::None;
    mutatorTimer        = 0.0f;
    gameWon             = false;
    victoryTimer        = 0.0f;
    finalBossSpawned    = false;
    finalBossAlive      = false;
    pendingLevelUps     = 0;
    pendingEvolutions   = 0;
    showLevelUpScreen   = false;
    showEvolutionScreen = false;
    showInventory = showEquipment = showQuestLog = false;
    shopSystem.close();
    craftingSystem.open = false;
    hasTarget   = false;
    rtsDragging = false;
    rtsHasUnits = false;

    // Reconstroi quests e mundo
    quests.clear();
    buildQuests();
    currentZone   = ZoneID::LARuins;
    currentRegion = ZoneID::LARuins;
    if (openWorldMode) {
        tilemap.generateOpenWorld();
        setupWorldRegions();
        buildOpenWorldScenery();
        float ox = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f;
        float oy = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
        player.position = {ox, oy};
    } else {
        tilemap.generate(currentZone);
    }
    setupZoneNPCs(currentZone);
    infernoZone.active = false;
    darkWorld.active   = false;
    darkZoneActive     = false;
    lightSystem.setEnabled(false);
    anomalySystem.storm.stop();
    audio.setZone(currentZone);

    showStoryBanner("PARTIDA REINICIADA", "Uma nova tentativa contra o KRONOS.", 3.0f);
    triggerPlayerSpeech("Reiniciando sistemas de combate.", 3.0f);
}

const char* Game::mutatorName(WorldMutator m) const {
    switch (m) {
        case WorldMutator::SwiftEnemies:  return "FRENESI VELOZ";
        case WorldMutator::ArmoredEnemies:return "BLINDAGEM PESADA";
        case WorldMutator::BloodMoon:     return "LUA DE SANGUE";
        case WorldMutator::LootRain:      return "CHUVA DE ESPOLIO";
        case WorldMutator::Frenzy:        return "INVASAO TOTAL";
        case WorldMutator::Berserk:       return "FUROR KRONOS";
        default:                          return "";
    }
}

const char* Game::mutatorDesc(WorldMutator m) const {
    switch (m) {
        case WorldMutator::SwiftEnemies:  return "Inimigos +35% velocidade. Reaja rapido!";
        case WorldMutator::ArmoredEnemies:return "Inimigos +60% vida. Traga poder de fogo.";
        case WorldMutator::BloodMoon:     return "Inimigos se curam ao te atingir.";
        case WorldMutator::LootRain:      return "Espolio +150%. Cace tudo agora!";
        case WorldMutator::Frenzy:        return "Inimigos surgem em dobro.";
        case WorldMutator::Berserk:       return "Inimigos +40% dano. Cuidado extremo.";
        default:                          return "";
    }
}

void Game::rollNewMutator() {
    // Escolhe um mutador diferente do atual (variedade garantida)
    int n = (int)WorldMutator::COUNT - 1; // exclui None
    WorldMutator next = activeMutator;
    for (int tries = 0; tries < 8 && next == activeMutator; ++tries)
        next = (WorldMutator)(1 + GetRandomValue(0, n - 1));
    activeMutator = next;
    mutatorTimer  = 0.0f;
    showStoryBanner(TextFormat("MUTADOR: %s", mutatorName(activeMutator)),
                    mutatorDesc(activeMutator), 4.0f);
    triggerPlayerSpeech("As regras mudaram. Adapte-se.", 3.0f);
}

void Game::updateEvolutionEngine(float dt) {
    if (inSafeZone(player.position)) return; // a base nao escala (refugio)

    // ── Nivel de Ameaca: sobe por TEMPO ou por KILLS — o que vier primeiro ────
    threatTimer += dt;
    bool levelByTime  = threatTimer >= 100.0f;
    bool levelByKills = (totalKills - threatKillMark) >= 40;
    if (levelByTime || levelByKills) {
        threatLevel++;
        threatTimer    = 0.0f;
        threatKillMark = totalKills;
        // Recompensa de marco + anuncio de novidade
        int bonus = 50 * threatLevel;
        player.credits += bonus;
        showStoryBanner(TextFormat("NIVEL DE AMEACA %d", threatLevel),
            TextFormat("KRONOS escala. Inimigos +%.0f%% mais fortes. Bonus: $%d",
                       (threatStatMult()-1.0f)*100.0f, bonus), 4.0f);
        triggerPlayerSpeech("O KRONOS esta evoluindo. Eu tambem vou.", 3.0f);
        audio.playLevelUp();
    }

    // ── Mutadores rotativos: muda o "sabor" do mundo periodicamente ───────────
    mutatorTimer += dt;
    if (activeMutator == WorldMutator::None) {
        // primeiro mutador comeca apos ~60s de jogo
        if (sessionTime > 60.0f) rollNewMutator();
    } else if (mutatorTimer >= mutatorDuration) {
        rollNewMutator();
    }
}

void Game::playEnemyDeathSound(const Enemy& e) {
    // Som de morte por FACCAO/tipo do inimigo
    using ET = EnemyType;
    if (e.isFinalBoss || e.type==ET::Boss || e.type==ET::AlienBoss || e.type==ET::OmegaBoss ||
        e.type==ET::VoidColossus || e.type==ET::FrostWyrm || e.type==ET::InfernoHerald ||
        e.type==ET::VolcanicTitan || e.type==ET::Leviathan || e.type==ET::ZombieLord ||
        e.type==ET::PoltergeistBoss || e.type==ET::Broodmother) {
        audio.playBossRoar();                       // chefes
    } else if (e.type==ET::Zergling || e.type==ET::Hydra || e.type==ET::CorrupterDrone ||
               e.type==ET::AcidSpitter || e.type==ET::NeuralParasite || e.type==ET::AbyssalEel ||
               e.type==ET::MorphX || e.type==ET::ChaosSpawn) {
        audio.playAlienScream();                    // aliens/orgânicos
    } else if (e.type==ET::Ghost || e.type==ET::GhostElite || e.type==ET::ShadowWraith ||
               e.type==ET::BansheeHowler || e.type==ET::GhostSniper || e.type==ET::VoidStalker ||
               e.type==ET::DarkMatter || e.type==ET::SoulReaper) {
        audio.playGhostWail();                      // fantasmas/sombras
    } else if (e.type==ET::Zombie || e.type==ET::ZombieRager || e.type==ET::ZombieHorde ||
               e.type==ET::UndeadEnforcer || e.type==ET::Necromancer || e.type==ET::PlagueDoctor) {
        audio.playGhostWail();                      // mortos-vivos (gemido)
    } else if (e.type==ET::MoltenGolem || e.type==ET::CrimsonBat || e.type==ET::LichKnight ||
               e.type==ET::DemonHunter || e.type==ET::BloodBerserker) {
        audio.playExplosion(false);                 // infernais/demônios
    } else {
        audio.playEnemyDeath(false);                // robôs/mechs/padrão (mecânico)
    }
}

void Game::update(float dt) {
    // VITORIA — congela o mundo e mostra a tela de fim de jogo
    if (gameWon) {
        victoryTimer += dt;
        particles.update(dt);
        audio.updateMusic();
        if (playerSpeechTimer > 0.0f) playerSpeechTimer -= dt;
        // Bot: registra a vitoria UMA vez e encerra o teste
        static bool victoryReported = false;
        if (botController.active && !victoryReported) {
            victoryReported = true;
            botController.addLog("=== JOGO ZERADO! NUCLEO KRONOS DESTRUIDO ===");
            botController.addLog(TextFormat("Nivel final %d, kills %d", player.level, totalKills));
            botController.writeReport("C:\\Users\\ricar\\darknet-prototype\\bot_report_VITORIA.txt");
        }
        // Apos 2s, ENTER volta ao menu principal
        if (victoryTimer > 2.0f && IsKeyPressed(KEY_ENTER)) {
            inMainMenu = true;
            gameWon = false;
        }
        return;
    }

    // Dark world scenery update
    darkWorld.update(dt);

    // Inferno zone — lava + geyser damage
    if (infernoZone.active) {
        infernoZone.update(dt, player.position,
                           player.health, player.maxHealth,
                           player.isShielded());
    }

    // Tela de level up / evolucao — SO pausa quando o jogador escolheu abrir.
    // Ela e aberta deliberadamente (tecla L / K) e fecha sozinha quando os
    // pontos acabam, ou com ESC (pontos ficam guardados). O level up em si
    // NUNCA forca essa tela — apenas acumula pontos.
    if (showLevelUpScreen || showEvolutionScreen) {
        levelUpAnimTimer += dt;
        if (playerSpeechTimer > 0.0f) playerSpeechTimer -= dt;
        audio.updateMusic();
        // ESC fecha sem gastar
        if (IsKeyPressed(KEY_ESCAPE)) {
            showLevelUpScreen = false; showEvolutionScreen = false;
            return;
        }
        if (showEvolutionScreen) {
            if (IsKeyPressed(KEY_LEFT))  evolutionChoice = (evolutionChoice - 1 + 3) % 3;
            if (IsKeyPressed(KEY_RIGHT)) evolutionChoice = (evolutionChoice + 1) % 3;
            int chosen = -1;
            if (IsKeyPressed(KEY_A)) chosen = 0;
            if (IsKeyPressed(KEY_S)) chosen = 1;
            if (IsKeyPressed(KEY_D)) chosen = 2;
            if (IsKeyPressed(KEY_ENTER)) chosen = evolutionChoice;
            if (chosen >= 0) {
                applyEvolutionPath(chosen);
                if (pendingEvolutions > 0) pendingEvolutions--;
                // Fecha se nao houver mais pontos de evolucao
                if (pendingEvolutions <= 0) showEvolutionScreen = false;
            }
        } else {
            if (IsKeyPressed(KEY_LEFT))  levelUpChoice = (levelUpChoice - 1 + 3) % 3;
            if (IsKeyPressed(KEY_RIGHT)) levelUpChoice = (levelUpChoice + 1) % 3;
            int chosen = -1;
            if (IsKeyPressed(KEY_ONE))   chosen = 0;
            if (IsKeyPressed(KEY_TWO))   chosen = 1;
            if (IsKeyPressed(KEY_THREE)) chosen = 2;
            if (IsKeyPressed(KEY_ENTER)) chosen = levelUpChoice;
            if (chosen >= 0) {
                applyLevelUpChoice(chosen);
                if (pendingLevelUps > 0) pendingLevelUps--;
                // Se ainda houver pontos, gera novas opcoes e mantem a tela aberta
                if (pendingLevelUps > 0) {
                    levelUpChoice = 1;
                    generateLevelUpChoices();
                } else {
                    showLevelUpScreen = false;
                }
            }
        }
        return;
    }

    // Pulso visual do aviso de pontos disponiveis
    if (pendingNotifyPulse > 0.0f) pendingNotifyPulse -= dt;

    // Abrir tela de pontos quando o JOGADOR quiser (sem travar o jogo no level up)
    if (IsKeyPressed(KEY_L) && pendingLevelUps > 0) {
        shopSystem.close(); craftingSystem.open = false;
        showInventory = false; showEquipment = false; showQuestLog = false;
        levelUpChoice = 1;
        generateLevelUpChoices();
        showLevelUpScreen   = true;
        showEvolutionScreen = false;
        return;
    }
    if (IsKeyPressed(KEY_K) && pendingEvolutions > 0) {
        shopSystem.close(); craftingSystem.open = false;
        showInventory = false; showEquipment = false; showQuestLog = false;
        evolutionChoice     = 1;
        showEvolutionScreen = true;
        showLevelUpScreen   = false;
        return;
    }

    // Bot: gasta pontos acumulados automaticamente (escolha 1 / meio)
    if (botController.active) {
        if (pendingLevelUps > 0) {
            generateLevelUpChoices();
            applyLevelUpChoice(1);
            pendingLevelUps--;
        }
        if (pendingEvolutions > 0) {
            applyEvolutionPath(1);
            pendingEvolutions--;
        }
    }

    // HIT-STOP — congela a simulacao por alguns frames no impacto (juice de combate).
    // Particulas continuam animando para a "pancada" ficar visivel.
    if (hitStopTimer > 0.0f) {
        hitStopTimer -= dt;
        particles.update(dt);
        return;
    }
    // Decalques de chao (sangue/queimado) somem aos poucos
    for (auto& d : decals) d.life -= dt;
    decals.erase(std::remove_if(decals.begin(), decals.end(),
        [](const GroundDecal& d){ return d.life <= 0.0f; }), decals.end());

    // F12 toggles the bot
    if (IsKeyPressed(KEY_F12)) botController.toggle();

    // F10 alterna o modo de renderização 2.5D isométrico (migração em andamento)
    if (IsKeyPressed(KEY_F10)) {
        render3D = !render3D;
        triggerPlayerSpeech(render3D ? "Modo 2.5D ativado (F10)" : "Modo 2D", 2.0f);
    }

    // F9 = jump to Inferno Zone (test shortcut)
    if (IsKeyPressed(KEY_F9)) {
        transitionToZone(ZoneID::InfernoZone);
    }

    // P = cycle zones (debug / test) — 11 zones total including InfernoZone
    if (IsKeyPressed(KEY_P) && !shopSystem.open) {   // P na loja = aba premium
        int next = ((int)currentZone + 1) % 11;
        transitionToZone((ZoneID)next);
    }

    shopSystem.update(dt);
    updatePremiumStore(dt);   // aba premium (gems/Stripe) + refresh de saldo
    updateParty();            // grupo/aliança (party multiplayer — tecla O)

    // Cosméticos aplicados ao modelo do player: tinta da loja comum + skins premium.
    player.hasCosmeticTint = shopSystem.hasCosmeticColor;
    player.cosmeticTint    = shopSystem.playerColor;
    player.skinNeon        = store.ownsItem("skin_neon");
    player.skinDragon      = store.ownsItem("skin_dragon");
    player.petDrone        = store.ownsItem("pet_drone");
    craftingSystem.update(dt);

    handleInput(dt);
    player.update(dt);
    updateCompanions(dt);
    particles.update(dt);
    background.update(dt);
    audio.updateMusic();

    // Light system — dark zone detection and flicker
    {
        bool isDark = render3D || lightSystem.isDarkZone((int)currentZone); // 3D = clima Diablo sempre
        if (isDark != darkZoneActive) {
            darkZoneActive = isDark;
            lightSystem.setEnabled(isDark);
            if (isDark) {
                lightSystem.clear();
                lightSystem.addPlayerLight(player.position);
                // Torches at fixed scenic positions (spaced around map)
                lightSystem.addTorchLight({350, 420});
                lightSystem.addTorchLight({850, 310});
                lightSystem.addTorchLight({520, 750});
                lightSystem.addTorchLight({1100, 580});
                lightSystem.addTorchLight({200, 700});
                anomalySystem.storm.startAtmospheric();
            } else if (!anomalySystem.waveActive) {
                anomalySystem.storm.stop();
            }
        }
        if (darkZoneActive) {
            if (render3D) {
                switch (currentZone) {
                    case ZoneID::LARuins:       lightSystem.ambientColor = {212,196,165,255}; lightSystem.ambientDark = 0.28f; break;
                    case ZoneID::Bunker:        lightSystem.ambientColor = {150,165,188,255}; lightSystem.ambientDark = 0.40f; break;
                    case ZoneID::KronosForge:   lightSystem.ambientColor = {224,158,116,255}; lightSystem.ambientDark = 0.34f; break;
                    case ZoneID::KronosNexus:   lightSystem.ambientColor = {150,202,222,255}; lightSystem.ambientDark = 0.36f; break;
                    case ZoneID::Cemetery:      lightSystem.ambientColor = {138,158,212,255}; lightSystem.ambientDark = 0.48f; break;
                    case ZoneID::CursedFarm:    lightSystem.ambientColor = {174,182,136,255}; lightSystem.ambientDark = 0.40f; break;
                    case ZoneID::GhostCity:     lightSystem.ambientColor = {160,176,202,255}; lightSystem.ambientDark = 0.44f; break;
                    case ZoneID::DarkForest:    lightSystem.ambientColor = {128,176,140,255}; lightSystem.ambientDark = 0.46f; break;
                    case ZoneID::Catacombs:     lightSystem.ambientColor = {188,150,118,255}; lightSystem.ambientDark = 0.54f; break;
                    case ZoneID::AbandonedManor:lightSystem.ambientColor = {180,156,200,255}; lightSystem.ambientDark = 0.48f; break;
                    case ZoneID::InfernoZone:   lightSystem.ambientColor = {236,150, 98,255}; lightSystem.ambientDark = 0.28f; break;
                    default:                    lightSystem.ambientColor = {190,196,212,255}; lightSystem.ambientDark = 0.34f; break;
                }
                lightSystem.clear();
                lightSystem.addPlayerLight(player.position);
                for (int i = 0; i < 5; ++i) { float a = i * 1.25664f;
                    lightSystem.addTorchLight({ player.position.x + cosf(a)*360.0f, player.position.y + sinf(a)*360.0f }); }
                int lit = 0;
                for (auto& b : buildingSystem.buildings) { if (b.active && lit < 10 && Vector2Distance(b.position, player.position) < 850.0f) { lightSystem.addBuildingLight(b.position); lit++; } }
            }
            lightSystem.updateFlicker(dt);
            lightSystem.updatePlayerPos(player.position);
        }
    }

    // SlowMo timer
    float effectiveDt = dt;
    if (slowMoTimer > 0.0f) {
        slowMoTimer -= dt;
        effectiveDt = dt * 0.25f;
    }
    (void)effectiveDt; // used for visual effects in future

    // Screen shake — sinusoidal decay
    // Session time + story banner timer
    sessionTime += dt;
    if (storyBannerTimer  > 0.0f) storyBannerTimer  -= dt;
    if (playerSpeechTimer > 0.0f) playerSpeechTimer -= dt;

    // Tick down active chats timers and remove expired ones
    for (auto it = activeChats.begin(); it != activeChats.end();) {
        it->second.timer -= dt;
        if (it->second.timer <= 0.0f) {
            it = activeChats.erase(it);
        } else {
            ++it;
        }
    }

    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        float env = std::max(0.0f, shakeTimer / 0.3f);
        float s   = shakeIntensity * env;
        float t   = shakeTimer * 40.0f;
        camera.offset = {screenWidth/2.0f  + std::cos(t * 1.3f) * s,
                         screenHeight/2.0f + std::sin(t)         * s};
    } else {
        camera.offset = {screenWidth/2.0f, screenHeight/2.0f};
    }

    // Smooth camera follow
    float camT = 1.0f - std::exp(-8.0f * dt);
    camera.target.x += (player.position.x - camera.target.x) * camT;
    camera.target.y += (player.position.y - camera.target.y) * camT;
    if (render3D && !buildingSystem.buildModeActive && !shopSystem.open && !craftingSystem.open && !showInventory) {
        float wh = GetMouseWheelMove();
        if (wh != 0.0f) { cameraZoom -= wh * 0.06f;
            if (cameraZoom < 0.85f) cameraZoom = 0.85f;
            if (cameraZoom > 1.50f) cameraZoom = 1.50f; }
    }

    // Câmera 3D (2.5D) acompanha o jogador — usada quando render3D está ativo (F10).
    updateCamera3D();

    // Mundo infinito: auto-gera/descarrega cenário em chunks ao redor do player.
    updateSceneryChunks(player.position);

    // Open World region detection (SEM clamp de câmera — mundo é infinito)
    if (openWorldMode) {
        ZoneID newRegion = getRegionAt(player.position);
        if (newRegion != currentRegion) {
            currentRegion       = newRegion;
            currentZone         = newRegion;
            tilemap.currentZone = newRegion;
            zoneNameTimer       = 4.0f;
            spawnInterval       = getZoneInfo(newRegion).spawnInterval / getDifficulty().spawnRateMult;
            audio.setZone(newRegion);

            for (auto& r : worldRegions) {
                if (r.zoneType == newRegion && !r.discovered) {
                    r.discovered = true;
                    ZoneInfo zi  = getZoneInfo(newRegion);
                    showStoryBanner(zi.name.c_str(), zi.description.c_str(), 4.0f);
                    break;
                }
            }

            switch (newRegion) {
                case ZoneID::Cemetery:
                    triggerPlayerSpeech("Lugar sombrio... almas presas aqui.", 3.0f); break;
                case ZoneID::CursedFarm:
                    triggerPlayerSpeech("Algo muito errado nessa fazenda...", 3.0f); break;
                case ZoneID::GhostCity:
                    triggerPlayerSpeech("Uma cidade inteira... silenciada.", 3.5f); break;
                case ZoneID::DarkForest:
                    triggerPlayerSpeech("Visibilidade zero. Cuidado com a nevoa.", 3.0f); break;
                case ZoneID::KronosForge:
                    triggerPlayerSpeech("Forja KRONOS. Calor extremo detectado.", 3.0f); break;
                case ZoneID::AbandonedManor:
                    triggerPlayerSpeech("Mansao abandonada. Presencas sobrenaturais.", 3.5f); break;
                case ZoneID::KronosNexus:
                    triggerPlayerSpeech("Nucleo do KRONOS. Fim da linha.", 4.0f); break;
                case ZoneID::Bunker:
                    triggerPlayerSpeech("Bunker NEXUS. Area aliada.", 2.5f); break;
                default: break;
            }

            bool isDark = lightSystem.isDarkZone((int)newRegion);
            darkZoneActive = isDark;
            lightSystem.setEnabled(isDark);
            if (isDark) {
                lightSystem.clear();
                lightSystem.addPlayerLight(player.position);
                lightSystem.addTorchLight({player.position.x + 350, player.position.y + 200});
                lightSystem.addTorchLight({player.position.x - 280, player.position.y + 310});
                lightSystem.addTorchLight({player.position.x + 180, player.position.y - 300});
                lightSystem.addTorchLight({player.position.x - 400, player.position.y - 180});
                lightSystem.addTorchLight({player.position.x + 120, player.position.y + 450});
                // Chuva e vento ambiente nas zonas sombrias
                anomalySystem.storm.startAtmospheric();
            } else if (!anomalySystem.waveActive) {
                // Saiu da zona sombria e nao ha onda — para a chuva
                anomalySystem.storm.stop();
            }

            infernoZone.active = (newRegion == ZoneID::InfernoZone);

            bool hasDarkScenery = ((int)newRegion >= (int)ZoneID::Cemetery &&
                                   newRegion != ZoneID::InfernoZone);
            if (hasDarkScenery) {
                darkWorld.load((int)newRegion, (unsigned int)GetRandomValue(1000, 99999));
                for (const auto& wr : worldRegions) {
                    if (wr.zoneType == newRegion) {
                        darkWorld.applyWorldOffset({wr.bounds.x, wr.bounds.y});
                        break;
                    }
                }
            } else {
                darkWorld.active = false;
            }

            setupZoneNPCs(newRegion);

            // BOSS FINAL — ao chegar no Nucleo KRONOS, invoca o clímax do jogo
            if (newRegion == ZoneID::KronosNexus && !finalBossSpawned && !gameWon) {
                spawnFinalBoss();
            }
        }
    }

    // Hit flash timer (player takes damage)
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;

    // Melee cooldown
    if (meleeCooldown > 0.0f) meleeCooldown -= dt;

    // Combo decay
    if (comboTimer > 0.0f) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) comboCount = 0;
    }

    // Footstep audio
    if (player.isMoving) {
        footstepTimer += dt;
        if (footstepTimer >= 0.30f) {
            audio.playFootstep();
            footstepTimer = 0.0f;
        }
    } else {
        footstepTimer = 0.0f;
    }

    // Ground equipment update + E-to-equip
    int nearEquipIdx = -1;
    float nearEquipDist = 60.0f;
    for (int i = 0; i < (int)groundEquips.size(); ++i) {
        auto& ge = groundEquips[i];
        ge.pulseTimer += dt;
        ge.lifetime   -= dt;
        float d = Vector2Distance(player.position, ge.position);
        if (d < nearEquipDist) { nearEquipIdx = i; nearEquipDist = d; }
    }
    if (nearEquipIdx >= 0 && IsKeyPressed(KEY_E) && !dialogOpen) {
        player.equipItem(groundEquips[nearEquipIdx].equip);
        groundEquips[nearEquipIdx].collected = true;
        audio.playPickup();
        particles.spawnLevelUp(groundEquips[nearEquipIdx].position);
    }
    groundEquips.erase(
        std::remove_if(groundEquips.begin(), groundEquips.end(),
                       [](const GroundEquipment& g){ return g.collected || g.lifetime <= 0.0f; }),
        groundEquips.end());

    // Damage numbers update
    for (auto& dn : damageNumbers) {
        dn.pos.y -= 38.0f * dt;
        dn.life  -= dt;
    }
    damageNumbers.erase(
        std::remove_if(damageNumbers.begin(), damageNumbers.end(),
                       [](const DamageNumber& d){ return d.life <= 0.0f; }),
        damageNumbers.end());
    if (zoneNameTimer > 0.0f) zoneNameTimer -= dt;

    // Aviso ao CRUZAR a fronteira da zona segura (portao da base)
    if (openWorldMode) {
        bool nowInSafe = inSafeZone(player.position);
        if (wasInSafeZone && !nowInSafe) {
            // Saindo da base para o perigo
            showStoryBanner("!! SAINDO DA ZONA SEGURA !!",
                            "Territorio hostil a frente. Fique alerta.", 3.0f);
            triggerPlayerSpeech("Saindo da base. Modo de combate ativo.", 2.5f);
        } else if (!wasInSafeZone && nowInSafe) {
            // Voltando para a base
            showStoryBanner("ZONA SEGURA",
                            "Voce esta protegido. Recupere-se e prepare-se.", 2.5f);
            triggerPlayerSpeech("De volta a base. Em seguranca.", 2.0f);
        }
        wasInSafeZone = nowInSafe;
    }

    // Motor de Evolucao Infinita — sobe ameaca e rotaciona mutadores
    updateEvolutionEngine(dt);

    // Coleta de recursos naturais (segurar H perto de um nó)
    updateResourceGathering(dt);

    // Multiplayer LAN — envia o estado local e recebe os outros jogadores
    if (netActive) {
        float vm = std::sqrt(player.velocity.x*player.velocity.x + player.velocity.y*player.velocity.y);
        net.sendState(player.position.x, player.position.y,
                      (int)player.charClass, player.facing, vm > 12.0f);
        net.poll(dt);

        // Process incoming enemy deaths from network
        auto netDeaths = net.drainEnemyDeaths();
        for (uint32_t compId : netDeaths) {
            uint32_t cx = (compId >> 16) & 0xFFFF;
            uint32_t cy = compId & 0xFFFF;
            Vector2 netPos = { cx * 10.0f + 5.0f, cy * 10.0f + 5.0f };

            // Find closest local active enemy within 80 pixels
            Enemy* closest = nullptr;
            float minDist = 80.0f;
            for (auto& e : enemies) {
                if (e.isDead()) continue;
                float d = Vector2Distance(e.position, netPos);
                if (d < minDist) {
                    minDist = d;
                    closest = &e;
                }
            }
            if (closest) {
                closest->health = 0.0f;
                // Add to netKilledEnemies so we don't send edeath for it
                netKilledEnemies.push_back(closest);
            }
        }

        // Process incoming chat messages from network
        auto incomingChats = net.drainChats();
        for (const auto& ch : incomingChats) {
            uint32_t senderId = ch.first;
            const std::string& text = ch.second;
            activeChats[senderId] = { text, 4.0f };
        }
    }

    // Animais / vida selvagem
    updateAnimals(dt);

    // Spawn enemies — COM LIMITE para nao acumular sem fim (perf + estabilidade).
    // O cap escala um pouco com a dificuldade; bosses/minions ainda podem somar.
    {
        const int baseCap   = 55;
        const int diffBonus  = (int)difficulty * 12;   // Historia 0 .. Apocalipse +48
        const int enemyCap   = baseCap + diffBonus + threatLevel * 3; // mais ameaca = mais inimigos
        spawnTimer += dt;
        // Mutador "Invasao Total" acelera o spawn
        float effectiveInterval = spawnInterval * mutatorSpawnMult();
        if (spawnTimer >= effectiveInterval) {
            // ZONA SEGURA: nao spawna inimigos enquanto o player esta no refugio
            if ((int)enemies.size() < enemyCap && !inSafeZone(player.position))
                spawnEnemy();
            spawnTimer = 0.0f;
        }
    }

    // Boss tambem nao surge dentro da zona segura
    if (enemiesKilled >= bossSpawnThreshold && !bossSpawned && !inSafeZone(player.position)) {
        spawnBoss();
        bossSpawned = true;
    }

    // Auto-save
    saveTimer += dt;
    if (saveTimer >= 30.0f) { autoSave(); saveTimer = 0.0f; }

    // Update enemies and collect shooting requests
    for (auto& enemy : enemies) {
        Vector2 prevPos = enemy.position;
        enemy.update(dt, player.position);

        // Colisao com paredes — inimigos NAO atravessam mais paredes.
        // Desliza ao longo da parede (separacao por eixo) em vez de parar seco.
        // Bosses voadores/sobrenaturais ignoram (atravessam de proposito).
        bool ghostly = (enemy.type == EnemyType::Ghost ||
                        enemy.type == EnemyType::GhostElite ||
                        enemy.type == EnemyType::ShadowWraith ||
                        enemy.type == EnemyType::BansheeHowler ||
                        enemy.type == EnemyType::PoltergeistBoss);
        if (!ghostly && tilemap.isWallAtPosition(enemy.position)) {
            Vector2 tryX = {enemy.position.x, prevPos.y};
            Vector2 tryY = {prevPos.x, enemy.position.y};
            if      (!tilemap.isWallAtPosition(tryX)) enemy.position = tryX;
            else if (!tilemap.isWallAtPosition(tryY)) enemy.position = tryY;
            else                                      enemy.position = prevPos;
        }

        // ZONA SEGURA: inimigo que entra no refugio e empurrado para fora (recua).
        // O jogador fica seguro mesmo se for perseguido ate a base.
        if (inSafeZone(enemy.position)) {
            Vector2 away = { enemy.position.x - safeZoneCenter.x,
                             enemy.position.y - safeZoneCenter.y };
            float len = std::sqrt(away.x*away.x + away.y*away.y);
            if (len < 1.0f) { away = {1.0f, 0.0f}; len = 1.0f; }
            float push = enemy.speed * 1.5f * dt;
            enemy.position.x += (away.x / len) * push;
            enemy.position.y += (away.y / len) * push;
        }

        // Auto-evolution notification
        if (enemy.justEvolved) {
            enemy.justEvolved = false;
            if (Vector2Distance(enemy.position, player.position) < 420.0f) {
                const char* evolMsgs[] = {
                    "Inimigo evoluiu — cuidado!",
                    "Ameaca escalando!",
                    "Inimigo ficou mais forte!",
                    "Evolucao detectada — atencao!"
                };
                triggerPlayerSpeech(evolMsgs[GetRandomValue(0, 3)], 2.0f);
                triggerShake(3.0f, 0.18f);
            }
        }

        // Contact damage
        if (!player.isShielded()) {
            float dmg = enemy.attackIfReady(dt, player.position);
            if (dmg > 0.0f) {
                player.takeDamage(dmg);
                // Mutador LUA DE SANGUE: o inimigo se cura ao te atingir
                if (mutatorBloodMoon())
                    enemy.health = std::min(enemy.maxHealth, enemy.health + dmg * 0.5f);
                hitFlashTimer = 0.25f;
                triggerShake(5.0f, 0.18f);
                botController.damageEvents++;
                botController.totalDmgTaken += dmg;
                // Low HP warning speech
                float hpPct = player.health / player.maxHealth;
                if (hpPct < 0.20f && playerSpeechTimer <= 0.0f) {
                    triggerPlayerSpeech("ALERTA: Integridade critica. Recuando!", 3.0f);
                } else if (hpPct < 0.40f && playerSpeechTimer <= 0.0f
                           && GetRandomValue(0,3) == 0) {
                    triggerPlayerSpeech("Dano severo detectado.", 2.5f);
                }
            }
        }

        // Enemy shoots
        if (enemy.wantsToShoot) {
            enemyProjectiles.emplace_back(enemy.position, enemy.shootDirection,
                                          enemy.shootDamage, enemy.shootSpeed,
                                          enemy.projectileColor);
        }

        // Kamikaze / Zergling explosion
        if (enemy.wantsToExplode && !enemy.isDead()) {
            bool isZergling = (enemy.type == EnemyType::Zergling);
            float explodeRadius = isZergling ? 60.0f : 110.0f;
            float dist = Vector2Distance(enemy.position, player.position);
            if (dist <= explodeRadius && !player.isShielded()) {
                float falloff = 1.0f - dist / explodeRadius;
                float dmg = isZergling ? 35.0f : enemy.damage;
                player.takeDamage(dmg * falloff);
                hitFlashTimer = 0.35f;
                triggerShake(isZergling ? 5.0f : 9.0f, 0.22f);
            }
            for (auto& other : enemies) {
                if (&other == &enemy) continue;
                if (Vector2Distance(enemy.position, other.position) <= explodeRadius)
                    other.takeDamage(enemy.damage * 0.6f);
            }
            if (isZergling) {
                particles.spawnExplosion(enemy.position, {0,200,50,255}, 18);
                particles.spawnExplosion(enemy.position, {100,255,80,255}, 8);
            } else {
                particles.spawnExplosion(enemy.position, {255,120,0,255}, 25);
                particles.spawnExplosion(enemy.position, {255,220,80,255}, 12);
            }
            enemy.takeDamage(9999.0f);
            enemy.wantsToExplode = false;
        }
    }

    // Separation steering com GRID ESPACIAL — evita O(N^2) em Threat alto.
    // So compara inimigos na mesma celula e nas 8 adjacentes.
    {
        const float CELL = 64.0f;
        std::unordered_map<long long, std::vector<int>> grid;
        grid.reserve(enemies.size() * 2);
        auto key = [](int cx, int cy) {
            return ((long long)cx << 32) ^ (long long)(unsigned int)cy;
        };
        for (int i = 0; i < (int)enemies.size(); ++i)
            grid[key((int)(enemies[i].position.x / CELL),
                     (int)(enemies[i].position.y / CELL))].push_back(i);

        for (int i = 0; i < (int)enemies.size(); ++i) {
            int cx = (int)(enemies[i].position.x / CELL);
            int cy = (int)(enemies[i].position.y / CELL);
            for (int ox = -1; ox <= 1; ++ox)
            for (int oy = -1; oy <= 1; ++oy) {
                auto it = grid.find(key(cx+ox, cy+oy));
                if (it == grid.end()) continue;
                for (int j : it->second) {
                    if (j <= i) continue;   // cada par so uma vez
                    float dx = enemies[i].position.x - enemies[j].position.x;
                    float dy = enemies[i].position.y - enemies[j].position.y;
                    float minDist = enemies[i].radius + enemies[j].radius + 4.0f;
                    float d2 = dx*dx + dy*dy;
                    if (d2 < minDist*minDist && d2 > 0.0001f) {
                        float d = std::sqrt(d2);
                        float push = (minDist - d) * 0.5f;
                        float nx = dx / d, ny = dy / d;
                        enemies[i].position.x += nx * push;
                        enemies[i].position.y += ny * push;
                        enemies[j].position.x -= nx * push;
                        enemies[j].position.y -= ny * push;
                    }
                }
            }
        }
    }

    // Group alert: if any enemy took damage, alert nearby (not-yet-alerted) allies.
    // Pular allies ja alertados evita trabalho O(n^2) redundante todo frame.
    for (auto& hit : enemies) {
        if (hit.hitFlashTimer > 0.05f) {
            for (auto& ally : enemies) {
                if (&ally != &hit && !ally.alerted) {
                    float d = Vector2Distance(hit.position, ally.position);
                    if (d < 200.0f) ally.alert();
                }
            }
        }
    }

    // Building system update
    {
        std::vector<Enemy*> enemyPtrs;
        enemyPtrs.reserve(enemies.size());
        for (auto& e : enemies) enemyPtrs.push_back(&e);
        buildingSystem.update(dt, player.position, &enemyProjectiles, enemyPtrs);

        // Collect building-generated resources
        int genCredits  = buildingSystem.collectCredits();
        int genMaterials = buildingSystem.collectMaterials();
        if (genCredits  > 0) { player.credits += genCredits;   damageNumbers.push_back({player.position, (float)genCredits,  {255,220,0,255}, 1.2f, "$"}); }
        if (genMaterials > 0) materialMetal   += genMaterials;

        // Building heals
        buildingSystem.healPlayerIfNear(player.position, player.health, player.maxHealth);

        // Spawn friend projectiles
        for (auto& shot : buildingSystem.pendingShots) {
            projectiles.emplace_back(shot.origin, shot.dir, shot.damage, shot.speed, 300.f, shot.color);
        }
    }

    // ── Anomaly Portal System ─────────────────────────────────────────────────
    if (!anomalySystem.hasActiveWave()) {
        anomalyWaveTimer += dt;
        if (anomalyWaveTimer >= anomalyWaveCooldown) {
            anomalyWaveTimer = 0.0f;
            int mapW = tilemap.width  * Tilemap::tileSize;
            int mapH = tilemap.height * Tilemap::tileSize;
            anomalySystem.spawnWave(mapW, mapH, player.position);
            triggerPlayerSpeech("Anomalias detectadas! Feche os portais!", 3.5f);
            showStoryBanner("!! ANOMALIA DETECTADA !!", "Feche todos os portais para continuar");
        }
    }
    anomalySystem.update(dt, player.position);

    // Spawn enemies from portals
    {
        int portalEnemyType; Vector2 portalSpawnPos;
        if (anomalySystem.pollSpawn(portalEnemyType, portalSpawnPos)) {
            enemies.emplace_back(portalSpawnPos, (EnemyType)portalEnemyType);
        }
    }

    // Player projectiles vs portals
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        if (anomalySystem.checkProjectileHit(proj.position, proj.radius, proj.damage)) {
            proj.active = false;
        }
    }

    // All portals closed — reward
    if (anomalySystem.waveActive && anomalySystem.countOpen() == 0) {
        anomalySystem.waveActive = false;
        player.credits += 500;
        player.addXP(2500);
        triggerPlayerSpeech("Todas anomalias fechadas! Zona segura.", 3.5f);
    }
    // ──────────────────────────────────────────────────────────────────────────

    updateProjectiles(dt);
    updateEnemyProjectiles(dt);
    updateItems(dt);
    updateXPOrbs(dt);
    checkCollisions();
    checkPortalTransition();

    // Grito de "estou morrendo" quando a vida fica critica (antes de morrer)
    {
        float hpPct = player.health / player.maxHealth;
        if (hpPct > 0.0f && hpPct < 0.18f) {
            dyingCryCooldown -= dt;
            if (dyingCryCooldown <= 0.0f) {
                dyingCryCooldown = 4.0f;
                audio.playDeathCry();
                triggerPlayerSpeech("ESTOU MORRENDO! ME AJUDE!", 3.0f);
            }
        } else if (hpPct >= 0.30f) {
            dyingCryCooldown = 0.0f; // recuperou — pode gritar de novo se cair
        }
    }

    // Player death - respawn (na Arca, se houver uma construida)
    if (player.health <= 0.0f) {
        audio.playDeathCry();
        triggerPlayerSpeech("NAO... nao acabou ainda!", 3.0f);
        enemies.clear();
        items.clear();
        projectiles.clear();
        enemyProjectiles.clear();
        xpOrbs.clear();
        particles.spawnExplosion(player.position, BLUE, 30);

        Vector2 arkPos;
        if (buildingSystem.getArkPosition(arkPos)) {
            // Renasce na Arca com mais vida (75%) — a Arca e seu ponto de retorno
            player.position = arkPos;
            player.health   = player.maxHealth * 0.75f;
            triggerPlayerSpeech("Renascido na Arca. De volta a luta.", 3.0f);
        } else {
            // Sem Arca: renasce na ZONA SEGURA (refugio inicial) com 60%
            player.health  = player.maxHealth * 0.6f;
            player.position = safeZoneCenter;
            triggerPlayerSpeech("De volta a base segura. Recupere-se e prepare-se.", 3.5f);
        }
    }

    // Process dead enemies
    nearNpcIndex = -1;
    std::vector<Enemy> splitSpawns;

    for (auto it = enemies.begin(); it != enemies.end();) {
        if (it->isDead()) {
            if (it->shouldDropLoot()) {
                it->markLootDropped();

                // Check if this death was triggered by network sync
                auto netIt = std::find(netKilledEnemies.begin(), netKilledEnemies.end(), &(*it));
                if (netIt != netKilledEnemies.end()) {
                    netKilledEnemies.erase(netIt);
                } else {
                    if (netActive) {
                        uint32_t cx = (uint32_t)(it->position.x / 10.0f) & 0xFFFF;
                        uint32_t cy = (uint32_t)(it->position.y / 10.0f) & 0xFFFF;
                        uint32_t compId = (cx << 16) | cy;
                        net.sendEnemyDeath(compId);
                    }
                }

                playEnemyDeathSound(*it);   // som de morte por facção/tipo
                // Decalque no chão: sangue (orgânicos) ou queimado (máquinas)
                {
                    using ET = EnemyType;
                    bool organic = (it->type==ET::Zergling||it->type==ET::Hydra||it->type==ET::Broodmother||
                                    it->type==ET::Zombie||it->type==ET::ZombieRager||it->type==ET::ZombieHorde||
                                    it->type==ET::AcidSpitter||it->type==ET::AbyssalEel||it->type==ET::MorphX||
                                    it->type==ET::CrimsonBat||it->type==ET::ChaosSpawn||it->type==ET::NeuralParasite);
                    float sz = it->isBoss() ? 28.0f : 12.0f + it->radius * 0.4f;
                    if (organic) addDecal(it->position, Color{120,20,18,255}, 0, sz);
                    else         addDecal(it->position, Color{30,28,26,255}, 1, sz);
                }

                // XP orb — scales by elite/boss status, evolTier, and difficulty
                int xpAmt = it->isElite ? it->xpReward * 2 :
                            (it->type == EnemyType::Boss) ? it->xpReward * 3 : it->xpReward;
                float tierMult = 1.0f + it->evolTier * 0.75f; // Lendário = 3.25x
                xpAmt = (int)(xpAmt * tierMult * getDifficulty().xpMult * mutatorDropMult());
                xpOrbs.emplace_back(it->position, xpAmt);

                // Credits drop — every kill drops some credits
                int credAmt = 0;
                switch (it->type) {
                    case EnemyType::Boss:        credAmt = GetRandomValue(150, 300); break;
                    case EnemyType::Tank:        credAmt = GetRandomValue(30, 60);   break;
                    case EnemyType::Shooter:     credAmt = GetRandomValue(20, 45);   break;
                    case EnemyType::MorphX:       credAmt = GetRandomValue(40, 80);   break;
                    case EnemyType::HunterDrone:     credAmt = GetRandomValue(35, 70);   break;
                    case EnemyType::KronosSentry:credAmt = GetRandomValue(25, 50);   break;
                    case EnemyType::Sniper:      credAmt = GetRandomValue(25, 55);   break;
                    case EnemyType::Kamikaze:    credAmt = GetRandomValue(10, 25);   break;
                    default:                     credAmt = GetRandomValue(8, 20);    break;
                }
                if (it->isElite) credAmt = (int)(credAmt * 2.5f);
                credAmt = (int)(credAmt * getDifficulty().creditMult * mutatorDropMult());
                if (credAmt > 0) {
                    // Scatter credits in a small arc so they're visible
                    int numCoins = std::min(credAmt / 10 + 1, 5);
                    int coinAmt  = credAmt / numCoins;
                    for (int ci = 0; ci < numCoins; ++ci) {
                        float ang = (float)GetRandomValue(0, 628) / 100.0f;
                        float rad = (float)GetRandomValue(20, 55);
                        Vector2 cp = {it->position.x + std::cos(ang)*rad,
                                      it->position.y + std::sin(ang)*rad};
                        Item coin = Item::createCredits(cp, coinAmt);
                        coin.pickupDelay = 0.4f + ci * 0.05f;
                        items.push_back(coin);
                    }
                }

                // Loot drop — elites always drop, others scaled by difficulty drop chance
                int dropRoll = GetRandomValue(0, 100);
                int dropThresh = std::min((int)(55 * getDifficulty().dropChanceMult), 95);
                if (it->isElite || dropRoll < dropThresh) {
                    float ang = (float)GetRandomValue(0, 628) / 100.0f;
                    Vector2 dp = {it->position.x + std::cos(ang)*35.0f,
                                  it->position.y + std::sin(ang)*35.0f};
                    Item drop = Item::createRandom(dp);
                    drop.pickupDelay = 0.5f;
                    bool fromOmega = (it->type == EnemyType::OmegaBoss);
                    drop.rarity = Item::rollRarity(fromOmega);
                    if ((int)difficulty >= (int)DifficultyLevel::Guerreiro) {
                        ItemRarity r2 = Item::rollRarity(fromOmega);
                        if ((int)r2 > (int)drop.rarity) drop.rarity = r2;
                    }
                    drop.applyRarityBonus();
                    drop.isNew = true;
                    drop.dropBeamTimer = (drop.rarity >= ItemRarity::Rare) ? 4.0f : 0.0f;
                    if (drop.rarity >= ItemRarity::Legendary && playerSpeechTimer <= 0.5f)
                        triggerPlayerSpeech("Item LENDARIO detectado!", 3.0f);
                    else if (drop.rarity == ItemRarity::Epic && playerSpeechTimer <= 0.5f)
                        triggerPlayerSpeech("Item Epico encontrado!", 2.0f);
                    items.push_back(drop);
                }
                // Tech chip drop — 20% base, 60% from bosses, scaled by difficulty
                int techChanceBase = (it->type == EnemyType::Boss) ? 60 :
                                     (it->isElite)                 ? 45 : 20;
                int techChance = std::min((int)(techChanceBase * getDifficulty().dropChanceMult), 95);
                if (GetRandomValue(0, 100) < techChance) {
                    float ang = (float)GetRandomValue(0, 628) / 100.0f;
                    Vector2 tp = {it->position.x + std::cos(ang)*45.0f,
                                  it->position.y + std::sin(ang)*45.0f};
                    Item tech = Item::createTech(tp);
                    tech.pickupDelay = 0.6f;
                    items.push_back(tech);
                }

                // ── Material drops for Crafting System ───────────────────────
                auto spawnMaterial = [&](ItemType mtype, const char* mname, Color mcol) {
                    float ang = (float)GetRandomValue(0, 628) / 100.0f;
                    float rad = (float)GetRandomValue(25, 55);
                    Vector2 mp = {it->position.x + std::cos(ang)*rad,
                                  it->position.y + std::sin(ang)*rad};
                    Item mat;
                    mat.position    = mp;
                    mat.type        = mtype;
                    mat.name        = mname;
                    mat.color       = mcol;
                    mat.radius      = 7.f;
                    mat.lifetime    = 40.f;
                    mat.pickupDelay = 0.4f;
                    mat.rarity      = ItemRarity::Uncommon;
                    items.push_back(mat);
                };

                switch (it->type) {
                    case EnemyType::Scout:
                    case EnemyType::Tank:
                    case EnemyType::Shooter:
                    case EnemyType::KronosSentry:
                        // MetalScrap — 40%
                        if (GetRandomValue(0, 99) < 40)
                            spawnMaterial(ItemType::MetalScrap, "Sucata Metal", {180,180,180,255});
                        break;
                    case EnemyType::Zergling:
                    case EnemyType::Hydra:
                    case EnemyType::Broodmother:
                        // AlienCarapace — 50%
                        if (GetRandomValue(0, 99) < 50)
                            spawnMaterial(ItemType::AlienCarapace, "Carapaca Alien", {60,255,80,255});
                        break;
                    case EnemyType::HunterDrone:
                        // PlasmaCore — 35%
                        if (GetRandomValue(0, 99) < 35)
                            spawnMaterial(ItemType::PlasmaCore, "Nucleo Plasma", {0,180,255,255});
                        break;
                    case EnemyType::MorphX:
                        // NanoFiber — 45% (T-1000 analogue)
                        if (GetRandomValue(0, 99) < 45)
                            spawnMaterial(ItemType::NanoFiber, "Fibra Nano", {0,220,200,255});
                        break;
                    case EnemyType::OmegaBoss:
                        // OmegaEssence — 100% garantido
                        spawnMaterial(ItemType::OmegaEssence, "Essencia Omega", {255,215,0,255});
                        break;
                    default:
                        break;
                }
                // ─────────────────────────────────────────────────────────────

                // Elite-exclusive: drop equipment on ground (player must walk to E to equip)
                if (it->isElite && GetRandomValue(0, 100) < 70) {
                    int tier = (it->eliteMod == 1) ? 2 : GetRandomValue(1, 2);
                    Equipment eq;
                    int roll = GetRandomValue(0, 5);
                    switch (tier) {
                        case 1: eq = (roll < 3) ? EDB::submetMilitar() : EDB::chipVel(); break;
                        case 2: eq = (roll < 2) ? EDB::rifleEnergia()  :
                                     (roll < 4) ? EDB::armaduraAvan()  : EDB::neuralLink(); break;
                        default: eq = EDB::canhaoEMP(); break;
                    }
                    if (!eq.isEmpty()) {
                        float ang = (float)GetRandomValue(0, 628) / 100.0f;
                        GroundEquipment ge;
                        ge.position = {it->position.x + std::cos(ang)*50.0f,
                                       it->position.y + std::sin(ang)*50.0f};
                        ge.equip    = eq;
                        groundEquips.push_back(ge);
                    }
                    items.push_back(Item::createEliteDrop(it->position));
                    particles.spawnLevelUp(it->position);
                }

                enemiesKilled++;
                totalKills++;
                botController.killCount++;

                // VITORIA — o Nucleo KRONOS foi destruido
                if (it->isFinalBoss) {
                    finalBossAlive = false;
                    gameWon        = true;
                    victoryTimer   = 0.0f;
                    triggerShake(20.0f, 1.0f);
                    triggerPlayerSpeech("Acabou... a humanidade esta livre.", 6.0f);
                    audio.playLevelUp();
                }

                // Omega Boss trigger every 50 kills (desativado apos a vitoria)
                if (!gameWon && totalKills >= omegaKillThreshold) {
                    omegaKillThreshold += 50;
                    spawnOmegaBoss();
                }

                // First kill speech
                if (!firstKillTriggered && playerSpeechTimer <= 0.3f) {
                    firstKillTriggered = true;
                    static const char* fkl[] = { "Primeiro de muitos.", "NEXUS: 1. KRONOS: 0.", "Isso e por Lyra." };
                    triggerPlayerSpeech(fkl[GetRandomValue(0,2)], 2.5f);
                }
                // Player combat commentary (half-human reactions)
                if (enemiesKilled % 5 == 0 && playerSpeechTimer <= 0.5f) {
                    static const char* killLines[] = {
                        "Unidade neutralizada.", "Target eliminado.",
                        "Sistema de combate eficiente.", "Ameaca suprimida.",
                        "Protocolo de neutralizacao concluido.",
                        "Meus sensores detectam mais inimigos.", "Continuo a missao."
                    };
                    triggerPlayerSpeech(killLines[GetRandomValue(0,6)], 3.0f);
                }
                if (it->type == EnemyType::Boss || it->type == EnemyType::AlienBoss || it->type == EnemyType::OmegaBoss) {
                    static const char* bossLines[] = { "CHEFE ABATIDO. Missao cumprida.", "Um a menos pra humanidade.", "Era isso? Vim preparado.", "KRONOS - seu tempo acabou." };
                    triggerPlayerSpeech(bossLines[GetRandomValue(0,3)], 5.0f);
                }

                // Shake on kill
                triggerShake(4.0f, 0.12f);

                // Quest progress — GENERICO por tipo (qualquer kill conta nas quests
                // de caca; boss conta nas de boss). Garante que as barras enchem.
                bool isBossKill = (it->type == EnemyType::Boss ||
                                   it->type == EnemyType::AlienBoss ||
                                   it->type == EnemyType::OmegaBoss);
                for (auto& q : quests) {
                    if (q.completed || !q.active) continue;
                    if (q.type == QuestType::Kill) {
                        q.updateProgress(1);
                    } else if (q.type == QuestType::KillBoss && isBossKill) {
                        q.updateProgress(1);
                        slowMoTimer = 1.8f; // cinematic slow-mo on boss kill
                    }
                    if (q.isComplete() && !q.rewardGiven) grantQuestRewards(q);
                }

                // MorphX split
                if (it->type == EnemyType::MorphX && !it->hasSplit && !it->isMinion) {
                    it->hasSplit = true;
                    splitSpawns.emplace_back(it->position, EnemyType::MorphX, true);
                    Vector2 p2 = {it->position.x + 30, it->position.y - 20};
                    splitSpawns.emplace_back(p2, EnemyType::MorphX, true);
                }
            }

            // Volatile elite: huge AoE explosion on death
            if (it->isElite && it->eliteMod == 2) {
                float aoe = 120.0f;
                for (auto& other : enemies) {
                    if (&other != &(*it) && Vector2Distance(it->position, other.position) < aoe) {
                        other.takeDamage(it->maxHealth * 0.4f);
                    }
                }
                if (Vector2Distance(it->position, player.position) < aoe && !player.isShielded()) {
                    player.takeDamage(30.0f);
                    hitFlashTimer = 0.35f;
                }
                particles.spawnExplosion(it->position, {255, 0, 200, 255}, 28);
                audio.playExplosionBig();
                triggerShake(10.0f, 0.4f);
            } else {
                particles.spawnExplosion(it->position, it->isElite ?
                    Color{255,200,0,255} : it->bodyColor, it->isElite ? 18 : 10);
                audio.playExplosion();
            }

            // UndeadEnforcer ressurreicao — revive com 30% HP uma vez
            if (it->type == EnemyType::UndeadEnforcer && !it->hasRevived) {
                it->hasRevived = true;
                it->health     = it->maxHealth * 0.30f;
                particles.spawnExplosion(it->position, {160, 0, 220, 255}, 20);
                triggerShake(3.0f, 0.15f);
                ++it;
                continue;
            }

            it = enemies.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& s : splitSpawns) enemies.push_back(s);

    // Check NPC proximity
    for (int i = 0; i < (int)npcs.size(); ++i) {
        if (npcs[i].isPlayerNear(player.position)) {
            nearNpcIndex = i;
            break;
        }
    }

    // Quest rewards + conclusao de quests de Zona (ao estar na zona alvo)
    for (auto& q : quests) {
        if (!q.completed && q.active && q.type == QuestType::Zone &&
            (int)currentZone == q.target && !q.isComplete()) {
            q.updateProgress(q.target);   // chegou na zona — completa
        }
        if (q.isComplete() && !q.rewardGiven) {
            grantQuestRewards(q);
        }
    }
}

void Game::grantQuestRewards(Quest& q) {
    q.complete();
    if (q.rewardHP  > 0.0f) player.heal(q.rewardHP);
    if (q.rewardXP  > 0)    player.addXP(q.rewardXP);
    if (!q.rewardEquip.isEmpty()) player.equipItem(q.rewardEquip);
    particles.spawnLevelUp(player.position);
    audio.playLevelUp();
}

// ─── Input ───────────────────────────────────────────────────────────────────

void Game::handleInput(float dt) {
    if (paused || inMainMenu) return;

    // ── Chat absorbs all input when active ─────────────────────────────────────
    if (chatActive) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            chatActive = false;
            chatInput.clear();
            return;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (!chatInput.empty()) {
                if (netActive) {
                    net.sendChat(chatInput);
                }
                triggerPlayerSpeech(chatInput, 4.0f);
            }
            chatActive = false;
            chatInput.clear();
            return;
        }

        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (chatInput.size() < 64)) {
                chatInput.push_back((char)key);
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && !chatInput.empty()) {
            chatInput.pop_back();
        }
        return;
    }

    if (IsKeyPressed(KEY_ENTER) && !craftingSystem.open && !shopSystem.open && !showInventory) {
        chatActive = true;
        chatInput.clear();
        return;
    }

    // ── Crafting system absorbs all input when open ───────────────────────────
    if (craftingSystem.open) {
        if (IsKeyPressed(KEY_ESCAPE)) { craftingSystem.open = false; return; }
        if (IsKeyPressed(KEY_UP))   craftingSystem.selected = (craftingSystem.selected - 1 + (int)craftingSystem.recipes.size()) % (int)craftingSystem.recipes.size();
        if (IsKeyPressed(KEY_DOWN)) craftingSystem.selected = (craftingSystem.selected + 1) % (int)craftingSystem.recipes.size();
        Equipment outEquip;
        Item      outItem;
        bool      gotEquip = false;
        bool      crafted  = false;
        if (IsKeyPressed(KEY_ENTER) && !craftingSystem.crafting)
            crafted = craftingSystem.tryCraft(player.inventory, outEquip, outItem, gotEquip);
        // Mouse: hover/clique nas receitas e botao CRAFTAR
        {
            Vector2 vm = virtualizeMousePos(GetMousePosition());
            bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            bool closed = false;
            if (craftingSystem.handleMouse(vm, click, player.inventory, screenWidth, screenHeight,
                                           outEquip, outItem, gotEquip, closed))
                crafted = true;
            if (closed) { craftingSystem.open = false; return; }
        }
        if (crafted) {
            if (gotEquip) {
                player.equipItem(outEquip);
                particles.spawnLevelUp(player.position);
                audio.playLevelUp();
            } else {
                outItem.position   = player.position;
                outItem.pickupDelay = 0.0f;
                outItem.lifetime   = 60.0f;
                items.push_back(outItem);
                audio.playPickup();
            }
        }
        return;
    }

    // ── Shop absorbs all input when open ─────────────────────────────────────
    if (shopSystem.open) {
        shopSystem.handleInput();
        Equipment outEquip;
        Item      outItem;
        bool      gotEquip    = false;
        bool      gotCosmetic = false;
        Color     cosmeticCol = WHITE;
        bool      bought      = false;
        // Compra por TECLADO (ENTER) ...
        if (IsKeyPressed(KEY_ENTER)) {
            bought = shopSystem.tryBuy(player.credits, outEquip, outItem,
                                       gotEquip, gotCosmetic, cosmeticCol);
        }
        // ... ou por MOUSE (hover/clique nos botoes)
        {
            Vector2 vm = virtualizeMousePos(GetMousePosition());
            bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            bool closed = false;
            if (shopSystem.handleMouse(vm, click, screenWidth, screenHeight, player.credits,
                                       outEquip, outItem, gotEquip, gotCosmetic, cosmeticCol, closed))
                bought = true;
            if (closed) { shopSystem.close(); return; }
        }
        if (bought) {
            if (gotEquip) {
                player.equipItem(outEquip);
                particles.spawnLevelUp(player.position);
                audio.playLevelUp();
            } else if (gotCosmetic) {
                particles.spawnLevelUp(player.position);
                audio.playPickup();
            } else {
                outItem.position   = player.position;
                outItem.pickupDelay = 0.0f;
                outItem.lifetime   = 60.0f;
                items.push_back(outItem);
                audio.playPickup();
            }
        }
        return;
    }

    // ── Inventory absorbs 1/2/3/U when open ──────────────────────────────────
    if (showInventory) {
        player.handleInventoryInput();
        if (IsKeyPressed(KEY_I)) showInventory = false;
        return;
    }

    // Mouse no mundo: em 2.5D usa raycast no plano do chão; em 2D, transform da câmera.
    Vector2 mouseWorld = render3D ? mouseGround3D()
                                  : GetScreenToWorld2D(virtualizeMousePos(GetMousePosition()), camera);

    // ── Bot controller decisions ──────────────────────────────────────────────
    botMeleeRequest = false;
    if (botController.active) {
        std::vector<Vector2> enemyPos;
        enemyPos.reserve(enemies.size());
        for (const auto& e : enemies) enemyPos.push_back(e.position);

        std::vector<Vector2> itemPos;
        itemPos.reserve(items.size() + xpOrbs.size());
        for (const auto& it : items)   itemPos.push_back(it.position);
        for (const auto& xp : xpOrbs)  itemPos.push_back(xp.position);

        bool skillsReady[6] = {false,false,false,false,false,false};
        for (int si2 = 0; si2 < 6 && si2 < (int)player.skills.size(); ++si2)
            skillsReady[si2] = player.skills[si2].isReady();

        // Sensores de parede — 8 direcoes ao redor do player (alinhado a k8DirAngles)
        {
            static const float k8[8] = {
                0.0f, (float)(PI*0.25), (float)(PI*0.5), (float)(PI*0.75),
                (float)PI, (float)(PI*1.25), (float)(PI*1.5), (float)(PI*1.75)
            };
            for (int d = 0; d < 8; d++) {
                Vector2 probe = {
                    player.position.x + std::cos(k8[d]) * 52.0f,
                    player.position.y + std::sin(k8[d]) * 52.0f
                };
                botController.blockedDir[d] = tilemap.isWallAtPosition(probe);
            }
        }

        // ── Diagnostico de FPS: picos de entidades + snapshot no FPS mais baixo ──
        {
            int ne = (int)enemies.size(), ni = (int)items.size(), no = (int)xpOrbs.size();
            int np = (int)projectiles.size(), nep = (int)enemyProjectiles.size();
            int nu = (int)buildingSystem.tanks.size() + (int)buildingSystem.soldiers.size();
            if (ne  > botController.peakEnemies)     botController.peakEnemies = ne;
            if (ni  > botController.peakItems)       botController.peakItems = ni;
            if (no  > botController.peakOrbs)        botController.peakOrbs = no;
            if (np  > botController.peakProjectiles) botController.peakProjectiles = np;
            if (nep > botController.peakEnemyProj)   botController.peakEnemyProj = nep;
            if (nu  > botController.peakUnits)        botController.peakUnits = nu;
            float fps = (float)GetFPS();
            if (fps > 0.0f && fps < botController.fpsLowValue) {
                botController.fpsLowValue     = fps;
                botController.fpsLowEnemies   = ne;
                botController.fpsLowProj      = np + nep;
                botController.fpsLowParticles = 0; // preenchido abaixo se disponivel
            }
        }

        // Pathfinding global do bot: fornece a consulta de colisao do mapa.
        if (!botController.wallQuery)
            botController.wallQuery = [this](Vector2 p){ return tilemap.isWallAtPosition(p); };
        // Centro do mapa para o escape de bordas (width/height em tiles * tileSize/2).
        botController.worldCenter = { tilemap.width * 32.0f, tilemap.height * 32.0f };

        // Portais de saida da zona — para o bot avancar de fase (AdvancePhase).
        std::vector<Vector2> portalPos;
        portalPos.reserve(tilemap.portals.size());
        for (const auto& zp : tilemap.portals) portalPos.push_back(zp.position);

        auto dec = botController.update(
            dt,
            player.position, player.attackRange,
            player.health, player.maxHealth,
            player.level, player.credits,
            (float)GetFPS(),
            enemyPos, itemPos, skillsReady, portalPos, 0);

        if (dec.shouldQuit) {
            botController.writeReport("C:\\Users\\ricar\\darknet-prototype\\bot_report_final.txt");
            quitRequested = true;
            return;
        }

        // Auto-save parcial a cada 5 minutos de teste
        if (botController.autoTest) {
            static float reportSaveTimer = 0.0f;
            reportSaveTimer += dt;
            if (reportSaveTimer >= 300.0f) {
                reportSaveTimer = 0.0f;
                int minElapsed = (int)(botController.testTimer / 60.0f);
                std::string rpath = "C:\\Users\\ricar\\darknet-prototype\\bot_report_" +
                                    std::to_string(minElapsed) + "min.txt";
                botController.writeReport(rpath);
                botController.addLog("Relatorio parcial salvo (" + std::to_string(minElapsed) + "min)");
            }
        }

        if (dec.shouldMove) {
            moveTarget = dec.moveTarget;
            hasTarget  = true;

            // ── Anti-trava com PATHFINDING BFS global ────────────────────────
            // Detecta travamento (player-bot mal se move) e, em vez de oscilar
            // nos sensores de 8 direcoes, traca uma rota real ate o alvo.
            float moved = Vector2Distance(player.position, botPrevPos);
            botPrevPos  = player.position;
            if (moved < 2.5f) botStuckTime += dt; else botStuckTime = 0.0f;

            if (botStuckTime > 1.0f) {
                // (re)calcula a rota se ainda nao tem ou se o alvo mudou muito
                if (botPath.empty() || Vector2Distance(botPathGoal, dec.moveTarget) > 120.0f) {
                    if (botFindPath(player.position, dec.moveTarget, botPath)) {
                        botPathGoal = dec.moveTarget;
                        botPathIdx  = 1; // pula o tile atual
                    }
                }
            }
            // Segue a rota BFS se houver uma ativa
            if (!botPath.empty() && botPathIdx < (int)botPath.size()) {
                Vector2 wp = botPath[botPathIdx];
                if (Vector2Distance(player.position, wp) < 40.0f) botPathIdx++;
                if (botPathIdx < (int)botPath.size()) {
                    moveTarget = botPath[botPathIdx];
                    hasTarget  = true;
                }
                // chegou perto do alvo final -> abandona a rota
                if (Vector2Distance(player.position, botPathGoal) < 60.0f ||
                    botPathIdx >= (int)botPath.size()) {
                    botPath.clear(); botStuckTime = 0.0f;
                }
            }
        }
        if (dec.shouldMeleeAttack) {
            botMeleeRequest = true;
        }
        // Aim direction toward nearest enemy for all bot skills
        Vector2 botAimDir = {1.0f, 0.0f};
        if (dec.nearestEnemyIdx >= 0) {
            Vector2 toE = Vector2Subtract(dec.nearestEnemyPos, player.position);
            float   len = std::sqrt(toE.x*toE.x + toE.y*toE.y);
            if (len > 0.001f) botAimDir = {toE.x/len, toE.y/len};
        }
        // Skill 1 — Laser
        if (dec.shouldUseSkill1 && (int)player.skills.size() > 0 && player.skills[0].isReady()) {
            player.useSkill(0, dec.nearestEnemyPos);
            float dmgL = player.getEffectiveDamage() + player.skills[0].damage;
            projectiles.emplace_back(player.position, botAimDir, dmgL, 550.0f, 620.0f,
                                     Color{0,255,255,255});
            particles.spawnHit(player.position, Color{0,255,255,255}, 8);
            audio.playLaser();
        }
        // Skill 2 — EMP
        if (dec.shouldUseSkill2 && (int)player.skills.size() > 1 && player.skills[1].isReady()) {
            player.useSkill(1, dec.nearestEnemyPos);
            float empDmg = player.skills[1].damage * (player.isOverloaded() ? 1.5f : 1.0f);
            for (auto& enemy : enemies)
                if (Vector2Distance(player.position, enemy.position) <= player.skills[1].range)
                    enemy.takeDamage(empDmg);
            particles.spawnExplosion(player.position, YELLOW, 25);
            audio.playEMP();
        }
        // Skill 3 — Plasma Grenade
        if (dec.shouldUseSkill3 && (int)player.skills.size() > 2 && player.skills[2].isReady()) {
            player.useSkill(2, dec.nearestEnemyPos);
            float gDmg = player.skills[2].damage * (player.isOverloaded() ? 1.5f : 1.0f);
            projectiles.emplace_back(player.position, botAimDir, gDmg,
                                     player.skills[2].range, 280.0f,
                                     Color{255,120,0,255}, true);
            audio.playLaser();
        }
        // Skill 4 — Sobrecarga
        if (dec.shouldUseSkill4 && (int)player.skills.size() > 3 && player.skills[3].isReady()) {
            player.useSkill(3, dec.nearestEnemyPos);
            player.overloadTimer = 8.0f;
            particles.spawnLevelUp(player.position);
            audio.playLevelUp();
        }
        // Skill 5 — Barreira
        if (dec.shouldUseSkill5 && (int)player.skills.size() > 4 && player.skills[4].isReady()) {
            player.useSkill(4, dec.nearestEnemyPos);
            player.shieldTimer = 3.0f;
            particles.spawnLevelUp(player.position);
        }
        // Skill 6 — Rajada
        if (dec.shouldUseSkill6 && (int)player.skills.size() > 5 && player.skills[5].isReady()) {
            player.useSkill(5, dec.nearestEnemyPos);
            float baseAngle6 = std::atan2(botAimDir.y, botAimDir.x);
            float dmg6 = player.skills[5].damage * (player.isOverloaded() ? 1.5f : 1.0f);
            for (int i = -3; i <= 4; ++i) {
                float angle = baseAngle6 + 0.22f * (float)i;
                Vector2 d2 = {std::cos(angle), std::sin(angle)};
                Color col = (std::abs(i) <= 1) ? Color{0,255,100,255} : Color{0,200,80,200};
                projectiles.emplace_back(player.position, d2, dmg6, 420.0f, 660.0f, col);
            }
            audio.playLaser();
        }
        botAimTarget = dec.moveTarget;

        // ════════════════════════════════════════════════════════════════════
        // BOT: exercita TODOS os sistemas — convoca aliados, constroi tanques/
        // torres/quarteis, produz unidades e auto-evolui as estruturas.
        // ════════════════════════════════════════════════════════════════════
        {
            static float botAllyTimer    = 2.0f;
            static float botBuildTimer    = 4.0f;
            static float botProduceTimer  = 8.0f;
            static float botUpgradeTimer  = 12.0f;
            static float botStipendTimer  = 0.0f;
            static int   botBuildCycle    = 0;

            // Em autoTest garante um fluxo de recursos para conseguir exercitar
            // construcoes/evolucoes mesmo em fases iniciais.
            if (botController.autoTest) {
                botStipendTimer += dt;
                if (botStipendTimer >= 5.0f) {
                    botStipendTimer = 0.0f;
                    player.credits   += 120;
                    materialMetal    += 6;
                    materialCarapace += 4;
                }
            }

            // 1) Convocar aliados (companions MARCO VEIL / STEEL / REX)
            botAllyTimer -= dt;
            if (botAllyTimer <= 0.0f) {
                botAllyTimer = 25.0f;
                if ((int)companions.size() < 3) {
                    CompanionType t = companions.empty() ? CompanionType::MarcoVeil
                                    : ((int)companions.size() == 1 ? CompanionType::Steel
                                                                   : CompanionType::Rex);
                    spawnCompanion(t);
                    botController.addLog("Aliado convocado");
                }
            }

            // 2) Construir estruturas variadas num anel ao redor do bot
            botBuildTimer -= dt;
            if (botBuildTimer <= 0.0f && (int)buildingSystem.buildings.size() < 12) {
                botBuildTimer = 6.0f;
                static const int cycleTypes[] = {
                    (int)BuildingType::Turret,      (int)BuildingType::Barracks,
                    (int)BuildingType::TankFactory, (int)BuildingType::House,
                    (int)BuildingType::ResourceNode,(int)BuildingType::MedBay,
                    (int)BuildingType::Ark,         (int)BuildingType::Wall,
                };
                int nTypes = (int)(sizeof(cycleTypes)/sizeof(cycleTypes[0]));
                buildingSystem.selectedType = cycleTypes[botBuildCycle % nTypes];
                botBuildCycle++;
                for (int attempt = 0; attempt < 6; ++attempt) {
                    float ang = (float)(botBuildCycle * 1.7f + attempt * 1.05f);
                    float rad = 110.0f + attempt * 30.0f;
                    Vector2 spot = { player.position.x + std::cos(ang) * rad,
                                     player.position.y + std::sin(ang) * rad };
                    if (tilemap.isWallAtPosition(spot)) continue;
                    int cc = 0, mc = 0, ac = 0;
                    if (buildingSystem.tryPlace(spot, player.credits, materialMetal,
                                                materialCarapace, cc, mc, ac)) {
                        player.credits   -= cc;
                        materialMetal    -= mc;
                        materialCarapace -= ac;
                        botController.addLog(TextFormat("Estrutura construida (tipo %d)",
                                                        buildingSystem.selectedType));
                        break;
                    }
                }
            }

            // 3) Produzir unidades das fabricas/quarteis existentes
            botProduceTimer -= dt;
            if (botProduceTimer <= 0.0f && !buildingSystem.buildings.empty()) {
                botProduceTimer = 7.0f;
                for (const auto& b : buildingSystem.buildings) {
                    if (b.type == BuildingType::TankFactory ||
                        b.type == BuildingType::Barracks) {
                        if (buildingSystem.clickProduce(b.position, player.credits) == 1) {
                            botController.addLog("Unidade produzida");
                            break;
                        }
                    }
                }
            }

            // 4) Auto-evolucao: evolui a estrutura mais proxima
            botUpgradeTimer -= dt;
            if (botUpgradeTimer <= 0.0f) {
                botUpgradeTimer = 15.0f;
                if (buildingSystem.upgradeNearby(player.position, player.credits) == 1)
                    botController.addLog("Estrutura evoluida");
            }
        }
    }

    // ── Clique numa fabrica/quartel = produzir unidade na hora ────────────────
    bool producedThisClick = false;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !dialogOpen &&
        !buildingSystem.buildModeActive) {
        int r = buildingSystem.clickProduce(mouseWorld, player.credits);
        if (r == 1) {
            triggerPlayerSpeech("Unidade em producao!", 1.5f);
            audio.playPickup();
            producedThisClick = true;
        } else if (r == 2) {
            triggerPlayerSpeech("Creditos insuficientes.", 1.5f);
            producedThisClick = true;
        } else if (r == 3) {
            triggerPlayerSpeech("Limite de unidades atingido.", 1.5f);
            producedThisClick = true;
        }
    }

    // ── Clicar num NPC para conversar (selecao por clique) ────────────────────
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !dialogOpen &&
        !buildingSystem.buildModeActive && !producedThisClick) {
        for (int i = 0; i < (int)npcs.size(); ++i) {
            if (Vector2Distance(mouseWorld, npcs[i].position) <= npcs[i].radius + 18.0f) {
                if (Vector2Distance(player.position, npcs[i].position) <= 160.0f) {
                    nearNpcIndex = i;
                    dialogOpen   = true;
                    dialogLine   = 0;
                    producedThisClick = true; // nao mover o player neste clique
                } else {
                    triggerPlayerSpeech("Preciso chegar mais perto para conversar.", 2.0f);
                }
                break;
            }
        }
    }

    // ── Selecao RTS por arrasto do mouse esquerdo ─────────────────────────────
    // ── Selecao RTS: SO com SHIFT segurado (esquerdo sozinho = andar) ─────────
    // Assim segurar o esquerdo para CAMINHAR nunca desenha caixa de selecao.
    bool selectMod = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (!buildingSystem.buildModeActive && !dialogOpen) {
        if (selectMod && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !producedThisClick) {
            rtsDragStart = mouseWorld;
            rtsDragCur   = mouseWorld;
            rtsDragging  = true;   // entra em modo selecao imediatamente
        }
        if (rtsDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            rtsDragCur = mouseWorld;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && rtsDragging) {
            Rectangle box = { rtsDragStart.x, rtsDragStart.y,
                              rtsDragCur.x - rtsDragStart.x,
                              rtsDragCur.y - rtsDragStart.y };
            int sel = buildingSystem.selectUnitsInBox(box);
            rtsHasUnits = (sel > 0);
            if (sel > 0) triggerPlayerSpeech(TextFormat("%d unidade(s) selecionada(s)", sel), 1.5f);
            rtsDragging = false;
        }
        // Se soltar SHIFT no meio do arrasto, cancela a selecao (volta a andar)
        if (rtsDragging && !selectMod) rtsDragging = false;

        // Botao DIREITO = ordem de mover as unidades selecionadas
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && rtsHasUnits) {
            buildingSystem.orderMove(mouseWorld);
        }
    }

    // ── Click-to-move (Diablo) — esquerdo sozinho SEMPRE anda (sem marcar) ────
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !dialogOpen && !rtsDragging) {
        moveTarget = mouseWorld;
        hasTarget  = true;
    }

    // CORRER (segurar SHIFT) e PULAR (ESPAÇO) — pulo cruza obstaculos baixos
    player.sprinting = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (IsKeyPressed(KEY_SPACE)) { player.startJump(); audio.playFootstep(); }
    // Durante o pulo a colisao com parede e relaxada (passa por cima)
    bool airborne = player.isJumping && player.jumpZ > 8.0f;

    // WASD also sets move direction (alternative control)
    {
        Vector2 wasd = {0, 0};
        if (IsKeyDown(KEY_W)) wasd.y -= 1.0f;
        if (IsKeyDown(KEY_S)) wasd.y += 1.0f;
        if (IsKeyDown(KEY_A)) wasd.x -= 1.0f;
        if (IsKeyDown(KEY_D)) wasd.x += 1.0f;
        float wlen = std::sqrt(wasd.x*wasd.x + wasd.y*wasd.y);
        if (wlen > 0.0f) {
            Vector2 old = player.position;
            player.move(wasd, dt);
            if (!airborne && tilemap.isWallAtPosition(player.position) && !tilemap.isWallAtPosition(old)) player.position = old;
            hasTarget = false; // WASD cancels click target
        }
    }

    // Move toward click target
    if (hasTarget) {
        Vector2 toTarget = {moveTarget.x - player.position.x,
                            moveTarget.y - player.position.y};
        float dist = std::sqrt(toTarget.x*toTarget.x + toTarget.y*toTarget.y);
        if (dist > 10.0f) {
            Vector2 old = player.position;
            player.move({toTarget.x / dist, toTarget.y / dist}, dt);
            if (!airborne && tilemap.isWallAtPosition(player.position) && !tilemap.isWallAtPosition(old)) {
                player.position = old;
                hasTarget = false;
            }
        } else {
            hasTarget = false;
        }
    }

    // ── Right-click → melee attack (also triggered by bot) ───────────────────
    if ((IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || botMeleeRequest) && !dialogOpen && meleeCooldown <= 0.0f) {
        meleeCooldown = 0.35f;
        bool hitAny = false;
        float comboMult = 1.0f + std::min(comboCount, 10) * 0.15f;
        float dmg = player.getEffectiveDamage() * comboMult;
        for (auto& enemy : enemies) {
            float dist = Vector2Distance(player.position, enemy.position);
            if (dist <= player.attackRange) {
                enemy.takeDamage(dmg);

                // Knockback away from player
                Vector2 kb = {enemy.position.x - player.position.x,
                               enemy.position.y - player.position.y};
                enemy.applyKnockback(kb, 220.0f);

                particles.spawnBloodSparks(enemy.position, enemy.bodyColor, 10);
                particles.spawnHit(enemy.position, WHITE, 4);
                audio.playHit();
                hitAny = true;
                comboCount++;
                comboTimer = 2.5f;

                // Floating damage number — bigger/gold for combos
                Color col = comboCount >= 5 ? Color{255,220,0,255} :
                            player.isOverloaded() ? Color{255,200,0,255} : Color{255,80,80,255};
                damageNumbers.push_back({enemy.position, dmg, col, 1.2f});
            }
        }
        if (hitAny) {
            triggerShake(3.5f, 0.12f);
            // HIT-STOP: micro-congelamento no impacto (mais forte em combos altos)
            hitStopTimer = (comboCount >= 5) ? 0.09f : 0.05f;
        }
    }


    Vector2 aimDir = Vector2Subtract(mouseWorld, player.position);

    // Skill 1 - Laser (piercing: fires 3 staggered beams)
    if (IsKeyPressed(KEY_ONE) && player.skills[0].isReady()) {
        player.useSkill(0, mouseWorld);
        float dmg = player.getEffectiveDamage() + player.skills[0].damage;
        projectiles.emplace_back(player.position, aimDir, dmg, 550.0f, 620.0f, Color{0,255,255,255});
        float baseA = std::atan2(aimDir.y, aimDir.x);
        for (int s : {-1, 1}) {
            float a = baseA + s * 0.12f;
            Vector2 d = {std::cos(a), std::sin(a)};
            projectiles.emplace_back(player.position, d, dmg * 0.6f, 550.0f, 560.0f,
                                     Color{0,200,255,180});
        }
        particles.spawnHit(player.position, Color{0,255,255,255}, 8);
        audio.playLaser();
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Laser ativo!", 1.5f);
    }

    // Skill 2 - EMP Area
    if (IsKeyPressed(KEY_TWO) && player.skills[1].isReady()) {
        player.useSkill(1, mouseWorld);
        float empDmg = player.skills[1].damage * (player.isOverloaded() ? 1.5f : 1.0f);
        for (auto& enemy : enemies) {
            if (Vector2Distance(player.position, enemy.position) <= player.skills[1].range) {
                enemy.takeDamage(empDmg);
                particles.spawnHit(enemy.position, YELLOW, 10);
            }
        }
        particles.spawnExplosion(player.position, YELLOW, 25);
        audio.playEMP();
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("EMP liberado!", 1.5f);
    }

    // Skill 3 - Plasma Grenade
    if (IsKeyPressed(KEY_THREE) && player.skills[2].isReady()) {
        player.useSkill(2, mouseWorld);
        float gDmg = player.skills[2].damage * (player.isOverloaded() ? 1.5f : 1.0f);
        projectiles.emplace_back(player.position, aimDir, gDmg,
                                 player.skills[2].range, 280.0f,
                                 Color{255,120,0,255}, true);
        audio.playLaser();
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Granada de plasma!", 1.5f);
    }

    // Skill 4 - Sobrecarga
    if (IsKeyPressed(KEY_FOUR) && player.skills[3].isReady()) {
        player.useSkill(3, mouseWorld);
        player.overloadTimer = 8.0f;
        particles.spawnLevelUp(player.position);
        audio.playLevelUp();
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Sobrecarga ativada!", 2.0f);
    }

    // Skill 5 - Barreira de Escudo
    if (IsKeyPressed(KEY_FIVE) && player.skills[4].isReady()) {
        player.useSkill(4, mouseWorld);
        player.shieldTimer = 3.0f;
        particles.spawnLevelUp(player.position);
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Barreira de escudo!", 2.0f);
    }

    // Skill 6 - Rajada (8 projetos em leque)
    if (IsKeyPressed(KEY_SIX) && player.skills[5].isReady()) {
        player.useSkill(5, mouseWorld);
        float baseAngle = std::atan2(aimDir.y, aimDir.x);
        float spread = 0.22f;
        float dmg = player.skills[5].damage * (player.isOverloaded() ? 1.5f : 1.0f);
        for (int i = -3; i <= 4; ++i) {
            float angle = baseAngle + spread * (float)i;
            Vector2 d = {std::cos(angle), std::sin(angle)};
            Color col = (std::abs(i) <= 1) ? Color{0,255,100,255} : Color{0,200,80,200};
            projectiles.emplace_back(player.position, d, dmg, 420.0f, 660.0f, col);
        }
        particles.spawnHit(player.position, Color{0,255,100,255}, 6);
        audio.playLaser();
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Rajada maxima!", 1.5f);
    }

    // Contextual dialogue — first enemy nearby
    if (!firstCombatTriggered) {
        for (const auto& e : enemies) {
            if (Vector2Distance(player.position, e.position) < 400.0f) {
                firstCombatTriggered = true;
                static const char* lines[] = {
                    "Vou limpar essa zona.",
                    "KRONOS... sempre KRONOS.",
                    "Vem. Nao tenho o dia todo.",
                    "NEXUS nunca desiste."
                };
                triggerPlayerSpeech(lines[GetRandomValue(0, 3)], 2.5f);
                break;
            }
        }
    }

    // Surrounded by 5+ enemies
    if (surroundedCooldown > 0.0f) surroundedCooldown -= dt;
    if (surroundedCooldown <= 0.0f) {
        int nearby = 0;
        for (const auto& e : enemies) {
            if (Vector2Distance(player.position, e.position) < 220.0f) nearby++;
        }
        if (nearby >= 5) {
            surroundedCooldown = 8.0f;
            if (playerSpeechTimer <= 0.5f) {
                static const char* slines[] = {
                    "Cercado! Hora das skills.",
                    "Muitos... mas nao impossivel.",
                    "Vou derrubar todos!"
                };
                triggerPlayerSpeech(slines[GetRandomValue(0,2)], 2.5f);
            }
        }
    }

    // NPC dialog / Shop
    // E = conversar com o NPC proximo (TODOS contam sua historia em baloes).
    // Vendedores tambem conversam; a loja deles abre com [TAB].
    if (IsKeyPressed(KEY_E)) {
        if (nearNpcIndex >= 0 && nearNpcIndex < (int)npcs.size()) {
            int nLines = (int)npcs[nearNpcIndex].dialogLines.size();
            if (nLines > 0) {
                if (!dialogOpen) { dialogOpen = true; dialogLine = 0; }   // inicia a historia
                else             { dialogLine = (dialogLine + 1) % nLines; } // avanca linha
            }
        }
    }

    // Toggle UI
    // Overlay keys — mutually exclusive: opening one closes all others
    if (IsKeyPressed(KEY_I)) {
        showInventory = !showInventory;
        showEquipment = false; showQuestLog = false;
        shopSystem.close(); craftingSystem.open = false;
    }
    if (IsKeyPressed(KEY_G)) {
        showEquipment = !showEquipment;
        showInventory = false; showQuestLog = false;
        shopSystem.close(); craftingSystem.open = false;
    }
    if (IsKeyPressed(KEY_J)) {
        showQuestLog = !showQuestLog;
        showInventory = false; showEquipment = false;
        shopSystem.close(); craftingSystem.open = false;
    }
    if (IsKeyPressed(KEY_TAB)) {
        if (shopSystem.open) { shopSystem.close(); }
        else {
            shopSystem.buildShop(-1, "NEXUS Supply Terminal");
            shopSystem.open = true;
            // Close everything else
            craftingSystem.open = false;
            showInventory = false; showEquipment = false; showQuestLog = false;
        }
    }
    if (IsKeyPressed(KEY_C)) {
        craftingSystem.open = !craftingSystem.open;
        if (craftingSystem.open) {
            shopSystem.close();
            showInventory = false; showEquipment = false; showQuestLog = false;
        }
    }
    if (IsKeyPressed(KEY_F5)) autoSave();

    // Companion spawn keys
    if (IsKeyPressed(KEY_F2)) { if (companions.empty()) spawnCompanion(CompanionType::MarcoVeil); }
    if (IsKeyPressed(KEY_F3)) spawnCompanion(CompanionType::Steel);
    if (IsKeyPressed(KEY_F4)) spawnCompanion(CompanionType::Rex);

    // Poder de cura (estilo Diablo 3) — tecla Q: cura instantanea + regeneracao.
    if (IsKeyPressed(KEY_Q)) {
        if (player.potionReady()) {
            player.usePotion();
            particles.spawnLevelUp(player.position);
            audio.playLevelUp();
            triggerPlayerSpeech("Cura ativada!", 1.5f);
        } else {
            triggerPlayerSpeech(TextFormat("Cura recarregando (%.0fs)", player.healCooldown), 1.5f);
        }
    }

    // Building system
    if (IsKeyPressed(KEY_B)) buildingSystem.toggleBuildMode();

    // Tecla U — evoluir o predio mais proximo
    if (IsKeyPressed(KEY_U) && !buildingSystem.buildModeActive) {
        int r = buildingSystem.upgradeNearby(player.position, player.credits);
        if (r == 1)      { triggerPlayerSpeech("Estrutura evoluida!", 1.8f); audio.playLevelUp(); }
        else if (r == 2) triggerPlayerSpeech("Creditos insuficientes para evoluir.", 2.5f);
        else if (r == 3) triggerPlayerSpeech("Esta estrutura ja esta no nivel maximo.", 2.5f);
        else if (r == 0) triggerPlayerSpeech("Chegue perto de uma construcao para evoluir.", 2.5f);
    }

    if (buildingSystem.buildModeActive) {
        // Scroll wheel changes selected building type
        float wheel = GetMouseWheelMove();
        if (wheel > 0.f) buildingSystem.selectedType = (buildingSystem.selectedType + 1) % BuildingSystem::NUM_TYPES;
        if (wheel < 0.f) buildingSystem.selectedType = (buildingSystem.selectedType + BuildingSystem::NUM_TYPES - 1) % BuildingSystem::NUM_TYPES;

        // Number keys 1-8 select building
        for (int k = 0; k < BuildingSystem::NUM_TYPES; k++) {
            if (IsKeyPressed(KEY_ONE + k)) buildingSystem.selectedType = k;
        }

        // CLIQUE no painel do menu = escolher o predio (mouse virtualizado)
        Vector2 vmouse = virtualizeMousePos(GetMousePosition());
        int menuCell = buildingSystem.menuCellAt(vmouse, screenWidth, screenHeight);

        // Left click: se foi no menu -> seleciona; senao -> coloca no mapa
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && menuCell >= 0) {
            buildingSystem.selectedType = menuCell;
            audio.playPickup();
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && menuCell < 0) {
            int creditCost = 0, metalCost = 0, carapaceCost = 0;
            bool ok = buildingSystem.tryPlace(mouseWorld, player.credits, materialMetal, materialCarapace,
                                              creditCost, metalCost, carapaceCost);
            if (ok) {
                player.credits   -= creditCost;
                materialMetal    -= metalCost;
                materialCarapace -= carapaceCost;
                audio.playPickup();
                triggerPlayerSpeech("Construido!", 1.2f);
            } else {
                // Diz EXATAMENTE o que falta para conseguir construir
                const BuildingCost& c = BuildingSystem::COSTS[buildingSystem.selectedType];
                if (player.credits < c.credits) {
                    triggerPlayerSpeech(TextFormat("Faltam creditos: tem $%d, precisa $%d. Mate inimigos p/ ganhar.",
                                        player.credits, c.credits), 3.5f);
                } else if (materialMetal < c.metalScrap) {
                    triggerPlayerSpeech(TextFormat("Falta Sucata de Metal: tem %d, precisa %d. Derrote robos/mecas.",
                                        materialMetal, c.metalScrap), 3.5f);
                } else if (materialCarapace < c.alienCarapace) {
                    triggerPlayerSpeech(TextFormat("Falta Carapaca Alien: tem %d, precisa %d. Derrote aliens.",
                                        materialCarapace, c.alienCarapace), 3.5f);
                } else {
                    triggerPlayerSpeech("Nao da pra construir aqui (local bloqueado).", 2.5f);
                }
            }
        }
        // Right click / B again to cancel
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) buildingSystem.buildModeActive = false;
    }
}

// ─── Spawn ───────────────────────────────────────────────────────────────────

void Game::spawnEnemy() {
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    float dist  = 380.0f;
    Vector2 pos = {
        player.position.x + std::cos(angle) * dist,
        player.position.y + std::sin(angle) * dist
    };

    int attempts = 0;
    while (tilemap.isWallAtPosition(pos) && attempts < 10) {
        angle = GetRandomValue(0, 360) * DEG2RAD;
        pos.x = player.position.x + std::cos(angle) * dist;
        pos.y = player.position.y + std::sin(angle) * dist;
        attempts++;
    }

    ZoneInfo info = getZoneInfo(currentZone);
    int roll = GetRandomValue(0, 100);

    EnemyType type;
    switch (currentZone) {
        case ZoneID::LARuins:
            // 3% OrcCibernetico (roll 97-99), rest shifted down
            if      (roll > 97) type = EnemyType::OrcCibernetico;
            else if (roll > 95) type = EnemyType::Zergling;
            else if (roll > 90) type = EnemyType::Kamikaze;
            else if (roll > 78) type = EnemyType::Shooter;
            else if (roll > 65) type = EnemyType::Tank;
            else if (roll > 55) type = EnemyType::Sniper;
            else                type = EnemyType::Scout;
            break;
        case ZoneID::Bunker:
            // 3% PaladinCorrompido (97-99), 3% OrcCibernetico (94-96)
            if      (roll > 97) type = EnemyType::PaladinCorrompido;
            else if (roll > 94) type = EnemyType::OrcCibernetico;
            else if (roll > 91) type = EnemyType::Hydra;
            else if (roll > 82) type = EnemyType::Zergling;
            else if (roll > 77) type = EnemyType::HunterDrone;
            else if (roll > 65) type = EnemyType::Sniper;
            else if (roll > 51) type = EnemyType::Shooter;
            else if (roll > 41) type = EnemyType::Kamikaze;
            else                type = EnemyType::Scout;
            break;
        case ZoneID::KronosForge:
            // SkynetFactory: 5% UndeadEnforcer, 2% OrcCibernetico, 2% PaladinCorrompido
            if      (roll > 95) type = EnemyType::UndeadEnforcer;
            else if (roll > 93) type = EnemyType::OrcCibernetico;
            else if (roll > 91) type = EnemyType::PaladinCorrompido;
            else if (roll > 90) type = EnemyType::Broodmother;
            else if (roll > 74) type = EnemyType::Zergling;
            else if (roll > 59) type = EnemyType::Hydra;
            else if (roll > 48) type = EnemyType::MorphX;
            else if (roll > 36) type = EnemyType::HunterDrone;
            else if (roll > 24) type = EnemyType::Shooter;
            else if (roll > 12) type = EnemyType::Kamikaze;
            else                type = EnemyType::Tank;
            break;
        case ZoneID::KronosNexus:
            // CoreFacility: 8% UndeadEnforcer, 4% OrcCibernetico, 3% PaladinCorrompido
            if      (roll > 92) type = EnemyType::UndeadEnforcer;
            else if (roll > 88) type = EnemyType::OrcCibernetico;
            else if (roll > 85) type = EnemyType::PaladinCorrompido;
            else if (roll > 82) type = EnemyType::Broodmother;
            else if (roll > 65) type = EnemyType::Zergling;
            else if (roll > 49) type = EnemyType::Hydra;
            else if (roll > 37) type = EnemyType::MorphX;
            else if (roll > 26) type = EnemyType::HunterDrone;
            else if (roll > 16) type = EnemyType::KronosSentry;
            else if (roll > 8)  type = EnemyType::Sniper;
            else                type = EnemyType::Tank;
            break;

        // ── Fases Sombrias — dark zone spawn tables ────────────────────────────
        case ZoneID::Cemetery:
        case ZoneID::CursedFarm:
            if      (roll > 94) type = EnemyType::ZombieLord;
            else if (roll > 85) type = EnemyType::ZombieRager;
            else if (roll > 72) type = EnemyType::PoltergeistBoss;
            else if (roll > 58) type = EnemyType::Ghost;
            else if (roll > 40) type = EnemyType::ZombieHorde;
            else                type = EnemyType::Zombie;
            break;
        case ZoneID::GhostCity:
        case ZoneID::DarkForest:
            if      (roll > 93) type = EnemyType::BansheeHowler;
            else if (roll > 84) type = EnemyType::ShadowWraith;
            else if (roll > 72) type = EnemyType::GhostElite;
            else if (roll > 58) type = EnemyType::ZombieRager;
            else if (roll > 42) type = EnemyType::Ghost;
            else                type = EnemyType::Zombie;
            break;
        case ZoneID::Catacombs:
        case ZoneID::AbandonedManor:
            if      (roll > 92) type = EnemyType::ZombieLord;
            else if (roll > 84) type = EnemyType::ShadowWraith;
            else if (roll > 76) type = EnemyType::BansheeHowler;
            else if (roll > 64) type = EnemyType::GhostElite;
            else if (roll > 50) type = EnemyType::ZombieRager;
            else if (roll > 35) type = EnemyType::PoltergeistBoss;
            else if (roll > 20) type = EnemyType::ZombieHorde;
            else                type = EnemyType::Ghost;
            break;

        // ── Zona Inferno — aliens + zumbis em ambiente vulcanico ──────────────
        case ZoneID::InfernoZone:
            if      (roll > 95) type = EnemyType::AlienBoss;
            else if (roll > 89) type = EnemyType::OmegaBoss;
            else if (roll > 79) type = EnemyType::Ghost;
            else if (roll > 69) type = EnemyType::ZombieHorde;
            else if (roll > 57) type = EnemyType::ZombieRager;
            else if (roll > 44) type = EnemyType::Zombie;
            else if (roll > 33) type = EnemyType::Broodmother;
            else if (roll > 19) type = EnemyType::Hydra;
            else                type = EnemyType::Zergling;
            break;

        default:
            type = EnemyType::Scout;
            break;
    }

    Enemy& e = enemies.emplace_back(pos, type);

    // Scale enemy with player level — harder as you get stronger
    if (player.level > 1) {
        float scale = 1.0f + (player.level - 1) * 0.12f;
        e.health    *= scale;
        e.maxHealth *= scale;
        e.damage    *= (1.0f + (player.level - 1) * 0.08f);
        e.xpReward  = (int)(e.xpReward * scale);
    }

    // Global progression scaling — gets harder as kills and wave count rise
    {
        float gs = Enemy::getGlobalScaling(totalKills, anomalySystem.waveNumber);
        if (gs > 1.0f) {
            e.health    *= gs;
            e.maxHealth *= gs;
            e.damage    *= gs;
            e.speed     *= (1.0f + (gs - 1.0f) * 0.35f); // speed scales slower
            e.xpReward  = (int)(e.xpReward * gs);
        }
    }

    // Difficulty scaling
    {
        const DifficultySettings& diff = getDifficulty();
        e.health    *= diff.enemyHPMult;
        e.maxHealth *= diff.enemyHPMult;
        e.damage    *= diff.enemyDmgMult;
        e.speed     *= diff.enemySpeedMult;
    }

    // Motor de Evolucao Infinita — Nivel de Ameaca + mutador ativo
    {
        float ts = threatStatMult();
        e.health    *= ts * mutatorHPMult();
        e.maxHealth *= ts * mutatorHPMult();
        e.damage    *= ts * mutatorDmgMult();
        e.speed     *= mutatorSpeedMult();
        e.xpReward   = (int)(e.xpReward * ts);
    }

    // Zergling swarm — spawn 2 more in formation (StarCraft feel)
    if (type == EnemyType::Zergling) {
        for (int z = 0; z < 2; ++z) {
            float za = angle + (z == 0 ? 0.25f : -0.25f);
            float zd = 360.0f + z * 30.0f;
            Vector2 zp = {
                player.position.x + std::cos(za) * zd,
                player.position.y + std::sin(za) * zd
            };
            enemies.emplace_back(zp, EnemyType::Zergling);
        }
    }

    // 15% chance to spawn as elite (no minions, no bosses)
    if (type != EnemyType::Boss && GetRandomValue(0, 100) < 15) {
        e.makeElite(GetRandomValue(0, 2));
    }
}

void Game::spawnBoss() {
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    Vector2 pos = {
        player.position.x + std::cos(angle) * 420.0f,
        player.position.y + std::sin(angle) * 420.0f
    };
    for (int i = 0; i < 10 && tilemap.isWallAtPosition(pos); ++i) {
        angle = GetRandomValue(0, 360) * DEG2RAD;
        pos.x = player.position.x + std::cos(angle) * 420.0f;
        pos.y = player.position.y + std::sin(angle) * 420.0f;
    }
    enemies.emplace_back(pos, EnemyType::Boss);
    {
        Enemy& boss = enemies.back();
        const DifficultySettings& diff = getDifficulty();
        boss.health    *= diff.enemyHPMult * diff.bossHPMult;
        boss.maxHealth *= diff.enemyHPMult * diff.bossHPMult;
        boss.damage    *= diff.enemyDmgMult;
        boss.speed     *= diff.enemySpeedMult;
    }
}

// ─── Companion System ─────────────────────────────────────────────────────────

void Game::spawnCompanion(CompanionType t) {
    // Spawn slightly offset from player
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    Vector2 spawnPos = {
        player.position.x + std::cos(angle) * 60.f,
        player.position.y + std::sin(angle) * 60.f
    };
    companions.emplace_back(t);
    companions.back().position = spawnPos;
}

void Game::updateCompanions(float dt) {
    // Build a list of enemy raw pointers for companion AI
    std::vector<Enemy*> enemyPtrs;
    enemyPtrs.reserve(enemies.size());
    for (auto& e : enemies) {
        if (!e.isDead()) enemyPtrs.push_back(&e);
    }

    for (auto& c : companions) {
        if (!c.active) continue;
        c.update(dt, player.position, enemyPtrs);

        // Handle companion projectile requests (KyleReese)
        if (c.wantsToShoot && !c.isDead()) {
            projectiles.emplace_back(c.position, c.shootDir,
                c.shootDamage, c.shootRange,
                c.shootSpeed,  c.projectileColor);
        }

        // Handle companion AoE stomp (T800Ally skill)
        if (c.wantsAoE && !c.isDead()) {
            particles.spawnExplosion(c.position, {0, 180, 255, 255}, 20);
            triggerShake(4.f, 0.15f);
            for (auto& e : enemies) {
                if (e.isDead()) continue;
                if (Vector2Distance(c.position, e.position) <= c.aoeRadius) {
                    e.takeDamage(c.aoeDamage);
                    particles.spawnHit(e.position, {0, 200, 255, 255}, 5);
                }
            }
        }

        // Damage companions from enemies (melee contact)
        for (auto& e : enemies) {
            if (e.isDead() || c.isDead()) continue;
            float dist = Vector2Distance(c.position, e.position);
            if (dist <= c.radius + e.radius) {
                // Enemy deals contact damage to companion
                float contactDmg = e.damage * dt * 2.f;
                c.takeDamage(contactDmg);
            }
        }

        // Damage companions from enemy projectiles
        for (auto& ep : enemyProjectiles) {
            if (!ep.active || c.isDead()) continue;
            if (Vector2Distance(c.position, ep.position) <= c.radius + 5.f) {
                c.takeDamage(ep.damage);
                ep.active = false;
            }
        }
    }
}

void Game::drawCompanions() const {
    for (const auto& c : companions) {
        if (c.active) c.render();
    }
}

void Game::spawnOmegaBoss() {
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    Vector2 pos = {
        player.position.x + std::cos(angle) * 480.0f,
        player.position.y + std::sin(angle) * 480.0f
    };
    for (int i = 0; i < 12 && tilemap.isWallAtPosition(pos); ++i) {
        angle = GetRandomValue(0, 360) * DEG2RAD;
        pos.x = player.position.x + std::cos(angle) * 480.0f;
        pos.y = player.position.y + std::sin(angle) * 480.0f;
    }
    Enemy omega(pos, EnemyType::OmegaBoss);
    // Scale with player level
    float lvlScale = 1.0f + (player.level - 1) * 0.2f;
    omega.health    *= lvlScale;
    omega.maxHealth *= lvlScale;
    omega.damage    *= (1.0f + (player.level - 1) * 0.12f);
    // Difficulty scaling for omega boss
    {
        const DifficultySettings& diff = getDifficulty();
        omega.health    *= diff.enemyHPMult * diff.bossHPMult;
        omega.maxHealth *= diff.enemyHPMult * diff.bossHPMult;
        omega.damage    *= diff.enemyDmgMult;
        omega.speed     *= diff.enemySpeedMult;
    }
    enemies.push_back(omega);
    showStoryBanner("!! OMEGA BOSS !!", "Uma ameaca de nivel extinção detectada. BOA SORTE.", 4.0f);
    triggerPlayerSpeech("PERIGO EXTREMO. Protocolo de sobrevivencia ativado!", 4.5f);
    audio.playBossRoar();
    triggerShake(12.0f, 0.5f);
}

void Game::spawnFinalBoss() {
    // NUCLEO KRONOS — o boss final. Sua morte vence o jogo.
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    Vector2 pos = {
        player.position.x + std::cos(angle) * 520.0f,
        player.position.y + std::sin(angle) * 520.0f
    };
    for (int i = 0; i < 12 && tilemap.isWallAtPosition(pos); ++i) {
        angle = GetRandomValue(0, 360) * DEG2RAD;
        pos.x = player.position.x + std::cos(angle) * 520.0f;
        pos.y = player.position.y + std::sin(angle) * 520.0f;
    }
    Enemy core(pos, EnemyType::OmegaBoss);
    core.isFinalBoss = true;
    // Muito mais forte que o OmegaBoss normal — e o clímax do jogo
    float lvlScale = 1.0f + (player.level - 1) * 0.25f;
    core.health    *= lvlScale * 3.0f;
    core.maxHealth *= lvlScale * 3.0f;
    core.damage    *= (1.0f + (player.level - 1) * 0.14f) * 1.4f;
    {
        const DifficultySettings& diff = getDifficulty();
        core.health    *= diff.enemyHPMult * diff.bossHPMult;
        core.maxHealth *= diff.enemyHPMult * diff.bossHPMult;
        core.damage    *= diff.enemyDmgMult;
        core.speed     *= diff.enemySpeedMult;
    }
    enemies.push_back(core);
    finalBossSpawned = true;
    finalBossAlive   = true;
    showStoryBanner("== NUCLEO KRONOS ==", "O coracao da IA. Destrua-o e liberte a humanidade.", 5.0f);
    triggerPlayerSpeech("KRONOS... e aqui que tudo termina. Por todos nos!", 5.0f);
    audio.playBossRoar();
    triggerShake(16.0f, 0.7f);
}

void Game::drawVictoryScreen() const {
    // Fundo escuro com brilho dourado pulsante
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.86f));
    float t     = victoryTimer;
    float pulse = 0.6f + 0.4f * sinf(t * 2.0f);

    // Raios de luz dourada saindo do centro
    int cx = screenWidth / 2, cy = screenHeight / 2;
    for (int i = 0; i < 24; i++) {
        float a  = i * (PI / 12.0f) + t * 0.3f;
        float len = 400.0f + 120.0f * sinf(t * 1.5f + i);
        Vector2 e = {cx + cosf(a) * len, cy + sinf(a) * len};
        DrawLineEx({(float)cx, (float)cy}, e, 2.0f,
                   ColorAlpha({255, 215, 80, 255}, 0.06f * pulse));
    }

    // Titulo
    const char* title = "VITORIA";
    int tw = MeasureText(title, 80);
    DrawText(title, cx - tw/2 + 3, cy - 120 + 3, 80, ColorAlpha(BLACK, 0.7f));
    DrawText(title, cx - tw/2,     cy - 120,     80, ColorAlpha({255, 215, 80, 255}, pulse));

    // Subtitulo
    const char* sub = "O NUCLEO KRONOS FOI DESTRUIDO";
    int sw = MeasureText(sub, 24);
    DrawText(sub, cx - sw/2, cy - 20, 24, {220, 220, 255, 255});

    const char* sub2 = "A humanidade esta livre. Voce venceu DARKNET.";
    int sw2 = MeasureText(sub2, 18);
    DrawText(sub2, cx - sw2/2, cy + 14, 18, ColorAlpha(WHITE, 0.8f));

    // Estatisticas finais
    int sy = cy + 60;
    const char* stats[] = {
        TextFormat("Nivel alcancado: %d", player.level),
        TextFormat("Inimigos eliminados: %d", totalKills),
        TextFormat("Creditos acumulados: %d", player.credits),
    };
    for (int i = 0; i < 3; i++) {
        int w = MeasureText(stats[i], 16);
        DrawText(stats[i], cx - w/2, sy + i*24, 16, ColorAlpha({180, 220, 255, 255}, 0.9f));
    }

    // Prompt para continuar
    if (victoryTimer > 2.0f && ((int)(t * 2) % 2 == 0)) {
        const char* prompt = "[ENTER] voltar ao menu";
        int pw = MeasureText(prompt, 18);
        DrawText(prompt, cx - pw/2, sy + 90, 18, ColorAlpha({255, 215, 80, 255}, 0.9f));
    }
}

// ─── Zone Transition ─────────────────────────────────────────────────────────

void Game::checkPortalTransition() {
    ZoneID dest;
    if (tilemap.isPortalAtPosition(player.position, dest)) {
        transitionToZone(dest);
    }
}

void Game::transitionToZone(ZoneID dest) {
    currentZone = dest;
    enemies.clear();
    items.clear();
    projectiles.clear();
    enemyProjectiles.clear();
    xpOrbs.clear();
    groundEquips.clear();      // evita drops orfaos da zona anterior
    damageNumbers.clear();
    dialogOpen   = false;
    nearNpcIndex = -1;         // invalida indice antes de reconstruir os NPCs
    particles.spawnExplosion(player.position, SKYBLUE, 20);

    tilemap.generate(dest);
    setupZoneNPCs(dest);
    background.generate(dest, tilemap.width, tilemap.height, Tilemap::tileSize);

    // Inferno zone — reset when leaving, generate when entering
    if (dest == ZoneID::InfernoZone) {
        unsigned int seed = (unsigned int)GetRandomValue(1000, 99999);
        infernoZone.generate(tilemap.width * Tilemap::tileSize,
                             tilemap.height * Tilemap::tileSize, seed);
    } else {
        infernoZone.reset();
    }

    // Load dark world scenery for sombre zones (Cemetery..AbandonedManor only)
    if ((int)dest >= (int)ZoneID::Cemetery && dest != ZoneID::InfernoZone) {
        darkWorld.load((int)dest, (unsigned int)GetRandomValue(1000, 99999));
    } else {
        darkWorld.active = false;
    }

    float cx = static_cast<float>(tilemap.width  * Tilemap::tileSize) / 2.0f;
    float cy = static_cast<float>(tilemap.height * Tilemap::tileSize) / 2.0f;
    player.position = {cx, cy};

    bossSpawned        = false;
    spawnTimer         = 0.0f;
    spawnInterval      = getZoneInfo(dest).spawnInterval;
    bossSpawnThreshold = (dest == ZoneID::KronosNexus) ? 10 : 20;
    zoneNameTimer      = 4.0f;
    dialogOpen         = false;
    nearNpcIndex       = -1;

    // Story chapter banners
    switch (dest) {
        case ZoneID::Bunker:
            storyChapter = 2;
            showStoryBanner("CAPITULO 2: O BUNKER NEXUS",
                "Base do NEXUS localizada. Mas KRONOS nos rastreia...", 5.0f);
            triggerPlayerSpeech("Chegando ao Bunker NEXUS. Aliados detectados.", 4.0f);
            break;
        case ZoneID::KronosForge:
            storyChapter = 3;
            showStoryBanner("CAPITULO 3: A FORJA",
                "Dentro das entranhas de KRONOS. Cada maquina foi construida para matar.", 5.0f);
            triggerPlayerSpeech("Forja KRONOS infiltrada. Alerta maximo.", 4.0f);
            break;
        case ZoneID::KronosNexus:
            storyChapter = 4;
            showStoryBanner("CAPITULO FINAL: O NUCLEO",
                "Este e o coracao de KRONOS. Destrua-o e liberte a humanidade.", 6.0f);
            triggerPlayerSpeech("Nucleo KRONOS localizado. Hora de terminar isso.", 5.0f);
            break;
        case ZoneID::Cemetery:
            showStoryBanner("CEMITERIO ABANDONADO", "Os mortos nao descansam aqui...", 5.0f);
            triggerPlayerSpeech("Lugar sombrio. Mas nao tenho medo de morte.", 4.0f);
            break;
        case ZoneID::CursedFarm:
            showStoryBanner("FAZENDA MALDITA", "A terra esta podre. As colheitas, corrompidas.", 5.0f);
            triggerPlayerSpeech("Algo muito errado nessa fazenda...", 3.5f);
            break;
        case ZoneID::GhostCity:
            showStoryBanner("CIDADE FANTASMA", "Ruas vazias. Mas nao desertas.", 5.0f);
            triggerPlayerSpeech("Uma cidade inteira... silenciada.", 3.5f);
            break;
        case ZoneID::DarkForest:
            showStoryBanner("FLORESTA NEGRA", "A nevoa esconde o que mora entre as arvores.", 5.0f);
            triggerPlayerSpeech("Visibilidade zero. Cuidado.", 3.0f);
            break;
        case ZoneID::Catacombs:
            showStoryBanner("CATACUMBAS", "Passagens de pedra. Cheiro de morte antiga.", 5.0f);
            triggerPlayerSpeech("Catacumbas. Quantos anos de morte aqui?", 3.5f);
            break;
        case ZoneID::AbandonedManor:
            showStoryBanner("MANSAO ABANDONADA", "O boss aguarda nas profundezas.", 6.0f);
            triggerPlayerSpeech("A Mansao. Sinto algo poderoso aqui dentro.", 4.0f);
            break;
        case ZoneID::InfernoZone:
            showStoryBanner("ZONA INFERNO", "Lava, cinzas e criaturas do abismo. Bem-vindo ao inferno.", 6.0f);
            triggerPlayerSpeech("Temperatura critica. Solo derretendo sob meus pes.", 4.5f);
            break;
        default: break;
    }

    autoSave();
}

// ─── Collisions ──────────────────────────────────────────────────────────────

void Game::checkCollisions() {
    // Player projectiles vs enemies
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        if (proj.isGrenade) continue; // handled in updateProjectiles on expire

        for (auto& enemy : enemies) {
            if (enemy.isDead()) continue;   // nao desperdicar tiro em cadaver pendente
            if (Vector2Distance(proj.position, enemy.position) <= enemy.radius + proj.radius) {
                enemy.takeDamage(proj.damage);
                particles.spawnHit(enemy.position, Color{0,255,255,255}, 6);
                proj.active = false;
                audio.playHit();
                comboCount++;
                comboTimer = 2.5f;
                Color projDmgCol = comboCount >= 5 ? Color{0,255,200,255} : Color{0,255,255,255};
                damageNumbers.push_back({enemy.position, proj.damage, projDmgCol, 1.0f});
                break;
            }
        }
    }

    // Items pickup — RAIO DE COLETA AUTOMATICA + magnetismo
    // Itens dentro do raio de coleta sao pegos automaticamente; itens dentro do
    // raio de atracao voam em direcao ao jogador.
    const float ftime     = GetFrameTime();
    const float COLLECT_R = player.radius + 52.0f;   // coleta automatica
    const float MAGNET_R  = 230.0f;                   // atracao magnetica
    for (auto it = items.begin(); it != items.end();) {
        float d = Vector2Distance(player.position, it->position);
        // Magnetismo: puxa o item para o jogador quando dentro do raio de atracao
        if (it->pickupDelay <= 0.0f && d > COLLECT_R && d < MAGNET_R) {
            Vector2 dir = Vector2Normalize(Vector2Subtract(player.position, it->position));
            float pull  = (1.0f - d / MAGNET_R) * 560.0f + 140.0f;
            it->position.x += dir.x * pull * ftime;
            it->position.y += dir.y * pull * ftime;
        }
        if (it->pickupDelay <= 0.0f && d <= COLLECT_R) {
            switch (it->type) {
                case ItemType::HealthPack:
                    player.heal(30.0f);
                    damageNumbers.push_back({it->position, 30.0f, {0,210,80,255}, 1.2f, "+"});
                    break;
                case ItemType::Credits:
                    player.credits += it->value;
                    damageNumbers.push_back({it->position, (float)it->value, {255,210,0,255}, 1.4f, "$"});
                    break;
                case ItemType::TechChip:
                    player.addXP(50);
                    damageNumbers.push_back({it->position, 50.0f, {0,200,255,255}, 1.4f, "XP+"});
                    break;
                case ItemType::NanoCore:
                    player.increaseBaseMaxHP(25.0f);
                    player.heal(25.0f);
                    damageNumbers.push_back({it->position, 25.0f, {255,80,200,255}, 1.6f, "HP+"});
                    particles.spawnLevelUp(it->position);
                    break;
                case ItemType::PlasmaCell:
                    for (auto& s : player.skills) s.currentCooldown *= 0.3f;
                    damageNumbers.push_back({it->position, 0.0f, {180,0,255,255}, 1.2f, "CD-"});
                    break;
                case ItemType::ScrapMetal:
                    player.heal(8.0f);
                    player.credits += 8;
                    damageNumbers.push_back({it->position, 8.0f, {160,160,170,255}, 0.9f, "+"});
                    break;
                case ItemType::WeaponPart:
                    player.credits += 20;
                    damageNumbers.push_back({it->position, 20.0f, {255,130,0,255}, 1.1f, "$"});
                    break;
                case ItemType::EnergyCore:
                    // Grants a shield burst
                    player.shieldTimer = std::max(player.shieldTimer, 2.5f);
                    damageNumbers.push_back({it->position, 0.0f, {0,255,255,255}, 1.2f, "SHD"});
                    break;
                // ── Crafting materials — go into inventory for use in crafting ──
                case ItemType::MetalScrap:
                    player.inventory.push_back(*it);
                    damageNumbers.push_back({it->position, 0.f, {180,180,180,255}, 0.9f, "MAT+"});
                    break;
                case ItemType::AlienCarapace:
                    player.inventory.push_back(*it);
                    damageNumbers.push_back({it->position, 0.f, {60,255,80,255}, 0.9f, "MAT+"});
                    break;
                case ItemType::PlasmaCore:
                    player.inventory.push_back(*it);
                    damageNumbers.push_back({it->position, 0.f, {0,180,255,255}, 0.9f, "MAT+"});
                    break;
                case ItemType::NanoFiber:
                    player.inventory.push_back(*it);
                    damageNumbers.push_back({it->position, 0.f, {0,220,200,255}, 0.9f, "MAT+"});
                    break;
                case ItemType::OmegaEssence:
                    player.inventory.push_back(*it);
                    damageNumbers.push_back({it->position, 0.f, {255,215,0,255}, 1.4f, "OMEGA+"});
                    particles.spawnLevelUp(it->position);
                    break;
                default:
                    player.addItem(*it);
                    break;
            }

            // Quest tracking — quests de Collect avancam ao pegar QUALQUER item
            // (exceto creditos). Garante que as barras de coleta enchem.
            if (it->type != ItemType::Credits) {
                for (auto& q : quests) {
                    if (!q.completed && q.active && q.type == QuestType::Collect) {
                        q.updateProgress(1);
                        if (q.isComplete() && !q.rewardGiven) grantQuestRewards(q);
                    }
                }
            }

            audio.playPickup();
            it = items.erase(it);
        } else {
            ++it;
        }
    }

    // XP orbs — mesmo raio de coleta + magnetismo (atracao mais forte)
    for (auto it = xpOrbs.begin(); it != xpOrbs.end();) {
        float dxp = Vector2Distance(player.position, it->position);
        if (dxp > COLLECT_R && dxp < MAGNET_R + 60.0f) {
            Vector2 dir = Vector2Normalize(Vector2Subtract(player.position, it->position));
            float pull  = (1.0f - dxp / (MAGNET_R + 60.0f)) * 640.0f + 160.0f;
            it->position.x += dir.x * pull * ftime;
            it->position.y += dir.y * pull * ftime;
        }
        if (dxp <= COLLECT_R) {
            player.addXP(it->amount);
            if (player.leveledUp) {
                particles.spawnLevelUp(player.position);
                audio.playLevelUp();
                static const char* lvlLines[] = {
                    "Estou ficando mais forte.",
                    "Experiencia e a melhor arma.",
                    "KRONOS nao sabe o que vem ai.",
                    "Modulo de combate expandido.",
                    "Capacidade elevada. Missao continua."
                };
                triggerPlayerSpeech(lvlLines[player.level % 5], 3.0f);
                // NAO trava o jogo — apenas acumula pontos e avisa o jogador.
                // Ele escolhe quando quiser: tecla L (level up) / tecla K (evolucao).
                levelUpAnimTimer   = 0.0f;
                pendingNotifyPulse = 1.0f;
                static const int EVO_LEVELS[] = {10, 25, 40, 60};
                bool isEvoLevel = false;
                for (int el : EVO_LEVELS) if (player.level == el) { isEvoLevel = true; break; }
                if (isEvoLevel) {
                    pendingEvolutions++;
                    triggerPlayerSpeech("EVOLUCAO disponivel! Pressione K para escolher.", 4.0f);
                } else {
                    pendingLevelUps++;
                }
            }
            it = xpOrbs.erase(it);
        } else {
            ++it;
        }
    }
}

// ─── Update Helpers ──────────────────────────────────────────────────────────

void Game::updateItems(float dt) {
    for (auto it = items.begin(); it != items.end();) {
        it->update(dt);
        if (it->lifetime <= 0.0f) it = items.erase(it);
        else ++it;
    }
    // Teto rígido: impede acúmulo ilimitado de drops pelo mundo (causa de FPS=1).
    const size_t MAX_ITEMS = 220;
    if (items.size() > MAX_ITEMS)
        items.erase(items.begin(), items.begin() + (items.size() - MAX_ITEMS));
}

void Game::updateXPOrbs(float dt) {
    for (auto it = xpOrbs.begin(); it != xpOrbs.end();) {
        it->update(dt, player.position, player.radius);
        if (it->isExpired()) it = xpOrbs.erase(it);
        else ++it;
    }
    const size_t MAX_ORBS = 280;
    if (xpOrbs.size() > MAX_ORBS)
        xpOrbs.erase(xpOrbs.begin(), xpOrbs.begin() + (xpOrbs.size() - MAX_ORBS));
}

void Game::updateProjectiles(float dt) {
    for (auto it = projectiles.begin(); it != projectiles.end();) {
        it->update(dt);

        bool hitWall = tilemap.isWallAtPosition(it->position);

        if (it->isGrenade && (hitWall || it->isOutOfRange())) {
            // Grenade explosion
            if (!it->exploded) {
                it->exploded = true;
                for (auto& enemy : enemies) {
                    if (Vector2Distance(it->position, enemy.position) <= it->explodeRadius) {
                        enemy.takeDamage(it->damage);
                        particles.spawnHit(enemy.position, Color{255,120,0,255}, 12);
                        audio.playHit();
                    }
                }
                particles.spawnExplosion(it->position, Color{255,140,0,255}, 35);
                audio.playExplosion();
            }
            it = projectiles.erase(it);
        } else if (hitWall || !it->active || it->isOutOfRange()) {
            it = projectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::updateEnemyProjectiles(float dt) {
    for (auto it = enemyProjectiles.begin(); it != enemyProjectiles.end();) {
        it->update(dt);

        bool hitWall = tilemap.isWallAtPosition(it->position);

        if (it->hitsPlayer(player.position, player.radius)) {
            if (!player.isShielded()) {
                player.takeDamage(it->damage);
                hitFlashTimer = 0.30f;
                particles.spawnHit(player.position, RED, 6);
                triggerShake(6.0f, 0.22f);
                audio.playHit();
            }
            it->active = false;
        }

        if (!it->active || hitWall || it->isOutOfRange()) {
            it = enemyProjectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::autoSave() {
    SaveManager::save(player, quests, currentZone);
}

void Game::showStoryBanner(const std::string& title, const std::string& sub, float dur) {
    storyBannerText  = title;
    storyBannerSub   = sub;
    storyBannerTimer = dur;
}

void Game::triggerPlayerSpeech(const std::string& text, float dur) {
    playerSpeechText  = text;
    playerSpeechTimer = dur;
}

void Game::drawPlayerSpeech() const {
    auto drawBubble = [&](Vector2 worldPos, float height3D, float offset2D, float offset3D, const std::string& text, float timer) {
        if (timer <= 0.0f) return;
        float alpha = timer < 0.8f ? timer / 0.8f : 1.0f;

        const char* txt = text.c_str();
        int fontSize = 14;
        int tw = MeasureText(txt, fontSize);
        int bw = tw + 24, bh = 28;

        Vector2 sp;
        int by = 0;
        if (render3D) {
            sp = GetWorldToScreenEx({ worldPos.x, height3D, worldPos.y }, camera3D, screenWidth, screenHeight);
            by = (int)(sp.y) - (int)offset3D;
        } else {
            sp = GetWorldToScreen2D(worldPos, camera);
            by = (int)(sp.y) - (int)offset2D;
        }
        int bx = (int)(sp.x) - bw / 2;

        // Clamp to screen bounds
        if (bx < 5) bx = 5;
        if (bx + bw > screenWidth - 5) bx = screenWidth - bw - 5;
        if (by < 5) by = 5;

        // Bubble Panel
        DrawRectangleRounded({(float)bx,(float)by,(float)bw,(float)bh}, 0.35f, 6,
                             ColorAlpha(BLACK, 0.88f * alpha));
        DrawRectangleLinesEx({(float)bx,(float)by,(float)bw,(float)bh}, 1.5f,
                             ColorAlpha({0,200,255,255}, 0.9f * alpha));

        // Tail pointing down to character
        int tx = (int)sp.x;
        if (tx < bx + 8) tx = bx + 8;
        if (tx > bx + bw - 8) tx = bx + bw - 8;
        int tailY = by + bh;
        DrawTriangle({(float)(tx-7),(float)tailY},{(float)(tx+7),(float)tailY},
                     {(float)tx,(float)(tailY+12)}, ColorAlpha(BLACK, 0.88f * alpha));
        DrawLineEx({(float)(tx-6),(float)tailY},{(float)tx,(float)(tailY+11)},
                   1.5f, ColorAlpha({0,200,255,255}, 0.8f * alpha));
        DrawLineEx({(float)(tx+6),(float)tailY},{(float)tx,(float)(tailY+11)},
                   1.5f, ColorAlpha({0,200,255,255}, 0.8f * alpha));

        DrawText(txt, bx + 12, by + 7, fontSize, ColorAlpha({0,240,255,255}, alpha));
    };

    // 1. Local Player
    drawBubble(player.position, 60.0f, 100.0f, 40.0f, playerSpeechText, playerSpeechTimer);

    // 2. Remote Players (activeChats)
    if (netActive) {
        for (const auto& pair : activeChats) {
            uint32_t peerId = pair.first;
            const ChatBubble& cb = pair.second;
            
            // Find the peer's position
            for (const auto& p : net.peers()) {
                if (p.id == peerId) {
                    drawBubble({ p.x, p.y }, 42.0f, 80.0f, 32.0f, cb.text, cb.timer);
                    break;
                }
            }
        }
    }
}

void Game::drawStoryBanner() const {
    if (storyBannerTimer <= 0.0f) return;
    float alpha = std::min(storyBannerTimer, 1.0f);
    // Fade out last 1s
    if (storyBannerTimer < 1.0f) alpha = storyBannerTimer;

    // Barra compacta no terco superior (nao cobre o centro do jogo)
    int barY = screenHeight / 5;        // bem acima do personagem
    int barH = 48;
    DrawRectangle(0, barY, screenWidth, barH,
                  ColorAlpha(BLACK, 0.78f * alpha));
    DrawLineEx({0, (float)barY},
               {(float)screenWidth, (float)barY},
               2, ColorAlpha({0,200,255,255}, 0.6f * alpha));
    DrawLineEx({0, (float)(barY + barH)},
               {(float)screenWidth, (float)(barY + barH)},
               2, ColorAlpha({0,200,255,255}, 0.6f * alpha));

    // Chapter title
    int tw = MeasureText(storyBannerText.c_str(), 20);
    DrawText(storyBannerText.c_str(),
             screenWidth/2 - tw/2, barY + 6, 20,
             ColorAlpha({0,220,255,255}, alpha));

    // Subtitle
    int sw = MeasureText(storyBannerSub.c_str(), 12);
    DrawText(storyBannerSub.c_str(),
             screenWidth/2 - sw/2, barY + 30, 12,
             ColorAlpha(WHITE, alpha * 0.85f));
}

// ─── Render ──────────────────────────────────────────────────────────────────

// ─── 2.5D isométrico (Incremento 1: câmera + tilemap 3D + raycast) ───────────

void Game::updateCamera3D() {
    float z = cameraZoom;
    camera3D.position   = { camera.target.x, cameraHeight * z, camera.target.y + cameraDistY * z };
    camera3D.target     = { camera.target.x, 0.0f, camera.target.y };
    camera3D.up         = { 0.0f, 1.0f, 0.0f };
    camera3D.fovy       = 30.0f;
    camera3D.projection = CAMERA_PERSPECTIVE;
}

// Lança um raio do mouse (virtualizado p/ a render-texture 1280x720) e intersecta
// o plano do chão Y=0, devolvendo a posição em coordenadas de mundo 2D (x, z).
Vector2 Game::mouseGround3D() const {
    Ray ray = GetScreenToWorldRayEx(virtualizeMousePos(GetMousePosition()),
                                    camera3D, screenWidth, screenHeight);
    float t = (std::fabs(ray.direction.y) > 1e-5f) ? (-ray.position.y / ray.direction.y) : 0.0f;
    return { ray.position.x + ray.direction.x * t,
             ray.position.z + ray.direction.z * t };
}

void Game::drawProceduralEntity3D(Vector2 pos, float heightOffset, std::function<void()> drawFunc) {
    // 1. Temporarily exit 3D mode and gameTarget FBO
    EndMode3D();
    EndTextureMode();

    // 2. Render entity to temp target
    g_renderPass3D = true;
    BeginTextureMode(tempEntityTarget);
    ClearBackground(BLANK);

    Camera2D entityCam = { 0 };
    entityCam.target = pos;
    entityCam.offset = { 64.0f, 64.0f };
    entityCam.rotation = 0.0f;
    entityCam.zoom = 1.0f;

    BeginMode2D(entityCam);
    drawFunc();
    EndMode2D();
    EndTextureMode(); // resets FBO to screen
    g_renderPass3D = false;

    // 3. Re-enter gameTarget FBO and 3D mode
    BeginTextureMode(gameTarget);
    rlSetClipPlanes(10.0, 4000.0);
    BeginMode3D(camera3D);

    // 4. Draw billboard in 3D space
    Rectangle source = { 0.0f, 0.0f, (float)tempEntityTarget.texture.width, -(float)tempEntityTarget.texture.height };
    Vector3 pos3D = { pos.x, 44.0f, pos.y }; Vector3 upv = { 0.0f, 1.0f, 0.0f }; Vector2 org = { 0.0f, 0.0f };
    Vector2 size = { 120.0f, 120.0f };
    DrawBillboardPro(camera3D, tempEntityTarget.texture, source, pos3D, upv, size, org, 0.0f, WHITE);
}

void Game::ensureVoxel(int key, Vector2 capPos, std::function<void()> drawFn) {
    if (m_voxModels.count(key)) return;
    g_renderPass3D = true;
    Image img = SpriteExtrude::CaptureToImage(96, capPos, drawFn);
    g_renderPass3D = false;
    m_voxModels[key] = SpriteExtrude::BuildVoxelModel(img, 1.6f, 6.0f);
    UnloadImage(img);
}

void Game::drawVoxel(int key, Vector2 pos, float rotDeg) {
    auto it = m_voxModels.find(key);
    if (it == m_voxModels.end() || it->second.meshCount == 0) return;
    Model& mdl = it->second;

    // SOMBRA REAL: projeta a SILHUETA do modelo no chão, deslocada pela DIREÇÃO da
    // luz (matriz de projeção em y=0). Não é disco — tem o formato do personagem.
    const Vector3 L = { -0.42f, -1.0f, -0.30f };       // direção da luz (de cima/frente)
    Matrix saved = mdl.transform;
    Matrix sm = MatrixIdentity();
    sm.m4 = -L.x / L.y;   // x deslocado pela altura (alonga na direção oposta à luz)
    sm.m5 = 0.0f;         // achata a altura (projeta no chão)
    sm.m6 = -L.z / L.y;   // z deslocado pela altura
    mdl.transform = sm;
    rlDisableDepthMask();                               // evita z-fight da silhueta
    DrawModel(mdl, { pos.x, 0.07f, pos.y }, 1.0f, ColorAlpha(BLACK, 0.42f));
    rlEnableDepthMask();
    mdl.transform = saved;

    // Modelo 3D real (com leve bob de vida)
    float bob = sinf((float)GetTime() * 2.4f + pos.x * 0.05f) * 1.2f;
    DrawModelEx(mdl, { pos.x, bob, pos.y }, { 0.0f, 1.0f, 0.0f }, rotDeg, { 1.0f, 1.0f, 1.0f }, WHITE);
}

void Game::renderWorld3D() {
    // PRE-PASS (sem FBO ativo): captura/voxeliza o sprite 2D em MODELO 3D real, por tipo.
    ensureVoxel((int)player.charClass, player.position, [this](){ player.render(); });
    for (auto& e : enemies)    ensureVoxel(100 + (int)e.type, e.position, [&e](){ e.render(); });
    for (auto& n : npcs)       ensureVoxel(300 + (int)n.role, n.position, [&n](){ n.render(); });
    for (auto& c : companions) if (c.active) ensureVoxel(500 + (int)c.type, c.position, [&c](){ c.render(); });

    // Prepare light mask before drawing (uses screen-space projection of 3D lights)
    lightSystem.prepareMask3D(camera3D, screenWidth, screenHeight);

    BeginTextureMode(gameTarget);
    ClearBackground(Color{10, 12, 20, 255});

    // ── 1. Modo 3D: Chão, Paredes, Sombras e Entidades (Billboards) ───────────
    rlSetClipPlanes(10.0, 4000.0);
    BeginMode3D(camera3D);
        // Render do mapa 3D
        tilemap.render3D(camera.target);

        // Decalques de chão em 3D (sangue/queimado)
        for (const auto& d : decals) {
            if (std::fabs(d.pos.x - camera.target.x) > 1000 || std::fabs(d.pos.y - camera.target.y) > 650) continue;
            float a = (d.life / d.maxLife);
            if (d.type == 0) { // sangue
                DrawPlane({ d.pos.x, 0.12f, d.pos.y }, { d.size * 2.0f, d.size * 1.2f }, ColorAlpha(d.color, 0.45f * a));
                DrawPlane({ d.pos.x - d.size * 0.8f, 0.12f, d.pos.y + 4.0f }, { d.size * 0.7f, d.size * 0.7f }, ColorAlpha(d.color, 0.40f * a));
                DrawPlane({ d.pos.x + d.size * 1.0f, 0.12f, d.pos.y - 2.0f }, { d.size * 0.6f, d.size * 0.6f }, ColorAlpha(d.color, 0.35f * a));
            } else { // queimado
                DrawPlane({ d.pos.x, 0.12f, d.pos.y }, { d.size * 1.4f, d.size * 1.4f }, ColorAlpha(Color{20,18,16,255}, 0.5f * a));
                DrawPlane({ d.pos.x, 0.13f, d.pos.y }, { d.size * 1.5f, d.size * 1.5f }, ColorAlpha(Color{255,120,30,255}, 0.2f * a));
            }
        }

        // Sombras e luzes de poste do cenário (owDecor) + billboards 3D reais
        if (openWorldMode && owDecorBuilt) {
            SpriteBank& sb = SpriteBank::get();
            float time = (float)GetTime();
            for (const auto& obj : owDecor.scenery) {
                float dx = obj.position.x - camera.target.x;
                float dy = obj.position.y - camera.target.y;
                if (dx < -1400 || dx > 1400 || dy < -1400 || dy > 1400) continue;

                float w = 64.0f, h = 64.0f;
                bool hasSprite = (sb.ready && obj.type >= 0 && obj.type < SpriteBank::NUM_SCENERY);

                // Estruturas grandes = MODELOS 3D REAIS (não billboard 2.5D).
                Model* mdl = nullptr; float mscale = 40.0f;
                switch (obj.type) {
                    case 0: mdl = &m_houseModel;    mscale = m_houseScale; break; // casa
                    case 1: mdl = &m_barracksModel; mscale = m_barracksScale; break; // celeiro
                    case 7: mdl = &m_castleModel;   mscale = m_castleScale; break; // predio
                    case 8: mdl = &m_wellModel;     mscale = m_wellScale; break; // silo
                    case 6: mdl = &m_carModel;      mscale = m_carScale; break; // carro
                    default: break;
                }
                if (mdl && m_modelsLoaded && mdl->meshCount > 0) {
                    float s = mscale * (obj.scale > 0.01f ? obj.scale : 1.0f);
                    DrawModelEx(*mdl, { obj.position.x, 0.0f, obj.position.y }, { 0.0f, 1.0f, 0.0f },
                                obj.rotation * RAD2DEG, { s, s, s }, WHITE);
                    w = s; h = s;
                } else {
                    // Props do cenario em PRIMITIVAS 3D (sem billboard "tabua de pe").
                    float sc = (obj.scale > 0.01f ? obj.scale : 1.0f);
                    float x = obj.position.x, zz = obj.position.y;
                    float H = 78.0f * sc, rr = 17.0f * sc;
                    w = rr * 2.0f; h = H;
                    switch (obj.type) {
                        case 2: { // arvore: tronco conico + copa de esferas
                            DrawCylinderEx({x,0,zz},{x,H*0.5f,zz}, rr*0.30f, rr*0.18f, 7, {78,54,30,255});
                            DrawSphereEx({x, H*0.74f, zz}, rr*0.98f, 8, 8, {34,80,44,255});
                            DrawSphereEx({x-rr*0.45f, H*0.58f, zz}, rr*0.66f, 8, 8, {26,62,36,255});
                            DrawSphereEx({x+rr*0.45f, H*0.62f, zz+rr*0.2f}, rr*0.70f, 8, 8, {42,92,50,255});
                        } break;
                        case 3: { // lapide: laje + topo curvo + base
                            DrawCubeV({x, H*0.32f, zz}, {rr*1.1f, H*0.55f, rr*0.35f}, {120,122,130,255});
                            DrawSphereEx({x, H*0.58f, zz}, rr*0.55f, 8, 8, {120,122,130,255});
                            DrawCubeV({x, H*0.06f, zz}, {rr*1.4f, H*0.12f, rr*0.6f}, {92,92,98,255});
                            h = H*0.64f;
                        } break;
                        case 4: { // cerca: 2 postes + travessa (orientada por rot)
                            float c=cosf(obj.rotation), s2=sinf(obj.rotation), L=rr*1.6f;
                            DrawCylinderEx({x-c*L,0,zz-s2*L},{x-c*L,H*0.5f,zz-s2*L}, rr*0.12f, rr*0.12f, 6, {70,52,34,255});
                            DrawCylinderEx({x+c*L,0,zz+s2*L},{x+c*L,H*0.5f,zz+s2*L}, rr*0.12f, rr*0.12f, 6, {70,52,34,255});
                            DrawCubeV({x, H*0.40f, zz}, {L*2.0f, rr*0.18f, rr*0.14f}, {84,62,40,255});
                            h = H*0.5f;
                        } break;
                        case 5: { // poste de luz: haste + luminaria emissiva
                            DrawCylinderEx({x,0,zz},{x,H,zz}, rr*0.12f, rr*0.09f, 6, {46,46,54,255});
                            DrawSphereEx({x, H*0.97f, zz}, rr*0.28f, 8, 8, {255,224,150,255});
                        } break;
                        case 9: { // arco/catacumba: 2 pilares + lintel
                            float c=cosf(obj.rotation), s2=sinf(obj.rotation), L=rr*1.2f;
                            DrawCubeV({x-c*L,H*0.45f,zz-s2*L},{rr*0.5f,H*0.9f,rr*0.5f},{96,90,80,255});
                            DrawCubeV({x+c*L,H*0.45f,zz+s2*L},{rr*0.5f,H*0.9f,rr*0.5f},{96,90,80,255});
                            DrawCubeV({x,H*0.92f,zz},{L*2.4f,rr*0.5f,rr*0.6f},{104,98,86,255});
                        } break;
                        case 10: { // estatua: pedestal + figura low-poly de pedra
                            DrawCubeV({x, H*0.10f, zz}, {rr*1.3f, H*0.2f, rr*1.3f}, {108,108,116,255});
                            DrawCapsule({x, H*0.25f, zz}, {x, H*0.74f, zz}, rr*0.45f, 8, 8, {150,150,158,255});
                            DrawSphereEx({x, H*0.84f, zz}, rr*0.42f, 8, 8, {150,150,158,255});
                        } break;
                        case 11: { // grama: lâminas finas verdes com balanço de vento
                            float gh = 20.0f * sc, gr = 11.0f * sc;
                            float sway = sinf((float)GetTime() * 1.8f + x * 0.06f) * gh * 0.35f;
                            Color gc = { 70, 150, 60, 255 };
                            for (int bld = 0; bld < 5; ++bld) {
                                float a = bld * 1.2566f;
                                float ox = cosf(a) * gr * 0.4f, oz = sinf(a) * gr * 0.4f;
                                DrawCylinderEx({ x+ox, 0, zz+oz }, { x+ox+sway, gh, zz+oz }, gr*0.10f, gr*0.02f, 4, gc);
                            }
                            w = gr; h = gh;
                        } break;
                        case 12: { // pedras/detritos: cluster baixo de esferas cinza
                            float pr = 9.0f * sc;
                            DrawSphereEx({ x, pr*0.5f, zz }, pr*0.6f, 6, 6, { 110,108,104,255 });
                            DrawSphereEx({ x+pr*0.5f, pr*0.35f, zz+pr*0.3f }, pr*0.4f, 6, 6, { 95,93,90,255 });
                            w = pr; h = pr;
                        } break;
                        default: if (hasSprite) { // fallback billboard so p/ tipos sem 3D
                            int variant = ((int)(obj.position.x*0.13f+obj.position.y*0.07f)) % SpriteBank::SCENERY_VARIANTS;
                            if (variant<0) variant+=SpriteBank::SCENERY_VARIANTS;
                            Texture2D tx=sb.scenery[obj.type][variant]; float K=1.7f*sc;
                            w=tx.width*K; h=tx.height*K;
                            Rectangle src={0.0f,0.0f,(float)tx.width,-(float)tx.height};
                            DrawBillboardRec(camera3D, tx, src, {x,h*0.5f,zz}, {w,h}, WHITE);
                        } break;
                    }
                }

                // Desenha plano horizontal de sombra
                DrawPlane({ obj.position.x, 0.11f, obj.position.y }, { w * 0.88f, h * 0.17f }, ColorAlpha(BLACK, 0.40f));

                // Poste de luz: cone/poça de luz amarela no chão
                bool lights = (obj.tint.r > 128);
                if (lights && obj.type == 5) {
                    float fl = std::sin(time * 7.3f + obj.position.x) * 0.5f + std::sin(time * 2.1f + obj.position.y) * 0.5f;
                    float pulse = 0.55f + 0.30f * fl;
                    if (pulse < 0.2f) pulse = 0.2f;
                    DrawPlane({ obj.position.x, 0.14f, obj.position.y }, { w * 1.0f, h * 0.20f }, ColorAlpha(Color{255,210,130,255}, 0.07f * pulse));
                }
            }
        }

        // ── Sombras REDONDAS suaves no chão (disco achatado, não retângulo) ──
        auto shadow = [](Vector2 pos, float r, float a) {
            DrawCylinderEx({ pos.x, 0.10f, pos.y }, { pos.x, 0.118f, pos.y }, r, r, 16, ColorAlpha(BLACK, a));
        };
        // player/inimigos/NPCs/companheiros têm SOMBRA PROJETADA (silhueta) no drawVoxel
        if (netActive)
            for (const auto& p : net.peers()) shadow({ p.x, p.y }, 11.0f, 0.34f);

        for (auto& it : items) shadow(it.position, 5.5f, 0.26f);

        for (const auto& a : animals) {
            if (std::fabs(a.position.x - camera.target.x) > 1100 || std::fabs(a.position.y - camera.target.y) > 700) continue;
            shadow(a.position, 6.0f, 0.26f);
        }

        // Sombras dos nós de recursos
        for (const auto& n : resourceNodes) {
            if (n.depleted) continue;
            if (std::fabs(n.position.x - camera.target.x) > 1100 || std::fabs(n.position.y - camera.target.y) > 700) continue;
            DrawPlane({ n.position.x, 0.1f, n.position.y }, { 24.0f, 12.0f }, ColorAlpha(BLACK, 0.35f));
        }

        // Sombras dos equipamentos no chão
        for (const auto& ge : groundEquips) {
            if (ge.collected) continue;
            if (std::fabs(ge.position.x - camera.target.x) > 1100 || std::fabs(ge.position.y - camera.target.y) > 700) continue;
            DrawPlane({ ge.position.x, 0.1f, ge.position.y }, { 16.0f, 8.0f }, ColorAlpha(BLACK, 0.3f));
        }

        // Sombras das construções
        for (const auto& b : buildingSystem.buildings) {
            if (!b.built) continue;
            DrawPlane({ b.position.x, 0.1f, b.position.y }, { 80.0f, 40.0f }, ColorAlpha(BLACK, 0.35f));
        }


        // ── Desenho dos Billboards 3D Reais (com oclusão e depth buffer) ──

        // Player — modelo 3D do PRÓPRIO personagem do jogo (render3D, não genérico)
        drawVoxel((int)player.charClass, player.position, 0.0f);

        // NPCs — modelos 3D próprios do jogo
        for (auto& n : npcs) {
            drawVoxel(300 + (int)n.role, n.position, 0.0f);
        }

        // Companheiros — modelos 3D próprios do jogo
        for (auto& c : companions) {
            if (c.active) drawVoxel(500 + (int)c.type, c.position, 0.0f);
        }

        // Inimigos — modelos 3D próprios do jogo (cada tipo com sua silhueta)
        for (auto& e : enemies) {
            drawVoxel(100 + (int)e.type, e.position, 0.0f);
        }

        // Itens — modelos 3D próprios do jogo
        for (auto& it : items) {
            it.render3D();
        }

        // Outros jogadores (Peers)
        if (netActive) {
            static const Color cols[6] = {
                {60,120,220,255},{220,80,140,255},{150,160,175,255},
                {120,80,220,255},{180,120,255,255},{200,130,60,255}
            };
            for (const auto& p : net.peers()) {
                drawProceduralEntity3D({ p.x, p.y }, 18.0f, [&, p]() {
                    Color c = cols[(p.charClass >= 0 && p.charClass < 6) ? p.charClass : 0];
                    DrawRectangle((int)p.x - 9, (int)p.y - 14, 18, 28, c);
                    DrawCircle((int)p.x, (int)(p.y - 20), 9.0f, c);
                    DrawCircleLines((int)p.x, (int)(p.y - 20), 9.0f, ColorAlpha(WHITE, 0.4f));
                });
            }
        }

        // Animais / Vida Selvagem
        for (const auto& a : animals) {
            if (std::fabs(a.position.x - camera.target.x) > 1100 || std::fabs(a.position.y - camera.target.y) > 700) continue;
            drawProceduralEntity3D(a.position, 10.0f, [&, a]() {
                float x = a.position.x, y = a.position.y;
                float bob = std::sin(a.animTimer * 8.0f) * 1.5f;
                switch (a.type) {
                    case AnimalType::Deer: {
                        Color body={150,110,70,255};
                        DrawEllipse((int)x,(int)(y+bob),12.0f,8.0f,body);
                        DrawCircleV({x+9,y-6+bob},5.0f,body);
                        DrawLine((int)x+9,(int)(y-10+bob),(int)x+6,(int)(y-16+bob),Color{90,60,30,255});
                        DrawLine((int)x+11,(int)(y-10+bob),(int)x+14,(int)(y-16+bob),Color{90,60,30,255});
                        DrawRectangle((int)x-8,(int)(y+6),2,8,body); DrawRectangle((int)x+6,(int)(y+6),2,8,body);
                        break;
                    }
                    case AnimalType::Rabbit: {
                        Color body={210,200,190,255};
                        DrawCircleV({x,y+bob},6.0f,body);
                        DrawEllipse((int)(x-2),(int)(y-8+bob),2.0f,5.0f,body);
                        DrawEllipse((int)(x+2),(int)(y-8+bob),2.0f,5.0f,body);
                        break;
                    }
                    case AnimalType::Boar: {
                        Color body={90,70,60,255};
                        DrawEllipse((int)x,(int)(y+bob),13.0f,8.0f,body);
                        DrawCircleV({x+10,y+bob},5.0f,body);
                        DrawCircleV({x+13,y+bob},2.0f,Color{40,30,25,255});
                        break;
                    }
                    case AnimalType::Wolf: {
                        Color body=a.fleeing?Color{120,120,130,255}:Color{90,95,105,255};
                        DrawEllipse((int)x,(int)(y+bob),12.0f,7.0f,body);
                        DrawCircleV({x+9,y-3+bob},5.0f,body);
                        DrawLine((int)x+7,(int)(y-7+bob),(int)x+6,(int)(y-11+bob),body);
                        DrawLine((int)x+11,(int)(y-7+bob),(int)x+12,(int)(y-11+bob),body);
                        DrawCircleV({x+11,y-3+bob},1.5f,Color{255,200,0,255});
                        break;
                    }
                    default: { // Bird
                        float fl = std::sin(a.animTimer*12.0f)*4.0f;
                        Color body={60,60,70,255};
                        DrawCircleV({x,y-20+bob*2},3.0f,body);
                        DrawLine((int)x,(int)(y-20+bob*2),(int)(x-6),(int)(y-20-fl+bob*2),body);
                        DrawLine((int)x,(int)(y-20+bob*2),(int)(x+6),(int)(y-20-fl+bob*2),body);
                        break;
                    }
                }
            });
        }

        // Nós de Recursos Naturais
        for (int i = 0; i < (int)resourceNodes.size(); ++i) {
            const auto& n = resourceNodes[i];
            if (n.depleted) continue;
            if (std::fabs(n.position.x - camera.target.x) > 1100 || std::fabs(n.position.y - camera.target.y) > 700) continue;

            drawProceduralEntity3D(n.position, 14.0f, [&, i, n]() {
                float sx = (n.shake > 0.0f) ? std::sin(n.shake * 30.0f) * 2.0f : 0.0f;
                float x = n.position.x + sx, y = n.position.y;
                Color c = resourceColor(n.type);
                switch (n.type) {
                    case ResourceType::Wood: {
                        DrawRectangle((int)(x-4), (int)(y-6), 8, 22, Color{90,60,30,255});
                        DrawCircleV({x, y-22}, 18.0f, Color{30,90,40,255});
                        DrawCircleV({x-10, y-14}, 12.0f, Color{36,100,46,255});
                        DrawCircleV({x+10, y-14}, 12.0f, Color{28,84,38,255});
                        break;
                    }
                    case ResourceType::Stone: {
                        DrawCircleV({x, y}, 15.0f, Color{120,120,128,255});
                        DrawCircleV({x-6, y+2}, 9.0f, Color{145,145,155,255});
                        DrawCircleV({x+7, y-1}, 8.0f, Color{100,100,110,255});
                        break;
                    }
                    default: {
                        DrawCircleV({x, y}, 15.0f, Color{80,72,66,255});
                        DrawCircleV({x-5, y+2}, 8.0f, Color{96,88,80,255});
                        for (int v = 0; v < 5; ++v) {
                            float a = v * 1.2f + i;
                            DrawCircleV({x + std::cos(a)*7.0f, y + std::sin(a)*7.0f}, 2.6f, c);
                        }
                        break;
                    }
                }
            });
        }

        // Equipamentos no chão
        for (const auto& ge : groundEquips) {
            if (ge.collected) continue;
            if (std::fabs(ge.position.x - camera.target.x) > 1100 || std::fabs(ge.position.y - camera.target.y) > 700) continue;

            drawProceduralEntity3D(ge.position, 8.0f, [&, ge]() {
                float pulse = 0.5f + 0.5f * std::sin(ge.pulseTimer * 4.0f);
                Color ec = ge.equip.color;
                float fade = (ge.lifetime < 5.0f) ? ge.lifetime / 5.0f : 1.0f;
                DrawCircleV(ge.position, 22.0f + pulse * 6.0f, ColorAlpha(ec, 0.18f * fade));
                DrawCircleV(ge.position, 16.0f + pulse * 4.0f, ColorAlpha(ec, 0.28f * fade));
                DrawCircleLines((int)ge.position.x, (int)ge.position.y, 18.0f + pulse * 4.0f, ColorAlpha(ec, 0.65f * fade));
                for (int s = 0; s < 2; ++s) {
                    float a = ge.pulseTimer * 3.5f + s * 3.14159f;
                    DrawCircleV({ge.position.x + std::cos(a) * 16.0f, ge.position.y + std::sin(a) * 16.0f}, 3.0f, ColorAlpha(WHITE, 0.85f * fade));
                }
                DrawCircleV(ge.position, 10.0f, ColorAlpha(ec, fade));
                DrawCircleV(ge.position, 5.0f, ColorAlpha(WHITE, 0.7f * fade));
            });
        }

        // Construções, Tanques e Soldados (Building System / RTS)
        for (const auto& b : buildingSystem.buildings) {
            if (m_modelsLoaded && b.built) {
                if (b.type == BuildingType::Ark && m_castleModel.meshCount > 0) {
                    DrawModelEx(m_castleModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_castleScale, m_castleScale, m_castleScale }, WHITE);
                } else if (b.type == BuildingType::House && m_houseModel.meshCount > 0) {
                    DrawModelEx(m_houseModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_houseScale, m_houseScale, m_houseScale }, WHITE);
                } else if (b.type == BuildingType::Barracks && m_barracksModel.meshCount > 0) {
                    DrawModelEx(m_barracksModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_barracksScale, m_barracksScale, m_barracksScale }, WHITE);
                } else if (b.type == BuildingType::Turret && m_turretModel.meshCount > 0) {
                    DrawModelEx(m_turretModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_turretScale, m_turretScale, m_turretScale }, WHITE);
                } else if (b.type == BuildingType::TankFactory && m_marketModel.meshCount > 0) {
                    DrawModelEx(m_marketModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_marketScale, m_marketScale, m_marketScale }, WHITE);
                } else if (b.type == BuildingType::MedBay && m_wellModel.meshCount > 0) {
                    DrawModelEx(m_wellModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_wellScale, m_wellScale, m_wellScale }, WHITE);
                } else if (b.type == BuildingType::Wall) {
                    SpriteBank& sb = SpriteBank::get();
                    if (sb.ready) {
                        DrawCubeTexture(sb.tileWall[(int)currentZone], { b.position.x, 32.0f, b.position.y }, 64.0f, 64.0f, 64.0f, WHITE);
                    } else {
                        DrawCube({ b.position.x, 32.0f, b.position.y }, 64.0f, 64.0f, 64.0f, GRAY);
                    }
                } else {
                    drawProceduralEntity3D(b.position, 24.0f, [&, b]() {
                        buildingSystem.renderBuilding(b);
                    });
                }
            } else {
                drawProceduralEntity3D(b.position, 24.0f, [&, b]() {
                    buildingSystem.renderBuilding(b);
                });
            }
        }
        for (const auto& t : buildingSystem.tanks) {
            if (t.isDead()) continue;
            drawProceduralEntity3D(t.position, 12.0f, [&, t]() {
                t.render();
            });
        }
        for (const auto& s : buildingSystem.soldiers) {
            if (s.isDead()) continue;
            drawProceduralEntity3D(s.position, 12.0f, [&, s]() {
                s.render();
            });
        }

        // RTS Building Preview Ghost
        if (buildingSystem.buildModeActive) {
            Vector2 mouseWorld = mouseGround3D();
            drawProceduralEntity3D(mouseWorld, 16.0f, [&]() {
                BuildingType preview = static_cast<BuildingType>(buildingSystem.selectedType);
                Color previewCol = {0, 255, 180, 100};
                bool canPlace = true;
                for (const auto& b : buildingSystem.buildings) {
                    if (Vector2Distance(b.position, mouseWorld) < 80.f) { canPlace = false; break; }
                }
                previewCol = canPlace ? Color{0, 255, 100, 80} : Color{255, 50, 50, 80};
                DrawRectangle((int)(mouseWorld.x - 32), (int)(mouseWorld.y - 32), 64, 64, previewCol);
                DrawRectangleLinesEx({mouseWorld.x - 32, mouseWorld.y - 32, 64, 64},
                                     2.f, canPlace ? Color{0, 255, 100, 200} : Color{255, 50, 50, 200});
            });
        }

        // ── VIDA AMBIENTE: partículas flutuando (poeira/brasas/pólen) por TEMA ──
        {
            float t = (float)GetTime();
            Color mc;
            switch (currentZone) {
                case ZoneID::InfernoZone: case ZoneID::KronosForge:
                    mc = {255,150,60,255}; break;                                  // brasas
                case ZoneID::Cemetery: case ZoneID::GhostCity: case ZoneID::AbandonedManor:
                    mc = {180,200,235,255}; break;                                 // névoa fria
                case ZoneID::DarkForest: case ZoneID::CursedFarm:
                    mc = {170,235,150,255}; break;                                 // pólen/vaga-lumes
                default:
                    mc = {255,215,160,255}; break;                                 // poeira dourada
            }
            const float RANGE = 720.0f;
            for (int i = 0; i < 120; ++i) {
                float hx = sinf(i * 12.9898f) * 43758.5453f; hx -= floorf(hx);
                float hz = sinf(i * 78.233f)  * 43758.5453f; hz -= floorf(hz);
                float hy = sinf(i * 37.719f)  * 43758.5453f; hy -= floorf(hy);
                float px = player.position.x + (hx - 0.5f) * 2.0f * RANGE + sinf(t * 0.25f + i) * 28.0f;
                float pz = player.position.y + (hz - 0.5f) * 2.0f * RANGE + cosf(t * 0.22f + i * 1.7f) * 28.0f;
                float py = 14.0f + hy * 160.0f + sinf(t * 0.6f + i * 1.3f) * 14.0f;
                DrawSphereEx({ px, py, pz }, 1.1f + hy * 1.3f, 4, 4, mc);
            }
        }

    EndMode3D();

    // ── 2. Overlay 2D Projetado: Projéteis, Partículas, Nomes e UI ────────────
    auto proj = [&](Vector2 w, float h) {
        return GetWorldToScreenEx({ w.x, h, w.y }, camera3D, screenWidth, screenHeight);
    };

    // ── RTS Unit Selection Indicators in 2D projected space ──
    for (const auto& t : buildingSystem.tanks) {
        if (!t.selected || t.isDead()) continue;
        Vector2 s = proj(t.position, 0.0f);
        DrawEllipseLines((int)s.x, (int)s.y, 22, 10, Color{0,255,80,255});
        if (t.hasMoveOrder) {
            Vector2 mS = proj(t.moveOrder, 0.0f);
            DrawLineEx(s, mS, 1.0f, ColorAlpha(Color{0,255,80,255}, 0.35f));
        }
    }
    for (const auto& s : buildingSystem.soldiers) {
        if (!s.selected || s.isDead()) continue;
        Vector2 feetS = proj(s.position, 0.0f);
        DrawEllipseLines((int)feetS.x, (int)feetS.y, 14, 7, Color{0,255,80,255});
        if (s.hasMoveOrder) {
            Vector2 mS = proj(s.moveOrder, 0.0f);
            DrawLineEx(feetS, mS, 1.0f, ColorAlpha(Color{0,255,80,255}, 0.35f));
        }
    }

    // ── Prompts de Produção dos Prédios RTS ──
    for (const auto& b : buildingSystem.buildings) {
        if (!b.built) continue;
        bool isFactory  = (b.type == BuildingType::TankFactory);
        bool isBarracks = (b.type == BuildingType::Barracks);
        if (!isFactory && !isBarracks) continue;

        int n   = isFactory ? (int)buildingSystem.tanks.size()    : (int)buildingSystem.soldiers.size();
        int cap = isFactory ? 8                      : 12;
        int cost= isFactory ? 40                     : 20;
        const char* unit = isFactory ? "Tanque" : "Soldado";

        Vector2 s = proj(b.position, 56.0f); // altura 56
        const char* lbl = TextFormat("[CLIQUE] %s  $%d   %d/%d", unit, cost, n, cap);
        int tw = MeasureText(lbl, 11);
        DrawRectangle((int)(s.x - tw/2 - 4), (int)(s.y - 2), tw + 8, 16, ColorAlpha(BLACK, 0.7f));
        DrawRectangleLines((int)(s.x - tw/2 - 4), (int)(s.y - 2), tw + 8, 16, ColorAlpha(Color{120,200,255,255}, 0.7f));
        DrawText(lbl, (int)(s.x - tw/2), (int)s.y, 11, Color{180,220,255,255});

        // Barra de producao automatica
        float pct = b.productionTimer / b.productionRate;
        DrawRectangle((int)(s.x - 24), (int)(s.y + 16), 48, 4, ColorAlpha(BLACK, 0.6f));
        DrawRectangle((int)(s.x - 24), (int)(s.y + 16), (int)(48 * pct), 4, Color{255,200,0,255});
    }

    // ── Prédios RTS Info (Level up e evolução) ──
    for (const auto& b : buildingSystem.buildings) {
        if (!b.built) continue;
        const char* name = BuildingSystem::COSTS[(int)b.type].name;
        // Badge de nivel
        const char* lvTxt = TextFormat("Lv%d", b.level);
        Vector2 s = proj(b.position, 0.0f);
        DrawText(lvTxt, (int)(s.x + 16), (int)(s.y - 38), 11,
                 b.level >= Building::MAX_LEVEL ? Color{255,215,0,255} : Color{120,220,255,255});

        // Painel completo se perto do jogador
        if (Vector2Distance(player.position, b.position) <= 120.f) {
            Vector2 pS = proj(b.position, 0.0f);
            int px = (int)pS.x;
            int py = (int)pS.y - 92;
            int pw = 230, ph = 64;
            DrawRectangle(px - pw/2, py, pw, ph, ColorAlpha(BLACK, 0.8f));
            DrawRectangleLinesEx({(float)(px-pw/2),(float)py,(float)pw,(float)ph}, 1.0f,
                                 ColorAlpha(Color{0,200,255,255}, 0.7f));
            DrawText(TextFormat("%s  [Lv %d/%d]", name, b.level, Building::MAX_LEVEL),
                     px - pw/2 + 6, py + 4, 12, Color{0,220,255,255});
            DrawText(BuildingSystem::COSTS[(int)b.type].desc, px - pw/2 + 6, py + 20, 9, Color{200,200,210,255});
            if (b.level < Building::MAX_LEVEL) {
                DrawText(TextFormat("[U] Evoluir Lv%d->Lv%d  ($%d)",
                         b.level, b.level+1, buildingSystem.upgradeCostFor(b)),
                         px - pw/2 + 6, py + 44, 11, Color{255,215,0,255});
            } else {
                DrawText("NIVEL MAXIMO", px - pw/2 + 6, py + 44, 11, Color{255,215,0,255});
            }
        }
    }

    // ── HUD/Indicador do nó de recurso mais próximo ──
    if (nearResourceIdx >= 0 && nearResourceIdx < (int)resourceNodes.size()) {
        const auto& n = resourceNodes[nearResourceIdx];
        if (!n.depleted) {
            Vector2 s = proj(n.position, 0.0f);
            Color c = resourceColor(n.type);
            DrawCircleLines((int)s.x, (int)s.y, 22.0f, ColorAlpha(c, 0.8f));
            const char* lbl = TextFormat("[H] Coletar %s (%d)", resourceName(n.type), n.amount);
            int w = MeasureText(lbl, 11);
            DrawRectangle((int)s.x - w/2 - 4, (int)s.y - 46, w + 8, 16, ColorAlpha(BLACK, 0.7f));
            DrawText(lbl, (int)s.x - w/2, (int)s.y - 44, 11, c);
            if (n.harvestProg > 0.0f) {
                DrawRectangle((int)s.x - 20, (int)s.y - 28, 40, 5, ColorAlpha(BLACK, 0.6f));
                DrawRectangle((int)s.x - 20, (int)s.y - 28, (int)(40 * n.harvestProg), 5, c);
            }
        }
    }

    // ── HUD de Equipamentos no chão ──
    for (const auto& ge : groundEquips) {
        if (ge.collected) continue;
        if (std::fabs(ge.position.x - camera.target.x) > 1100 || std::fabs(ge.position.y - camera.target.y) > 700) continue;
        float fade = (ge.lifetime < 5.0f) ? ge.lifetime / 5.0f : 1.0f;
        Vector2 s = proj(ge.position, 0.0f);
        // Name label
        const char* eName = ge.equip.name.c_str();
        int ew = MeasureText(eName, 11);
        DrawText(eName, (int)(s.x - ew/2), (int)(s.y - 30), 11, ColorAlpha(ge.equip.color, fade));
        // E-prompt when player nearby
        float dist = Vector2Distance(player.position, ge.position);
        if (dist < 60.0f) {
            const char* pr = "[E] EQUIPAR";
            int pw = MeasureText(pr, 12);
            DrawRectangle((int)(s.x - pw/2 - 4), (int)(s.y - 50), pw + 8, 18, ColorAlpha(BLACK, 0.7f));
            DrawText(pr, (int)(s.x - pw/2), (int)(s.y - 48), 12, ColorAlpha({255,210,0,255}, 1.0f));
        }
    }

    // Feixes de luz vertical e partículas de itens (estilo Diablo)
    for (auto& item : items) {
        if (item.dropBeamTimer > 0.0f && item.rarity >= ItemRarity::Uncommon) {
            float beamH = 200.0f + (int)item.rarity * 80.0f;
            Vector2 botS = proj(item.position, 0.0f);
            Vector2 topS = proj(item.position, beamH);

            DrawLineEx(botS, topS, 4.0f + (int)item.rarity * 2.0f, ColorAlpha(item.rarityColor, 0.35f));

            // Partículas subindo no feixe
            float t = (float)GetTime();
            for (int p = 0; p < 3 + (int)item.rarity * 2; ++p) {
                float py = botS.y - std::fmod(t * 55.0f + p * 38.0f, botS.y - topS.y);
                float px = botS.x + std::sin(t * 2.0f + p * 1.2f) * (6.0f + (int)item.rarity * 4.0f);
                DrawCircleV({ px, py }, 2.0f, ColorAlpha(item.rarityColor, 0.6f));
            }
        }
    }

    // Partículas
    for (const auto& p : particles.particles) {
        if (!p.active) continue;
        Vector2 s = proj(p.position, 8.0f);
        Particle tempP = p;
        tempP.position = s;
        tempP.render();
    }

    // Projéteis do player
    for (auto& p : projectiles) {
        Vector2 s = proj(p.position, 12.0f);
        Vector2 originalPos = p.position;
        p.position = s;
        p.render();
        p.position = originalPos;
    }

    // Projéteis dos inimigos
    for (auto& p : enemyProjectiles) {
        Vector2 s = proj(p.position, 12.0f);
        Vector2 originalPos = p.position;
        p.position = s;
        p.render();
        p.position = originalPos;
    }

    // Remote Players (Names and Online Indicators)
    if (netActive) {
        for (const auto& p : net.peers()) {
            Vector2 headS = proj({p.x, p.y}, 48.0f);
            int w = MeasureText(p.name, 11);
            DrawRectangle((int)headS.x - w/2 - 3, (int)headS.y - 12, w + 6, 14, ColorAlpha(BLACK, 0.6f));
            DrawText(p.name, (int)headS.x - w/2, (int)headS.y - 10, 11, ColorAlpha(WHITE, 0.95f));
            DrawCircle((int)headS.x + w/2 + 8, (int)headS.y - 5, 3.0f, Color{0,255,80,255}); // online dot
        }
    }

    // NPCs (Nomes e Tags de Quest)
    for (auto& n : npcs) {
        Vector2 feetS = proj(n.position, 0.0f);
        DrawText(n.name.c_str(), (int)feetS.x - (int)n.name.size() * 4, (int)feetS.y + 12, 14, WHITE);

        if (n.hasQuest) {
            Vector2 headS = proj(n.position, 48.0f);
            DrawText("!", (int)headS.x - 4, (int)headS.y - 12, 28, GOLD);
        } else if (n.hasNewDialogue && n.dialogues.size() > 0) {
            Vector2 headS = proj(n.position, 50.0f);
            DrawText("!", (int)headS.x + 6, (int)headS.y - 14, 20, Color{255, 220, 0, 255});
        }
    }

    // Inimigos (Barras de Vida e Nomes de Elite)
    for (auto& e : enemies) {
        Vector2 barPos = proj(e.position, e.radius + 16.0f);

        // Barra de HP
        float barW  = e.isBoss() ? 70.0f : (e.type == EnemyType::Tank ? 48.0f : 36.0f);
        float hpPct = e.health / e.maxHealth;
        Color hpCol = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
        DrawHealthBar(barPos, hpPct, barW, 5, hpCol);

        // Label de Tier
        if (e.evolTier > 0) {
            const char* tierLabel = e.evolTier == 1 ? "[VET]" : e.evolTier == 2 ? "[ELT]" : "[LND]";
            Color tierCol = e.evolTier == 1 ? Color{0,220,100,255} :
                            e.evolTier == 2 ? Color{100,180,255,255} : Color{255,160,0,255};
            int tw = MeasureText(tierLabel, 9);
            DrawText(tierLabel, (int)(barPos.x - tw/2), (int)(barPos.y - 11), 9, tierCol);
        }

        // Nome de Elite Mod
        if (e.isElite) {
            Color eliteCol;
            switch (e.eliteMod) {
                case 0:  eliteCol = {255, 60,  0,   255}; break;
                case 1:  eliteCol = {180, 180, 255, 255}; break;
                default: eliteCol = {255, 0,   200, 255}; break;
            }
            const char* tag = (e.eliteMod == 0) ? "BERSERK" : (e.eliteMod == 1) ? "BLINDADO" : "VOLATIL";
            DrawText(tag, (int)(barPos.x - MeasureText(tag, 10)/2), (int)(barPos.y - 21), 10, ColorAlpha(eliteCol, 0.9f));
        }
    }

    // Apply light mask overlay (darkens world except around light sources)
    lightSystem.applyMask();

    // Atmosfera Diablo: vignette nos cantos (escurece bordas)
    DrawVignette(screenWidth, screenHeight);

    // ── 3. Interface e HUD Final ─────────────────────────────────────────────
    drawUI();
    DrawText("MODO 2.5D (F10) - Incremento 3: depth sorting, billboards, sombras e iluminação 3D",
             12, screenHeight - 18, 11, ColorAlpha(Color{0,210,255,255}, 0.6f));

    EndTextureMode();
}

void Game::render() {
    // Caminho 2.5D isométrico (Incremento 1) — alternável por F10. Mantém o 2D
    // intacto como padrão até a migração amadurecer.
    if (render3D && !inMainMenu) { renderWorld3D(); return; }

    // Prepare light mask before drawing (uses own RenderTexture pass)
    lightSystem.prepareMask(camera);

    BeginTextureMode(gameTarget);
    ClearBackground(BLACK);

    // Sky gradient + skyline silhouette (screen-space, before world)
    background.drawSky(screenWidth, screenHeight, currentZone);

    // Storm overlay (screen-space — anomaly wave)
    anomalySystem.renderStorm(screenWidth, screenHeight);

    BeginMode2D(camera);

    // Parallax skyline buildings
    background.drawSkyline(camera.target, screenWidth, screenHeight, currentZone);

    tilemap.render(camera.target, camera.zoom);   // frustum culling (so tiles visiveis)

    // ZONA SEGURA — perimetro luminoso no chao (refugio sem inimigos)
    if (openWorldMode) {
        float tnow = (float)GetTime();
        float pulse = 0.5f + 0.5f * std::sin(tnow * 1.6f);
        Color zc = {0, 230, 160, 255};
        // Preenchimento sutil
        DrawCircleV(safeZoneCenter, safeZoneRadius, ColorAlpha(zc, 0.05f));
        // Aneis do perimetro
        DrawCircleLines((int)safeZoneCenter.x, (int)safeZoneCenter.y, safeZoneRadius,
                        ColorAlpha(zc, 0.35f + 0.25f * pulse));
        DrawCircleLines((int)safeZoneCenter.x, (int)safeZoneCenter.y, safeZoneRadius - 4.0f,
                        ColorAlpha(zc, 0.18f));
        // Marcadores girando no perimetro
        for (int i = 0; i < 12; ++i) {
            float a = tnow * 0.3f + i * (PI / 6.0f);
            Vector2 mk = { safeZoneCenter.x + std::cos(a) * safeZoneRadius,
                           safeZoneCenter.y + std::sin(a) * safeZoneRadius };
            DrawCircleV(mk, 3.0f, ColorAlpha(zc, 0.6f + 0.4f * pulse));
        }
        // Texto central da base
        const char* zlabel = "ZONA SEGURA - BASE";
        int zw = MeasureText(zlabel, 22);
        DrawText(zlabel, (int)(safeZoneCenter.x - zw/2), (int)(safeZoneCenter.y - safeZoneRadius - 30),
                 22, ColorAlpha(zc, 0.55f + 0.3f * pulse));
    }

    // Cenario PERSISTENTE do mundo aberto — casas, lapides, predios, etc. de TODAS
    // as regioes, sempre visiveis enquanto o jogador caminha (com culling por camera)
    if (openWorldMode && owDecorBuilt) {
        owDecor.renderScenery(camera.target, (float)GetTime());
    }

    // Nós de recursos naturais (árvores, pedras, minérios) — world-space
    renderDecals();                               // manchas de sangue/queimado no chão
    if (openWorldMode) renderResourceNodes();
    if (openWorldMode) renderAnimals();          // vida selvagem
    renderRemotePlayers();                        // outros jogadores na LAN

    // Dark world scenery (zonas isoladas fora do mundo aberto)
    if (!openWorldMode && darkWorld.active) {
        darkWorld.renderScenery(camera.target, camera.zoom);
    }

    // Inferno zone — lava pools on ground (drawn before entities)
    if (infernoZone.active) {
        infernoZone.renderGround(camera.target);
    }

    // Environment objects: cars, crates, fire pits, etc.
    background.drawEnvObjects();
    background.drawAsh();

    // ── Frustum culling de entidades: só renderiza o que está perto da câmera.
    // Evita desenhar milhares de itens/orbs/inimigos fora da tela (causa de FPS=1).
    float cullVX = camera.target.x, cullVY = camera.target.y;
    float cullHW = (screenWidth  * 0.5f) / camera.zoom + 96.0f;
    float cullHH = (screenHeight * 0.5f) / camera.zoom + 96.0f;
    auto inView = [&](Vector2 p) {
        return std::fabs(p.x - cullVX) <= cullHW && std::fabs(p.y - cullVY) <= cullHH;
    };

    for (auto& orb  : xpOrbs)  if (inView(orb.position))  orb.render();
    // Draw drop beams first (behind items) then items on top
    for (auto& item : items)   if (inView(item.position)) item.drawDropEffect();
    for (auto& item : items)   if (inView(item.position)) item.render();

    // Anomaly portals (world-space)
    anomalySystem.renderWorld();

    // Ground equipment drops — visible glowing item on floor
    for (const auto& ge : groundEquips) {
        if (ge.collected) continue;
        float pulse  = 0.5f + 0.5f * std::sin(ge.pulseTimer * 4.0f);
        Color ec     = ge.equip.color;
        float fade   = (ge.lifetime < 5.0f) ? ge.lifetime / 5.0f : 1.0f;
        // Outer glow ring
        DrawCircleV(ge.position, 22.0f + pulse * 6.0f, ColorAlpha(ec, 0.18f * fade));
        DrawCircleV(ge.position, 16.0f + pulse * 4.0f, ColorAlpha(ec, 0.28f * fade));
        DrawCircleLines((int)ge.position.x, (int)ge.position.y,
                        18.0f + pulse * 4.0f, ColorAlpha(ec, 0.65f * fade));
        // Two orbiting sparks
        for (int s = 0; s < 2; ++s) {
            float a = ge.pulseTimer * 3.5f + s * 3.14159f;
            DrawCircleV({ge.position.x + std::cos(a) * 16.0f,
                         ge.position.y + std::sin(a) * 16.0f}, 3.0f,
                        ColorAlpha(WHITE, 0.85f * fade));
        }
        // Core
        DrawCircleV(ge.position, 10.0f, ColorAlpha(ec, fade));
        DrawCircleV(ge.position, 5.0f,  ColorAlpha(WHITE, 0.7f * fade));
        // Name label
        const char* eName = ge.equip.name.c_str();
        int ew = MeasureText(eName, 11);
        DrawText(eName, (int)(ge.position.x - ew/2), (int)(ge.position.y - 30), 11,
                 ColorAlpha(ec, fade));
        // E-prompt when player nearby
        float dist = Vector2Distance(player.position, ge.position);
        if (dist < 60.0f) {
            const char* pr = "[E] EQUIPAR";
            int pw = MeasureText(pr, 12);
            DrawRectangle((int)(ge.position.x - pw/2 - 4), (int)(ge.position.y - 50),
                          pw + 8, 18, ColorAlpha(BLACK, 0.7f));
            DrawText(pr, (int)(ge.position.x - pw/2), (int)(ge.position.y - 48), 12,
                     ColorAlpha({255,210,0,255}, 1.0f));
        }
    }

    // Building system (world-space)
    {
        Vector2 mouseWorld = render3D ? mouseGround3D()
                                      : GetScreenToWorld2D(virtualizeMousePos(GetMousePosition()), camera);
        buildingSystem.render(player.position, mouseWorld);
        buildingSystem.renderUnitPrompts();  // "[CLIQUE] Produzir Tanque ..."
        buildingSystem.renderBuildingInfo(player.position); // nivel + descricao + [U] evoluir
        buildingSystem.renderSelection();    // aneis verdes sob unidades selecionadas

        // Caixa de selecao por arrasto (mundo)
        if (rtsDragging) {
            Rectangle box = { rtsDragStart.x, rtsDragStart.y,
                              rtsDragCur.x - rtsDragStart.x,
                              rtsDragCur.y - rtsDragStart.y };
            if (box.width  < 0) { box.x += box.width;  box.width  = -box.width; }
            if (box.height < 0) { box.y += box.height; box.height = -box.height; }
            DrawRectangleRec(box, ColorAlpha(Color{0,255,80,255}, 0.12f));
            DrawRectangleLinesEx(box, 1.5f, ColorAlpha(Color{0,255,80,255}, 0.8f));
        }
    }

    for (auto& npc  : npcs)          npc.render();

    // Vendor indicator — floating "$" + "LOJA" above vendor NPCs
    for (const auto& npc : npcs) {
        bool isVendor = (npc.role == NPCRole::Merchant ||
                         npc.role == NPCRole::WeaponDealer ||
                         npc.role == NPCRole::ArmorSmith);
        if (isVendor) {
            float pulse = 0.65f + 0.35f * std::sin((float)GetTime() * 2.8f);
            Color cGold = ColorAlpha({255,200,0,255}, pulse);
            int tx = (int)npc.position.x;
            int ty = (int)npc.position.y - 60;
            DrawRectangle(tx - 32, ty - 2, 64, 22, ColorAlpha(BLACK, 0.72f));
            DrawRectangleLinesEx({(float)(tx-32),(float)(ty-2),64,22}, 1, ColorAlpha(cGold, 0.8f));
            const char* lbl = "[TAB] LOJA";
            DrawText(lbl, tx - MeasureText(lbl, 11)/2, ty, 11, cGold);
        }
    }

    player.render();
    drawCompanions();

    for (auto& enemy : enemies)      if (inView(enemy.position)) enemy.render();
    for (auto& proj  : projectiles)  if (inView(proj.position))  proj.render();
    for (auto& ep    : enemyProjectiles) if (inView(ep.position)) ep.render();

    particles.render();

    // Click-to-move target indicator
    if (hasTarget) {
        float pulse = 0.5f + 0.5f * std::sin((float)GetTime() * 8.0f);
        Color tc = ColorAlpha(Color{0,220,255,255}, 0.55f + 0.35f * pulse);
        DrawCircleLines((int)moveTarget.x, (int)moveTarget.y, 10.0f, tc);
        DrawCircleLines((int)moveTarget.x, (int)moveTarget.y, 5.0f,  tc);
        DrawLineEx({moveTarget.x - 14, moveTarget.y},
                   {moveTarget.x + 14, moveTarget.y}, 1.5f, ColorAlpha(tc, 0.6f));
        DrawLineEx({moveTarget.x, moveTarget.y - 14},
                   {moveTarget.x, moveTarget.y + 14}, 1.5f, ColorAlpha(tc, 0.6f));
    }

    // Floating numbers (damage/credits/heals — world-space)
    for (const auto& dn : damageNumbers) {
        float alpha = std::min(dn.life / 0.45f, 1.0f);
        Color c = ColorAlpha(dn.color, alpha);
        // Fonte menor para nao poluir a tela perto do personagem
        int fontSize = (dn.value >= 100.0f) ? 15 :
                       (dn.value >= 50.0f)  ? 13 : 11;
        const char* txt = TextFormat("%s%.0f", dn.prefix, dn.value);
        int tw = MeasureText(txt, fontSize);
        // Shadow for readability
        DrawText(txt, (int)dn.pos.x - tw/2 + 1, (int)dn.pos.y - 14 + 1,
                 fontSize, ColorAlpha(BLACK, 0.6f * alpha));
        DrawText(txt, (int)dn.pos.x - tw/2,     (int)dn.pos.y - 14,
                 fontSize, c);
    }

    // Dialog balloon
    if (dialogOpen && nearNpcIndex >= 0 && nearNpcIndex < (int)npcs.size()) {
        npcs[nearNpcIndex].showDialog(dialogLine);
    }

    // Inferno zone effects — geysers + ash (drawn after entities)
    if (infernoZone.active) {
        infernoZone.renderEffects(camera.target);
    }

    // Bot world-space overlay — marcador discreto do alvo (sem a linha longa que
    // cruzava a tela e poluia o visual).
    if (botController.active) {
        DrawCircleLines((int)botAimTarget.x, (int)botAimTarget.y, 7.0f,
                        ColorAlpha(YELLOW, 0.35f));
    }

    EndMode2D();

    // Dark world fog overlay (screen-space, after world render)
    if (darkWorld.active) {
        darkWorld.renderFog(screenWidth, screenHeight);
    }

    // Inferno zone atmosphere — red haze overlay (screen-space)
    if (infernoZone.active) {
        infernoZone.renderAtmosphere(screenWidth, screenHeight);
    }

    // Anomaly portal HUD (screen-space)
    anomalySystem.renderHUD(screenWidth, screenHeight);

    // Building menu (screen-space)
    buildingSystem.renderBuildMenu(screenWidth, screenHeight);

    // Bot screen-space overlay
    if (botController.active) {
        // Bot HUD panel — positioned at y=200 to clear top bar (0–26) and any overlays
        DrawRectangle(8, 200, 220, 72, ColorAlpha(BLACK, 0.80f));
        DrawRectangleLinesEx({8,200,220,72}, 1.0f, ColorAlpha({0,255,80,255}, 0.5f));
        DrawText("[BOT ATIVO]", 12, 203, 12, Color{0,255,80,255});
        DrawText(TextFormat("HP:%.0f%%  Kills:%d  Lvl:%d",
                 player.health/player.maxHealth*100.f,
                 botController.killCount, player.level),
                 12, 218, 10, ColorAlpha(WHITE, 0.9f));
        DrawText(TextFormat("Melee:%d  Skills:%d  Itens:%d",
                 botController.meleeHits, botController.skillsFired, botController.itemsChased),
                 12, 231, 10, ColorAlpha(Color{0,220,100,255}, 0.9f));
        DrawText(TextFormat("Areas:%d/4  Mortes:%d",
                 botController.areasExplored, botController.deathCount),
                 12, 244, 10, ColorAlpha(Color{200,200,200,255}, 0.8f));
        // Log lines (last 6) — below panel at y=276
        int logStart = std::max(0, (int)botController.log.size() - 6);
        for (int i = logStart; i < (int)botController.log.size(); ++i) {
            int row = i - logStart;
            DrawText(botController.log[i].c_str(), 10, 276 + row * 14, 10,
                     ColorAlpha(Color{180,255,180,255}, 0.85f));
        }
    }

    // Post-process: scanlines + vignette
    DrawScanlines(screenWidth, screenHeight);
    DrawVignette(screenWidth, screenHeight);

    // Hit flash overlay (red flicker when player takes damage)
    if (hitFlashTimer > 0.0f) {
        float alpha = (hitFlashTimer / 0.30f) * 0.45f;
        DrawRectangle(0, 0, screenWidth, screenHeight,
                      ColorAlpha(RED, alpha));
    }

    // SlowMo border pulse (cyan outline when slow-mo active)
    if (slowMoTimer > 0.0f) {
        float alpha = std::min(slowMoTimer / 1.8f, 1.0f) * 0.6f;
        DrawRectangleLinesEx({0, 0, (float)screenWidth, (float)screenHeight},
                             8, ColorAlpha({0, 220, 255, 255}, alpha));
    }

    // Boss HP bar (top center, when boss alive)
    {
        const Enemy* boss = nullptr;
        for (const auto& e : enemies) {
            if (e.type == EnemyType::Boss) { boss = &e; break; }
        }
        if (boss) {
            int bx = screenWidth / 2 - 200;
            int by = 30;
            float pct = boss->health / boss->maxHealth;
            Color bossCol = pct > 0.5f ? Color{200,20,20,255}
                                       : pct > 0.25f ? ORANGE : RED;
            DrawRectangle(bx - 4, by - 4, 408, 28, ColorAlpha(BLACK, 0.85f));
            DrawRectangle(bx, by, 400, 20, ColorAlpha(BLACK, 0.6f));
            DrawRectangle(bx, by, (int)(400.0f * pct), 20, bossCol);
            DrawRectangleLinesEx({(float)bx, (float)by, 400, 20}, 1.5f,
                                 ColorAlpha(bossCol, 0.7f));
            DrawText("IRON-VIII BOSS", bx + 400/2 - 65, by + 2, 15,
                     ColorAlpha(WHITE, 0.9f));
            DrawText(TextFormat("%.0f / %.0f", boss->health, boss->maxHealth),
                     bx + 2, by + 2, 13, ColorAlpha(WHITE, 0.7f));
        }
    }

    // Combo counter (center screen, Diablo-style)
    if (comboCount >= 2) {
        float fade = std::min(comboTimer / 2.5f, 1.0f);
        // Contador de combo SUTIL — fonte pequena, num canto, sem cobrir o jogo.
        int   fontSize = 14 + std::min(comboCount, 10) * 1; // 15..24
        Color comboCol = comboCount >= 10 ? Color{255,80,40,255}
                       : comboCount >= 5  ? Color{255,190,60,255}
                                          : Color{255,150,80,255};
        const char* numTxt = TextFormat("x%d combo", comboCount);
        int nw = MeasureText(numTxt, fontSize);
        // Canto superior-direito, abaixo da top bar — não atrapalha a jogabilidade.
        int cx = screenWidth - nw - 18;
        int cy = 32;
        DrawText(numTxt, cx + 1, cy + 1, fontSize, ColorAlpha(BLACK, 0.7f * fade));
        DrawText(numTxt, cx,     cy,     fontSize, ColorAlpha(comboCol, fade));
    }

    // Apply light mask overlay (darkens world except around light sources)
    lightSystem.applyMask();

    drawUI();

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
    drawStoryBanner();
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

    EndTextureMode();
}

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
    if (dialogOpen && nearNpcIndex >= 0) {
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
    int px = screenWidth - panW - 8, py = 70;

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
        const char* prog = q.getProgressText().c_str();
        DrawText(prog, px + 8,  ry + 27, 11, ColorAlpha(WHITE, 0.85f));
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
}

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




