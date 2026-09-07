#pragma once

#include "Player.h"
#include "Enemy.h"
#include "Item.h"
#include "Projectile.h"
#include "EnemyProjectile.h"
#include "Tilemap.h"
#include "NPC.h"
#include "Quest.h"
#include "XPOrb.h"
#include "Particle.h"
#include "AudioManager.h"
#include "SaveManager.h"
#include "Zone.h"
#include "Equipment.h"
#include "Background.h"
#include "LightSystem.h"
#include "BotController.h"
#include "EnemyDirector.h"
#include "Companion.h"
#include "BuildingSystem.h"
#include "ShopSystem.h"
#include "AnomalyPortal.h"
#include "CraftingSystem.h"
#include "DarkWorld.h"
#include "InfernoZone.h"
#include "Achievement.h"
#include "TutorialSystem.h"
#include "NetClient.h"
#include "StoreClient.h"
#include "GfxResource.h"
#include <vector>
#include <string>
#include <raylib.h>
#include <unordered_map>
#include <set>

// Equipment piece lying on the ground — requires E to pick up
struct GroundEquipment {
    Vector2     position;
    Equipment   equip;
    float       pulseTimer = 0.0f;
    float       lifetime   = 45.0f;
    bool        collected  = false;
};

struct DamageNumber {
    Vector2     pos;           // ancora IN THE WORLD (not if move; the subida goes in `rise`)
    float       value;
    Color       color;
    float       life   = 1.2f;
    // std::string, not const char*: guardar the return of TextFormat() here era bug —
    // raylib devolve pointer to um buffer static rotativo (4 slots) that and
    // sobrescrito in poucas chamadas, entao the text virava other.
    std::string prefix;        // "$" credits, "+" healing, "Wood +" resource, "" damage
    float       rise   = 0.0f; // the already went up (px in the 2D / height in the 3D)
};

// ─── Difficulty System ───────────────────────────────────────────────────────

enum class DifficultyLevel {
    Historia = 0,  // Story Mode — enemies fracos, more drops
    Resistente,    // Normal — balanceado (default)
    Guerreiro,     // Hard — more enemies fortes
    Infiltrado,    // Nightmare — very hard
    Apocalipse     // INSANE — without misericordia
};

struct DifficultySettings {
    const char* name;
    const char* description;
    Color        labelColor;
    float        enemyHPMult;
    float        enemyDmgMult;
    float        enemySpeedMult;
    float        spawnRateMult;
    float        dropChanceMult;
    float        xpMult;
    float        creditMult;
    float        bossHPMult;
};

// ─────────────────────────────────────────────────────────────────────────────

class Game {
public:
    Game(bool headless = false, int startPhaseOverride = -1);
    ~Game();
    void run();
    unsigned worldSeed    = 0;      // 0 = random; >0 = world reprodutivel
    float autoTestSeconds = 0.0f;   // >0 encerra the autotest and grava the report
    bool  autoTestPassed  = true;   // result of the portao of validation (vira exit code)
    void runAutoTest(bool autoTest);
    void runHeadless();
    bool  headless = false;          // roda SEM window/GPU (CI)
    int   startPhaseOverride = -1;   // >=0: jump to this phase at start (audit)
    float headlessFps = 0.0f;   // FPS medido of the own loop headless (GetFPS()=0 without window)

private:
    // Update
    void update(float dt);
    void handleInput(float dt);
    void movePlayerWithSlide(Vector2 direction, float dt);
    // Decisoes of the bot/autotest (extraido of handleInput): roda in the same point,
    // sob the guard interna botController.active. shouldQuit sinaliza via quitRequested.
    void updateBotControl(float dt);
    void spawnEnemy();
    void spawnBoss();
    void checkCollisions();
    void drainLevelUps();   // converte player.unclaimedLevels in points/evolucoes pendentes
    // Numeros flutuantes — COMPARTILHADO pelos 2 caminhos of render. No 3D projeta
    // the position of the world to screen (in coords of world ficavam invisiveis in the 3D).
    void drawFloatingNumbers(bool project3D) const;
    // ANIMACAO 3D: cada type generates VOX_POSES modelos, um by quadro of the passo.
    // Antes existia UM model congelado by type: the character only transladava,
    // i.and., DESLIZAVA pelo scenario instead of andar.
    static constexpr int VOX_POSES = 4;
    static int  voxKey(int base, int pose) { return base * 8 + pose; }
    // O render 3D depende of the SPRITE 2D capturado; the model voxel and generated mas not usado.
    bool hasVoxelSprite(int key) const {
        auto it = m_voxSprites.find(key);
        return it != m_voxSprites.end() && it->second.valid();
    }
    bool hasVoxelPoses(int base) const { return hasVoxelSprite(voxKey(base, VOX_POSES - 1)); }
    // DEPRECATED: mantido only to compatibilidade of codigo old.
    bool hasVoxel(int key) const { return hasVoxelSprite(key); }
    void updateItems(float dt);
    void updateXPOrbs(float dt);
    void updateProjectiles(float dt);
    void updateEnemyProjectiles(float dt);
    void checkPortalTransition();
    void transitionToZone(ZoneID dest);
    void grantQuestRewards(Quest& q);

