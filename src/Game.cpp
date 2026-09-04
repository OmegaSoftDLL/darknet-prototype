#include "Game.h"
#include "SpriteGen.h"
#include "SpriteExtrude.h"
// true SOMENTE durante a captura de sprite para voxelização (Game::ensureVoxel →
// SpriteExtrude::CaptureToImage): as entidades suprimem sombras/textos 2D para não
// virarem "pedestal" na malha voxel. Não seleciona pipeline de render.
bool g_voxelCapture = false;

// ── ARQUITETURA POR BIOMA ────────────────────────────────────────────────────
// Todas as fases usavam os MESMOS 4 modelos (casa/celeiro/castelo/silo): trocava
// o chao e o ceu, mas a cidade era identica em Los Angeles, no cemiterio e no
// inferno. Aqui cada bioma tem seu proprio conjunto de tipos de estrutura.
//   0 casa   1 celeiro  7 castelo/predio  8 silo
//  14 cripta 15 bunker  16 espira infernal 17 monolito 18 cabana 19 torre

// Tinta das construcoes por bioma: o mesmo modelo lido como pedra clara em LA e
// como pedra queimada no inferno ja muda a leitura da cidade inteira.
static Color structureTintFor(ZoneID z) {
    switch (z) {
        case ZoneID::Cemetery:       return { 150, 158, 172, 255 };
        case ZoneID::DarkForest:     return { 148, 156, 132, 255 };
        case ZoneID::CursedFarm:     return { 198, 176, 132, 255 };
        case ZoneID::Bunker:         return { 138, 150, 140, 255 };
        case ZoneID::Catacombs:      return { 152, 140, 126, 255 };
        case ZoneID::AbandonedManor: return { 158, 140, 162, 255 };
        case ZoneID::KronosForge:    return { 186, 142, 110, 255 };
        case ZoneID::InfernoZone:    return { 150,  96,  80, 255 };
        case ZoneID::KronosNexus:    return { 130, 168, 196, 255 };
        case ZoneID::GhostCity:      return { 160, 168, 180, 255 };
        case ZoneID::LARuins:
        default:                     return { 255, 255, 255, 255 };
    }
}

// Zonas onde os modelos MEDIEVAIS (castle.obj / house.obj) sao coerentes: areas
// rurais/goticas. Nas zonas urbanas e sci-fi (LA, cidade fantasma, bunker, forja,
// nexus...) o castelo de torres e a casa de telha quebram a direcao de arte —
// la o BuildingSystem desenha estruturas modernas em primitivas (auditoria P1).
static bool isMedievalZone(ZoneID z) {
    return z == ZoneID::CursedFarm || z == ZoneID::DarkForest ||
           z == ZoneID::Cemetery  || z == ZoneID::AbandonedManor;
}

// ── ESCALA DO MUNDO ──────────────────────────────────────────────────────────
// Tudo ancorado no heroi: ~28 unidades de altura = 1,75 m, entao 1 metro ~ 16u.
// Os valores antigos (casa 110u = 7 m na MAIOR dimensao) deixavam predio menor
// que gente: a cidade lia como maquete e o personagem como um poste ao lado dela.
static constexpr float FIT_HOUSE    = 175.0f;   // casa de 2 andares ~11 m
static constexpr float FIT_BARRACKS = 190.0f;   // celeiro/galpao ~12 m
static constexpr float FIT_CASTLE   = 340.0f;   // predio/castelo ~21 m
static constexpr float FIT_TURRET   =  95.0f;
static constexpr float FIT_MARKET   = 200.0f;
static constexpr float FIT_WELL     = 130.0f;   // silo alto
static constexpr float FIT_CAR      =  68.0f;   // carro ~4,2 m de comprimento
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <string>
#include <filesystem>

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

    rlSetTexture(rlGetTextureIdDefault());   // P0: religa branca p/ não vazar textura nas primitivas
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
    initPostFX();       // bloom + tonemap
    initWorldShader();  // luz direcional + rim + nevoa nos modelos 3D
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
        m_houseScale    = _fit(m_houseModel,    FIT_HOUSE);
        m_barracksScale = _fit(m_barracksModel, FIT_BARRACKS);
        m_castleScale   = _fit(m_castleModel,   FIT_CASTLE);
        m_turretScale   = _fit(m_turretModel,   FIT_TURRET);
        m_marketScale   = _fit(m_marketModel,   FIT_MARKET);
        m_wellScale     = _fit(m_wellModel,     FIT_WELL);
        m_carScale      = _fit(m_carModel,      FIT_CAR);
    }
    // Liga a iluminacao nos modelos de cenario (os voxel recebem ao serem gerados)
    applyWorldShader(m_houseModel);    applyWorldShader(m_barracksModel);
    applyWorldShader(m_castleModel);   applyWorldShader(m_turretModel);
    applyWorldShader(m_marketModel);   applyWorldShader(m_wellModel);
    applyWorldShader(m_carModel);
    m_modelsLoaded = true;

    audio.init();
    loadPhaseDefs();   // campanha vem de content/phases.txt (editavel sem recompilar)
    buildQuests();
    buildNPCs();
    craftingSystem.buildRecipes();
    achievements.init();

    // Open world — generate unified 9-region map
    openWorldMode = true;
    tilemap.generateOpenWorld();
    setupWorldRegions();

    // Player starts in center of first region (LARuins) = ZONA SEGURA
    // (definido ANTES de buildOpenWorldScenery: o cenario usa safeZoneCenter e
    //  owPhaseRadius para limitar o mundo da fase — com o centro velho a
    //  construcao saia errada e era refeita depois, dobrando o trabalho)
    float cx = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f;
    float cy = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
    player.position  = {cx, cy};
    safeZoneCenter   = {cx, cy};   // refugio fica no centro da regiao inicial
    {   // fase 1 tambem sai da tabela (antes os valores viviam so no codigo)
        const PhaseDef& p0 = phaseDef(0);
        owPhaseGoal   = p0.goal;
        owPhaseRadius = p0.radius;
        owBossPhase   = p0.boss;
        currentZone   = p0.zone;
        currentRegion = p0.zone;
    }
    buildOpenWorldScenery();

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
        if (m_carModel.meshCount > 0) UnloadModel(m_carModel);
    }
    // Libera os modelos VOXEL gerados em runtime (eram leak de CPU+GPU).
    for (auto& kv : m_voxModels) if (kv.second.meshCount > 0) UnloadModel(kv.second);
    m_voxModels.clear();
    unloadPostFX();

    CloseWindow();
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

// ─── Run / Update ────────────────────────────────────────────────────────────

