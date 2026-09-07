#include "Game.h"
#include "SpriteGen.h"
#include "SpriteExtrude.h"
#include "SkillTree.h"
// Only true during sprite capture for voxelization (Game::ensureVoxel →
// SpriteExtrude::CaptureToImage): entities suppress 2D shadows/text only they of the not
// become the "pedestal" in the voxel mesh. Does not select the render pipeline.
bool g_voxelCapture = false;

// ── BIOME ARCHITECTURE ───────────────────────────────────────────────────────
// Previously every phase reused the SAME 4 models (house/barn/castle/silo): only
// the floor and sky changed, only the city looked identical in LA, the cemetery,
// and hell. Now each biome has its own structure type set.
//   0 house   1 barn   7 castle/building   8 silo
//  14 crypt  15 bunker  16 infernal spire  17 monolith  18 cabin  19 tower

// Structure tint per biome: the same model reads the light stone in LA and
// scorched stone in hell, which already changes the whole city's reading.
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

// Zones where MEDIEVAL models (castle.obj / house.obj) make sense: rural /
// gothic areas. In urban and sci-fi zones (LA, ghost city, bunker, forge,
// nexus...) towered castles and tiled houses break the art direction, only
// BuildingSystem draws modern structures with primitives there (audit P1).
static bool isMedievalZone(ZoneID z) {
    return z == ZoneID::CursedFarm || z == ZoneID::DarkForest ||
           z == ZoneID::Cemetery  || z == ZoneID::AbandonedManor;
}

// ── WORLD SCALE ──────────────────────────────────────────────────────────────
// Everything is anchored to the hero: ~28 height units = 1.75 m, only 1 meter ≈ 16u.
// Old values (house 110u = 7 m at the LARGEST dimension) made buildings smaller
// than people: the city read the the mock-up and the character the the lamppost beside it.
static constexpr float FIT_HOUSE    = 175.0f;   // two-story house ~11 m
static constexpr float FIT_BARRACKS = 190.0f;   // barn / warehouse ~12 m
static constexpr float FIT_CASTLE   = 340.0f;   // building / castle ~21 m
static constexpr float FIT_TURRET   =  95.0f;
static constexpr float FIT_MARKET   = 200.0f;
static constexpr float FIT_WELL     = 130.0f;   // tall silo
static constexpr float FIT_CAR      =  68.0f;   // car ~4.2 m long
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
        rlColor4ub(color.r, color.g, color.b, color.the);

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

    rlSetTexture(rlGetTextureIdDefault());   // P0: rebind white only primitives of the not leak the previous texture
}

// ─── Constructor / Destructor ────────────────────────────────────────────────