    // Render
    void render();
    void drawUI() const;
    void drawHudAndOverlays();   // HUD of resources/ameaca + pause/levelup/shop/etc. (2D E 3D)
    void drawMinimap() const;
    void drawMainMenu() const;
    void drawPauseMenu() const;
    void drawZoneInfo() const;
    void drawQuestLog() const;
    void drawQuestHUD() const;

    // Save
    void autoSave();

    static constexpr int screenWidth  = 1280;
    static constexpr int screenHeight = 720;

    Player                        player;
    std::vector<Enemy>            enemies;
    std::vector<Item>             items;
    std::vector<Projectile>       projectiles;
    std::vector<EnemyProjectile>  enemyProjectiles;
    std::vector<XPOrb>            xpOrbs;
    std::vector<GroundEquipment>  groundEquips;
    Tilemap                       tilemap;
    std::vector<NPC>              npcs;
    std::vector<Quest>            quests;
    ParticleSystem                particles;
    AudioManager                  audio;
    Background                    background;

    Camera2D camera;

    // ── Migracao 2.5D isometric (Incremento 1 & 2) ───────────────────────────
    // Camera 3D for the world (floor/walls). 2D continuous being the base stable; the
    // modo 3D is alternavel by F10 enquanto the migration avanca incremento the incremento.
    Camera3D camera3D{};
    float    cameraHeight = 740.0f;   // height of the camera above the plano
    float    cameraDistY  = 580.0f;   // recuo in the eixo Z (profundidade isometrica)
    float    cameraZoom   = 1.0f;
    float    camPunch     = 0.0f;   // "camera kick" of zoom: goes up in the cast/impacto and decai
    float    worldClock   = 0.32f;    // ciclo day/night (0=meia-night, 0.5=middle-day)
    int      bossPowersAbsorbed = 0;  // poderes of boss absorvidos (estilo V Rising)
    bool     victoryReported = false; // reset by match (era static of function = bug)
    float    worldSun    = 1.0f;      // 0=night, 1=day (deriva of the worldClock)     // roda of the mouse: <1 aproxima, >1 afasta (olhar of up)
    GfxRenderTexture tempEntityTarget; // alvo temporary p/ draw entidades procedurais
    void     updateCamera3D();
    Vector2  mouseGround3D() const;   // raycast of the mouse in the plano Y=0 -> world 2D
    void     renderWorld3D();         // path of render 2.5D complete (world 3D + outdoors procedurais)
    // Sprites 2D capturados by type — usados as texture in the billboards of the world 3D.
    std::unordered_map<int, GfxTexture> m_voxSprites;
    int      m_voxGenBudget = 0;   // limit of geracoes of sprite by frame (anti-engasgo)
    template<typename Fn>
    void     ensureVoxel(int key, Vector2 capPos, Fn&& drawFn);
    void     drawVoxel(int base, Vector2 pos, float rotDeg, float walkPhase = 0.0f,
                       bool moving = false);

    struct ChatBubble {
        std::string text;
        float       timer = 0.0f;
    };
    std::unordered_map<uint32_t, ChatBubble> activeChats;
    bool        chatActive = false;
    std::string chatInput;
    std::vector<const Enemy*> netKilledEnemies;

    ZoneID  currentZone       = ZoneID::LARuins;
    float   zoneNameTimer     = 0.0f;  // show zone name on transition