void Game::runAutoTest(bool autoTest) {
    if (autoTest) {
        // Limpa screenshots de runs anteriores: shot_NN.png antigo misturado com
        // o do run atual vira evidencia falsa (runs indistinguiveis no mesmo dir).
        try {
            for (const auto& e : std::filesystem::directory_iterator(".")) {
                const std::string fn = e.path().filename().string();
                if (fn.rfind("shot_", 0) == 0 && e.path().extension() == ".png")
                    std::filesystem::remove(e.path());
            }
        } catch (...) { /* sem permissao/dir estranho: segue o jogo */ }
        // Skip menu, start game immediately with bot active
        buildQuests();
        // O mundo ja foi construido UMA vez no construtor (com safeZoneCenter
        // correto) — regenerar aqui era a 2a construcao descartavel do log SCENERY.
        setupZoneNPCs(currentZone);
        // Sem isto o --autotest parava no MENU esperando um ENTER humano: o bot
        // so roda depois que a partida comeca. "Skip menu" era so o comentario.
        inMainMenu = false;
        audio.stopMenuMusic();
        botController.active   = true;
        botController.autoTest = true;
        botController.testDuration = (autoTestSeconds > 0.0f) ? autoTestSeconds : 7200.0f;
        botController.addLog("=== AUTO-BOT TEST MODE ATIVADO ===");
        botController.addLog("Duracao maxima: 7200s (2h)");
        startNetwork();   // testa o cliente WebSocket (multiplayer)
        startStore();     // testa login + catalogo da loja premium
    }
    run();
    // After run() exits, write report if bot was active
    if (botController.active || autoTest) {
        botController.writeReport("bot_report.txt");   // relativo ao CWD: funciona em qualquer maquina/CI
        // PORTAO DE VALIDACAO: veredito no console; o exit code sai por main.cpp.
        std::vector<std::string> why;
        autoTestPassed = botController.passed(&why);
        TraceLog(LOG_INFO, "VALIDACAO: %s", autoTestPassed ? "PASSOU" : "FALHOU");
        for (const auto& w : why) TraceLog(LOG_WARNING, "VALIDACAO: %s", w.c_str());
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
    victoryReported = false;
    buildQuests();
    currentZone   = ZoneID::LARuins;
    currentRegion = ZoneID::LARuins;
    if (openWorldMode) {
        tilemap.generateOpenWorld();
        setupWorldRegions();
        float ox = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f;
        float oy = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
        player.position = {ox, oy};
        safeZoneCenter  = {ox, oy};   // ANTES do cenario (ele limita pela barreira da fase)
        buildOpenWorldScenery();
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
    player.unclaimedLevels = 0;   // nivel veio do arquivo; nao e level-up novo
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
    victoryReported = false;
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

    // ── Reset do bot/autotest (eram static de funcao — vazavam entre partidas) ─
    botReportSaveTimer = 0.0f;
    botAllyTimer       = 2.0f;
    botBuildTimer      = 4.0f;
    botProduceTimer    = 8.0f;
    botUpgradeTimer    = 12.0f;
    botStipendTimer    = 0.0f;
    botBuildCycle      = 0;
    lastShot           = 0.0;
    shotN              = 0;
    botController.reset();       // telemetria, estado, timers e rota cacheada
    Companion::resetSpawnIndex(); // vagas de formacao voltam ao inicio

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

void Game::update(float dt) {
    // VITORIA — congela o mundo e mostra a tela de fim de jogo
    if (gameWon) {
        victoryTimer += dt;
        particles.update(dt);
        audio.updateMusic();
        if (playerSpeechTimer > 0.0f) playerSpeechTimer -= dt;
        // Bot: registra a vitoria UMA vez e encerra o teste
        if (botController.active && !victoryReported) {
            victoryReported = true;
            botController.addLog("=== JOGO ZERADO! NUCLEO KRONOS DESTRUIDO ===");
            botController.addLog(TextFormat("Nivel final %d, kills %d", player.level, totalKills));
            botController.writeReport("bot_report_VITORIA.txt");
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
        bool isDark = true; // pipeline 3D: clima Diablo sempre
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
            // Pipeline 3D: iluminação ambiente por zona sempre ativa.
            {
                Color tCol; float tDark;
                switch (currentZone) {
                    // ambientDark reduzido: a mascara e MULTIPLICATIVA e ja vinha
                    // depois do fog do chao — os dois somados apagavam a cena.
                    // Clima sombrio vem do MATIZ e do contraste, nao de apagar tudo.
                    // CONTRASTE ENTRE ZONAS aumentado: matizes quase neutros faziam
                    // toda fase ler igual. Inferno = laranja-sangue, nexus = cyan,
                    // floresta = verde profundo, fantasma = azul frio e mais escuro.
                    case ZoneID::LARuins:       tCol = {228,206,172,255}; tDark = 0.20f; break;
                    case ZoneID::Bunker:        tCol = {150,182,168,255}; tDark = 0.32f; break;
                    case ZoneID::KronosForge:   tCol = {250,150, 90,255}; tDark = 0.26f; break;
                    case ZoneID::KronosNexus:   tCol = {120,220,250,255}; tDark = 0.27f; break;
                    case ZoneID::Cemetery:      tCol = {140,150,220,255}; tDark = 0.40f; break;
                    case ZoneID::CursedFarm:    tCol = {206,190,120,255}; tDark = 0.30f; break;
                    case ZoneID::GhostCity:     tCol = {150,168,210,255}; tDark = 0.42f; break;
                    case ZoneID::DarkForest:    tCol = {120,200,140,255}; tDark = 0.37f; break;
                    case ZoneID::Catacombs:     tCol = {170,130,180,255}; tDark = 0.44f; break;
                    case ZoneID::AbandonedManor:tCol = {180,140,220,255}; tDark = 0.40f; break;
                    case ZoneID::InfernoZone:   tCol = {255,120, 60,255}; tDark = 0.25f; break;
                    default:                    tCol = {206,212,226,255}; tDark = 0.27f; break;
                }
                // Interpola em ~1,5s em vez de trocar de uma vez: cruzar a fronteira
                // de bioma vira transicao de luz, nao um corte seco de "outro mundo".
                {
                    float k = 1.0f - expf(-GetFrameTime() * 0.8f);
                    m_ambBaseDark += (tDark - m_ambBaseDark) * k;
                    m_ambBaseCol.r = (unsigned char)(m_ambBaseCol.r + (tCol.r - m_ambBaseCol.r) * k);
                    m_ambBaseCol.g = (unsigned char)(m_ambBaseCol.g + (tCol.g - m_ambBaseCol.g) * k);
                    m_ambBaseCol.b = (unsigned char)(m_ambBaseCol.b + (tCol.b - m_ambBaseCol.b) * k);
                    lightSystem.ambientColor = m_ambBaseCol;
                    lightSystem.ambientDark  = m_ambBaseDark;
                }

                // ── CICLO DIA/NOITE (mundo vivo): noite escura/azulada, dia claro ──
                worldClock += GetFrameTime() / 420.0f;          // ciclo completo ~7 min
                if (worldClock >= 1.0f) worldClock -= 1.0f;
                float sun = sinf(worldClock * 6.2831853f - 1.5707963f) * 0.5f + 0.5f; // 0=noite,1=meio-dia
                worldSun = sun;
                lightSystem.ambientDark += (1.0f - sun) * 0.20f; // escurece à noite
                if (lightSystem.ambientDark > 0.52f) lightSystem.ambientDark = 0.52f;  // teto: noite legivel
                {
                    Color d = lightSystem.ambientColor;
                    Color n = { 104, 132, 196, 255 };            // azul noturno (mais claro: luar, nao breu)
                    lightSystem.ambientColor = {
                        (unsigned char)(n.r + (int)((d.r - n.r) * sun)),
                        (unsigned char)(n.g + (int)((d.g - n.g) * sun)),
                        (unsigned char)(n.b + (int)((d.b - n.b) * sun)), 255 };
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
    if (!buildingSystem.buildModeActive && !shopSystem.open && !craftingSystem.open && !showInventory) {
        float wh = GetMouseWheelMove();
        if (wh != 0.0f) { cameraZoom -= wh * 0.06f;
            if (cameraZoom < 0.85f) cameraZoom = 0.85f;
            if (cameraZoom > 1.50f) cameraZoom = 1.50f; }
    }

    // Câmera 3D (2.5D) acompanha o jogador — sempre ativa.
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
            spawnInterval       = getZoneInfo(newRegion).spawnInterval / getDifficulty().spawnRateMult;
            audio.setZone(newRegion);

            for (auto& r : worldRegions)
                if (r.zoneType == newRegion && !r.discovered) { r.discovered = true; break; }

            // Anuncio UNICO da regiao: a barra do topo. Antes isto tambem ligava
            // zoneNameTimer e, na PRIMEIRA visita, o mesmo nome+descricao saia duas
            // vezes ao mesmo tempo (barra no topo + texto gigante no meio da tela).
            ZoneInfo zi = getZoneInfo(newRegion);
            showStoryBanner(zi.name.c_str(), zi.description.c_str(), 4.0f);

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
        dn.rise += 38.0f * dt;   // sobe em `rise`; no 3D pos.y e o eixo NORTE do chao
        dn.life -= dt;
    }
    damageNumbers.erase(
        std::remove_if(damageNumbers.begin(), damageNumbers.end(),
                       [](const DamageNumber& d){ return d.life <= 0.0f; }),
        damageNumbers.end());
    if (zoneNameTimer > 0.0f) zoneNameTimer -= dt;

    // Aviso ao CRUZAR a fronteira da zona segura (portao da base)
    if (openWorldMode) {
        bool nowInSafe = inSafeZone(player.position);
        player.inSafeRefuge = nowInSafe;   // invulnerável no refúgio (ninguém te mata na cidade)
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
                // Marca no PRÓPRIO inimigo (flag move junto na realocação do vetor) —
                // evita o use-after-free de guardar ponteiro em netKilledEnemies.
                closest->netKilled = true;
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
    updateCityFolk(dt);   // civis perambulando pela cidade

    // Spawn enemies — COM LIMITE para nao acumular sem fim (perf + estabilidade).
    // O cap escala um pouco com a dificuldade; bosses/minions ainda podem somar.
    {
        const int baseCap   = 55;
        const int diffBonus  = (int)difficulty * 12;   // Historia 0 .. Apocalipse +48
        const int enemyCap   = baseCap + diffBonus + threatLevel * 3; // mais ameaca = mais inimigos
        spawnTimer += dt;
        // Mutador "Invasao Total" acelera o spawn
        float dayNight = 0.62f + 0.38f * worldSun;   // noite: intervalo menor = mais inimigos
        float effectiveInterval = spawnInterval * mutatorSpawnMult() * dayNight;
        if (spawnTimer >= effectiveInterval) {
            // ZONA SEGURA: nao spawna inimigos enquanto o player esta no refugio
            if ((int)enemies.size() < enemyCap && !inSafeZone(player.position))
                spawnEnemy();
            spawnTimer = 0.0f;
        }
    }

    // Boss tambem nao surge dentro da zona segura.
    // Em FASE DE CHEFE o gatilho e a cota da fase: mata a cota -> o chefe aparece
    // -> so entao o portal abre. Da comeco, meio e fim para a fase.
    bool bossCue = openWorldMode
        ? (owBossPhase && owPhaseKills >= owPhaseGoal)
        : (enemiesKilled >= bossSpawnThreshold);
    if (bossCue && !bossSpawned && !inSafeZone(player.position)) {
        spawnBoss();
        bossSpawned = true;
        if (openWorldMode)
            showStoryBanner("O CHEFE APARECEU", "Derrote-o para abrir o portal.", 4.5f);
    }

    // Auto-save
    saveTimer += dt;
    if (saveTimer >= 30.0f) { autoSave(); saveTimer = 0.0f; }

    // Update enemies and collect shooting requests
    int enemyIdx = 0;
    for (auto& enemy : enemies) {
        Vector2 prevPos = enemy.position;
        // Ponto de aproximacao tatico: longe do jogador o inimigo vai pro flanco
        // ou corta a retaguarda; colado, recebe a posicao REAL (senao a mira e o
        // telegrafo de ataque apontariam para o lugar errado).
        Vector2 aim = enemy.isBoss()
                    ? player.position
                    : director.approachPoint(enemyIdx++, enemy.position, player.position);
        enemy.update(dt, aim);

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
            // Empurra forte; se entrou MUITO fundo, joga direto pra borda (não fica perseguindo).
            float push = enemy.speed * 4.0f * dt + 90.0f * dt;
            enemy.position.x += (away.x / len) * push;
            enemy.position.y += (away.y / len) * push;
            if (len < safeZoneRadius - 200.0f) {
                enemy.position.x = safeZoneCenter.x + (away.x / len) * (safeZoneRadius + 30.0f);
                enemy.position.y = safeZoneCenter.y + (away.y / len) * (safeZoneRadius + 30.0f);
            }
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
    drainLevelUps();      // credita os niveis ganhos neste frame (qualquer fonte)
    // A IA aprende com ESTE frame: distancia, movimentacao, ritmo de abate e dano
    // sofrido alimentam o diretor, que responde no spawn e na tatica.
    director.observe(dt, player.position, player.health, player.maxHealth,
                     enemiesKilled, (int)enemies.size());
    updatePhasePortal(dt);
    // ── LIMITE DA FASE ───────────────────────────────────────────────────────
    if (openWorldMode && owFadeTimer <= 0.0f) {
        Vector2 d = { player.position.x - safeZoneCenter.x, player.position.y - safeZoneCenter.y };
        float dl = sqrtf(d.x*d.x + d.y*d.y);
        if (dl > owPhaseRadius) {
            player.position.x = safeZoneCenter.x + d.x / dl * owPhaseRadius;
            player.position.y = safeZoneCenter.y + d.y / dl * owPhaseRadius;
            if (borderWarnTimer <= 0.0f) {
                borderWarnTimer = 2.0f;
                triggerPlayerSpeech("Barreira de KRONOS. Nao da pra ir alem daqui.", 2.5f);
            }
        }
        if (borderWarnTimer > 0.0f) borderWarnTimer -= dt;
    }
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

                // Morte por sync de rede (flag no inimigo) → não rebroadcastar.
                if (it->netKilled) {
                    // já tratada pela rede; nada a enviar
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

                // ── ABSORÇÃO DE PODER (estilo V Rising): matar BOSS = buff PERMANENTE ──
                if (it->isBoss()) {
                    bossPowersAbsorbed++;
                    int kind = bossPowersAbsorbed % 4;
                    const char* pname = (kind==0) ? "+10% Vida Maxima" : (kind==1) ? "+10% Dano"
                                      : (kind==2) ? "+4% Defesa" : "+6% Velocidade";
                    player.absorbBossEssence(kind);   // buff PERMANENTE (mexe no base + recalcula)
                    showStoryBanner("PODER ABSORVIDO",
                        TextFormat("Essencia do boss: %s   (total: %d)", pname, bossPowersAbsorbed), 3.5f);
                    triggerShake(6.0f, 0.4f);
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
        // Mouse: clique seleciona/equipa/usa; botão X fecha.
        bool lc = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool rc = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if (lc || rc) {
            Vector2 vm = virtualizeMousePos(GetMousePosition());
            if (player.handleInventoryMouse(vm, lc, rc)) showInventory = false;
        }
        if (IsKeyPressed(KEY_I)) showInventory = false;
        return;
    }

    // Mouse no mundo: raycast no plano do chão 3D (pipeline 2.5D).
    Vector2 mouseWorld = mouseGround3D();

    // ── Bot controller decisions ──────────────────────────────────────────────
    updateBotControl(dt);
    // shouldQuit do bot (autotest) pedia return imediato do handleInput —
    // quitRequested so e setado ali dentro, entao o early-return e equivalente.
    if (quitRequested) return;

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
        // Testa o clique contra o NPC PROJETADO NA TELA (o modelo voxel é
        // alto e aparece acima dos "pés"; o chão sob o cursor cai atrás do NPC).
        Vector2 cs = virtualizeMousePos(GetMousePosition());
        for (int i = 0; i < (int)npcs.size(); ++i) {
            Vector2 ns = GetWorldToScreenEx({ npcs[i].position.x, 28.0f, npcs[i].position.y },
                                            camera3D, screenWidth, screenHeight);
            bool hit = Vector2Distance(cs, ns) <= 44.0f;   // tolerância em pixels (corpo)
            if (hit) {
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
            if (!airborne && isBlocked(player.position) && !isBlocked(old)) player.position = old;
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
            if (!airborne && isBlocked(player.position) && !isBlocked(old)) {
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
    // [E] ou CLIQUE ESQUERDO (com diálogo aberto) avança a fala; ESC fecha.
    bool advanceDialog = IsKeyPressed(KEY_E) || (dialogOpen && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !producedThisClick);
    if (advanceDialog) {
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

// ─── Collisions ──────────────────────────────────────────────────────────────

// Unico ponto que converte niveis ganhos em pontos/evolucoes pendentes. Vale para
// QUALQUER fonte de XP (orbe, quest, TechChip, item usado do inventario), inclusive
// as que ficam dentro de Player e o Game nao enxerga.
void Game::drawFloatingNumbers(bool project3D) const {
    for (const auto& dn : damageNumbers) {
        float alpha = std::min(dn.life / 0.45f, 1.0f);
        Color c = ColorAlpha(dn.color, alpha);
        // Fonte menor para nao poluir a tela perto do personagem
        int fontSize = (dn.value >= 100.0f) ? 15 :
                       (dn.value >= 50.0f)  ? 13 : 11;
        const char* txt = TextFormat("%s%.0f", dn.prefix.c_str(), dn.value);
        int tw = MeasureText(txt, fontSize);
        Vector2 p;
        if (project3D) {
            // Sobe de verdade no eixo Y do mundo 3D e so entao vira coord de tela.
            p = GetWorldToScreenEx({ dn.pos.x, 30.0f + dn.rise, dn.pos.y },
                                   camera3D, screenWidth, screenHeight);
        } else {
            p = { dn.pos.x, dn.pos.y - dn.rise - 14.0f };
        }
        // Shadow for readability
        DrawText(txt, (int)p.x - tw/2 + 1, (int)p.y + 1, fontSize, ColorAlpha(BLACK, 0.6f * alpha));
        DrawText(txt, (int)p.x - tw/2,     (int)p.y,     fontSize, c);
    }
}

void Game::drainLevelUps() {
    int gained = player.unclaimedLevels;
    if (gained <= 0) return;
    player.unclaimedLevels = 0;

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

    // Quantos dos niveis CRUZADOS sao de evolucao (conta cada um; subir 2 de uma
    // vez passando por 10 e 11 da 1 evolucao + 1 ponto).
    static const int EVO_LEVELS[] = {10, 25, 40, 60};
    int evo = 0;
    for (int l = player.level - gained + 1; l <= player.level; ++l)
        for (int el : EVO_LEVELS) if (l == el) { ++evo; break; }

    pendingEvolutions += evo;
    pendingLevelUps   += (gained - evo);
    if (evo > 0) triggerPlayerSpeech("EVOLUCAO disponivel! Pressione K para escolher.", 4.0f);
}

void Game::checkCollisions() {
    // Player projectiles vs enemies
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        if (proj.isGrenade) continue; // handled in updateProjectiles on expire

        for (auto& enemy : enemies) {
            if (enemy.isDead()) continue;   // nao desperdicar tiro em cadaver pendente
            // ao quadrado: evita um sqrt por par projetil x inimigo (loop O(n*m) quente)
            float ddx = proj.position.x - enemy.position.x;
            float ddy = proj.position.y - enemy.position.y;
            float rsum = enemy.radius + proj.radius;
            if (ddx*ddx + ddy*ddy <= rsum*rsum) {
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
            // Coleta REAL (item saindo do vetor) — o bot contava por proximidade
            // (<20px) e o magnetismo/recolha automatica removia o item antes.
            if (botController.active) botController.itemsCollected++;
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
            player.addXP(it->amount);   // o credito de level-up sai em drainLevelUps()
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
    auto drawBubble = [&](Vector2 worldPos, float height3D, float offset3D, const std::string& text, float timer) {
        if (timer <= 0.0f) return;
        float alpha = timer < 0.8f ? timer / 0.8f : 1.0f;

        const char* txt = text.c_str();
        int fontSize = 14;
        int tw = MeasureText(txt, fontSize);
        int bw = tw + 24, bh = 28;

        Vector2 sp = GetWorldToScreenEx({ worldPos.x, height3D, worldPos.y }, camera3D, screenWidth, screenHeight);
        int by = (int)(sp.y) - (int)offset3D;
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
    drawBubble(player.position, 60.0f, 40.0f, playerSpeechText, playerSpeechTimer);

    // 2. Remote Players (activeChats)
    if (netActive) {
        for (const auto& pair : activeChats) {
            uint32_t peerId = pair.first;
            const ChatBubble& cb = pair.second;
            
            // Find the peer's position
            for (const auto& p : net.peers()) {
                if (p.id == peerId) {
                    drawBubble({ p.x, p.y }, 42.0f, 32.0f, cb.text, cb.timer);
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

    // Painel COMPACTO, do tamanho do texto. A versao antiga pintava uma barra
    // preta de LARGURA TOTAL da tela: tapava o jogo inteiro numa faixa so pra
    // mostrar duas linhas de texto.
    int tw = MeasureText(storyBannerText.c_str(), 20);
    int sw = MeasureText(storyBannerSub.c_str(), 12);
    int panW = (tw > sw ? tw : sw) + 40;
    int panH = storyBannerSub.empty() ? 34 : 52;
    int panX = screenWidth / 2 - panW / 2;
    int panY = screenHeight / 6;

    DrawRectangle(panX, panY, panW, panH, ColorAlpha(BLACK, 0.62f * alpha));
    DrawRectangleLinesEx({ (float)panX, (float)panY, (float)panW, (float)panH },
                         1.5f, ColorAlpha(Color{0,200,255,255}, 0.5f * alpha));

    DrawText(storyBannerText.c_str(), screenWidth/2 - tw/2, panY + 7, 20,
             ColorAlpha({0,220,255,255}, alpha));
    if (!storyBannerSub.empty())
        DrawText(storyBannerSub.c_str(), screenWidth/2 - sw/2, panY + 33, 12,
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
    // Blindagem: garante o raio apontando para BAIXO e limita o alcance, para que
    // cliques perto do horizonte NÃO gerem alvo no infinito (player disparava pra
    // longe e o mundo infinito colapsava — causa do "travou").
    float dy = ray.direction.y;
    if (dy > -0.08f) dy = -0.08f;
    float t = -ray.position.y / dy;
    if (t < 0.0f)    t = 0.0f;
    if (t > 5000.0f) t = 5000.0f;
    return { ray.position.x + ray.direction.x * t,
             ray.position.z + ray.direction.z * t };
}

void Game::ensureVoxel(int key, Vector2 capPos, std::function<void()> drawFn) {
    // hasVoxel() ja filtrou no call site — aqui e so cinto de seguranca.
    if (m_voxModels.count(key)) return;
    if (m_voxGenBudget <= 0) return;   // amortiza: poucas geracoes por frame (anti-engasgo)
    m_voxGenBudget--;
    g_voxelCapture = true;
    Image img = SpriteExtrude::CaptureToImage(96, capPos, drawFn);
    g_voxelCapture = false;
    // ESCALA — o unico lugar que define o tamanho de TODO personagem em 3D.
    // A captura tem 96px e a malha e reamostrada pra 34 celulas, entao
    // voxelSize = 2.82 reproduz EXATAMENTE o tamanho do sprite 2D em unidades de
    // mundo (1px 2D = 1 unidade). 3.4 deixava o personagem 20% MAIOR que a arte
    // 2D — perto da casa (110u) e do carro (40u) ele lia como gigante.
    // 1.95 = ~70% da arte 2D: heroi com ~28u (0,45 tile), ~1/4 da casa.
    const float VOX = 1.95f;    // tamanho do voxel (altura do personagem)
    const float VOX_DEPTH = 11.0f;  // espessura: com 7.5 o corpo lia como tabua/poste
    m_voxModels[key] = SpriteExtrude::BuildVoxelModel(img, VOX, VOX_DEPTH);
    applyWorldShader(m_voxModels[key]);
    {   // MEDIDA (nao chute): tamanho real do personagem em unidades de mundo,
        // pra comparar com casa/carro. tile = 64u.
        BoundingBox bb = GetModelBoundingBox(m_voxModels[key]);
        TraceLog(LOG_INFO, "VOXSIZE key=%d  L=%.1f  A=%.1f  P=%.1f", key,
                 bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.max.z - bb.min.z);
    }
    UnloadImage(img);
}

// Construcao generica do jogador (tipos sem modelo .obj proprio). Era um CUBO
// cinza com arestas - lia como placeholder de engine largado no cenario.
void Game::drawGenericStructure(Vector2 pos, float sc) const {
    float x = pos.x, z = pos.y;
    const Color CONCRETE = {  96,  98, 104, 255 };
    const Color METAL    = { 118, 122, 132, 255 };
    const Color TRIM     = {  60, 132, 150, 255 };
    DrawCylinderEx({ x, 0.08f, z }, { x, 0.09f, z }, 34.0f*sc, 34.0f*sc, 14,
                   ColorAlpha(BLACK, 0.32f));                                    // contato
    DrawCubeV({ x, 4.0f*sc,  z }, { 62.0f*sc,  8.0f*sc, 62.0f*sc }, CONCRETE);   // base
    DrawCubeV({ x, 26.0f*sc, z }, { 50.0f*sc, 36.0f*sc, 50.0f*sc }, METAL);      // corpo
    DrawCubeV({ x, 45.0f*sc, z }, { 56.0f*sc,  5.0f*sc, 56.0f*sc }, CONCRETE);   // beiral
    DrawCubeV({ x, 26.0f*sc, z - 25.0f*sc }, { 22.0f*sc, 20.0f*sc, 2.0f*sc }, TRIM);
    DrawCylinderEx({ x + 18.0f*sc, 47.0f*sc, z + 18.0f*sc },
                   { x + 18.0f*sc, 76.0f*sc, z + 18.0f*sc }, 1.8f*sc, 1.0f*sc, 5, METAL);
    DrawSphereEx({ x + 18.0f*sc, 78.0f*sc, z + 18.0f*sc }, 3.0f*sc, 5, 5,
                 Color{ 255, 120, 60, 255 });
}

// ARCA em zona urbana/sci-fi: base de respawn como FORTIFICACAO moderna — bunker
// de concreto com antena, holofotes e faixas de luz cyan. O castle.obj (torres
// vermelhas medievais) no meio do asfalto era o objeto mais olhado do jogo
// traindo a direcao de arte (auditoria P1). Mesma escala do castelo (~340u).
void Game::drawArkStructure(Vector2 pos) const {
    float x = pos.x, z = pos.y;
    const Color CONCRETE = { 104, 108, 114, 255 };
    const Color DARK     = {  66,  70,  76, 255 };
    const Color METAL    = { 128, 132, 142, 255 };
    const Color CYAN     = {  60, 220, 255, 255 };
    float t  = (float)GetTime();
    float pl = 0.55f + 0.45f * sinf(t * 2.2f);                 // pulso dos farois

    DrawCylinderEx({ x, 0.10f, z }, { x, 0.11f, z }, 165.0f, 165.0f, 18,
                   ColorAlpha(BLACK, 0.34f));                              // sombra de contato
    DrawCubeV({ x, 7.0f,   z }, { 300.0f, 14.0f, 300.0f }, DARK);          // plataforma
    DrawCubeV({ x, 42.0f,  z }, { 220.0f, 70.0f, 190.0f }, CONCRETE);      // corpo principal
    DrawCubeV({ x, 90.0f,  z }, { 150.0f, 28.0f, 130.0f }, METAL);         // convés superior
    DrawCubeV({ x, 106.0f, z }, { 90.0f,  8.0f,  74.0f }, DARK);           // casulo do topo
    // porta frontal com faixa luminosa (le como ENTRADA da base)
    DrawCubeV({ x, 26.0f, z - 96.0f }, { 54.0f, 52.0f, 6.0f }, DARK);
    DrawCubeV({ x, 56.0f, z - 97.0f }, { 66.0f, 5.0f, 4.0f }, ColorAlpha(CYAN, 0.65f + 0.30f * pl));
    // faixas de luz cyan nas laterais — marca "base da resistencia", nao ruina
    for (int sI = 0; sI < 2; ++sI) {
        float sx = sI ? 111.0f : -111.0f;
        DrawCubeV({ x + sx, 52.0f, z }, { 3.0f, 8.0f, 150.0f }, ColorAlpha(CYAN, 0.45f + 0.25f * pl));
    }
    // mastro de antena + farol pulsante (o marco que o jogador ve de longe)
    DrawCylinderEx({ x + 52.0f, 110.0f, z + 40.0f }, { x + 52.0f, 200.0f, z + 40.0f },
                   3.2f, 1.4f, 6, METAL);
    DrawSphereEx({ x + 52.0f, 204.0f, z + 40.0f }, 7.0f, 7, 7, ColorAlpha(CYAN, 0.55f + 0.45f * pl));
    DrawCylinderEx({ x - 58.0f, 104.0f, z - 34.0f }, { x - 58.0f, 158.0f, z - 34.0f },
                   2.6f, 1.2f, 6, METAL);
    DrawSphereEx({ x - 58.0f, 161.0f, z - 34.0f }, 5.0f, 6, 6,
                 ColorAlpha(Color{ 255, 130, 60, 255 }, 0.50f + 0.40f * (1.0f - pl)));
    // barricadas nos cantos da plataforma (leitura de fortificacao)
    for (int cI = 0; cI < 4; ++cI) {
        float bx = (cI & 1) ? 128.0f : -128.0f, bz = (cI & 2) ? 128.0f : -128.0f;
        DrawCubeV({ x + bx, 22.0f, z + bz }, { 34.0f, 30.0f, 34.0f }, DARK);
    }
}

void Game::drawVoxel(int base, Vector2 pos, float rotDeg, float walkPhase, bool moving) {
    // Quadro do passo pela fase da caminhada da PROPRIA entidade (nao pelo relogio):
    // parado = pose 0, andando = ciclo de VOX_POSES quadros.
    int pose = 0;
    if (moving) {
        float w = walkPhase * (float)VOX_POSES / (2.0f * PI);
        pose = ((int)floorf(w) % VOX_POSES + VOX_POSES) % VOX_POSES;
    }
    auto it = m_voxModels.find(voxKey(base, pose));
    if (it == m_voxModels.end()) it = m_voxModels.find(voxKey(base, 0));   // pose ainda nao gerada
    if (it == m_voxModels.end() || it->second.meshCount == 0) return;
    Model& mdl = it->second;

    // LOD de custo: alem de 600u o contorno escuro e a sombra de contato nao se
    // distinguem mais — economiza 2 draw calls por entidade distante (restam a
    // silhueta projetada, que le o relevo da sombra, e o modelo).
    float vdx = pos.x - camera3D.target.x, vdz = pos.y - camera3D.target.z;
    bool  vNear = (vdx*vdx + vdz*vdz) < 600.0f*600.0f;

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

    // Sombra de CONTATO: mancha curta EXATAMENTE sob os pes. A silhueta projetada
    // acima da a direcao da luz; esta aqui e a que prega o personagem no chao.
    if (vNear)
        DrawCylinderEx({ pos.x, 2.30f, pos.y }, { pos.x, 2.34f, pos.y },
                       9.0f, 9.0f, 12, ColorAlpha(BLACK, 0.34f));

    // "Respiro" em ESCALA, nunca em translacao: o bob antigo levantava o modelo
    // inteiro (ate 1.2u) enquanto a sombra ficava parada no chao - era isso que
    // fazia TODO personagem/NPC parecer flutuar. Agora os pes ficam colados e so
    // o corpo estica ~1,5%. SINK afunda um tico pra nao sobrar fresta sob os pes.
    float breath = 1.0f + sinf((float)GetTime() * 2.4f + pos.x * 0.05f) * 0.015f;
    // Balanco do passo: o corpo sobe no meio da passada e desce no apoio. Some
    // quando parado, entao nao volta a parecer que flutua.
    float step = moving ? fabsf(sinf(walkPhase)) : 0.0f;
    breath += step * 0.030f;
    rotDeg  += moving ? sinf(walkPhase) * 3.0f : 0.0f;   // leve gingado
    const float SINK = 0.6f;
    Vector3 at = { pos.x, -SINK, pos.y };
    // CONTORNO: mesma malha 6% maior, escura, desenhada ANTES. O modelo real
    // cobre o miolo e sobra so uma borda - separa o personagem do cenario, que
    // e o que faltava pra ele nao sumir no verde da floresta.
    // RIM LIGHT noturno: de noite o contorno escuro apaga JUNTO com o chao.
    // Conforme o ambiente fecha, a borda vira um fio de luar frio — a silhueta
    // do heroi e dos inimigos continua lendo no escuro (auditoria 2, P2).
    float rimK = (lightSystem.ambientDark - 0.30f) / 0.22f;  // 0 de dia, ~1 na noite fechada
    rimK = fminf(1.0f, fmaxf(0.0f, rimK));
    Color outlineC = { (unsigned char)(10.0f + rimK * 62.0f),
                       (unsigned char)(12.0f + rimK * 84.0f),
                       (unsigned char)(18.0f + rimK * 128.0f), 255 };
    if (vNear)
        DrawModelEx(mdl, at, { 0.0f, 1.0f, 0.0f }, rotDeg,
                    { 1.06f, 1.05f * breath, 1.06f }, outlineC);
    DrawModelEx(mdl, at, { 0.0f, 1.0f, 0.0f }, rotDeg, { 1.0f, breath, 1.0f }, WHITE);
}

// Teste esfera × frustum da camera 3D, em espaco de VIEW (raylib: frente = -Z).
// Barato: 1 transform + 3 comparacoes. O corte antigo de cenario era so uma CAIXA
// de distancia (±1400u): tudo ATRAS da camera e fora do cone estreito de 30°
// (a maior parte da caixa) ainda era desenhado primitiva por primitiva.
static bool sphereInCameraFrustum(const Camera3D& cam, const Matrix& view, float aspect,
                                  Vector3 p, float radius) {
    float vz = view.m2*p.x + view.m6*p.y + view.m10*p.z + view.m14;
    float dist = -vz;                                       // >0 = na frente
    if (dist < -radius) return false;                       // totalmente atras
    if (dist < 20.0f) return true;                          // colado na camera: aprova
    float vx = view.m0*p.x + view.m4*p.y + view.m8*p.z  + view.m12;
    float vy = view.m1*p.x + view.m5*p.y + view.m9*p.z  + view.m13;
    float tanH = tanf(cam.fovy * 0.5f * (float)DEG2RAD);
    float limY = dist * tanH + radius;
    if (vy < -limY || vy > limY) return false;
    float limX = dist * tanH * aspect + radius;
    return vx >= -limX && vx <= limX;
}

void Game::renderWorld3D() {
    // PRE-PASS (sem FBO ativo): captura/voxeliza o sprite 2D em MODELO 3D real, por tipo.
    // Amortizado: 1 geracao por frame. Cada geracao faz um readback GPU->CPU sincrono
    // (LoadImageFromTexture) que drena o pipeline; com 3/frame a soma derrubava um
    // frame pra ~22 FPS ao aparecer tipo novo (medido: 3 caps = 32ms num frame).
    // Com 1, o pior frame medido cai pra dentro do orcamento de 60 FPS; o custo e
    // as 4 poses de um tipo novo levarem 4 frames pra aparecer (pop-in imperceptivel).
    m_voxGenBudget = 1;
    // Gera os QUADROS DO PASSO de cada tipo: o render 2D e chamado com a fase da
    // caminhada forcada, entao cada pose sai com as pernas noutra posicao. Sem isto
    // o 3D tinha um unico modelo estatico por tipo e todo mundo deslizava.
    auto poseAngle = [](int pose) { return (float)pose * (PI * 0.5f); };
    // chave do player = classe + APARENCIA: trocar de arma/armadura gera modelo novo
    const int playerVoxBase = 1000 + (int)player.charClass * 1000 + player.visualSignature();
    if (!hasVoxelPoses(playerVoxBase)) {
        float sv = player.walkAnimTimer; bool mv = player.isMoving;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(playerVoxBase, po);
            if (hasVoxel(k)) continue;
            player.walkAnimTimer = poseAngle(po); player.isMoving = true;
            ensureVoxel(k, player.position, [this](){ player.render(); });
        }
        player.walkAnimTimer = sv; player.isMoving = mv;
    }
    for (auto& e : enemies) {
        if (hasVoxelPoses(100 + (int)e.type)) continue;
        float sv = e.walkAnimTimer;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(100 + (int)e.type, po);
            if (hasVoxel(k)) continue;
            e.walkAnimTimer = poseAngle(po);
            ensureVoxel(k, e.position, [&e](){ e.render(); });
        }
        e.walkAnimTimer = sv;
    }
    for (auto& n : npcs) {
        if (hasVoxelPoses(300 + (int)n.role)) continue;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(300 + (int)n.role, po);
            if (hasVoxel(k)) continue;
            ensureVoxel(k, n.position, [&n, po, &poseAngle](){
                NPC t; t.role = n.role; t.color = n.color; t.position = n.position;
                t.walkPhase = poseAngle(po); t.walking = true; t.render();
            });
        }
    }
    for (auto& f : cityFolk) {
        if (hasVoxelPoses(300 + f.role)) continue;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(300 + f.role, po);
            if (hasVoxel(k)) continue;
            ensureVoxel(k, f.position, [&f, po, &poseAngle](){
                NPC t; t.role = (NPCRole)f.role; t.position = f.position;
                t.walkPhase = poseAngle(po); t.walking = true; t.render();
            });
        }
    }
    for (auto& c : companions) {
        if (!c.active || hasVoxelPoses(500 + (int)c.type)) continue;
        float sv = c.walkTimer;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(500 + (int)c.type, po);
            if (hasVoxel(k)) continue;
            c.walkTimer = poseAngle(po);
            ensureVoxel(k, c.position, [&c](){ c.render(); });
        }
        c.walkTimer = sv;
    }

    // Prepare light mask before drawing (uses screen-space projection of 3D lights)
    lightSystem.prepareMask3D(camera3D, screenWidth, screenHeight);

    BeginTextureMode(gameTarget);
    {   // Ceu/horizonte com MATIZ PROPRIO por fase (skyColorFor): o ceu vermelho
        // do inferno e o azul-noite da cidade fantasma sao metade da leitura do
        // bioma. A MESMA cor alimenta o fog do shader (updateWorldShaderUniforms),
        // entao a geometria distante morre exatamente na cor do ceu.
        Color sk = skyColorFor(currentZone);
        float dk = 0.30f + (1.0f - lightSystem.ambientDark) * 0.50f;  // noite escurece o ceu
        ClearBackground(Color{ (unsigned char)(sk.r * dk), (unsigned char)(sk.g * dk),
                               (unsigned char)(sk.b * dk), 255 });
    }
    updateWorldShaderUniforms();

    // ── 1. Modo 3D: Chão, Paredes, Sombras e Entidades (Billboards) ───────────
    rlSetClipPlanes(10.0, 4000.0);
    BeginMode3D(camera3D);
        // Render do mapa 3D
        tilemap.render3D(camera.target, camera3D, (float)screenWidth / (float)screenHeight);

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
            // Frustum da camera (matriz de view + cone de 30°): calculado 1x por frame.
            const Matrix camView = MatrixLookAt(camera3D.position, camera3D.target, camera3D.up);
            const float camAspect = (float)screenWidth / (float)screenHeight;
            for (const auto& obj : owDecor.scenery) {
                float dx = obj.position.x - camera.target.x;
                float dy = obj.position.y - camera.target.y;
                if (dx < -1400 || dx > 1400 || dy < -1400 || dy > 1400) continue;
                // FRUSTUM CULLING real: mais da metade da caixa ±1400u fica atras
                // da camera ou fora do cone de 30°. Raio generoso (400u) cobre ate
                // os predios maiores (FIT_CASTLE=340 · scale) sem risco de pop-in.
                if (!sphereInCameraFrustum(camera3D, camView, camAspect,
                                           { obj.position.x, 100.0f, obj.position.y },
                                           400.0f)) continue;
                // LOD dos props pequenos. Grama tem 27 mil instancias no mundo; a
                // ~1200 delas caiam dentro do corte antigo e cada uma custava 10
                // primitivas = 12 mil draws por frame. Era isso que derrubava o FPS
                // pra ~20 (render 51ms). Props pequenos morrem cedo e simplificam.
                float d2cam = dx*dx + dy*dy;
                bool  smallProp = (obj.type == 11 || obj.type == 12 || obj.type == 13 ||
                                   obj.type == 21);
                if (smallProp && d2cam > 950.0f*950.0f) continue;
                bool  lodFar = d2cam > 560.0f*560.0f;

                float w = 64.0f, h = 64.0f;
                bool hasSprite = (sb.ready && obj.type >= 0 && obj.type < SpriteBank::NUM_SCENERY);

                // Estruturas grandes = MODELOS 3D REAIS (não billboard 2.5D).
                Model* mdl = nullptr; float mscale = 40.0f;
                switch (obj.type) {
                    case 0: mdl = &m_houseModel;    mscale = m_houseScale; break; // casa
                    case 1: mdl = &m_barracksModel; mscale = m_barracksScale; break; // celeiro
                    case 7: mdl = &m_castleModel;   mscale = m_castleScale; break; // predio
                    case 8: mdl = &m_wellModel;     mscale = m_wellScale; break; // silo
                    default: break;
                }
                if (mdl && m_modelsLoaded && mdl->meshCount > 0) {
                    float s = mscale * (obj.scale > 0.01f ? obj.scale : 1.0f);
                    // SOMBRA PROJETADA do predio: mesma malha achatada em y=0 e
                    // cisalhada pela direcao da luz (o truque usado nos personagens).
                    // E o que da profundidade a uma cidade - sem ela os predios
                    // parecem adesivos colados num chao chapado.
                    // 3 elipses concentricas deslocadas na direcao da luz. A versao
                    // anterior projetava a MALHA achatada e, num modelo caixote, saia
                    // uma LAJE PRETA retangular no chao - pior que nao ter sombra.
                    {
                        float fr = 60.0f * obj.scale;
                        switch (obj.type) {
                            case 0: fr = FIT_HOUSE    * obj.scale * 0.46f; break;
                            case 1: fr = FIT_BARRACKS * obj.scale * 0.46f; break;
                            case 7: fr = FIT_CASTLE   * obj.scale * 0.40f; break;
                            case 8: fr = FIT_WELL     * obj.scale * 0.42f; break;
                            default: break;
                        }
                        float offX = fr * 0.30f, offZ = fr * 0.22f;
                        const float rk[3] = { 1.06f, 0.78f, 0.50f };
                        const float ak[3] = { 0.10f, 0.12f, 0.14f };
                        rlDisableDepthMask();
                        for (int sI = 0; sI < 3; ++sI)
                            DrawCylinderEx({ obj.position.x + offX, 2.10f + sI*0.06f, obj.position.y + offZ },
                                           { obj.position.x + offX, 2.14f + sI*0.06f, obj.position.y + offZ },
                                           fr * rk[sI], fr * rk[sI], 18, ColorAlpha(BLACK, ak[sI]));
                        rlEnableDepthMask();
                    }
                    // OCLUSAO: a camera olha de +Z, entao construcao com y de mundo
                    // MAIOR que a do player fica na frente dele. Sumir atras de um
                    // predio e perder o proprio personagem de vista - ARPG isometrico
                    // resolve isso deixando o oclusor translucido.
                    // TINTA por bioma: o mesmo modelo lido como pedra clara em LA e
                    // como pedra queimada no inferno ja muda a cidade inteira.
                    Color zt  = structureTintFor(tilemap.biomeAtWorld(obj.position.x, obj.position.y));
                    float odx = fabsf(obj.position.x - player.position.x);
                    float odz = obj.position.y - player.position.y;
                    bool  occludes = (odz > 0.0f && odz < 620.0f && odx < 230.0f);
                    DrawModelEx(*mdl, { obj.position.x, 0.0f, obj.position.y }, { 0.0f, 1.0f, 0.0f },
                                obj.rotation * RAD2DEG, { s, s, s },
                                occludes ? ColorAlpha(zt, 0.30f) : zt);
                    w = s; h = s;
                } else {
                    // Fogueira acende luz de verdade (o bloom faz o resto)
                    if (obj.type == 22)
                        lightSystem.addTorchLight(obj.position);
                    // Props do cenario em PRIMITIVAS 3D (sem billboard "tabua de pe").
                    float sc = (obj.scale > 0.01f ? obj.scale : 1.0f);
                    float x = obj.position.x, zz = obj.position.y;
                    float H = 78.0f * sc, rr = 17.0f * sc;
                    w = rr * 2.0f; h = H;
                    switch (obj.type) {
                        case 2: { // arvore: tronco + galhos + copa em massa de volumes
                            // Antes eram 3 esferas concentricas = "bola verde no palito".
                            // Agora: tronco que afina, 3 galhos saindo dele e uma copa
                            // de 7 volumes irregulares com gradiente (claro em cima,
                            // escuro embaixo) — le como massa de folhagem, nao como bola.
                            // PALETA POR BIOMA: cemiterio/inferno/cidade fantasma tem
                            // ARVORE MORTA (sem copa, so galhos nus) — a grama verde e
                            // a arvore frondosa no inferno eram o maior delator de
                            // "mesmo mundo com outra tinta".
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            unsigned hsh = (unsigned)(x * 0.7f) * 73856093u ^ (unsigned)(zz * 0.7f) * 19349663u;
                            auto jit = [&](int k, float amp) {   // deslocamento estavel por arvore
                                return (float)(((hsh >> (k * 3)) & 15) - 7) / 7.0f * amp;
                            };
                            // altura do tronco varia: floresta com so uma altura le como
                            // stamp repetido. 0.44..0.72 do H do objeto.
                            float TH = H * (0.44f + (float)((hsh >> 5) & 15) / 15.0f * 0.28f);
                            bool dead = (cp.canopyBulk <= 0.01f);
                            Color trunkC = dead ? Color{66,58,50,255} : Color{84,58,34,255};
                            Color barkC  = dead ? Color{52,46,40,255} : Color{68,46,27,255};
                            DrawCylinderEx({x,0,zz}, {x, TH, zz}, rr*0.34f, rr*0.19f, 8, trunkC);
                            DrawCylinderEx({x,0,zz}, {x, TH*0.30f, zz}, rr*0.42f, rr*0.34f, 8, barkC); // raiz/base
                            for (int gI = 0; gI < 3; ++gI) {            // galhos
                                float ga = gI * 2.094f + jit(gI, 1.0f);
                                DrawCylinderEx({x, TH*0.72f, zz},
                                               {x + cosf(ga)*rr*0.85f, TH*1.02f, zz + sinf(ga)*rr*0.85f},
                                               rr*0.11f, rr*0.06f, 5, dead ? Color{58,50,44,255} : Color{74,52,30,255});
                            }
                            if (dead) {
                                // galhos NUS apontando pra cima: a silhueta seca e o
                                // que le "lugar morto" (cemiterio/inferno/fantasma).
                                for (int gI = 0; gI < 3; ++gI) {
                                    float ga = gI * 2.094f + jit(gI, 1.0f) + 0.7f;
                                    DrawCylinderEx({x, TH*0.88f, zz},
                                                   {x + cosf(ga)*rr*0.70f, TH*1.24f, zz + sinf(ga)*rr*0.70f},
                                                   rr*0.06f, rr*0.015f, 4, {58,50,44,255});
                                }
                                h = TH * 1.28f;
                                break;
                            }
                            struct Puff { float ox, oy, oz, r; float k; };
                            const Puff pf[7] = {
                                { 0.00f, 1.00f,  0.00f, 1.00f, 1.00f },   // topo (mais claro)
                                {-0.62f, 0.86f, -0.18f, 0.74f, 0.88f },
                                { 0.60f, 0.88f,  0.16f, 0.78f, 0.94f },
                                { 0.12f, 0.84f,  0.62f, 0.72f, 0.82f },
                                {-0.20f, 0.82f, -0.60f, 0.70f, 0.76f },
                                {-0.40f, 0.66f,  0.34f, 0.62f, 0.66f },   // base (mais escuro)
                                { 0.42f, 0.64f, -0.30f, 0.60f, 0.62f },
                            };
                            // ESPECIE por hash: 3 paletas de folhagem POR BIOMA. Um verde
                            // unico pra floresta inteira le como textura repetida, nao mata.
                            const Color CANOPY = cp.canopy[(hsh >> 17) % 3];
                            // porte e densidade tambem variam por arvore (e por bioma)
                            float bulk = (0.86f + (float)((hsh >> 11) & 15) / 15.0f * 0.34f) * cp.canopyBulk;
                            float tone = 0.84f + (float)((hsh >> 21) & 15) / 15.0f * 0.30f;
                            for (int pI = 0; pI < 7; ++pI) {
                                const Puff& q = pf[pI];
                                float k = q.k * tone;
                                Color cc = { (unsigned char)fminf(255.0f, CANOPY.r * k),
                                             (unsigned char)fminf(255.0f, CANOPY.g * k),
                                             (unsigned char)fminf(255.0f, CANOPY.b * k), 255 };
                                DrawSphereEx({ x  + (q.ox + jit(pI, 0.16f)) * rr * bulk,
                                               TH + q.oy * rr * 0.92f,
                                               zz + (q.oz + jit(pI + 4, 0.16f)) * rr * bulk },
                                             rr * q.r * bulk, 7, 7, cc);   // 7 segmentos = facetado low-poly
                            }
                            h = TH + rr * 1.9f * bulk;
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
                        case 14: { // CRIPTA / MAUSOLEU (cemiterio, catacumbas)
                            float W = 68.0f * sc, D = 54.0f * sc, Hh = 66.0f * sc;
                            Color stone = { 138, 140, 148, 255 };
                            Color dark2 = {  92,  94, 102, 255 };
                            rlPushMatrix(); rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0, 1, 0);
                            DrawCube({0, 5.0f*sc, 0}, W*1.18f, 10.0f*sc, D*1.18f, dark2);
                            DrawCube({0, Hh*0.5f, 0}, W, Hh, D, stone);
                            DrawCylinderEx({-W*0.5f, Hh, 0}, {W*0.5f, Hh, 0}, D*0.62f, D*0.62f, 3, dark2);
                            DrawCube({0, Hh*0.34f, -D*0.52f}, W*0.34f, Hh*0.62f, 4.0f*sc, {52,54,60,255});
                            for (int cI = 0; cI < 2; ++cI)
                                DrawCylinderEx({ (cI?1:-1)*W*0.40f, 0.0f, -D*0.5f },
                                               { (cI?1:-1)*W*0.40f, Hh*0.92f, -D*0.5f },
                                               6.0f*sc, 5.0f*sc, 7, stone);
                            rlPopMatrix();
                            w = W * 1.3f; h = Hh + D*0.6f;
                        } break;
                        case 15: { // BUNKER de concreto (bunker, forja, cidade fantasma)
                            float W = 96.0f * sc, D = 78.0f * sc, Hh = 42.0f * sc;
                            Color conc  = { 120, 124, 118, 255 };
                            Color dark2 = {  78,  82,  78, 255 };
                            Color slit  = {  30,  32,  30, 255 };
                            // OCLUSAO: mesma regra do predio moderno (janela pela
                            // pegada real) — bunker e baixo, mas largo o bastante
                            // pra esconder o heroi agachado atras dele.
                            float odxB = fabsf(x - player.position.x);
                            float odzB = zz - player.position.y;
                            bool  occludes = (odzB > -D * 0.5f &&
                                              odzB <  Hh * 1.25f + D * 0.5f &&
                                              odxB <  W * 0.55f + 40.0f);
                            if (occludes) {
                                conc  = ColorAlpha(conc,  0.30f);
                                dark2 = ColorAlpha(dark2, 0.30f);
                                slit  = ColorAlpha(slit,  0.30f);
                                rlDisableDepthMask();   // translucido nao grava profundidade
                            }
                            rlPushMatrix(); rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0, 1, 0);
                            DrawCube({0, Hh*0.5f, 0}, W, Hh, D, conc);
                            DrawCube({0, Hh + 6.0f*sc, 0}, W*0.82f, 12.0f*sc, D*0.82f, dark2);
                            DrawCube({0, Hh*0.55f, -D*0.52f}, W*0.52f, 9.0f*sc, 5.0f*sc, slit);
                            DrawCylinderEx({W*0.28f, Hh+12.0f*sc, D*0.22f},
                                           {W*0.28f, Hh+52.0f*sc, D*0.22f}, 2.4f*sc, 1.2f*sc, 5, dark2);
                            for (int sI = 0; sI < 3; ++sI)
                                DrawCube({ (sI-1)*W*0.38f, 9.0f*sc, D*0.72f },
                                         W*0.22f, 18.0f*sc, 12.0f*sc, dark2);
                            rlPopMatrix();
                            if (occludes) rlEnableDepthMask();
                            w = W * 1.2f; h = Hh + 60.0f*sc;
                        } break;
                        case 16: { // ESPIRA INFERNAL (inferno, forja)
                            float R = 26.0f * sc, Hh = 190.0f * sc;
                            Color rock = { 62, 44, 42, 255 };
                            Color glow = { 226, 96, 40, 255 };
                            float t2 = (float)GetTime();
                            DrawCylinderEx({x, 0.0f, zz}, {x, Hh*0.45f, zz}, R, R*0.62f, 7, rock);
                            DrawCylinderEx({x, Hh*0.45f, zz}, {x, Hh, zz}, R*0.60f, R*0.10f, 7, rock);
                            for (int sI = 0; sI < 3; ++sI) {
                                float a = sI * 2.094f + obj.rotation;
                                DrawCylinderEx({x + cosf(a)*R*1.3f, 0.0f, zz + sinf(a)*R*1.3f},
                                               {x + cosf(a)*R*0.9f, Hh*0.34f, zz + sinf(a)*R*0.9f},
                                               R*0.34f, R*0.06f, 5, rock);
                            }
                            float pulse = 0.55f + 0.45f * sinf(t2 * 2.0f + x * 0.01f);
                            DrawSphereEx({x, Hh*0.98f, zz}, R*0.38f, 7, 7, ColorAlpha(glow, pulse));
                            DrawCylinderEx({x, Hh*0.5f, zz}, {x, Hh*0.92f, zz}, R*0.20f, R*0.06f, 6,
                                           ColorAlpha(glow, 0.30f * pulse));
                            w = R * 2.6f; h = Hh;
                        } break;
                        case 17: { // MONOLITO ALIENIGENA (nexus)
                            float W = 34.0f * sc, Hh = 170.0f * sc;
                            Color body2 = { 42, 58, 74, 255 };
                            Color neon  = { 90, 220, 255, 255 };
                            float t2 = (float)GetTime();
                            rlPushMatrix(); rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0, 1, 0);
                            DrawCube({0, Hh*0.5f, 0}, W, Hh, W*0.55f, body2);
                            DrawCube({0, 6.0f*sc, 0}, W*1.6f, 12.0f*sc, W*1.2f, {32,42,54,255});
                            for (int lI = 0; lI < 4; ++lI) {
                                float k = 0.25f + lI * 0.20f;
                                float pulse = 0.35f + 0.45f * sinf(t2 * 1.6f + lI * 1.3f);
                                DrawCube({0, Hh*k, -W*0.30f}, W*0.70f, 5.0f*sc, 2.0f*sc,
                                         ColorAlpha(neon, pulse));
                            }
                            rlPopMatrix();
                            w = W * 2.0f; h = Hh;
                        } break;
                        case 18: { // CABANA DE MADEIRA (floresta, fazenda)
                            float W = 74.0f * sc, D = 62.0f * sc, Hh = 46.0f * sc;
                            Color wood  = { 104, 74, 46, 255 };
                            Color roof2 = {  74, 58, 40, 255 };
                            rlPushMatrix(); rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0, 1, 0);
                            for (int lg = 0; lg < 4; ++lg)
                                DrawCylinderEx({-W*0.5f, 9.0f*sc + lg*11.0f*sc, -D*0.5f},
                                               { W*0.5f, 9.0f*sc + lg*11.0f*sc, -D*0.5f},
                                               5.5f*sc, 5.5f*sc, 6, wood);
                            DrawCube({0, Hh*0.5f, 0}, W, Hh, D, wood);
                            DrawCylinderEx({-W*0.5f, Hh, 0}, {W*0.5f, Hh, 0}, D*0.60f, D*0.60f, 3, roof2);
                            DrawCube({0, Hh*0.34f, -D*0.52f}, W*0.28f, Hh*0.60f, 3.0f*sc, {44,32,22,255});
                            DrawCylinderEx({W*0.32f, Hh, D*0.20f}, {W*0.32f, Hh+34.0f*sc, D*0.20f},
                                           6.0f*sc, 5.0f*sc, 6, {86,84,80,255});
                            rlPopMatrix();
                            w = W * 1.3f; h = Hh + D*0.6f;
                        } break;
                        case 19: { // TORRE DE VIGIA (universal: muda a silhueta da cidade)
                            float R = 15.0f * sc, Hh = 132.0f * sc;
                            Color post = { 96, 78, 56, 255 };
                            Color top2 = { 74, 60, 44, 255 };
                            for (int lI = 0; lI < 4; ++lI) {
                                float a = lI * 1.5708f + obj.rotation;
                                DrawCylinderEx({x + cosf(a)*R*1.5f, 0.0f, zz + sinf(a)*R*1.5f},
                                               {x + cosf(a)*R*0.55f, Hh*0.78f, zz + sinf(a)*R*0.55f},
                                               4.0f*sc, 3.0f*sc, 5, post);
                            }
                            DrawCube({x, Hh*0.82f, zz}, R*3.0f, 8.0f*sc, R*3.0f, top2);
                            DrawCube({x, Hh*0.95f, zz}, R*2.6f, 20.0f*sc, R*2.6f, ColorAlpha(post, 0.85f));
                            DrawCylinderEx({x, Hh, zz}, {x, Hh + 16.0f*sc, zz}, R*2.0f, 0.5f*sc, 4, top2);
                            w = R * 3.4f; h = Hh + 20.0f*sc;
                        } break;
                        case 22: { // FOGUEIRA / barril em chamas: luz, cor e vida
                            // Blizzard amarra COR a evento e usa luz para guiar o olho.
                            // Cinza uniforme nao guia nada: a fogueira e ancora visual,
                            // ponto de referencia e o unico calor da rua.
                            unsigned hsh = (unsigned)(x * 0.8f) * 2246822519u ^ (unsigned)(zz * 0.8f) * 374761393u;
                            float t2 = (float)GetTime() + (float)(hsh & 255) * 0.01f;
                            float R  = 13.0f * sc;
                            Color drum = { 96, 72, 52, 255 };
                            // barril
                            DrawCylinderEx({ x, 0.0f, zz }, { x, R * 1.5f, zz }, R, R * 0.96f, 10, drum);
                            DrawCylinderEx({ x, R * 1.5f, zz }, { x, R * 1.56f, zz }, R * 1.06f, R * 1.06f, 10,
                                           Color{ 68, 52, 38, 255 });
                            // chama: 3 lambidas pulsando em alturas diferentes
                            for (int fI = 0; fI < 3; ++fI) {
                                float ph = t2 * (2.6f + fI * 0.7f) + fI * 2.1f;
                                float hgt = R * (1.5f + 0.9f + 0.35f * sinf(ph));
                                float wob = sinf(ph * 1.7f) * R * 0.18f;
                                Color c1 = (fI == 0) ? Color{ 255, 210, 120, 235 }
                                         : (fI == 1) ? Color{ 250, 140,  50, 205 }
                                                     : Color{ 200,  70,  30, 170 };
                                DrawCylinderEx({ x + wob * 0.3f, R * 1.5f, zz + wob * 0.2f },
                                               { x + wob,        hgt,      zz + wob * 0.6f },
                                               R * (0.62f - fI * 0.14f), R * 0.05f, 6, c1);
                            }
                            // brasa no chao + fumaca
                            DrawCylinderEx({ x, 2.0f, zz }, { x, 2.2f, zz }, R * 1.5f, R * 1.5f, 12,
                                           ColorAlpha(Color{ 255, 120, 40, 255 }, 0.14f));
                            DrawSphereEx({ x + sinf(t2) * 4.0f, R * 3.6f, zz + cosf(t2 * 0.7f) * 3.0f },
                                         R * 0.5f, 5, 5, ColorAlpha(Color{ 60, 58, 56, 255 }, 0.22f));
                            w = R * 2.4f; h = R * 3.0f;
                        } break;
                        case 21: { // ENTULHO: laje partida, viga exposta, tijolo
                            // Diablo enche o chao de destroco com volume. Chao limpo
                            // entre predios e o que fazia a cidade parecer maquete.
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            unsigned hsh = (unsigned)(x * 1.1f) * 2654435761u ^ (unsigned)(zz * 1.1f) * 668265263u;
                            float R = 26.0f * sc;
                            Color slab  = cp.slabA;
                            Color slab2 = cp.slabB;
                            Color rebar = { 122,  78,  48, 255 };
                            // monte de lajes inclinadas
                            for (int i = 0; i < (lodFar ? 2 : 5); ++i) {
                                float a  = i * 1.257f + (float)((hsh >> (i * 3)) & 7) * 0.22f;
                                float rd = R * (0.20f + (float)((hsh >> (i * 2)) & 7) / 7.0f * 0.62f);
                                float sx = R * (0.42f + (float)((hsh >> i) & 3) * 0.12f);
                                float sy = R * (0.16f + (float)((hsh >> (i + 4)) & 3) * 0.10f);
                                rlPushMatrix();
                                rlTranslatef(x + cosf(a) * rd, sy * 0.55f, zz + sinf(a) * rd);
                                rlRotatef(a * RAD2DEG, 0.0f, 1.0f, 0.0f);
                                rlRotatef(12.0f + (float)((hsh >> i) & 15), 0.0f, 0.0f, 1.0f);
                                DrawCube({ 0.0f, 0.0f, 0.0f }, sx, sy, sx * 0.72f,
                                         (i & 1) ? slab : slab2);
                                rlPopMatrix();
                            }
                            // vergalhoes tortos saindo do monte
                            for (int i = 0; i < 3; ++i) {
                                float a = i * 2.0f + (float)((hsh >> (i * 5)) & 7) * 0.3f;
                                DrawCylinderEx({ x + cosf(a) * R * 0.3f, 0.0f, zz + sinf(a) * R * 0.3f },
                                               { x + cosf(a) * R * 0.7f, R * 0.85f, zz + sinf(a) * R * 0.5f },
                                               1.2f * sc, 0.7f * sc, 4, rebar);
                            }
                            // cascalho miudo em volta
                            for (int i = 0; i < (lodFar ? 0 : 6); ++i) {
                                float a  = i * 1.047f + (float)((hsh >> (i + 2)) & 7) * 0.25f;
                                float rd = R * (0.75f + (float)((hsh >> i) & 3) * 0.16f);
                                DrawSphereEx({ x + cosf(a) * rd, 2.4f * sc, zz + sinf(a) * rd },
                                             (2.0f + (float)((hsh >> i) & 3)) * sc, 5, 5, slab2);
                            }
                            w = R * 2.2f; h = R;
                        } break;
                        case 20: { // PREDIO MODERNO (cidade: LA, fantasma, forja, nexus)
                            // Um castelo de torres nao tem o que fazer numa rua com
                            // asfalto e carro. Predio de concreto com fileiras de
                            // janela e caixa d'agua no topo: e isso que faz o lugar
                            // ler como cidade destruida, e nao como cenario medieval.
                            unsigned hsh = (unsigned)(x * 0.6f) * 374761393u ^ (unsigned)(zz * 0.6f) * 668265263u;
                            int   floors = 3 + (int)(hsh % 4);            // 3..6 andares
                            float FH     = 78.0f * sc;                    // pe-direito estilizado
                            float W      = (250.0f + (float)((hsh >> 5) & 15) * 5.0f) * sc;   // ~3,8x o heroi
                            float D      = (220.0f + (float)((hsh >> 9) & 15) * 4.0f) * sc;
                            float Hh     = FH * floors;
                            // OCLUSAO: mesma regra do branch de modelos .obj, mas com a
                            // janela baseada na PEGADA REAL do predio (a fixa 620/230
                            // nao escala com W/D/Hh). Player atras da massa (entre o
                            // predio e a camera, que olha de +Z) => predio translucido;
                            // senao o heroi some 100% atras dele no meio do combate
                            // (auditoria 2, P1 — oclusor dominante da fase urbana).
                            float odxB = fabsf(x - player.position.x);
                            float odzB = zz - player.position.y;
                            bool  occludes = (odzB > -D * 0.5f &&
                                              odzB <  Hh * 1.25f + D * 0.5f &&
                                              odxB <  W * 0.55f + 60.0f);
                            // PALETA por bioma (nao cinza universal): concreto quente
                            // e ocre em LA, concreto frio na cidade fantasma, metal
                            // queimado na forja. Cor amarrada ao lugar, como no D3.
                            // MATERIAL por predio, nao um bege universal com +-15 de
                            // variacao (era isso que fazia a cidade inteira ter a mesma
                            // cor). Cada predio sorteia um material real da rua.
                            ZoneID pz = tilemap.biomeAtWorld(x, zz);
                            const Color MAT_CITY[6] = {
                                { 148, 132, 104, 255 },   // reboco ocre
                                { 122, 118, 112, 255 },   // concreto cinza
                                { 132,  80,  62, 255 },   // tijolo vermelho
                                {  86, 104, 118, 255 },   // torre de vidro azulada
                                { 108,  96,  84, 255 },   // concreto sujo
                                {  74,  78,  84, 255 },   // aco escuro
                            };
                            const Color MAT_GHOST[6] = {
                                {  96, 104, 116, 255 }, {  78,  88, 100, 255 },
                                { 110, 112, 118, 255 }, {  70,  84,  96, 255 },
                                {  92,  90,  96, 255 }, {  62,  70,  80, 255 },
                            };
                            const Color MAT_FORGE[6] = {
                                { 128,  96,  74, 255 }, { 146, 110,  70, 255 },
                                { 104,  84,  70, 255 }, { 118,  78,  56, 255 },
                                {  96,  86,  78, 255 }, { 140, 120,  86, 255 },
                            };
                            const Color* MAT = (pz == ZoneID::GhostCity)   ? MAT_GHOST
                                             : (pz == ZoneID::KronosForge) ? MAT_FORGE
                                             : MAT_CITY;
                            Color base2 = MAT[(hsh >> 17) % 6];
                            float wear  = 0.86f + (float)((hsh >> 21) & 15) / 15.0f * 0.28f;
                            Color conc  = { (unsigned char)fminf(255.0f, base2.r * wear),
                                            (unsigned char)fminf(255.0f, base2.g * wear),
                                            (unsigned char)fminf(255.0f, base2.b * wear), 255 };
                            Color dark2  = { (unsigned char)(conc.r * 0.72f),
                                             (unsigned char)(conc.g * 0.72f),
                                             (unsigned char)(conc.b * 0.72f), 255 };
                            Color win    = { 44, 54, 64, 255 };
                            bool  lit    = (obj.tint.r > 128);            // predio com luz acesa
                            if (occludes) {   // alpha 0.30: o heroi le atraves da massa
                                conc  = ColorAlpha(conc,  0.30f);
                                dark2 = ColorAlpha(dark2, 0.30f);
                                win   = ColorAlpha(win,   0.30f);
                            }

                            if (occludes) rlDisableDepthMask();   // translucido NAO grava profundidade: o heroi atras continua desenhando
                            rlPushMatrix();
                            rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0.0f, 1.0f, 0.0f);
                            DrawCube({0.0f, 3.0f * sc, 0.0f}, W * 1.14f, 6.0f * sc, D * 1.14f, dark2); // calcada/base
                            DrawCube({0.0f, Hh * 0.5f, 0.0f}, W, Hh, D, conc);                          // massa
                            // fileiras de janela nas 4 faces (faixa por andar)
                            for (int f = 0; f < floors; ++f) {
                                float wy = FH * (f + 0.62f);
                                Color wc = win;
                                if (lit && ((hsh >> f) & 3) == 0) wc = Color{ 226, 198, 128, conc.a };
                                DrawCube({0.0f, wy, -D * 0.51f}, W * 0.76f, FH * 0.34f, 1.5f * sc, wc);
                                DrawCube({0.0f, wy,  D * 0.51f}, W * 0.76f, FH * 0.34f, 1.5f * sc, wc);
                                DrawCube({-W * 0.51f, wy, 0.0f}, 1.5f * sc, FH * 0.34f, D * 0.76f, wc);
                                DrawCube({ W * 0.51f, wy, 0.0f}, 1.5f * sc, FH * 0.34f, D * 0.76f, wc);
                            }
                            // RUINA: 40% dos predios perdem o topo. Silhueta quebrada
                            // e o que separa "cidade destruida" de "conjunto habitacional".
                            bool ruined = ((hsh >> 12) % 100) < 40;
                            if (ruined) {
                                // laje partida: 3 pedacos irregulares no lugar do topo
                                for (int rI = 0; rI < 3; ++rI) {
                                    float rw2 = W * (0.24f + (float)((hsh >> (rI * 3)) & 7) / 7.0f * 0.30f);
                                    float rh2 = FH * (0.30f + (float)((hsh >> rI) & 3) * 0.22f);
                                    float rx2 = ((float)((hsh >> (rI * 4)) & 15) / 15.0f - 0.5f) * W * 0.6f;
                                    float rz2 = ((float)((hsh >> (rI * 2)) & 15) / 15.0f - 0.5f) * D * 0.6f;
                                    DrawCube({ rx2, Hh + rh2 * 0.5f, rz2 }, rw2, rh2, rw2 * 0.8f, conc);
                                }
                                // buraco na laje: da pra ver o andar de baixo
                                DrawCube({ W * 0.10f, Hh - FH * 0.42f, -D * 0.10f },
                                         W * 0.44f, FH * 0.10f, D * 0.44f,
                                         Color{ (unsigned char)(conc.r * 0.42f),
                                                (unsigned char)(conc.g * 0.42f),
                                                (unsigned char)(conc.b * 0.46f), conc.a });
                                for (int pI = 0; pI < 3; ++pI)   // laje partida na borda do buraco
                                    DrawCube({ W * (0.10f + (pI - 1) * 0.20f), Hh + 2.0f * sc,
                                               -D * (0.10f + (pI - 1) * 0.16f) },
                                             W * 0.16f, 5.0f * sc, D * 0.16f, dark2);
                                // vigas expostas
                                for (int rI = 0; rI < 2; ++rI)
                                    DrawCylinderEx({ W * (rI ? 0.3f : -0.3f), Hh, D * 0.2f },
                                                   { W * (rI ? 0.42f : -0.42f), Hh + FH * 0.7f, D * 0.1f },
                                                   1.6f * sc, 1.0f * sc, 4, Color{ 122, 78, 48, conc.a });
                            } else {
                                // ── LAJE COMPLETA ────────────────────────────────
                                Color roofC = { (unsigned char)(conc.r * 0.80f),
                                                (unsigned char)(conc.g * 0.82f),
                                                (unsigned char)(conc.b * 0.86f), conc.a };
                                DrawCube({0.0f, Hh + 3.0f * sc, 0.0f}, W * 1.02f, 6.0f * sc, D * 1.02f, roofC);
                                // platibanda (mureta) nas 4 bordas: da espessura ao topo
                                float pb = 9.0f * sc;
                                DrawCube({0.0f, Hh + pb * 0.5f + 6.0f * sc, -D * 0.51f}, W * 1.06f, pb, 6.0f * sc, dark2);
                                DrawCube({0.0f, Hh + pb * 0.5f + 6.0f * sc,  D * 0.51f}, W * 1.06f, pb, 6.0f * sc, dark2);
                                DrawCube({-W * 0.51f, Hh + pb * 0.5f + 6.0f * sc, 0.0f}, 6.0f * sc, pb, D * 1.06f, dark2);
                                DrawCube({ W * 0.51f, Hh + pb * 0.5f + 6.0f * sc, 0.0f}, 6.0f * sc, pb, D * 1.06f, dark2);
                                float ry2 = Hh + 8.0f * sc;
                                // caixa d'agua sobre pes
                                float tkx = W * 0.24f, tkz = -D * 0.20f, tkr = 17.0f * sc;
                                for (int lI = 0; lI < 4; ++lI)
                                    DrawCylinderEx({ tkx + ((lI & 1) ? tkr*0.6f : -tkr*0.6f), ry2,
                                                     tkz + ((lI & 2) ? tkr*0.6f : -tkr*0.6f) },
                                                   { tkx + ((lI & 1) ? tkr*0.6f : -tkr*0.6f), ry2 + 14.0f*sc,
                                                     tkz + ((lI & 2) ? tkr*0.6f : -tkr*0.6f) },
                                                   2.0f*sc, 2.0f*sc, 4, dark2);
                                DrawCylinderEx({ tkx, ry2 + 14.0f*sc, tkz }, { tkx, ry2 + 40.0f*sc, tkz },
                                               tkr, tkr * 0.96f, 10, Color{ 128, 118, 104, conc.a });
                                DrawCylinderEx({ tkx, ry2 + 40.0f*sc, tkz }, { tkx, ry2 + 44.0f*sc, tkz },
                                               tkr * 1.08f, tkr * 0.5f, 10, dark2);
                                // casa de maquinas / saida de escada
                                DrawCube({ -W * 0.26f, ry2 + 15.0f * sc, D * 0.22f },
                                         W * 0.26f, 30.0f * sc, D * 0.24f, conc);
                                DrawCube({ -W * 0.26f, ry2 + 31.0f * sc, D * 0.22f },
                                         W * 0.28f, 4.0f * sc, D * 0.26f, dark2);
                                // condensadoras (caixas de ar-condicionado)
                                for (int aI = 0; aI < 3; ++aI) {
                                    float ax = (-0.30f + aI * 0.28f) * W;
                                    DrawCube({ ax, ry2 + 6.0f * sc, -D * 0.30f },
                                             20.0f * sc, 12.0f * sc, 16.0f * sc, Color{ 116, 116, 112, conc.a });
                                    DrawCylinderEx({ ax, ry2 + 12.0f * sc, -D * 0.30f },
                                                   { ax, ry2 + 14.0f * sc, -D * 0.30f },
                                                   6.0f * sc, 6.0f * sc, 8, dark2);
                                }
                                // dutos correndo pela laje
                                DrawCylinderEx({ -W * 0.34f, ry2 + 4.0f * sc, -D * 0.06f },
                                               {  W * 0.30f, ry2 + 4.0f * sc, -D * 0.06f },
                                               3.4f * sc, 3.4f * sc, 6, Color{ 104, 100, 94, conc.a });
                                // entulho e manchas na laje
                                for (int dI = 0; dI < 4; ++dI) {
                                    float dx2 = ((float)((hsh >> (dI * 3)) & 15) / 15.0f - 0.5f) * W * 0.8f;
                                    float dz2 = ((float)((hsh >> (dI * 2)) & 15) / 15.0f - 0.5f) * D * 0.8f;
                                    DrawCube({ dx2, ry2 + 2.0f * sc, dz2 },
                                             (8.0f + (float)((hsh >> dI) & 7)) * sc, 4.0f * sc,
                                             (7.0f + (float)((hsh >> dI) & 5)) * sc, dark2);
                                }
                            }
                            DrawCylinderEx({ W * 0.22f, Hh + 8.0f * sc,  D * 0.20f },
                                           { W * 0.22f, Hh + 30.0f * sc, D * 0.20f },
                                           9.0f * sc, 9.0f * sc, 8, dark2);
                            DrawCylinderEx({ -W * 0.26f, Hh + 8.0f * sc, -D * 0.24f },
                                           { -W * 0.26f, Hh + 46.0f * sc, -D * 0.24f },
                                           1.8f * sc, 1.0f * sc, 5, dark2);
                            rlPopMatrix();
                            if (occludes) rlEnableDepthMask();
                            w = W * 1.2f; h = Hh + 46.0f * sc;
                        } break;
                        case 6: { // VEICULO: carro / van / caminhao
                            // A primeira versao era literalmente caixa sobre caixa.
                            // Carro nao le por volume, le por SILHUETA: capo baixo,
                            // para-brisa inclinado, teto curto e recuado, para-lamas
                            // salientes sobre as rodas. E isso que esta montado aqui.
                            unsigned hsh = (unsigned)(x * 0.5f) * 2246822519u ^ (unsigned)(zz * 0.5f) * 3266489917u;
                            int   kind = (int)(hsh % 3);                 // 0 carro 1 van 2 caminhao
                            // 4,2 m de carro = 158u nesta regua; caminhao 7 m = 264u.
                            float L    = (kind == 2 ? 264.0f : kind == 1 ? 196.0f : 158.0f) * sc;
                            float WD   = (kind == 2 ?  84.0f :  68.0f) * sc;
                            float wr   = 14.0f * sc;                     // pneu ~0,37 m
                            const Color PAL[6] = { {168, 74, 62,255}, { 84,112,152,255},
                                                   {138,140,146,255}, {110,124, 92,255},
                                                   {176,158,104,255}, {104,106,112,255} };
                            Color body = PAL[(hsh >> 7) % 6];
                            float rust = 0.88f + (float)((hsh >> 13) & 7) / 7.0f * 0.16f;
                            body = { (unsigned char)(body.r * rust), (unsigned char)(body.g * rust),
                                     (unsigned char)(body.b * rust), 255 };
                            Color dark  = { (unsigned char)(body.r * 0.62f), (unsigned char)(body.g * 0.62f),
                                            (unsigned char)(body.b * 0.62f), 255 };
                            Color glass = { 62, 78, 92, 255 };
                            Color tire  = { 26, 26, 28, 255 };
                            Color chrome= { 172, 176, 184, 255 };

                            rlPushMatrix();
                            rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0.0f, 1.0f, 0.0f);

                            float sill = wr + 5.0f * sc;          // linha da soleira
                            float bodyH = 15.0f * sc;             // altura da lataria
                            float beltY = sill + bodyH;           // linha da cintura (base do vidro)

                            // ── lataria: 3 secoes com larguras diferentes = ombro ──
                            DrawCube({ 0.0f, sill + bodyH * 0.5f, 0.0f }, L * 0.74f, bodyH, WD, body);
                            DrawCube({ -L * 0.40f, sill + bodyH * 0.42f, 0.0f },
                                     L * 0.22f, bodyH * 0.82f, WD * 0.90f, body);          // traseira afunilada
                            DrawCube({  L * 0.40f, sill + bodyH * 0.40f, 0.0f },
                                     L * 0.22f, bodyH * 0.78f, WD * 0.88f, body);          // capo afunilado

                            // ── cabine: teto recuado + vidros inclinados ──
                            if (kind == 2) {   // caminhao: cabine na frente, bau atras
                                DrawCube({ -L * 0.30f, beltY + 20.0f * sc, 0.0f },
                                         L * 0.34f, 40.0f * sc, WD * 0.96f, body);         // bau
                                DrawCube({  L * 0.26f, beltY + 13.0f * sc, 0.0f },
                                         L * 0.26f, 26.0f * sc, WD * 0.86f, dark);         // cabine
                                DrawCube({  L * 0.34f, beltY + 15.0f * sc, 0.0f },
                                         L * 0.10f, 17.0f * sc, WD * 0.78f, glass);        // para-brisa
                            } else {
                                float roofH = (kind == 1 ? 26.0f : 18.0f) * sc;
                                DrawCube({ -L * 0.04f, beltY + roofH * 0.5f, 0.0f },
                                         L * 0.40f, roofH, WD * 0.80f, dark);              // teto
                                // vidros: faixa continua um pouco mais estreita que o teto
                                DrawCube({ -L * 0.04f, beltY + roofH * 0.62f, 0.0f },
                                         L * 0.36f, roofH * 0.46f, WD * 0.84f, glass);
                                // para-brisa e vidro traseiro INCLINADOS (rotacao em Z)
                                rlPushMatrix();
                                rlTranslatef(L * 0.19f, beltY + roofH * 0.42f, 0.0f);
                                rlRotatef(-32.0f, 0.0f, 0.0f, 1.0f);
                                DrawCube({ 0.0f, 0.0f, 0.0f }, L * 0.10f, roofH * 0.95f, WD * 0.80f, glass);
                                rlPopMatrix();
                                rlPushMatrix();
                                rlTranslatef(-L * 0.26f, beltY + roofH * 0.40f, 0.0f);
                                rlRotatef(30.0f, 0.0f, 0.0f, 1.0f);
                                DrawCube({ 0.0f, 0.0f, 0.0f }, L * 0.09f, roofH * 0.85f, WD * 0.78f, glass);
                                rlPopMatrix();
                            }

                            // ── para-lamas: arcos salientes sobre cada roda ──
                            for (int wI = 0; wI < 4; ++wI) {
                                float wx = (wI < 2 ? -L * 0.30f : L * 0.30f);
                                float wz = ((wI & 1) ? -WD * 0.5f : WD * 0.5f);
                                DrawCylinderEx({ wx, sill * 0.92f, wz - 3.0f * sc },
                                               { wx, sill * 0.92f, wz + 3.0f * sc },
                                               wr * 1.45f, wr * 1.45f, 10, dark);
                                DrawCylinderEx({ wx, wr, wz - 5.0f * sc }, { wx, wr, wz + 5.0f * sc },
                                               wr, wr, 10, tire);
                                DrawCylinderEx({ wx, wr, wz - 5.6f * sc }, { wx, wr, wz + 5.6f * sc },
                                               wr * 0.42f, wr * 0.42f, 8, chrome);          // calota
                            }

                            // ── faroies, lanternas e para-choques ──
                            for (int sI = 0; sI < 2; ++sI) {
                                float sz2 = (sI ? 1.0f : -1.0f) * WD * 0.30f;
                                DrawSphereEx({ L * 0.49f, sill + bodyH * 0.55f, sz2 }, 3.4f * sc, 6, 6,
                                             Color{ 236, 226, 190, 255 });
                                DrawCube({ -L * 0.49f, sill + bodyH * 0.55f, sz2 },
                                         2.5f * sc, 5.0f * sc, 8.0f * sc, Color{ 168, 46, 40, 255 });
                            }
                            DrawCube({  L * 0.50f, sill + bodyH * 0.18f, 0.0f },
                                     4.0f * sc, 5.0f * sc, WD * 0.92f, chrome);
                            DrawCube({ -L * 0.50f, sill + bodyH * 0.18f, 0.0f },
                                     4.0f * sc, 5.0f * sc, WD * 0.92f, chrome);

                            rlPopMatrix();
                            w = L; h = beltY + 40.0f * sc;
                        } break;
                        case 13: { // MARCAS NO CHAO: riscos, trilhas e cascalho
                            // A textura do piso tem 128px por tile de 64u: nessa
                            // distancia de camera o mipmap come o detalhe e o chao
                            // vira cinza chapado. Marcas em ESCALA DE MUNDO resolvem.
                            // NADA de disco escuro grande: circulo cheio no chao le
                            // como buraco/cratera, nao como sujeira.
                            if (lodFar) break;   // marca de chao so aparece perto
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            unsigned hsh = (unsigned)(x * 0.9f) * 2654435761u ^ (unsigned)(zz * 0.9f) * 2246822519u;
                            float R = 17.0f * sc;
                            rlDisableDepthMask();
                            if (hsh & 1) {                      // riscos/trilhas finas
                                for (int dI = 0; dI < 3; ++dI) {
                                    float a   = obj.rotation + dI * 0.22f;
                                    float len = R * (0.9f + (float)((hsh >> (dI*5)) & 7) / 7.0f * 0.5f);
                                    float off = (dI - 1) * R * 0.28f;
                                    float ox  = cosf(a) * len, oz = sinf(a) * len;
                                    float px2 = x - sinf(a) * off, pz2 = zz + cosf(a) * off;
                                    DrawCylinderEx({ px2-ox, 1.80f, pz2-oz }, { px2+ox, 1.83f, pz2+oz },
                                                   1.4f*sc, 0.7f*sc, 4, ColorAlpha(Color{16,15,14,255}, 0.13f));
                                }
                            } else {                            // cascalho: pontos CLAROS, nao mancha escura
                                for (int dI = 0; dI < 7; ++dI) {
                                    float a  = dI * 0.897f + (float)((hsh >> (dI*3)) & 7) * 0.24f;
                                    float rd = R * (0.25f + (float)((hsh >> (dI*2)) & 7) / 7.0f * 0.85f);
                                    float ox = cosf(a) * rd, oz = sinf(a) * rd;
                                    float pr = (1.6f + (float)((hsh >> dI) & 3) * 0.7f) * sc;
                                    DrawCylinderEx({ x+ox, 1.80f, zz+oz }, { x+ox, 1.84f, zz+oz },
                                                   pr, pr, 6, ColorAlpha(cp.gravel, 0.16f));
                                }
                            }
                            rlEnableDepthMask();
                            w = R * 2.0f; h = 0.0f;
                        } break;
                        case 11: { // grama: tufo denso, alturas/tons variados, ancorado no chao
                            // Antes: 5 laminas iguais, mesma altura, mesma cor, mesmo balanco —
                            // lia como espetinhos 2D flutuando. Agora o tufo tem base escura
                            // no chao, 9 laminas com altura/tom/fase proprios.
                            // COR POR BIOMA: grama verde no inferno era o maior delator de
                            // "mesmo mundo". Seca em LA/fantasma, queimada no inferno,
                            // cinza-purpura nas catacumbas, cyan no nexus.
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            float gh = 9.0f * sc, gr = 8.0f * sc;   // ~25% da altura do heroi (era 2x!)
                            float t  = (float)GetTime();
                            unsigned hsh = (unsigned)(x * 1.3f) * 374761393u ^ (unsigned)(zz * 1.3f) * 668265263u;
                            // mancha de terra/raiz: cola o tufo no piso (sem ela ele "flutua")
                            if (!lodFar)
                                DrawCylinderEx({ x, 0.13f, zz }, { x, 0.16f, zz }, gr*0.95f, gr*0.95f, 9,
                                               ColorAlpha(cp.grassDirt, 0.55f));
                            int blades = lodFar ? 4 : 9;   // LOD: longe nao da pra ver 9 laminas
                            for (int bld = 0; bld < blades; ++bld) {
                                float u  = (float)((hsh >> (bld * 2)) & 31) / 31.0f;   // 0..1 estavel
                                float a  = bld * 0.698f + u * 0.9f;
                                float rad = gr * (0.18f + u * 0.62f);
                                float ox = cosf(a) * rad, oz = sinf(a) * rad;
                                float bh = gh * (0.55f + u * 0.65f);                   // alturas diferentes
                                float sway = sinf(t * 1.7f + x * 0.05f + bld * 0.8f) * bh * 0.26f;
                                float k = 0.62f + u * 0.55f;                           // tons diferentes
                                Color gc = { (unsigned char)(cp.grass.r * k), (unsigned char)(cp.grass.g * k),
                                             (unsigned char)(cp.grass.b * k), 255 };
                                DrawCylinderEx({ x+ox, 0.0f, zz+oz },
                                               { x+ox+sway, bh, zz+oz+sway*0.35f },
                                               gr*0.085f, gr*0.012f, 3, gc);
                            }
                            w = gr * 2.0f; h = gh;
                        } break;
                        case 12: { // pedras/detritos: cluster facetado com tons variados
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            float pr = 7.0f * sc;
                            unsigned hsh = (unsigned)(x) * 2654435761u ^ (unsigned)(zz) * 40503u;
                            const float px2[5] = { 0.0f,  0.62f, -0.55f,  0.30f, -0.28f };
                            const float pz2[5] = { 0.0f,  0.30f,  0.22f, -0.58f, -0.40f };
                            const float ps [5] = { 0.68f, 0.44f,  0.40f,  0.32f,  0.26f };
                            for (int i = 0; i < (lodFar ? 2 : 5); ++i) {
                                float k = 0.74f + (float)((hsh >> (i * 3)) & 7) / 7.0f * 0.44f;
                                Color rc = { (unsigned char)(cp.rock.r * k), (unsigned char)(cp.rock.g * k),
                                             (unsigned char)(cp.rock.b * k), 255 };
                                // 5 segmentos = facetas visiveis (pedra), nao bolinha lisa
                                DrawSphereEx({ x + px2[i]*pr, pr*ps[i]*0.75f, zz + pz2[i]*pr },
                                             pr*ps[i], 5, 5, rc);
                            }
                            w = pr*2.0f; h = pr;
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

                // Sombra de contato: era um RETANGULO preto chapado (borda dura,
                // formato errado). 3 discos concentricos com alpha decrescente dao
                // penumbra e assentam o objeto no chao.
                {
                    float sr = w * 0.46f;
                    if (sr > 1.0f && !(smallProp && lodFar)) {
                        const float rk[3] = { 1.00f, 0.68f, 0.40f };
                        const float ak[3] = { 0.13f, 0.15f, 0.17f };
                        for (int sI = 0; sI < 3; ++sI)
                            DrawCylinderEx({ obj.position.x, 2.20f + sI * 0.06f, obj.position.y },
                                           { obj.position.x, 2.24f + sI * 0.06f, obj.position.y },
                                           sr * rk[sI], sr * rk[sI], 14, ColorAlpha(BLACK, ak[sI]));
                    }
                }

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

        // ── BARREIRA DA FASE ──────────────────────────────────────────────────
        // So o arco proximo do jogador e desenhado: a borda inteira seriam
        // centenas de painentes fora de tela. Ela aparece de longe pra o jogador
        // entender que o mundo da fase TEM fim - e o que faz virar "fase".
        if (openWorldMode) {
            Vector2 rel = { player.position.x - safeZoneCenter.x,
                            player.position.y - safeZoneCenter.y };
            float pd = sqrtf(rel.x*rel.x + rel.y*rel.y);
            if (pd > owPhaseRadius - 1500.0f) {
                float base = atan2f(rel.y, rel.x);
                float tt   = (float)GetTime();
                for (int i = -9; i <= 9; ++i) {
                    float a  = base + i * 0.045f;
                    float bx = safeZoneCenter.x + cosf(a) * owPhaseRadius;
                    float bz = safeZoneCenter.y + sinf(a) * owPhaseRadius;
                    float pulse = 0.16f + 0.10f * sinf(tt * 2.0f + i * 0.7f);
                    // painel vertical de energia
                    DrawCylinderEx({ bx, 0.0f, bz }, { bx, 210.0f, bz }, 26.0f, 20.0f, 6,
                                   ColorAlpha(Color{ 70, 190, 255, 255 }, pulse));
                    DrawCylinderEx({ bx, 0.0f, bz }, { bx, 8.0f, bz }, 30.0f, 30.0f, 8,
                                   ColorAlpha(Color{ 140, 230, 255, 255 }, 0.35f));
                }
            }
        }

        // ── PORTAL DA FASE ────────────────────────────────────────────────────
        if (openWorldMode && owPortalOpen) {
            float px = owPortalPos.x, pz = owPortalPos.y;
            float t  = (float)GetTime();
            // base/plataforma
            DrawCylinderEx({px, 0.10f, pz}, {px, 4.0f, pz}, 62.0f, 58.0f, 24,
                           Color{38, 44, 58, 255});
            // anel girando (3 aros inclinados)
            for (int r = 0; r < 3; ++r) {
                float rr = 46.0f - r * 7.0f;
                float yy = 30.0f + r * 26.0f;
                float ph = t * (1.1f + r * 0.35f);
                for (int seg = 0; seg < 16; ++seg) {
                    float a0 = seg * 0.3927f + ph, a1 = a0 + 0.28f;
                    DrawCylinderEx({px + cosf(a0)*rr, yy + sinf(a0)*4.0f, pz + sinf(a0)*rr},
                                   {px + cosf(a1)*rr, yy + sinf(a1)*4.0f, pz + sinf(a1)*rr},
                                   3.2f, 3.2f, 5, Color{0, 220, 255, 255});
                }
            }
            // coluna de energia
            for (int c = 0; c < 5; ++c) {
                float k = 1.0f - c * 0.17f;
                float pulse = 0.6f + 0.4f * sinf(t * 3.0f + c);
                DrawCylinderEx({px, 4.0f, pz}, {px, 4.0f + 150.0f * k, pz},
                               34.0f * k, 10.0f * k, 14,
                               ColorAlpha(Color{90, 210, 255, 255}, 0.16f * pulse));
            }
            lightSystem.addLight(owPortalPos, 300.0f, 0.9f, Color{80,210,255,255}, true);
        }

        // Player — modelo voxel 3D do PRÓPRIO personagem do jogo (não genérico)
        drawVoxel(1000 + (int)player.charClass * 1000 + player.visualSignature(),
                  player.position, 0.0f, player.walkAnimTimer, player.isMoving);

        // ── PICARETA: golpe animado quando minerando um nó perto ──
        if (mineFxIdx >= 0 && mineFxIdx < (int)resourceNodes.size() && !resourceNodes[mineFxIdx].depleted) {
            const auto& mn = resourceNodes[mineFxIdx];
            Vector2 dir = { mn.position.x - player.position.x, mn.position.y - player.position.y };
            float dl = std::sqrt(dir.x*dir.x + dir.y*dir.y); if (dl < 1.0f) { dir = {1,0}; dl = 1; }
            dir.x /= dl; dir.y /= dl;
            float sw  = 1.0f - mineSwingAnim;                 // 0=erguida, 1=batendo
            float ang = 1.15f - sw * 1.65f;                   // arco: ergue (+) → desce (-)
            float L = 28.0f;
            Vector3 piv = { player.position.x + dir.x*10.0f, 22.0f, player.position.y + dir.y*10.0f };
            Vector3 tip = { piv.x + dir.x * L * cosf(ang), piv.y + L * sinf(ang), piv.z + dir.y * L * cosf(ang) };
            DrawCylinderEx(piv, tip, 1.7f, 1.4f, 6, Color{120, 78, 40, 255});           // cabo
            DrawCubeV(tip, {9.0f, 4.0f, 4.0f}, Color{180, 185, 195, 255});               // cabeça (metal)
            // Impacto no nó no momento do golpe
            if (sw > 0.7f) {
                Vector3 ip = { mn.position.x, 12.0f, mn.position.y };
                DrawSphereEx(ip, 4.0f + (sw-0.7f)*14.0f, 6, 6, ColorAlpha(Color{255,245,200,255}, 0.6f));
            }
        }

        // Culling de entidades — o caminho 2D sempre teve (inView); o 3D nao tinha
        // NENHUM, e drawVoxel desenha 2x (silhueta de sombra + modelo). Mesmos
        // limites usados pelos civis.
        auto offScreen = [&](Vector2 p) {
            return std::fabs(p.x - camera.target.x) > 1200.0f ||
                   std::fabs(p.y - camera.target.y) > 800.0f;
        };

        // NPCs — modelos 3D próprios do jogo
        for (auto& n : npcs) {
            if (offScreen(n.position)) continue;
            drawVoxel(300 + (int)n.role, n.position, 0.0f);   // NPC de posto: parado
        }

        // Civis da cidade (cada um na sua tarefa) — com culling pra perto da câmera
        for (auto& f : cityFolk) {
            if (std::fabs(f.position.x - camera.target.x) > 1200 ||
                std::fabs(f.position.y - camera.target.y) > 800) continue;
            drawVoxel(300 + f.role, f.position, 0.0f, f.walkPhase, !f.atStation);
            // Trabalhador martelando: faísca pulsante (sinal de "fazendo algo")
            if (f.job == FolkJob::Worker && f.atStation) {
                float ph = std::sin(f.work * 9.0f);
                if (ph > 0.2f) {
                    Vector3 sp = { f.position.x + 10.0f * f.facing, 30.0f + ph * 4.0f, f.position.y };
                    DrawSphereEx(sp, 2.2f, 5, 5, Color{255, 210, 90, 255});
                    DrawSphereEx(sp, 3.6f, 5, 5, ColorAlpha(Color{255,160,40,255}, 0.4f));
                }
            }
        }

        // Companheiros — modelos 3D próprios do jogo
        for (auto& c : companions) {
            if (!c.active || offScreen(c.position)) continue;
            drawVoxel(500 + (int)c.type, c.position, 0.0f, c.walkTimer, true);
        }

        // Inimigos — modelos 3D próprios do jogo (cada tipo com sua silhueta)
        for (auto& e : enemies) {
            if (offScreen(e.position)) continue;
            drawVoxel(100 + (int)e.type, e.position, 0.0f, e.walkAnimTimer, true);
        }

        // Itens — modelos 3D próprios do jogo
        for (auto& it : items) {
            if (offScreen(it.position)) continue;
            it.render3D();
        }

        // Outros jogadores (Peers)
        if (netActive) {
            static const Color cols[6] = {
                {60,120,220,255},{220,80,140,255},{150,160,175,255},
                {120,80,220,255},{180,120,255,255},{200,130,60,255}
            };
            for (const auto& p : net.peers()) {
                Color c = cols[(p.charClass >= 0 && p.charClass < 6) ? p.charClass : 0];
                DrawCylinderEx({p.x, 0.12f, p.y}, {p.x, 0.13f, p.y}, 11.0f, 11.0f, 12, ColorAlpha(BLACK, 0.34f));
                DrawCapsule({p.x, 6.0f, p.y}, {p.x, 34.0f, p.y}, 7.0f, 8, 8, c);
                DrawSphereEx({p.x, 44.0f, p.y}, 8.0f, 8, 8, c);
            }
        }

        // Animais / Vida Selvagem
        for (const auto& a : animals) {
            if (std::fabs(a.position.x - camera.target.x) > 1100 || std::fabs(a.position.y - camera.target.y) > 700) continue;
            {
                float x = a.position.x, zz = a.position.y;
                float bob = std::sin(a.animTimer * 8.0f) * 1.2f;
                DrawCylinderEx({x,0.12f,zz},{x,0.13f,zz}, 12.0f,12.0f,10, ColorAlpha(BLACK,0.30f));
                switch (a.type) {
                    case AnimalType::Deer: {
                        Color body={150,110,70,255};
                        DrawCapsule({x-9,16.0f+bob,zz},{x+9,16.0f+bob,zz}, 6.0f,8,8, body);
                        DrawSphereEx({x+12,24.0f+bob,zz}, 5.0f,7,7, body);
                        DrawCylinderEx({x+12,28.0f,zz},{x+10,36.0f,zz}, 1.2f,0.4f,5, Color{90,60,30,255});
                        DrawCylinderEx({x+14,28.0f,zz},{x+16,36.0f,zz}, 1.2f,0.4f,5, Color{90,60,30,255});
                        for(int lg=0;lg<4;++lg){float lx=x+(lg<2?-7:7),lz=zz+((lg%2)?4:-4);DrawCylinderEx({lx,0,lz},{lx,12.0f,lz},1.6f,1.6f,5,Color{110,80,50,255});}
                    } break;
                    case AnimalType::Rabbit: {
                        Color body={210,200,190,255};
                        DrawSphereEx({x,6.0f+bob,zz}, 6.0f,7,7, body);
                        DrawCapsule({x-2,10.0f,zz},{x-2,17.0f,zz}, 1.6f,5,5, body);
                        DrawCapsule({x+2,10.0f,zz},{x+2,17.0f,zz}, 1.6f,5,5, body);
                    } break;
                    case AnimalType::Boar: {
                        Color body={90,70,60,255};
                        DrawCapsule({x-10,9.0f+bob,zz},{x+8,9.0f+bob,zz}, 7.0f,8,8, body);
                        DrawSphereEx({x+12,9.0f+bob,zz}, 5.0f,7,7, body);
                        for(int lg=0;lg<4;++lg){float lx=x+(lg<2?-6:6),lz=zz+((lg%2)?4:-4);DrawCylinderEx({lx,0,lz},{lx,6.0f,lz},1.8f,1.8f,5,Color{70,55,48,255});}
                    } break;
                    case AnimalType::Wolf: {
                        Color body=a.fleeing?Color{120,120,130,255}:Color{90,95,105,255};
                        DrawCapsule({x-10,11.0f+bob,zz},{x+8,11.0f+bob,zz}, 5.0f,8,8, body);
                        DrawSphereEx({x+12,14.0f+bob,zz}, 4.5f,7,7, body);
                        for(int lg=0;lg<4;++lg){float lx=x+(lg<2?-7:7),lz=zz+((lg%2)?4:-4);DrawCylinderEx({lx,0,lz},{lx,9.0f,lz},1.5f,1.5f,5,body);}
                    } break;
                    default: {
                        float fl = std::sin(a.animTimer*12.0f)*5.0f;
                        Color body={60,60,70,255};
                        DrawSphereEx({x,34.0f+bob*2,zz}, 3.2f,6,6, body);
                        DrawCapsule({x-7,34.0f+fl,zz},{x,34.0f,zz}, 1.4f,5,5, body);
                        DrawCapsule({x+7,34.0f+fl,zz},{x,34.0f,zz}, 1.4f,5,5, body);
                    } break;
                }
            }
        }

        // Nós de Recursos Naturais
        for (int i = 0; i < (int)resourceNodes.size(); ++i) {
            const auto& n = resourceNodes[i];
            if (n.depleted) continue;
            if (std::fabs(n.position.x - camera.target.x) > 1100 || std::fabs(n.position.y - camera.target.y) > 700) continue;

            {
                float sx = (n.shake > 0.0f) ? std::sin(n.shake * 30.0f) * 2.0f : 0.0f;
                float x = n.position.x + sx, zz = n.position.y;
                Color c = resourceColor(n.type);
                DrawCylinderEx({x, 0.12f, zz}, {x, 0.14f, zz}, 14.0f, 14.0f, 12, ColorAlpha(BLACK, 0.35f));
                if (n.type == ResourceType::Wood) {
                    DrawCylinderEx({x,0,zz},{x,26.0f,zz}, 5.0f, 3.5f, 8, Color{82,56,30,255});
                    DrawSphereEx({x, 40.0f, zz}, 20.0f, 8, 8, Color{30,86,42,255});
                    DrawSphereEx({x-12, 32.0f, zz}, 13.0f, 8, 8, Color{26,72,36,255});
                    DrawSphereEx({x+12, 34.0f, zz}, 14.0f, 8, 8, Color{36,96,46,255});
                } else if (n.type == ResourceType::Stone) {
                    DrawSphereEx({x, 9.0f, zz}, 14.0f, 8, 8, Color{120,120,128,255});
                    DrawSphereEx({x-8, 6.0f, zz+5}, 9.0f, 7, 7, Color{145,145,155,255});
                    DrawSphereEx({x+8, 5.0f, zz-4}, 8.0f, 7, 7, Color{100,100,110,255});
                } else {
                    DrawSphereEx({x, 9.0f, zz}, 14.0f, 8, 8, Color{80,72,66,255});
                    DrawSphereEx({x-5, 6.0f, zz+3}, 8.0f, 7, 7, Color{96,88,80,255});
                    for (int v = 0; v < 6; ++v) { float a = v * 1.05f + i;
                        DrawSphereEx({x + cosf(a)*9.0f, 12.0f + sinf(a)*4.0f, zz + sinf(a)*9.0f}, 2.8f, 5, 5, c); }
                }
            }
        }

        // Equipamentos no chão
        for (const auto& ge : groundEquips) {
            if (ge.collected) continue;
            if (std::fabs(ge.position.x - camera.target.x) > 1100 || std::fabs(ge.position.y - camera.target.y) > 700) continue;

            {
                float pulse = 0.5f + 0.5f * std::sin(ge.pulseTimer * 4.0f);
                Color ec = ge.equip.color;
                float fade = (ge.lifetime < 5.0f) ? ge.lifetime / 5.0f : 1.0f;
                float gy = 13.0f + std::sin(ge.pulseTimer * 2.0f) * 3.0f;   // gema flutua suave
                DrawCylinderEx({ge.position.x,0.12f,ge.position.y},{ge.position.x,0.14f,ge.position.y},
                               14.0f + pulse*4.0f, 14.0f + pulse*4.0f, 16, ColorAlpha(ec, 0.30f * fade));
                DrawSphereEx({ge.position.x, gy, ge.position.y}, 9.0f, 8, 8, ColorAlpha(ec, 0.40f * fade));
                DrawSphereEx({ge.position.x, gy, ge.position.y}, 5.5f, 8, 8, ColorAlpha(ec, fade));
                DrawSphereEx({ge.position.x, gy + 1.5f, ge.position.y}, 2.5f, 6, 6, ColorAlpha(WHITE, 0.8f * fade));
            }
        }

        // Construções, Tanques e Soldados (Building System / RTS)
        // Arca/Casa: modelos MEDIEVAIS (castle.obj/house.obj) so em zona rural/
        // gotica. Em zona urbana/sci-fi a Arca vira bunker de concreto com
        // antenas e a Casa vira estrutura moderna (auditoria P1: telhado de
        // telha e torre de castelo no asfalto quebravam a cidade).
        const bool medievalZone = isMedievalZone(currentZone);
        for (const auto& b : buildingSystem.buildings) {
            if (m_modelsLoaded && b.built) {
                if (b.type == BuildingType::Ark && medievalZone && m_castleModel.meshCount > 0) {
                    DrawModelEx(m_castleModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_castleScale, m_castleScale, m_castleScale }, WHITE);
                } else if (b.type == BuildingType::Ark && !medievalZone) {
                    drawArkStructure(b.position);
                } else if (b.type == BuildingType::House && medievalZone && m_houseModel.meshCount > 0) {
                    DrawModelEx(m_houseModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_houseScale, m_houseScale, m_houseScale }, WHITE);
                } else if (b.type == BuildingType::House && !medievalZone) {
                    drawGenericStructure(b.position, 1.0f);   // casa moderna (concreto+metal)
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
                    drawGenericStructure(b.position, 1.0f);
                }
            } else {
                drawGenericStructure(b.position, 1.0f);
            }
        }
        // Tanque com escala e forma de tanque: 96u de casco contra 66u de heroi.
        // Os 28u antigos faziam o "tanque" caber embaixo do braco do personagem.
        for (const auto& t : buildingSystem.tanks) {
            if (t.isDead()) continue;
            const Color HULL = { 78, 96, 74, 255 };
            const Color TRK  = { 44, 50, 44, 255 };
            const Color TUR  = { 92, 112, 88, 255 };
            float tx = t.position.x, tz = t.position.y;
            DrawCylinderEx({tx, 0.12f, tz}, {tx, 0.13f, tz}, 46.0f, 40.0f, 16,
                           ColorAlpha(BLACK, 0.34f));
            DrawCylinderEx({tx, 0.14f, tz}, {tx, 0.15f, tz}, 20.0f, 16.0f, 14,
                           ColorAlpha(Color{60, 230, 120, 255}, 0.30f));   // marca de aliado
            for (int sI = 0; sI < 2; ++sI)                                  // esteiras
                DrawCubeV({tx, 11.0f, tz + (sI ? 30.0f : -30.0f)}, {96.0f, 22.0f, 18.0f}, TRK);
            DrawCubeV({tx, 20.0f, tz}, {92.0f, 20.0f, 62.0f}, HULL);        // casco
            DrawCubeV({tx, 34.0f, tz}, {52.0f, 18.0f, 44.0f}, TUR);         // torre
            DrawCylinderEx({tx + 20.0f, 36.0f, tz}, {tx + 74.0f, 36.0f, tz},
                           5.0f, 4.0f, 8, TRK);                              // canhao
            DrawSphereEx({tx - 14.0f, 46.0f, tz}, 5.0f, 6, 6, TUR);          // escotilha
        }
        // Aliados usam o MESMO modelo dos NPCs (soldado). Antes eram uma capsula
        // verde com uma bola bege por cabeca: um "pino" andando pelo cenario, sem
        // nenhuma relacao com o resto da arte do jogo.
        for (const auto& s : buildingSystem.soldiers) {
            if (s.isDead()) continue;
            // anel verde no chao = marca de ALIADO (o modelo e o mesmo dos NPCs)
            DrawCylinderEx({s.position.x, 0.12f, s.position.y},
                           {s.position.x, 0.13f, s.position.y}, 15.0f, 12.0f, 14,
                           ColorAlpha(Color{60, 230, 120, 255}, 0.35f));
            drawVoxel(300 + (int)NPCRole::Soldier, s.position, 0.0f,
                      (float)GetTime() * 6.0f + s.position.x * 0.05f, true);
        }

        // RTS Building Preview Ghost
        if (buildingSystem.buildModeActive) {
            Vector2 mouseWorld = mouseGround3D();
            {
                BuildingType preview = static_cast<BuildingType>(buildingSystem.selectedType); (void)preview;
                bool canPlace = true;
                for (const auto& b : buildingSystem.buildings) {
                    if (Vector2Distance(b.position, mouseWorld) < 80.f) { canPlace = false; break; }
                }
                Color pc = canPlace ? Color{0,255,100,255} : Color{255,50,50,255};
                DrawPlane({mouseWorld.x, 0.2f, mouseWorld.y}, {64.0f, 64.0f}, ColorAlpha(pc, 0.25f));
                DrawCubeWires({mouseWorld.x, 32.0f, mouseWorld.y}, 64.0f, 64.0f, 64.0f, pc);
            }
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

        // ── FX em 3D (com PROFUNDIDADE/OCLUSÃO): projéteis e feixes de loot ──
        for (auto& p : projectiles) {
            DrawSphereEx({ p.position.x, 14.0f, p.position.y }, p.radius * 0.9f, 7, 7, p.color);
            DrawSphereEx({ p.position.x, 14.0f, p.position.y }, p.radius * 1.6f, 6, 6, ColorAlpha(p.color, 0.28f));
        }
        for (auto& p : enemyProjectiles) {
            DrawSphereEx({ p.position.x, 13.0f, p.position.y }, p.radius * 0.9f, 7, 7, p.color);
            DrawSphereEx({ p.position.x, 13.0f, p.position.y }, p.radius * 1.6f, 6, 6, ColorAlpha(p.color, 0.28f));
        }
        // Feixe de luz vertical do loot (raro+) — pilar 3D que ilumina e é ocluído
        for (auto& item : items) {
            if (item.dropBeamTimer > 0.0f && item.rarity >= ItemRarity::Uncommon) {
                float beamH = 160.0f + (int)item.rarity * 70.0f;
                float rB    = 3.0f + (int)item.rarity * 1.6f;
                DrawCylinderEx({ item.position.x, 0.2f, item.position.y },
                               { item.position.x, beamH, item.position.y },
                               rB, rB * 0.35f, 10, ColorAlpha(item.rarityColor, 0.40f));
                DrawCylinderEx({ item.position.x, 0.2f, item.position.y },
                               { item.position.x, beamH * 0.9f, item.position.y },
                               rB * 0.4f, rB * 0.12f, 8, ColorAlpha(item.rarityColor, 0.85f));
            }
        }

    EndMode3D();

    // Máscara de luz/noite + vignette ANTES dos overlays — assim barras de vida,
    // nomes e prompts ficam por cima e LEGÍVEIS mesmo no escuro/noite.
    lightSystem.applyMask();
    DrawVignette(screenWidth, screenHeight);

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
            const char* lbl = TextFormat("[H] Minerar %s (%d)", resourceName(n.type), n.amount);
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

    // (Feixes de loot agora são pilares 3D dentro do BeginMode3D — com oclusão)

    // Partículas
    for (const auto& p : particles.particles) {
        if (!p.active) continue;
        Vector2 s = proj(p.position, 8.0f);
        Particle tempP = p;
        tempP.position = s;
        tempP.render();
    }

    // (Projéteis do player e dos inimigos agora são esferas 3D dentro do BeginMode3D)

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

    // ── HOVER TOOLTIP universal: passe o mouse em cima e veja O QUE É ─────────
    // (NPCs, armas/equipamentos no chão, itens/consumíveis e recursos ouro/prata/ferro).
    if (!showInventory && !shopSystem.open && !craftingSystem.open && !dialogOpen
        && !buildingSystem.buildModeActive) {
        Vector2 vm = virtualizeMousePos(GetMousePosition());
        float bestD = 1e9f; bool hov = false;
        std::string hTitle, hDesc; Color hCol = {255,255,255,255};
        auto consider = [&](Vector2 wp, float wh, float rad, std::string title, std::string desc, Color col) {
            if (Vector2Distance(wp, camera.target) > 1500.0f) return;          // cull longe
            Vector2 s = proj(wp, wh);
            float d = Vector2Distance(s, vm);
            if (d <= rad && d < bestD) { bestD = d; hTitle = std::move(title); hDesc = std::move(desc); hCol = col; hov = true; }
        };
        for (const auto& n : npcs)
            consider(n.position, 30.0f, 44.0f, n.name,
                     (n.title.empty() ? std::string("Personagem") : n.title) + "   [E] falar", n.color);
        for (const auto& ge : groundEquips) {
            if (ge.collected) continue;
            consider(ge.position, 14.0f, 34.0f, ge.equip.name,
                     ge.equip.description + "   Tier " + std::to_string(ge.equip.tier) + "   [E] equipar",
                     ge.equip.color);
        }
        for (const auto& it : items) {
            if (it.pickedUp) continue;
            consider(it.position, 10.0f, 38.0f, it.name,
                     std::string(Item::rarityToName(it.rarity)) + "   item", it.rarityColor);
        }
        for (const auto& nd : resourceNodes) {
            if (nd.depleted) continue;
            consider(nd.position, 12.0f, 34.0f, resourceName(nd.type),
                     "Recurso   x" + std::to_string(nd.amount) + "   [H] minerar (picareta)", resourceColor(nd.type));
        }
        if (hov) {
            int padX = 10, padY = 8;
            int tw = MeasureText(hTitle.c_str(), 16);
            int dw = MeasureText(hDesc.c_str(), 12);
            int boxW = (tw > dw ? tw : dw) + padX * 2;
            int boxH = 46;
            int bx = (int)vm.x + 18, by = (int)vm.y + 12;
            if (bx + boxW > screenWidth)  bx = (int)vm.x - boxW - 12;   // não sai da tela
            if (by + boxH > screenHeight) by = screenHeight - boxH - 4;
            DrawRectangleRounded({(float)bx,(float)by,(float)boxW,(float)boxH}, 0.12f, 5, ColorAlpha(Color{8,12,22,255}, 0.95f));
            DrawRectangleLinesEx({(float)bx,(float)by,(float)boxW,(float)boxH}, 1.5f, hCol);
            DrawText(hTitle.c_str(), bx + padX, by + padY, 16, hCol);
            DrawText(hDesc.c_str(),  bx + padX, by + padY + 21, 12, ColorAlpha(WHITE, 0.82f));
        }
    }

    // (máscara de luz + vignette já aplicadas logo após EndMode3D — overlays acima ficam legíveis)

    drawFloatingNumbers(true);   // dano/creditos/cura/recursos — NAO existiam no 3D

    // ── 3. Interface e HUD Final ─────────────────────────────────────────────
    drawUI();
    drawHudAndOverlays();   // recursos/ameaça + pause/levelup/evolução/loja/crafting/party (faltava no 3D!)

    EndTextureMode();
}

void Game::render() {
    // Pipeline único: 2.5D isométrico (mundo 3D voxelizado). O menu principal é
    // desenhado por drawMainMenu() diretamente no loop de run() e nunca passa por aqui.
    renderWorld3D();
}