Game::Game(bool headless_, int startPhaseOverride_) {
    headless = headless_;
    startPhaseOverride = startPhaseOverride_;
    // Headless (CI without display/GPU): skip the entire graphics block of the
    // constructor (window, render textures, sprites, models and shaders) and only
    // set up data and simulation. The bot runs the same — scenario, enemies, phases,
    // collision.
    // Graphics block: resizable window — content (1280x720) is scaled with
    // letterboxing in presentFrame(), only nothing is cropped. F11 toggles fullscreen.
    if (!headless) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "DARKNET - ARPG Futurista | Guerra contra KRONOS");
    SetExitKey(KEY_NULL);   // ESC does NOT close the game — opens the pause menu
    SetTargetFPS(60);

    // Start in the WINDOW that fits the usable monitor area (avoids being larger
    // than the screen and getting cropped). Scales down while keeping the aspect
    // ratio if the monitor is small.
    {
        // Open on the RIGHTMOST monitor (where the Antigravity sits). Find the
        // monitor with the largest virtual X.
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
    gameTarget = GfxRenderTexture(LoadRenderTexture(screenWidth, screenHeight));
    // POINT (nearest) keeps the text sharp when scaling to fullscreen (BILINEAR blurred it).
    SetTextureFilter(gameTarget.get().texture, TEXTURE_FILTER_POINT);
    tempEntityTarget = GfxRenderTexture(LoadRenderTexture(128, 128));
    SetTextureFilter(tempEntityTarget.get().texture, TEXTURE_FILTER_POINT);
    initPostFX();       // bloom + tonemap
    initWorldShader();  // directional light + rim + fog on 3D models
    {   // 1x1 white texture for vertex-color materials (character voxels)
        Image white = GenImageColor(1, 1, WHITE);
        m_whiteTex = GfxTexture(LoadTextureFromImage(white));
        UnloadImage(white);
    }
    lightSystem.init(screenWidth, screenHeight);

    SpriteBank::get().init();   // generate pixel-art sprites (needs GL context)

    // Load 3D models for real graphics
    if (FileExists("resources/models/house.obj")) {
        m_houseModel = GfxModel(LoadModel("resources/models/house.obj"));
        m_houseTex = GfxTexture(LoadTexture("resources/models/house_diffuse.png"));
        m_houseModel.get().materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_houseTex.get();
    }
    if (FileExists("resources/models/turret.obj")) {
        m_turretModel = GfxModel(LoadModel("resources/models/turret.obj"));
        m_turretTex = GfxTexture(LoadTexture("resources/models/turret_diffuse.png"));
        m_turretModel.get().materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_turretTex.get();
    }
    if (FileExists("resources/models/barracks.obj")) {
        m_barracksModel = GfxModel(LoadModel("resources/models/barracks.obj"));
        m_barracksTex = GfxTexture(LoadTexture("resources/models/barracks_diffuse.png"));
        m_barracksModel.get().materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_barracksTex.get();
    }
    if (FileExists("resources/models/castle.obj")) {
        m_castleModel = GfxModel(LoadModel("resources/models/castle.obj"));
        m_castleTex = GfxTexture(LoadTexture("resources/models/castle_diffuse.png"));
        m_castleModel.get().materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_castleTex.get();
    }
    if (FileExists("resources/models/market.obj")) {
        m_marketModel = GfxModel(LoadModel("resources/models/market.obj"));
        m_marketTex = GfxTexture(LoadTexture("resources/models/market_diffuse.png"));
        m_marketModel.get().materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_marketTex.get();
    }
    if (FileExists("resources/models/well.obj")) {
        m_wellModel = GfxModel(LoadModel("resources/models/well.obj"));
        m_wellTex = GfxTexture(LoadTexture("resources/models/well_diffuse.png"));
        m_wellModel.get().materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = m_wellTex.get();
    }
    if (FileExists("resources/models/old_car_new.glb")) {
        m_carModel = GfxModel(LoadModel("resources/models/old_car_new.glb"));
    }
    {
        auto _fit = [](const GfxModel& m, float target)->float {
            if (!m.valid()) return 1.0f;
            BoundingBox bb = GetModelBoundingBox(m.get());
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
    // Apply lighting to scenery models (voxels receive it when generated)
    applyWorldShader(m_houseModel);    applyWorldShader(m_barracksModel);
    applyWorldShader(m_castleModel);   applyWorldShader(m_turretModel);
    applyWorldShader(m_marketModel);   applyWorldShader(m_wellModel);
    applyWorldShader(m_carModel);
    m_modelsLoaded = true;
    }   // end of graphics block — headless does not create window/GL/models/textures

    audio.init();
    loadPhaseDefs();   // campaign comes from content/phases.txt (editable without recompiling)
    buildQuests();
    buildNPCs();
    craftingSystem.buildRecipes();
    achievements.init();

    // Open world — centered on the base. The phase must be defined
    // BEFORE regions: setupWorldRegions derives the grid from owPhaseRadius and the
    // phase biome (currentZone). With the old default radius (3000) the regions were
    // smaller than the barrier and the outer ring was populated by chunks
    // with the different density — the first phase edge looked different from the rest.
    int startPhase = (startPhaseOverride >= 0) ? startPhaseOverride : 0;
    const PhaseDef& p0 = phaseDef(startPhase);
    owPhase = startPhase; owPhaseKills = 0; owKillsAtStart = 0;
    owPhaseGoal = p0.goal; owPhaseRadius = p0.radius;
    owBossPhase = p0.boss; owBossDown = false; owPortalOpen = false;
    openWorldMode = true;
    tilemap.generateOpenWorld();
    setupWorldRegions();

    // Player starts in the center of the first region (LARuins) = SAFE ZONE
    // (defined BEFORE buildOpenWorldScenery: scenery uses safeZoneCenter and
    //  owPhaseRadius to limit the phase world — with the old center the
    //  structure came out wrong and was rebuilt later, doubling the work)
    float cx = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f;
    float cy = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
    player.position  = {cx, cy};
    safeZoneCenter   = {cx, cy};   // refuge stays at the center of the initial region
    {   // phase 1 also comes from the table (before the values only lived in code)
        currentZone   = p0.zone;
        currentRegion = p0.zone;
    }
    buildOpenWorldScenery();

    camera.offset   = {screenWidth / 2.0f, screenHeight / 2.0f};
    camera.target   = player.position;
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    // 3D camera (2.5D) — valid initial values before the first update.
    updateCamera3D();

    spawnInterval = getZoneInfo(currentZone).spawnInterval;
    if (!headless)   // background is the GPU texture (render only), the simulation does not touch it
        background.generate(currentZone, tilemap.width, tilemap.height, Tilemap::tileSize);
}

Game::~Game() {
    if (headless) {
        audio.shutdown();   // in the GL context: in the GPU/textures/models to release
        return;
    }
    // The RAII wrappers (GfxRenderTexture, GfxModel, GfxTexture, GfxShader) release
    // their resources automatically in the destructor, even if exceptions occurred.
    lightSystem.shutdown();
    SpriteBank::get().shutdown();
    audio.shutdown();
    unloadPostFX();

    CloseWindow();
}

// ─── Difficulty System ───────────────────────────────────────────────────────

const DifficultySettings& Game::getDifficulty() const {
    return DIFFICULTY_TABLE[(int)difficulty];
}

void Game::drawDifficultyScreen() const {
    BeginTextureMode(gameTarget.get());  // overlay on top of menu (in the ClearBackground)

    float t = (float)GetTime();

    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.88f));

    // Title
    const char* title = "SELECT DIFFICULTY";
    int titleW = MeasureText(title, 30);
    DrawText(title, screenWidth/2 - titleW/2 + 3, 101, 30, ColorAlpha({0,140,255,255}, 0.25f));
    DrawText(title, screenWidth/2 - titleW/2, 98, 30, Color{0,235,255,255});
    {
        int ty = 134;
        DrawLine(screenWidth/2 - 340, ty, screenWidth/2 + 340, ty, ColorAlpha({0,235,255,255}, 0.45f));
        DrawLine(screenWidth/2 - 346, ty - 5, screenWidth/2 - 346, ty + 5, ColorAlpha({0,235,255,255}, 0.6f));
        DrawLine(screenWidth/2 - 340, ty, screenWidth/2 - 346, ty + 5, ColorAlpha({0,235,255,255}, 0.8f));
        DrawLine(screenWidth/2 + 340, ty, screenWidth/2 + 346, ty + 5, ColorAlpha({0,235,255,255}, 0.8f));
        DrawRectangle(screenWidth/2 - 3, ty - 4, 6, 8, ColorAlpha({255,180,40,255}, 0.9f));
    }

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
            { "ENEMIES",    ds.enemyHPMult,   3.5f, {220,60, 60,255} },
            { "SPEED",      ds.spawnRateMult,  2.5f, {255,160,0, 255} },
            { "REWARD",     ds.dropChanceMult, 2.5f, {0, 200,100,255} },
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

        // ── "SELECTED" badge at bottom ──────────────────────────────────
        if (isSel) {
            DrawRectangle(cx+6, cy+cardH-22, cardW-12, 18,
                         ColorAlpha(ds.labelColor, 0.25f));
            const char* selTxt = "SELECTED";
            DrawText(selTxt, cx + cardW/2 - MeasureText(selTxt,11)/2,
                     cy+cardH-20, 11, ds.labelColor);
        }
    }

    // ── Instructions ───────────────────────────────────────────────────────────
    int hy = startY + cardH + 18;
    const char* h1 = "< Setas/Mouse: navegar >";
    const char* h2 = "ENTER or click: confirm";
    const char* h3 = "ESC: return";
    DrawText(h1, screenWidth/2 - MeasureText(h1,13)/2, hy,    13, ColorAlpha(WHITE, 0.55f));
    DrawText(h2, screenWidth/2 - MeasureText(h2,14)/2, hy+20, 14, ColorAlpha({0,235,255,255}, 0.85f));
    DrawText(h3, screenWidth/2 - MeasureText(h3,12)/2, hy+42, 12, ColorAlpha(WHITE, 0.38f));

    EndTextureMode();
}