    float   spawnTimer        = 0.0f;
    float   spawnInterval     = 3.0f;
    int     enemiesKilled     = 0;
    int     bossSpawnThreshold= 20;
    bool    bossSpawned       = false;

    // Screen shake
    float   shakeIntensity    = 0.0f;
    float   shakeTimer        = 0.0f;
    void    triggerShake(float intensity, float dur) {
        shakeIntensity = intensity; shakeTimer = dur;
    }

    // Environmental war events (ruins/city ghost): impacto distante with
    // flash, estrondo abafado and micro-tremor — the world "continuous in guerra".
    float   owWarTimer  = 0.0f;   // regressiva until the next impacto environment
    float   owWarFlash  = 0.0f;   // >0 = impacto active (render usa to draw the flash)
    Vector2 owWarPos    = { 0, 0 };
    float   owWarSeed   = 0.0f;   // estabiliza angle/scale of the impacto corrente

    // Indicador direcional of damage — aponta to QUEM feriu the player (shooter juice)
    Vector2 hurtDir      = {0, 0};   // vector unitario (world) fonte -> player
    float   hurtDirTimer = 0.0f;     // time restante of the indicador in the edge of the screen
    void    noteHurtDir(Vector2 src);

    // Click-to-move target
    Vector2 moveTarget        = {0, 0};
    bool    hasTarget         = false;

    // Combat timers & FX
    std::vector<DamageNumber> damageNumbers;
    float companionHealAccum = 0.0f;
    // Environment BASE of the biome current, interpolado (the troca of region not pisca)
    Color m_ambBaseCol  = {206,212,226,255};
    float m_ambBaseDark = 0.27f;   // healing of the Healer acumulada p/ 1 number the cada ~5 HP
    float meleeCooldown  = 0.0f;
    float hitFlashTimer  = 0.0f;
    float eliteFlashTimer = 0.0f;   // flash BRANCO (damage heavy: elite/boss)
    float slowMoTimer    = 0.0f;
    float footstepTimer  = 0.0f;

    // Story mode
    int   storyChapter   = 1;
    float storyBannerTimer = 0.0f;
    std::string storyBannerText;
    std::string storyBannerSub;
    void  showStoryBanner(const std::string& title, const std::string& sub, float dur = 4.0f);
    void  drawStoryBanner() const;

    // Passive unlock announce
    std::string passiveMsg;
    float       passiveMsgTimer = 0.0f;

    // Combo system
    int   comboCount     = 0;
    float comboTimer     = 0.0f;

    // Runtime telemetry
    int   totalKills     = 0;
    int   totalDamage    = 0;
    float sessionTime    = 0.0f;

    // ── Juice of combat: hit-stop + decalques of floor (blood/faiscas) ───────
    float hitStopTimer = 0.0f;   // freezes the world by some frames in the impacto
    // Alvo of the aim assist: position of the enemy "grudado" in the cursor for the reticulo
    // of the HUD pintar the lock. {-1,-1} = without alvo.
    Vector2 hudAimLock = {-1.0f, -1.0f};
    struct GroundDecal {
        Vector2 pos; Color color; float life; float maxLife; float size; int type; // 0=blood 1=scorched
    };
    std::vector<GroundDecal> decals;
    void  addDecal(Vector2 p, Color c, int type, float size);
    void  renderDecals() const;

    // ── Infinite Evolution Engine — novidade constant, never estagna ────────
    // Threat Level: goes up with the time/kills, scale enemies and rewards.
    int   threatLevel    = 1;
    // IA that EVOLUI during the match: observa as the player luta and responde
    // in the composicao of the spawn and in the tactic of group. See EnemyDirector.h.
    EnemyDirector director;
    float threatTimer    = 0.0f;
    int   threatKillMark = 0;     // kills in the start of the level current
    // Mutadores rotativos of the world — effects globais that mudam the cada ciclo.
    enum class WorldMutator {
        None, SwiftEnemies, ArmoredEnemies, BloodMoon, LootRain, Frenzy, Berserk, COUNT
    };
    WorldMutator activeMutator = WorldMutator::None;
    float        mutatorTimer  = 0.0f;
    float        mutatorDuration = 75.0f;
    void         rollNewMutator();
    const char*  mutatorName(WorldMutator m) const;
    const char*  mutatorDesc(WorldMutator m) const;
    float        threatStatMult() const { return 1.0f + (threatLevel - 1) * 0.12f; }
    float        mutatorSpeedMult() const { return activeMutator == WorldMutator::SwiftEnemies ? 1.35f : 1.0f; }
    float        mutatorHPMult()    const { return activeMutator == WorldMutator::ArmoredEnemies ? 1.6f : 1.0f; }
    float        mutatorDropMult()  const { return activeMutator == WorldMutator::LootRain ? 2.5f : 1.0f; }
    float        mutatorSpawnMult() const { return activeMutator == WorldMutator::Frenzy ? 0.55f : 1.0f; }
    float        mutatorDmgMult()   const { return activeMutator == WorldMutator::Berserk ? 1.4f : 1.0f; }
    bool         mutatorBloodMoon() const { return activeMutator == WorldMutator::BloodMoon; }
    void         updateEvolutionEngine(float dt);
    void         playEnemyDeathSound(const class Enemy& and); // sound by faction/type

    // Render helpers
    void drawObjectivesPanel() const;
    void drawCharacterPanel()  const;
    void drawSkillsPanel()     const;

    // Hack Tree (skill tree of perks)
    void updateSkillTreePanel();
    void drawSkillTreePanel()  const;
    void autoSpendSkillPoints();
    bool buyPerk(int idx);
    static void DrawPanel(int x, int y, int w, int h, Color border, float alpha = 0.82f);
    static void DrawBarH(int x, int y, int w, int h, float pct, Color fill, Color bg);

    std::vector<Companion> companions;

    BuildingSystem buildingSystem;

    // Crafting materials (used by BuildingSystem costs + crafting UI)
    int     materialMetal    = 0;
    int     materialCarapace = 0;

    BotController botController;
    bool    botMeleeRequest   = false;
    bool    botWantsPortal    = false;  // bot pediu the avanco of phase (consumido in updatePhasePortal)
    Vector2 botAimTarget      = {0, 0};

    // ── Timers of the bot/autotest (eram static of function: not resetavam between
    // partidas — restartRun zera all) ──
    float   botReportSaveTimer = 0.0f;  // auto-save partial of the report (5min)
    float   botAllyTimer       = 2.0f;
    float   botBuildTimer      = 4.0f;
    float   botProduceTimer    = 8.0f;
    float   botUpgradeTimer    = 12.0f;
    float   botStipendTimer    = 0.0f;
    int     botBuildCycle      = 0;
    mutable double lastShot      = 0.0;   // TEMP-SHOT: last screenshot of the autotest
    mutable int    shotN         = 0;

    bool    showInventory     = false;
    bool    showEquipment     = false;
    bool    showQuestLog      = false;
    bool    showSkillTree     = false;
    int     perkCursor        = 0;
    bool    paused            = false;
    mutable int pauseHovered  = -1;   // option destacada in the menu of pause
    float   dyingCryCooldown  = 0.0f; // evita spam of the grito of death
    bool    inMainMenu        = true;

    void    restartRun();             // reinicia the match of the zero
    void    startLoadedGame();        // loads the save and enters (without screen of difficulty)
    bool    quitRequested     = false;  // bot autotest quit signal
    float   saveTimer         = 0.0f;

    // Player speech bubble
    std::string playerSpeechText;
    float       playerSpeechTimer = 0.0f;
    void        triggerPlayerSpeech(const std::string& text, float dur = 3.5f);
    void        drawPlayerSpeech() const;

    // Menu hover tracking (not const — mouse is stateful)
    mutable int menuHoveredBtn = -1;

    // Difficulty system
    DifficultyLevel difficulty          = DifficultyLevel::Resistente;
    bool            selectingDifficulty = false;
    bool            pendingNewGame      = false;
    mutable int     difficultyHovered   = 1;