// ─── Combat juice: floor decals (blood / scorched) ───────────────────
void Game::addDecal(Vector2 p, Color c, int type, float size) {
    if (decals.size() > 120) decals.erase(decals.begin());  // ceiling p/ perf
    decals.push_back({ p, c, 10.0f, 10.0f, size, type });
}

void Game::renderDecals() const {
    Vector2 cam = camera.target;
    for (const auto& d : decals) {
        if (std::fabs(d.pos.x - cam.x) > 1000 || std::fabs(d.pos.y - cam.y) > 650) continue;
        float the = (d.life / d.maxLife);   // some to the few
        if (d.type == 0) { // blood stain — irregular blotches
            DrawEllipse((int)d.pos.x, (int)d.pos.y, d.size, d.size*0.6f, ColorAlpha(d.color, 0.45f*the));
            DrawCircleV({d.pos.x - d.size*0.4f, d.pos.y + 2}, d.size*0.35f, ColorAlpha(d.color, 0.4f*the));
            DrawCircleV({d.pos.x + d.size*0.5f, d.pos.y - 1}, d.size*0.3f,  ColorAlpha(d.color, 0.35f*the));
        } else {           // scorched / spark mark — dark with ember
            DrawCircleV(d.pos, d.size*0.7f, ColorAlpha(Color{20,18,16,255}, 0.5f*the));
            DrawCircleLines((int)d.pos.x, (int)d.pos.y, d.size*0.7f, ColorAlpha(Color{255,120,30,255}, 0.3f*the));
        }
    }
}

// ─── Run / Update ────────────────────────────────────────────────────────────

void Game::runAutoTest(bool autoTest) {
    if (autoTest) {
        // Clear screenshots from previous runs: old shot_NN.png mixed with
        // the current run becomes false evidence (runs indistinguishable in the same dir).
        try {
            for (const auto& and : std::filesystem::directory_iterator(".")) {
                const std::string fn = and.path().filename().string();
                if (fn.rfind("shot_", 0) == 0 && and.path().extension() == ".png")
                    std::filesystem::remove(and.path());
            }
        } catch (...) { /* in the permission / odd directory: keep running */ }
        // Skip menu, start game immediately with bot active
        buildQuests();
        // The world was already built ONCE in the constructor (with the correct safeZoneCenter
        // center) — regenerating here would be the second discarded structure in the SCENERY log.
        setupZoneNPCs(currentZone);
        // Without this --autotest stopped at the MENU waiting for the human ENTER: the bot
        // only runs after the match starts. "Skip menu" was just the comment.
        inMainMenu = false;
        audio.stopMenuMusic();
        botController.active   = true;
        botController.autoTest = true;
        botController.testDuration = (autoTestSeconds > 0.0f) ? autoTestSeconds : 7200.0f;
        botController.addLog("=== AUTO-BOT TEST MODE ENABLED ===");
        botController.addLog("Max duration: 7200s (2h)");
        startNetwork();   // tests the WebSocket client (multiplayer)
        startStore();     // tests login + premium store catalog
    }
    run();
    // After run() exits, write report if bot was active
    if (botController.active || autoTest) {
        botController.writeReport("bot_report.txt");   // relative to CWD: works on any machine/CI
        // VALIDATION GATE: console verdict; the exit code is set by main.cpp.
        std::vector<std::string> why;
        autoTestPassed = botController.passed(&why);
        TraceLog(LOG_INFO, "VALIDATION: %s", autoTestPassed ? "PASSED" : "FAILED");
        for (const auto& w : why) TraceLog(LOG_WARNING, "VALIDATION: %s", w.c_str());
    }
}

void Game::runHeadless() {
    // CI/validation without display or GPU: runs the SAME game update() in real
    // time (real clock dt), with in the window/menu/render/screenshot. Bot-reported
    // FPS is measured by this own loop (GetFPS() stays at 0 without the window).
    auto tLast = std::chrono::steady_clock::now();
    while (!quitRequested) {
        auto tNow = std::chrono::steady_clock::now();
        float dt  = std::chrono::duration<float>(tNow - tLast).count();
        tLast     = tNow;
        if (dt <= 0.0f) {
            std::this_thread::yield();
            continue;
        }
        if (dt > 0.25f) dt = 0.25f;   // same window quarantine (load frame)
        headlessFps = 1.0f / dt;
        update(dt);
        // update() can set quitRequested (autotest finished / game ended)
    }
}