    // Selecao of character (after the difficulty, in new game)
    bool            selectingCharacter  = false;
    mutable int     characterHovered    = 0;   // indice in CharacterClass
    void            startNewGame();            // inicia the match apos the escolhas
    void            drawCharacterSelectScreen() const;
    const DifficultySettings& getDifficulty() const;
    void            drawDifficultyScreen() const;
    static constexpr DifficultySettings DIFFICULTY_TABLE[5] = {
        { "STORY",   "Apenas the story",          {100,200,255,255}, 0.55f,0.50f,0.80f,0.70f,1.60f,0.80f,1.20f,0.50f },
        { "RESISTENTE", "Experience balanceada",      {0,220,100,255},   1.00f,1.00f,1.00f,1.00f,1.00f,1.00f,1.00f,1.00f },
        { "GUERREIRO",  "Hard - more rewards",     {255,180,0,255},   1.45f,1.35f,1.15f,1.30f,1.35f,1.25f,1.30f,1.45f },
        { "INFILTRADO", "So the melhores sobrevivem",   {255,80,0,255},    2.10f,1.85f,1.35f,1.70f,1.70f,1.60f,1.75f,2.00f },
        { "APOCALIPSE", "Sem misericordia",            {200,0,255,255},   3.20f,2.60f,1.60f,2.20f,2.20f,2.00f,2.40f,3.00f }
    };

    // Shop system
    ShopSystem shopSystem;

    // Crafting system
    CraftingSystem craftingSystem;

    // Dialog state
    int     nearNpcIndex      = -1;
    int     dialogLine        = 0;
    bool    dialogOpen        = false;

    // Fullscreen render target (virtual 1280x720 always)
    GfxRenderTexture gameTarget;

    // ── POS-PROCESSAMENTO (bloom + tonemap) ──
    // O frame integer (world + HUD) leaves of the gameTarget and passes by: glow ->
    // blur H -> blur V -> composicao with tonemap filmico. Se algum shader falhar
    // in compile, m_postFX stays false and the apresentacao returns to the path old
    // (DrawTexturePro puro) — never screen preta.
    GfxShader          m_shBright, m_shBlur, m_shGrade;
    GfxRenderTexture   m_bloomA, m_bloomB;
    bool            m_postFX = false;
    int m_locThreshold = -1, m_locKnee = -1, m_locBlurDir = -1;
    int m_locBloomTex = -1, m_locBloomStr = -1, m_locExposure = -1,
        m_locSaturation = -1, m_locContrast = -1;
    // Shader of ILUMINACAO of the world 3D (direcional + environment + rim + fog).
    // Sem ele all poligono saia with the color written, without volume: papelao colorido.
    GfxShader m_shWorld;
    bool      m_worldLit = false;
    GfxTexture m_whiteTex;   // texture 1x1 branca: voxels usam color by vertex
    int    m_locLightDir = -1, m_locLightCol = -1, m_locAmbCol = -1, m_locCamPos = -1,
           m_locFogCol = -1, m_locFogStart = -1, m_locFogEnd = -1, m_locRim = -1,
           m_locSpecK = -1, m_locWorldPer = -1;
    void   initWorldShader();
    void   applyWorldShader(Model& m) const;   // liga the shader in the material of the model
    void   applyWorldShader(GfxModel& m) const;
    void   updateWorldShaderUniforms();

    void    drawGenericStructure(Vector2 pos, float sc) const;
    void    drawArkStructure(Vector2 pos) const;   // Arca sci-fi (bunker) p/ zones urbanas
    void    initPostFX();
    void    unloadPostFX();

    void    presentFrame() const;
    Vector2 virtualizeMousePos(Vector2 m) const;

    // Omega boss tracking
    int     omegaKillThreshold = 50;
    void    spawnOmegaBoss();

    void    buildNPCs();
    void    buildQuests();
    void    setupZoneNPCs(ZoneID zone);
    void    setupBaseNPCs();          // NPCs of servico inside the zone segura

    // Open world
    struct WorldRegion {
        Rectangle   bounds;
        ZoneID      zoneType;
        std::string name;
        bool        discovered = false;
        Color       mapColor;
    };
    std::vector<WorldRegion> worldRegions;

    // Scenario persistente of the world open — TODAS the regions populadas of uma vez,
    // espalhadas by all the area and always renderizadas (houses, lapides, lava, etc.)
    DarkWorld owDecor;
    bool      owDecorBuilt = false;

    // E.1 — geracao amortizada of the scenario fixed: objetos are enfileirados during
    // buildOpenWorldScenery and transferidos to owDecor.scenery in lotes during
    // the update, instead of realocar tudo num single frame.
    std::vector<SceneryObject> m_sceneryBuildQueue;
    bool      m_sceneryPostProcessNeeded = false;
    static constexpr int SCENERY_BUILD_BUDGET = 300; // objetos by frame
    void      streamSceneryBuild();

    // ── FASES (world open) ─────────────────────────────────────────────────
    // Cada phase and um WORLD integer of um biome only. Some the number of kills of the
    // phase; to the bater the meta the PORTAL opens and leva to the next world with screen of
    // transition. E the that of the the sensation of "passei of phase" that andar not dava.
    int       owPhase        = 0;
    int       owPhaseKills   = 0;
    int       owPhaseGoal    = 20;
    int       owKillsAtStart = 0;
    bool      owPortalOpen   = false;
    Vector2   owPortalPos    = { 0.0f, 0.0f };
    float     owFadeTimer    = 0.0f;   // >0 = screen of transition of phase
    std::string owFadeText;
    // A phase and um LUGAR with limit, not um tapete infinito: barrier of energy
    // in the radius below. Sem edge, "pass of phase" not existe - the player only anda.
    float     owPhaseRadius  = 3000.0f;
    float     borderWarnTimer = 0.0f;   // anti-spam of the warning of barrier
    bool      owBossPhase    = false;   // the cada 3 phases the portal only opens with the boss dead
    bool      owBossDown     = false;
    void      updatePhasePortal(float dt);
    void      advanceOpenWorldPhase();
    void      drawPhaseFade() const;
    // Campanha vinda of content/phases.txt (dado, not codigo). If the file not
    // existir falls numa table embutida, entao the game NUNCA deixa of open by
    // causa of conteudo faltando.
    struct PhaseDef {
        ZoneID      zone   = ZoneID::LARuins;
        int         goal   = 20;
        bool        boss   = false;
        float       radius = 3000.0f;
        std::string title;
    };
    std::vector<PhaseDef> phaseDefs;
    void            loadPhaseDefs();
    const PhaseDef& phaseDef(int phase) const;
    static ZoneID phaseZone(int phase);
    // Infinite world: scenario generated by CHUNKS around of the player (auto-structure)
    std::set<long long> m_sceneryChunks;
    int  m_lastChunkX = -999999, m_lastChunkY = -999999;
    void updateSceneryChunks(Vector2 playerPos);
    std::vector<Vector3> m_chunkSolids;  // collision of estruturas of the chunks (x,y=pos, z=radius)
    bool isBlocked(Vector2 pos) const;   // wall of the grid OU struct of chunk in the infinito
    void clearBlockingAt(Vector2 pos, float radius);  // fallback of the portal: removes collision of scenario num radius
    void      buildOpenWorldScenery();
    void      placeBaseShops();   // barracas/lojas of the NPCs of the zone segura (types 23-27)

    // Zone Segura / Base — refuge without enemies to preparar and construir.
    // You nasce and renasce here; enemies not spawnam nem perseguem inside dela.
    Vector2   safeZoneCenter = {1280.0f, 1280.0f}; // center of the region inicial
    float     safeZoneRadius = 1050.0f;
    bool      wasInSafeZone   = true;   // p/ detectar when the player leaves of the base
    bool      inSafeZone(Vector2 pos) const {
        float dx = pos.x - safeZoneCenter.x, dy = pos.y - safeZoneCenter.y;
        return (dx*dx + dy*dy) <= (safeZoneRadius * safeZoneRadius);
    }

    // Returns true if the position estiver outside the disco of the phase in the world open.
    bool      isOutsideOpenWorldBounds(Vector2 pos) const {
        if (!openWorldMode) return false;
        float dx = pos.x - safeZoneCenter.x, dy = pos.y - safeZoneCenter.y;
        return (dx*dx + dy*dy) > (owPhaseRadius * owPhaseRadius);
    }

    // Projeta the position of returns to inside the limit of the phase, if required.
    void      clampInsideOpenWorldBounds(Vector2& pos, float margin) const {
        if (!openWorldMode) return;
        float dx = pos.x - safeZoneCenter.x, dy = pos.y - safeZoneCenter.y;
        float l2 = dx*dx + dy*dy;
        float limit = owPhaseRadius - margin;
        if (limit < 0.0f) limit = 0.0f;
        if (l2 > limit * limit) {
            float l = sqrtf(l2); if (l < 1.0f) { dx = 1.0f; dy = 0.0f; l = 1.0f; }
            pos.x = safeZoneCenter.x + dx / l * limit;
            pos.y = safeZoneCenter.y + dy / l * limit;
        }
    }