void Game::run() {
    if (headless) { runHeadless(); return; }
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
                        // New game: choose CHARACTER before beginning
                        selectingCharacter = true;
                        characterHovered   = 0;
                        drawMainMenu(); drawCharacterSelectScreen(); presentFrame(); continue;
                    } else {
                        if (hasSave) SaveManager::load(player, quests, currentZone, 0, &totalKills);
                        if (openWorldMode) {
                            // Rebuild the phase from the saved zone (see startLoadedGame).
                            owPhase = 0; owPhaseRadius = 3000.0f; owPhaseGoal = 20; owBossPhase = false;
                            for (int i = 0; i < (int)phaseDefs.size(); ++i)
                                if (phaseDefs[i].zone == currentZone) {
                                    owPhase = i; owPhaseGoal = phaseDefs[i].goal;
                                    owPhaseRadius = phaseDefs[i].radius; owBossPhase = phaseDefs[i].boss; break;
                                }
                            owPhaseKills = 0; owKillsAtStart = 0; owBossDown = false; owPortalOpen = false;
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
                        triggerPlayerSpeech("Quest started. Eliminando ameacas KRONOS.", 4.0f);
                    }
                    drawMainMenu(); presentFrame(); continue;
                }
                if (IsKeyPressed(KEY_ESCAPE)) selectingDifficulty = false;
                drawMainMenu(); drawDifficultyScreen(); presentFrame(); continue;
            }

            // ── CHARACTER SELECTION (after difficulty, in new game) ──────
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

            // Button 0 / ENTER:
            //  - with save  = CONTINUE (loads directly, WITHOUT difficulty screen)
            //  - without save  = NEW GAME (shows difficulty)
            if (IsKeyPressed(KEY_ENTER) || (mouseClicked && menuHoveredBtn == 0)) {
                if (hasSave) {
                    startLoadedGame();
                    presentFrame();
                    continue;
                } else {
                    pendingNewGame = true; selectingDifficulty = true;
                }
            }
            // New game (key N or button 1, only exists when there is the save) — shows difficulty
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
            // Renders and SKIPS input processing for this frame, otherwise the
            // same ESC would be read by the pause menu and close immediately.
            render();
            presentFrame();
            continue;
        }

        if (paused) {
            // Pause menu (9 options): Continue/Save/Difficulty/Tree/Effects/
            //                            Vozes/Restart/Menu/Leave
            const int PAUSE_OPTS = 9;
            Vector2 pm = virtualizeMousePos(GetMousePosition());
            int pcx = screenWidth / 2;
            int pby = screenHeight / 2 - 150;  // same base the drawPauseMenu
            int pbw = 340, pbh = 32, pgap = 6;
            pauseHovered = -1;
            for (int i = 0; i < PAUSE_OPTS; ++i) {
                int y = pby + i * (pbh + pgap);
                if (pm.x >= pcx - pbw/2 && pm.x <= pcx + pbw/2 &&
                    pm.y >= y && pm.y <= y + pbh) pauseHovered = i;
            }
            // Keyboard navigation
            if (IsKeyPressed(KEY_DOWN)) pauseHovered = (pauseHovered + 1 + PAUSE_OPTS) % PAUSE_OPTS;
            if (IsKeyPressed(KEY_UP))   pauseHovered = (pauseHovered - 1 + PAUSE_OPTS) % PAUSE_OPTS;

            int chosen = -1;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && pauseHovered >= 0) chosen = pauseHovered;
            if (IsKeyPressed(KEY_ENTER) && pauseHovered >= 0) chosen = pauseHovered;
            // Atalhos diretos
            if (IsKeyPressed(KEY_ESCAPE)) chosen = 0;   // ESC continuous
            if (IsKeyPressed(KEY_F5))     chosen = 1;   // F5 saves

            if (chosen == 0) {            // Continue
                paused = false;
            } else if (chosen == 1) {     // Save
                autoSave();
                showStoryBanner("GAME SAVED", "Progress saved successfully.", 2.0f);
            } else if (chosen == 2) {     // Difficulty — cycles and applies (stays paused)
                difficulty = (DifficultyLevel)(((int)difficulty + 1) % 5);
                spawnInterval = getZoneInfo(currentZone).spawnInterval / getDifficulty().spawnRateMult;
            } else if (chosen == 3) {     // Trilha sonora ON/OFF
                audio.setMusicEnabled(!audio.musicEnabled);
            } else if (chosen == 4) {     // All sounds ON/OFF (master)
                audio.setAllSoundOn(!audio.allSoundOn);
            } else if (chosen == 5) {     // Vozes/characters ON/OFF
                audio.setVoiceEnabled(!audio.voiceEnabled);
            } else if (chosen == 6) {     // Restart match
                paused = false;
                restartRun();
            } else if (chosen == 7) {     // Return to the menu main
                paused = false;
                inMainMenu = true;
            } else if (chosen == 8) {     // Leave game
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
    // Start the match from scratch after choosing difficulty and character.
    victoryReported = false;
    tutorial.init();
    tutorialRewardGiven = false;
    achievements.playerPtr = &player;
    achievements.audioPtr = &audio;
    buildQuests();
    currentZone   = ZoneID::LARuins;
    currentRegion = ZoneID::LARuins;
    if (openWorldMode) {
        // Complete phase state reset: the new world starts at phase 1 of the
        // zero (radius/biome/c1da phase). Antes owPhaseRadius ou owPhaseGoal podiam
        // left over from the previous match — "New Game" inherited an advanced phase.
        const PhaseDef& pd0 = phaseDef(0);
        owPhase = 0; owPhaseKills = 0; owKillsAtStart = 0;
        owPhaseGoal = pd0.goal; owPhaseRadius = pd0.radius;
        owBossPhase = pd0.boss; owBossDown = false; owPortalOpen = false;
        tilemap.generateOpenWorld();
        setupWorldRegions();
        float ox = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f;
        float oy = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
        player.position = {ox, oy};
        safeZoneCenter  = {ox, oy};   // BEFORE scenery (it limits by the phase barrier)
        buildOpenWorldScenery();
    } else {
        tilemap.generate(currentZone);
    }
    setupZoneNPCs(currentZone);
    spawnInterval = getZoneInfo(currentZone).spawnInterval / getDifficulty().spawnRateMult;
    inMainMenu = false; storyChapter = 1;
    // Reset the evolution engine for the new match
    threatLevel = 1; threatTimer = 0.0f; threatKillMark = 0;
    activeMutator = WorldMutator::None; mutatorTimer = 0.0f;
    totalKills = 0; enemiesKilled = 0; sessionTime = 0.0f;
    audio.stopMenuMusic(); audio.setZone(currentZone);
    showStoryBanner("CAPITULO 1: O JULGAMENTO",
        "2047 - KRONOS rules. The NEXUS is humanity's last hope.", 5.0f);
    triggerPlayerSpeech(TextFormat("%s ready for the combat.",
                        Player::className(player.charClass)), 4.0f);
    startNetwork();   // real-time multiplayer (shows other players)
    startStore();     // premium shop (login + gem catalog)
}