    // ── Natural resource gathering (wood/stone/iron/silver/gold) ──────────
    enum class ResourceType { Wood = 0, Stone, Iron, Silver, Gold, COUNT };
    struct ResourceNode {
        Vector2      position;
        ResourceType type;
        int          amount;       // unidades restantes in the in the
        int          maxAmount;
        float        harvestProg;  // 0..1 progress of the coleta current
        float        respawnTimer; // >0 = depletado, contando to return
        bool         depleted;
        float        shake;        // tremor visual to the collect
    };
    std::vector<ResourceNode> resourceNodes;
    int   playerResources[(int)ResourceType::COUNT] = {0,0,0,0,0};

    // ── Animais / health selvagem (NPCs of animais) ─────────────────────────────
    enum class AnimalType { Deer = 0, Rabbit, Boar, Wolf, Bird, COUNT };
    struct Animal {
        Vector2    position;
        Vector2    velocity;
        AnimalType type;
        float      health, maxHealth;
        float      wanderTimer;
        Vector2    wanderDir;
        bool       fleeing;
        bool       hostile;     // wolf persegue/ataca
        float      animTimer;
        float      attackCD;
        bool       dead;
    };
    std::vector<Animal> animals;
    void  setupAnimals();
    void  updateAnimals(float dt);
    void  renderAnimals() const;

    // ── Civis of the city (health environment: cada um TEM UMA TASK, not slot to the toa) ──
    enum class FolkJob { Guard, Worker, Chatter, Vendor };
    struct CityFolk {
        Vector2 position, target, home, anchor;  // anchor = posto of trabalho / point B of the ronda / roda of conversa
        float   speed       = 55.0f;
        float   timer       = 0.0f;   // generico (ronda/troca of alvo)
        float   work        = 0.0f;   // animation/progress of the task
        float   pauseTimer  = 0.0f;   // stopped executing the task
        int     role        = 0;      // NPCRole → model voxel + color
        int     facing      = 1;
        FolkJob job         = FolkJob::Worker;
        float   walkPhase   = 0.0f;   // phase of the passo (avanca with the deslocamento)
        bool    atStation   = false;  // chegou in the posto and is trabalhando/conversando
    };
    std::vector<CityFolk> cityFolk;
    void  spawnCityFolk();
    void  updateCityFolk(float dt);

    // ── Multiplayer in time real (NetClient — WebSocket) ─────────────────────
    NetClient net;
    uint32_t  netId       = 0;
    bool      netActive   = false;
    bool      netPending_ = false;   // waiting the login (JWT) p/ start the WS
    void      startNetwork();
    void      pollStartNetwork();    // inicia the WS so that the JWT chegar
    void      renderRemotePlayers() const;

    // ── Grupo / Alianca (party multiplayer) ───────────────────────────────────
    bool        partyPanel = false;       // painel of group open (key O)
    std::string partyInput;               // codigo being digitado
    void        updateParty();
    void        drawPartyPanel() const;

    // ── Shop premium (Gems / Stripe via backend Node) ─────────────────────────
    StoreClient store;
    bool        storeStarted   = false;  // login + fetch disparados
    bool        premiumView    = false;  // premium tab opened inside the shop
    int         premiumSel     = 0;      // item selected in the premium tab
    float       storeRefreshT  = 0.0f;   // timer p/ update saldo of gems
    void        startStore();            // login + catalog (uma vez)
    void        updatePremiumStore(float dt);
    void        drawPremiumStore() const;

    // ── Pathfinding of the bot: unificado in the BotController (BFS in computePathDir) ──
    int   nearResourceIdx = -1;    // in the more next coletavel
    float mineSwingCD     = 0.0f;  // cadencia between golpes of the picareta
    float mineSwingAnim   = 0.0f;  // 0..1 animation of the golpe current (1=acabou of bater)
    int   mineFxIdx       = -1;    // in the being golpeado (p/ draw picareta/impacto)
    void  setupResourceNodes();
    void  updateResourceGathering(float dt);
    void  renderResourceNodes() const;   // world-space (inside of BeginMode2D)
    void  drawResourceHUD() const;       // screen-space
    static const char* resourceName(ResourceType t);
    static Color       resourceColor(ResourceType t);