void Game::drawCharacterSelectScreen() const {
    BeginTextureMode(gameTarget.get());  // overlay about the menu
    float t = (float)GetTime();
    int cx = screenWidth / 2;

    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.86f));

    float tp = 0.9f + 0.1f * std::sin(t * 1.4f);
    const char* title = "ESCOLHA SEU CHARACTER";
    int tFont = 36;
    int tw = MeasureText(title, tFont);
    int txc = cx - tw/2;
    DrawText(title, txc + 3, 83, tFont, ColorAlpha({0,140,255,255}, 0.25f));
    DrawText(title, txc,     80, tFont, ColorAlpha({0,235,255,255}, tp));
    int bl = tw / 2 + 20;
    DrawLine(cx - bl, 128, cx + bl, 128, ColorAlpha({0,235,255,255}, 0.35f));
    DrawLine(cx - bl - 8, 121, cx - bl, 129, ColorAlpha({0,235,255,255}, 0.6f));
    DrawLine(cx + bl + 8, 121, cx + bl, 129, ColorAlpha({0,235,255,255}, 0.6f));
    DrawRectangle(cx + bl - 2, 121, 2, 8, ColorAlpha({255,180,40,255}, 0.9f));
    const char* sub = "Each class has its own visual, stats and style.";
    int sw = MeasureText(sub, 16);
    DrawText(sub, cx - sw/2, 136, 16, ColorAlpha({175,195,220,255}, 0.65f));

    const int total = (int)CharacterClass::COUNT;
    const int cardW = 188, cardG = 10;
    const int totalW = total*cardW + (total-1)*cardG;
    const int csx = (screenWidth - totalW) / 2;
    const int csy = 150, cardH = 300;

    // Colors per class (match the Player visual)
    const Color cardCols[6] = {
        {60,90,150,255},   // Soldier
        {180,70,120,255},  // Guerreira
        {120,130,150,255}, // Robo
        {90,60,160,255},   // Mage
        {140,60,170,255},  // Bruxa
        {150,90,40,255},   // HomemFera
    };

    for (int i = 0; i < total; ++i) {
        int bx = csx + i*(cardW+cardG);
        bool sel = (i == characterHovered);
        Color col   = cardCols[i];
        Color cardBg = sel ? Color{10,22,38,255} : Color{8,14,26,255};
        // Card (beveled panel, energy rail in class color)
        DrawRectangle(bx, csy, cardW, cardH, ColorAlpha(cardBg, sel ? 0.92f : 0.84f));
        DrawRectangle(bx, csy, 3, cardH, ColorAlpha(col, sel ? 1.0f : 0.45f));
        if (sel) { // topo pulsante
            float p = 0.5f + 0.5f * std::sin(t*4.0f);
            DrawRectangle(bx, csy, cardW, 3, ColorAlpha(Color{0,235,255,255}, p));
        }
        int cut = 8;
        Color brd = sel ? Color{0,235,255,255} : ColorAlpha(col, 0.65f);
        DrawLine(bx+cut, csy,    bx+cardW-cut, csy,    brd);
        DrawLine(bx,     csy+cut, bx,    csy+cardH-cut, brd);
        DrawLine(bx+cut, csy+cardH, bx+cardW-cut, csy+cardH, brd);
        DrawLine(bx+cardW, csy+cut, bx+cardW, csy+cardH-cut, brd);
        DrawLine(bx,     csy+cut,  bx+cut, csy,     brd);
        DrawLine(bx+cardW-cut, csy, bx+cardW, csy+cut, brd);
        DrawLine(bx,     csy+cardH-cut, bx+cut, csy+cardH, brd);
        DrawLine(bx+cardW-cut, csy+cardH, bx+cardW, csy+cardH-cut, brd);
        if (sel) { // pulso external
            float p2 = 0.45f + 0.35f * std::sin(t*3.0f);
            DrawLine(bx-2, csy-2, bx+cardW+2, csy-2, ColorAlpha(Color{0,235,255,255}, p2));
            DrawLine(bx-2, csy+cardH+2, bx+cardW+2, csy+cardH+2, ColorAlpha(Color{0,235,255,255}, p2));
        }
        // Nome
        CharacterClass cc = (CharacterClass)i;
        const char* nm = Player::className(cc);
        int nw = MeasureText(nm, 20);
        DrawText(nm, bx + cardW/2 - nw/2, csy + 12, 20, sel ? WHITE : col);

        // Avatar (class pixel-art portrait)
        int ax = bx + cardW/2, ay = csy + 108;
        SpriteBank& sb = SpriteBank::get();
        if (sb.ready && i < SpriteBank::NUM_CHAR_AVATARS) {
            Texture2D av = sb.charAvatar[i];
            float scale = 1.0f;
            float aw = av.width * scale, ah = av.height * scale;
            // class color halo behind
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
        // quebra simple
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

    {
        const char* hk = "Arrows/Mouse to choose  -  ENTER/Click to confirm  -  ESC returns";
        int hw = MeasureText(hk, 14);
        int hy = csy + cardH + 22;
        DrawRectangle(cx - hw/2 - 16, hy - 8, hw + 32, 26, ColorAlpha({8,16,34,255}, 0.85f));
        int hc = 6;
        DrawLine(cx-hw/2 - 16 + hc, hy - 8, cx+hw/2 + 16 - hc, hy - 8, ColorAlpha({0,235,255,255}, 0.5f));
        DrawLine(cx-hw/2 - 16, hy - 8 + hc, cx-hw/2 - 16, hy + 18 - hc, ColorAlpha({0,235,255,255}, 0.5f));
        DrawLine(cx-hw/2 - 16 + hc, hy + 18, cx+hw/2 + 16 - hc, hy + 18, ColorAlpha({0,235,255,255}, 0.5f));
        DrawLine(cx+hw/2 + 16, hy - 8 + hc, cx+hw/2 + 16, hy + 18 - hc, ColorAlpha({0,235,255,255}, 0.5f));
        DrawText(hk, cx - hw/2, hy - 3, 14, ColorAlpha({200,215,235,255}, 0.75f));
    }
    EndTextureMode();
}

void Game::startLoadedGame() {
    // Load the save and enter the game directly — WITHOUT difficulty screen.
    // Difficulty is saved and kept (only changes in New Game or via the pause menu).
    std::vector<std::string> buildingLines;
    if (SaveManager::exists()) SaveManager::load(player, quests, currentZone, 0, &totalKills, &buildingLines);

    // Clear residual state from the previous session before rebuilding the world.
    enemies.clear();
    items.clear();
    projectiles.clear();
    enemyProjectiles.clear();
    xpOrbs.clear();
    groundEquips.clear();
    companions.clear();
    damageNumbers.clear();
    particles.particles.clear();
    anomalySystem.portals.clear();
    anomalySystem.waveActive = false;

    // Open-world phase state is not stored in .json: rebuild owPhase/radius/meta/boss from
    // the saved ZONE, otherwise regions spawn for phase 1 (radius 5200) in the
    // phase 10 save (radius 7800) — grid smaller than the barrier, outer ring without scenery.
    {
        owPhase = 0; owPhaseRadius = 3000.0f; owPhaseGoal = 20; owBossPhase = false;
        for (int i = 0; i < (int)phaseDefs.size(); ++i) {
            if (phaseDefs[i].zone == currentZone) {
                owPhase = i; owPhaseGoal = phaseDefs[i].goal;
                owPhaseRadius = phaseDefs[i].radius; owBossPhase = phaseDefs[i].boss;
                break;
            }
        }
        owPhaseKills = 0; owKillsAtStart = 0; owBossDown = false; owPortalOpen = false;
    }
    player.unclaimedLevels = 0;   // level came from file; not the new level-up
    if (openWorldMode) {
        tilemap.generateOpenWorld();
        setupWorldRegions();
        buildOpenWorldScenery();
        currentRegion = currentZone;
    } else {
        tilemap.generate(currentZone);
    }
    setupZoneNPCs(currentZone);

    // Restore structures and allied units from the save.
    if (!buildingLines.empty()) buildingSystem.load(buildingLines);

    spawnInterval = getZoneInfo(currentZone).spawnInterval / getDifficulty().spawnRateMult;
    inMainMenu = false;
    selectingDifficulty = false;
    audio.stopMenuMusic(); audio.setZone(currentZone);
    triggerPlayerSpeech("Match loaded. Resuming the quest.", 4.0f);
    startNetwork();   // real-time multiplayer
    startStore();     // premium shop (login + gem catalog)
}

void Game::restartRun() {
    // Reset the player (the constructor reconfigures skills and base stats)
    victoryReported = false;
    player = Player();
    tutorial.init();
    tutorialRewardGiven = false;
    achievements.playerPtr = &player;
    achievements.audioPtr = &audio;

    // Clear all entities in the game
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

    // Reset progression and flags
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
    showSkillTree   = false;
    perkCursor      = 0;
    showInventory = showEquipment = showQuestLog = false;
    shopSystem.close();
    craftingSystem.open = false;
    hasTarget   = false;
    rtsDragging = false;
    rtsHasUnits = false;

    // ── Reset bot/autotest (were function-static — leaked between matches) ─
    botReportSaveTimer = 0.0f;
    botAllyTimer       = 2.0f;
    botBuildTimer      = 4.0f;
    botProduceTimer    = 8.0f;
    botUpgradeTimer    = 12.0f;
    botStipendTimer    = 0.0f;
    botBuildCycle      = 0;
    lastShot           = 0.0;
    shotN              = 0;
    botController.reset();       // telemetry, state, timers and cached route
    Companion::resetSpawnIndex(); // formation slots return to the start

    // Rebuild quests and world
    quests.clear();
    buildQuests();
    currentZone   = ZoneID::LARuins;
    currentRegion = ZoneID::LARuins;
    if (openWorldMode) {
        // Same phase reset the New Game: "match restarted" also returns
        // to phase 1 (correct radius/biome before generating scenery).
        const PhaseDef& pd0 = phaseDef(0);
        owPhase = 0; owPhaseKills = 0; owKillsAtStart = 0;
        owPhaseGoal = pd0.goal; owPhaseRadius = pd0.radius;
        owBossPhase = pd0.boss; owBossDown = false; owPortalOpen = false;
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

    showStoryBanner("MATCH RESTARTED", "A new attempt against KRONOS.", 3.0f);
    triggerPlayerSpeech("Rebooting combat systems.", 3.0f);
}


void Game::grantQuestRewards(Quest& q) {
    q.complete();
    if (q.rewardHP  > 0.0f) player.heal(q.rewardHP);
    audio.playHeal();
    if (q.rewardXP  > 0)    player.addXP(q.rewardXP);
    if (!q.rewardEquip.isEmpty()) player.equipItem(q.rewardEquip);
    particles.spawnLevelUp(player.position);
    audio.playLevelUp();
}

// ─── Input ───────────────────────────────────────────────────────────────────


// ─── Collisions ──────────────────────────────────────────────────────────────

// Single point that converts earned levels into pending points/evolutions. Applies to
// ANY XP source (orb, quest, TechChip, used inventory item), including
// those that live inside Player and Game does not see.
void Game::drawFloatingNumbers(bool project3D) const {
    for (const auto& dn : damageNumbers) {
        float alpha = std::min(dn.life / 0.45f, 1.0f);
        Color c = ColorAlpha(dn.color, alpha);
        // Smaller font only it does not clutter the screen near the character
        int fontSize = (dn.value >= 100.0f) ? 15 :
                       (dn.value >= 50.0f)  ? 13 : 11;
        const char* txt = TextFormat("%s%.0f", dn.prefix.c_str(), dn.value);
        int tw = MeasureText(txt, fontSize);
        Vector2 p;
        if (project3D) {
            // Goes up the real 3D world Y axis and only then becomes the screen coordinate.
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

    tutorial.onLeveledUp();
    achievements.onLevelUp(player.level);

    particles.spawnLevelUp(player.position);
    audio.playLevelUp();
    static const char* lvlLines[] = {
        "Estou staying more strong.",
        "Experience is the best weapon.",
        "KRONOS does not know what is coming.",
        "Combat module expanded.",
        "Capacity elevada. Quest continuous."
    };
    triggerPlayerSpeech(lvlLines[player.level % 5], 3.0f);
    // Does NOT block the game — just accumulates points and notifies the player.
    // Ele escolhe when quiser: key L (level up) / key K (evolution).
    levelUpAnimTimer   = 0.0f;
    pendingNotifyPulse = 1.0f;

    // How many of the crossed levels are evolution levels (count each; going up 2 at
    // once passing through 10 and 11 of the 1st evolution + 1 point).
    static const int EVO_LESPEEDS[] = {10, 25, 40, 60};
    int evo = 0;
    for (int l = player.level - gained + 1; l <= player.level; ++l)
        for (int el : EVO_LESPEEDS) if (l == el) { ++evo; break; }

    pendingEvolutions += evo;
    pendingLevelUps   += (gained - evo);
    if (evo > 0) triggerPlayerSpeech("EVOLUTION available! Press K to choose.", 4.0f);
}

void Game::checkCollisions() {
    // Player projectiles vs enemies
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        if (proj.isGrenade) continue; // handled in updateProjectiles on expire

        for (auto& enemy : enemies) {
            if (enemy.isDead()) continue;   // of the not waste the shot on the pending corpse
            // squared distance: avoids the sqrt per projectile-enemy pair (hot O(n*m) loop)
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

    // Item pickup — AUTO-PICKUP RADIUS + magnetism
    // Items inside the pickup radius are grabbed automatically; items inside the
    // attraction radius fly toward the player.
    const float ftime     = GetFrameTime();
    const float COLLECT_R = player.radius + 52.0f;   // coleta automatic
    const float MAGNET_R  = 230.0f;                   // atracao magnetica
    for (auto it = items.begin(); it != items.end();) {
        float d = Vector2Distance(player.position, it->position);
        // Magnetism: pulls the item toward the player when inside the attraction radius
        if (it->pickupDelay <= 0.0f && d > COLLECT_R && d < MAGNET_R) {
            Vector2 dir = Vector2Normalize(Vector2Subtract(player.position, it->position));
            float pull  = (1.0f - d / MAGNET_R) * 560.0f + 140.0f;
            it->position.x += dir.x * pull * ftime;
            it->position.y += dir.y * pull * ftime;
        }
        if (it->pickupDelay <= 0.0f && d <= COLLECT_R) {
            // REAL pickup (item leaves the vector) — the bot counted by proximity
            // (<20px) and magnetism/auto-pickup removed the item before.
            if (botController.active) botController.itemsCollected++;
            tutorial.onItemPickedUp();
            switch (it->type) {
                case ItemType::HealthPack:
    audio.playHeal();
                    player.heal(30.0f);
                    damageNumbers.push_back({it->position, 30.0f, {0,210,80,255}, 1.2f, "+"});
                    break;
                case ItemType::Credits:
                    player.credits += it->value;
                    totalCreditsEarned += it->value;
                    achievements.onCreditsEarned(totalCreditsEarned);
                    damageNumbers.push_back({it->position, (float)it->value, {255,210,0,255}, 1.4f, "$"});
                    break;
                case ItemType::TechChip:
                    player.addXP(50);
                    damageNumbers.push_back({it->position, 50.0f, {0,200,255,255}, 1.4f, "XP+"});
                    break;
                case ItemType::NanoCore:
    audio.playHeal();
                    player.increaseBaseMaxHP(25.0f);
                    player.heal(25.0f);
                    damageNumbers.push_back({it->position, 25.0f, {255,80,200,255}, 1.6f, "HP+"});
                    particles.spawnLevelUp(it->position);
                    break;
                case ItemType::PlasmaCell:
                    for (auto& s : player.skills) s.currentCooldown *= 0.3f;
                    damageNumbers.push_back({it->position, 0.0f, {180,0,255,255}, 1.2f, "CD-"});
    audio.playHeal();
                    break;
                case ItemType::ScrapMetal:
                    player.heal(8.0f);
                    player.credits += 8;
                    totalCreditsEarned += 8;
                    achievements.onCreditsEarned(totalCreditsEarned);
                    damageNumbers.push_back({it->position, 8.0f, {160,160,170,255}, 0.9f, "+"});
                    break;
                case ItemType::WeaponPart:
                    player.credits += 20;
                    totalCreditsEarned += 20;
                    achievements.onCreditsEarned(totalCreditsEarned);
                    damageNumbers.push_back({it->position, 20.0f, {255,130,0,255}, 1.1f, "$"});
                    break;
                case ItemType::EnergyCore:
                    // Grants the shield burst
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

            // Quest tracking — Collect quests advance on picking up ANY item
            // (except credits). CollectRare only advances on rare or higher.
            if (it->type != ItemType::Credits) {
                bool isRareOrBetter = static_cast<int>(it->rarity) >= static_cast<int>(ItemRarity::Rare);
                for (auto& q : quests) {
                    if (q.completed || !q.active) continue;
                    if (q.type == QuestType::Collect) {
                        q.updateProgress(1);
                    } else if (q.type == QuestType::CollectRare && isRareOrBetter) {
                        q.updateProgress(1);
                    }
                    if (q.isComplete() && !q.rewardGiven) grantQuestRewards(q);
                }
            }

            if (it->type != ItemType::Credits && static_cast<int>(it->rarity) >= static_cast<int>(ItemRarity::Rare))
                audio.playItemPickup(static_cast<int>(it->rarity));
            else
                audio.playPickup();
            it = items.erase(it);
        } else {
            ++it;
        }
    }

    // XP orbs — same pickup radius + magnetism (stronger attraction)
    for (auto it = xpOrbs.begin(); it != xpOrbs.end();) {
        float dxp = Vector2Distance(player.position, it->position);
        if (dxp > COLLECT_R && dxp < MAGNET_R + 60.0f) {
            Vector2 dir = Vector2Normalize(Vector2Subtract(player.position, it->position));
            float pull  = (1.0f - dxp / (MAGNET_R + 60.0f)) * 640.0f + 160.0f;
            it->position.x += dir.x * pull * ftime;
            it->position.y += dir.y * pull * ftime;
        }
        if (dxp <= COLLECT_R) {
            player.addXP(it->amount);   // level-up credit is consumed in drainLevelUps()
            // Pickup juice: cyan spark + XP float pop
            if (it->amount >= 3) {
                particles.spawnHit(it->position, Color{120, 240, 255, 255}, 5);
                particles.spawnExplosion(it->position, Color{90, 190, 255, 255}, 3);
            }
            damageNumbers.push_back({it->position, (float)it->amount,
                                     Color{120, 240, 255, 255}, 0.8f, "XP "});
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
    // Hard ceiling: prevents unlimited drop accumulation in the world (cause of FPS=1).
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

        bool hitWall = tilemap.isWallAtPosition(it->position, it->radius) || isOutsideOpenWorldBounds(it->position);

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
                triggerShake(6.0f, 0.25f);
                hitStopTimer = 0.07f;
                camPunch = std::max(camPunch, 0.07f);
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

        bool hitWall = tilemap.isWallAtPosition(it->position, it->radius) || isOutsideOpenWorldBounds(it->position);

        if (it->hitsPlayer(player.position, player.radius)) {
            if (!player.isShielded()) {
                player.takeDamage(it->damage);
                audio.playPlayerHurt();
                noteHurtDir(it->position);
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

void Game::noteHurtDir(Vector2 src) {
    Vector2 from = Vector2Subtract(player.position, src);
    hurtDir = Vector2LengthSqr(from) > 1.0f ? Vector2Normalize(from) : Vector2{0, 1};
    hurtDirTimer = 1.15f;
}

void Game::autoSave() {
    std::vector<std::string> buildingLines;
    buildingSystem.save(buildingLines);
    SaveManager::save(player, quests, currentZone, 0,
                      sessionTime / 60.0f, player.totalKills,
                      totalDeaths, totalBossesKilled, totalPortalsClosed,
                      (int)difficulty, totalKills, &buildingLines);
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

    // COMPACT panel sized to the text. The old version painted the black bar
    // across the FULL WIDTH of the screen: hid the whole game in the single range just to
    // show two lines of text.
    int tw = MeasureText(storyBannerText.c_str(), 20);
    int sw = MeasureText(storyBannerSub.c_str(), 12);
    int panW = (tw > sw ? tw : sw) + 40;
    int panH = storyBannerSub.empty() ? 34 : 52;
    int panX = screenWidth / 2 - panW / 2;
    int panY = screenHeight / 6;

    DrawRectangle(panX, panY, panW, panH, ColorAlpha(BLACK, 0.84f * alpha));
    DrawRectangleLinesEx({ (float)panX, (float)panY, (float)panW, (float)panH },
                         1.5f, ColorAlpha(Color{0,200,255,255}, 0.65f * alpha));

    DrawText(storyBannerText.c_str(), screenWidth/2 - tw/2 + 2, panY + 9, 20,
             ColorAlpha(BLACK, alpha * 0.8f));
    DrawText(storyBannerText.c_str(), screenWidth/2 - tw/2, panY + 7, 20,
             ColorAlpha({0,220,255,255}, alpha));
    if (!storyBannerSub.empty())
        DrawText(storyBannerSub.c_str(), screenWidth/2 - sw/2 + 1, panY + 34, 12,
                 ColorAlpha(BLACK, alpha * 0.8f));
    if (!storyBannerSub.empty())
        DrawText(storyBannerSub.c_str(), screenWidth/2 - sw/2, panY + 33, 12,
                 ColorAlpha({235,245,255,255}, alpha));
}

// ─── Render ──────────────────────────────────────────────────────────────────

// ─── 2.5D isometric (Incremento 1: camera + tilemap 3D + raycast) ───────────

void Game::updateCamera3D() {
    float z = cameraZoom * (1.0f + camPunch);
    camera3D.position   = { camera.target.x, cameraHeight * z, camera.target.y + cameraDistY * z };
    camera3D.target     = { camera.target.x, 0.0f, camera.target.y };
    camera3D.up         = { 0.0f, 1.0f, 0.0f };
    camera3D.fovy       = 30.0f;
    camera3D.projection = CAMERA_PERSPECTIVE;
}

// Casts the ray from the mouse (virtualized to the 1280x720 render texture) and intersects
// the floor plane Y=0, returning the position in 2D world coordinates (x, z).
Vector2 Game::mouseGround3D() const {
    Ray ray = GetScreenToWorldRayEx(virtualizeMousePos(GetMousePosition()),
                                    camera3D, screenWidth, screenHeight);
    // Safety: ensure the ray points DOWN and limit the range, only that
    // clicks near the horizon of the NOT generate the target at infinity (the player fired
    // far away and the infinite world collapsed — cause of the "freeze").
    float dy = ray.direction.y;
    if (dy > -0.08f) dy = -0.08f;
    float t = -ray.position.y / dy;
    if (t < 0.0f)    t = 0.0f;
    if (t > 5000.0f) t = 5000.0f;
    return { ray.position.x + ray.direction.x * t,
             ray.position.z + ray.direction.z * t };
}







void Game::render() {
    // Single pipeline: 2.5D isometric (voxelized 3D world). The main menu is
    // drawn by drawMainMenu() directly in the run() loop and never passes through here.
    renderWorld3D();
}