    // Controle RTS — selecao by arrasto of the mouse
    bool      rtsDragging   = false;   // arrasto active (passed of the threshold)
    Vector2   rtsDragStart  = {0, 0};  // point inicial of the arrasto (world)
    Vector2   rtsDragCur    = {0, 0};  // point current (world)
    bool      rtsHasUnits   = false;   // ha unidades selecionadas
    ZoneID                   currentRegion = ZoneID::LARuins;
    bool                     openWorldMode = false;
    void   setupWorldRegions();
    ZoneID getRegionAt(Vector2 pos) const;

    // Companion system
    void    spawnCompanion(CompanionType t);
    void    updateCompanions(float dt);
    void    drawCompanions() const;

    // Level Up / Evolution screen
    bool          showLevelUpScreen   = false;
    bool          showEvolutionScreen = false;
    int           levelUpChoice       = 1;
    int           evolutionChoice     = 1;
    LevelUpChoice levelUpOptions[3];
    float         levelUpAnimTimer    = 0.0f;

    // Points acumulados — NOT travam the game; player escolhe when quiser
    int           pendingLevelUps     = 0;   // points of level up not gastos (key L)
    int           pendingEvolutions   = 0;   // escolhas of evolution not gastas (key K)
    float         pendingNotifyPulse  = 0.0f; // pulso visual of the warning in the HUD

    // Condicao of victory — final boss NUCLEO KRONOS in the KronosNexus
    bool          gameWon             = false;
    float         victoryTimer        = 0.0f;
    bool          finalBossSpawned    = false; // already went invocado nesta match
    bool          finalBossAlive      = false; // is vivo now
    void          spawnFinalBoss();
    void          drawVictoryScreen() const;

    void generateLevelUpChoices();
    void applyLevelUpChoice(int idx);
    void drawLevelUpScreen() const;
    void drawEvolutionScreen() const;
    void applyEvolutionPath(int pathIdx);

    // Contextual dialogue state
    bool  firstCombatTriggered  = false;
    bool  firstKillTriggered    = false;
    float surroundedCooldown    = 0.0f;

    // Anomaly portal system
    AnomalySystem anomalySystem;
    float         anomalyWaveTimer    = 0.0f;
    static constexpr float anomalyWaveCooldown = 45.0f;

    // 2D lighting system (active in dark zones: Cemetery..AbandonedManor)
    LightSystem   lightSystem;
    bool          darkZoneActive = false;

    // Dark world scenery system
    DarkWorldSystem darkWorld;

    // Inferno zone — volcanic lava + geysers
    InfernoZoneSystem infernoZone;

    // Achievement system
    AchievementSystem achievements;

    // Tutorial system
    TutorialSystem tutorial;
    bool tutorialRewardGiven = false;

    // Telemetry for achievements
    int   totalKillsEver   = 0;
    int   totalBossesKilled= 0;
    int   totalPortalsClosed=0;
    int   totalCreditsEarned=0;
    int   totalDeaths      = 0;
    float totalPlaytime    = 0.0f;
    int   zonesVisitedSet  = 0;  // bitmask of visited zone ids

    // 3D Models and textures for realistic 3D graphics
    GfxModel m_houseModel;
    GfxModel m_turretModel;
    GfxModel m_barracksModel;
    GfxModel m_castleModel; // used for Town Hall / Arca
    GfxModel m_marketModel; // used for Tank Factory
    GfxModel m_wellModel;   // used for MedBay
    GfxModel m_carModel;    // old_car_new.glb — cars of the scenario 3D
    float m_houseScale=1,m_turretScale=1,m_barracksScale=1,m_castleScale=1,m_marketScale=1,m_wellScale=1,m_carScale=1;
    GfxTexture m_houseTex;
    GfxTexture m_turretTex;
    GfxTexture m_barracksTex;
    GfxTexture m_castleTex;
    GfxTexture m_marketTex;
    GfxTexture m_wellTex;
    bool m_modelsLoaded = false;
};
