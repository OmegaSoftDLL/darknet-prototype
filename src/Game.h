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
#include "Effects.h"
#include "Background.h"
#include "LightSystem.h"
#include "BotController.h"
#include "Companion.h"
#include "BuildingSystem.h"
#include "ShopSystem.h"
#include "AnomalyPortal.h"
#include "CraftingSystem.h"
#include "DarkWorld.h"
#include "InfernoZone.h"
#include "Achievement.h"
#include "NetClient.h"
#include "StoreClient.h"
#include <vector>
#include <string>
#include <raylib.h>
#include <functional>
#include <unordered_map>

// Equipment piece lying on the ground — requires E to pick up
struct GroundEquipment {
    Vector2     position;
    Equipment   equip;
    float       pulseTimer = 0.0f;
    float       lifetime   = 45.0f;
    bool        collected  = false;
};

struct DamageNumber {
    Vector2     pos;
    float       value;
    Color       color;
    float       life   = 1.2f;
    const char* prefix = "";   // "$" for credits, "+" for heals, "" for damage
};

// ─── Difficulty System ───────────────────────────────────────────────────────

enum class DifficultyLevel {
    Historia = 0,  // Story Mode — inimigos fracos, mais drops
    Resistente,    // Normal — balanceado (default)
    Guerreiro,     // Hard — mais inimigos fortes
    Infiltrado,    // Nightmare — muito difícil
    Apocalipse     // INSANE — sem misericordia
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
    Game();
    ~Game();
    void run();
    void runAutoTest(bool autoTest);

private:
    // Update
    void update(float dt);
    void handleInput(float dt);
    void spawnEnemy();
    void spawnBoss();
    void checkCollisions();
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
    void drawSkills() const;
    void drawMinimap() const;
    void drawMainMenu() const;
    void drawPauseMenu() const;
    void drawZoneInfo() const;
    void drawQuestLog() const;
    void drawQuestHUD() const;

    // Save
    void tryAutoSave();
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

    // ── Migração 2.5D isométrico (Incremento 1 & 2) ───────────────────────────
    // Câmera 3D para o mundo (chão/paredes). 2D continua sendo a base estável; o
    // modo 3D é alternável por F10 enquanto a migração avança incremento a incremento.
    Camera3D camera3D{};
    float    cameraHeight = 380.0f;   // altura da câmera acima do plano
    float    cameraDistY  = 280.0f;   // recuo no eixo Z (profundidade isométrica)
    bool     render3D     = true;     // JOGO É 3D (2.5D isométrico) por padrão em todo gameplay
    RenderTexture2D tempEntityTarget{}; // alvo temporário p/ desenhar entidades procedurais
    void     updateCamera3D();
    Vector2  mouseGround3D() const;   // raycast do mouse no plano Y=0 -> mundo 2D
    void     renderWorld3D();         // caminho de render 2.5D completo (mundo 3D + outdoors procedurais)
    void     drawProceduralEntity3D(Vector2 pos, float heightOffset, std::function<void()> drawFunc);

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

    // Click-to-move target
    Vector2 moveTarget        = {0, 0};
    bool    hasTarget         = false;

    // Combat timers & FX
    std::vector<DamageNumber> damageNumbers;
    float meleeCooldown  = 0.0f;
    float hitFlashTimer  = 0.0f;
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

    // ── Juice de combate: hit-stop + decalques de chão (sangue/faíscas) ───────
    float hitStopTimer = 0.0f;   // congela o mundo por alguns frames no impacto
    struct GroundDecal {
        Vector2 pos; Color color; float life; float maxLife; float size; int type; // 0=sangue 1=queimado
    };
    std::vector<GroundDecal> decals;
    void  addDecal(Vector2 p, Color c, int type, float size);
    void  renderDecals() const;

    // ── Motor de Evolucao Infinita — novidade constante, nunca estagna ────────
    // Nivel de Ameaca: sobe com o tempo/kills, escala inimigos e recompensas.
    int   threatLevel    = 1;
    float threatTimer    = 0.0f;
    int   threatKillMark = 0;     // kills no inicio do nivel atual
    // Mutadores rotativos do mundo — efeitos globais que mudam a cada ciclo.
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
    void         playEnemyDeathSound(const class Enemy& e); // som por facção/tipo

    // Render helpers
    void drawObjectivesPanel() const;
    void drawCharacterPanel()  const;
    void drawSkillsPanel()     const;
    static void DrawPanel(int x, int y, int w, int h, Color border, float alpha = 0.82f);
    static void DrawBarH(int x, int y, int w, int h, float pct, Color fill, Color bg);

    std::vector<Companion> companions;

    BuildingSystem buildingSystem;

    // Crafting materials (used by BuildingSystem costs + crafting UI)
    int     materialMetal    = 0;
    int     materialCarapace = 0;

    BotController botController;
    bool    botMeleeRequest   = false;
    Vector2 botAimTarget      = {0, 0};

    bool    showInventory     = false;
    bool    showEquipment     = false;
    bool    showQuestLog      = false;
    bool    paused            = false;
    mutable int pauseHovered  = -1;   // opcao destacada no menu de pause
    float   dyingCryCooldown  = 0.0f; // evita spam do grito de morte
    bool    inMainMenu        = true;

    void    restartRun();             // reinicia a partida do zero
    void    startLoadedGame();        // carrega o save e entra (sem tela de dificuldade)
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

    // Selecao de personagem (apos a dificuldade, em novo jogo)
    bool            selectingCharacter  = false;
    mutable int     characterHovered    = 0;   // indice em CharacterClass
    void            startNewGame();            // inicia a partida apos as escolhas
    void            drawCharacterSelectScreen() const;
    const DifficultySettings& getDifficulty() const;
    void            drawDifficultyScreen() const;
    static constexpr DifficultySettings DIFFICULTY_TABLE[5] = {
        { "HISTORIA",   "Apenas a historia",          {100,200,255,255}, 0.55f,0.50f,0.80f,0.70f,1.60f,0.80f,1.20f,0.50f },
        { "RESISTENTE", "Experiencia balanceada",      {0,220,100,255},   1.00f,1.00f,1.00f,1.00f,1.00f,1.00f,1.00f,1.00f },
        { "GUERREIRO",  "Hard - mais recompensas",     {255,180,0,255},   1.45f,1.35f,1.15f,1.30f,1.35f,1.25f,1.30f,1.45f },
        { "INFILTRADO", "So os melhores sobrevivem",   {255,80,0,255},    2.10f,1.85f,1.35f,1.70f,1.70f,1.60f,1.75f,2.00f },
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
    RenderTexture2D gameTarget;
    void    presentFrame() const;
    Vector2 virtualizeMousePos(Vector2 m) const;

    // Omega boss tracking
    int     omegaKillThreshold = 50;
    void    spawnOmegaBoss();

    void    buildNPCs();
    void    buildQuests();
    void    setupZoneNPCs(ZoneID zone);
    void    setupBaseNPCs();          // NPCs de servico dentro da zona segura

    // Open world
    struct WorldRegion {
        Rectangle   bounds;
        ZoneID      zoneType;
        std::string name;
        bool        discovered = false;
        Color       mapColor;
    };
    std::vector<WorldRegion> worldRegions;

    // Cenario persistente do mundo aberto — TODAS as regioes populadas de uma vez,
    // espalhadas por toda a area e sempre renderizadas (casas, lapides, lava, etc.)
    DarkWorld owDecor;
    bool      owDecorBuilt = false;
    void      buildOpenWorldScenery();

    // Zona Segura / Base — refugio sem inimigos para preparar e construir.
    // Voce nasce e renasce aqui; inimigos nao spawnam nem perseguem dentro dela.
    Vector2   safeZoneCenter = {1280.0f, 1280.0f}; // centro da regiao inicial
    float     safeZoneRadius = 1050.0f;
    bool      wasInSafeZone   = true;   // p/ detectar quando o jogador sai da base
    bool      inSafeZone(Vector2 pos) const {
        float dx = pos.x - safeZoneCenter.x, dy = pos.y - safeZoneCenter.y;
        return (dx*dx + dy*dy) <= (safeZoneRadius * safeZoneRadius);
    }

    // ── Coleta de recursos naturais (madeira/pedra/ferro/prata/ouro) ──────────
    enum class ResourceType { Wood = 0, Stone, Iron, Silver, Gold, COUNT };
    struct ResourceNode {
        Vector2      position;
        ResourceType type;
        int          amount;       // unidades restantes no nó
        int          maxAmount;
        float        harvestProg;  // 0..1 progresso da coleta atual
        float        respawnTimer; // >0 = depletado, contando para voltar
        bool         depleted;
        float        shake;        // tremor visual ao coletar
    };
    std::vector<ResourceNode> resourceNodes;
    int   playerResources[(int)ResourceType::COUNT] = {0,0,0,0,0};

    // ── Animais / vida selvagem (NPCs de animais) ─────────────────────────────
    enum class AnimalType { Deer = 0, Rabbit, Boar, Wolf, Bird, COUNT };
    struct Animal {
        Vector2    position;
        Vector2    velocity;
        AnimalType type;
        float      health, maxHealth;
        float      wanderTimer;
        Vector2    wanderDir;
        bool       fleeing;
        bool       hostile;     // lobo persegue/ataca
        float      animTimer;
        float      attackCD;
        bool       dead;
    };
    std::vector<Animal> animals;
    void  setupAnimals();
    void  updateAnimals(float dt);
    void  renderAnimals() const;

    // ── Multiplayer em tempo real (NetClient — WebSocket) ─────────────────────
    NetClient net;
    uint32_t  netId     = 0;
    bool      netActive = false;
    void      startNetwork();
    void      renderRemotePlayers() const;

    // ── Grupo / Aliança (party multiplayer) ───────────────────────────────────
    bool        partyPanel = false;       // painel de grupo aberto (tecla O)
    std::string partyInput;               // codigo sendo digitado
    void        updateParty();
    void        drawPartyPanel() const;

    // ── Loja premium (Gems / Stripe via backend Node) ─────────────────────────
    StoreClient store;
    bool        storeStarted   = false;  // login + fetch disparados
    bool        premiumView    = false;  // aba premium aberta dentro da loja
    int         premiumSel     = 0;      // item selecionado na aba premium
    float       storeRefreshT  = 0.0f;   // timer p/ atualizar saldo de gems
    void        startStore();            // login + catalogo (uma vez)
    void        updatePremiumStore(float dt);
    void        drawPremiumStore() const;

    // ── Pathfinding BFS para o bot (evita travar em cantos) ───────────────────
    bool      botFindPath(Vector2 from, Vector2 to, std::vector<Vector2>& outPath) const;
    std::vector<Vector2> botPath;
    int       botPathIdx     = 0;
    float     botStuckTime   = 0.0f;
    Vector2   botPrevPos      = {0, 0};
    Vector2   botPathGoal     = {0, 0};
    int   nearResourceIdx = -1;    // nó mais próximo coletável
    void  setupResourceNodes();
    void  updateResourceGathering(float dt);
    void  renderResourceNodes() const;   // world-space (dentro de BeginMode2D)
    void  drawResourceHUD() const;       // screen-space
    static const char* resourceName(ResourceType t);
    static Color       resourceColor(ResourceType t);

    // Controle RTS — selecao por arrasto do mouse
    bool      rtsDragging   = false;   // arrasto ativo (passou do limiar)
    Vector2   rtsDragStart  = {0, 0};  // ponto inicial do arrasto (mundo)
    Vector2   rtsDragCur    = {0, 0};  // ponto atual (mundo)
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

    // Pontos acumulados — NAO travam o jogo; jogador escolhe quando quiser
    int           pendingLevelUps     = 0;   // pontos de level up nao gastos (tecla L)
    int           pendingEvolutions   = 0;   // escolhas de evolucao nao gastas (tecla K)
    float         pendingNotifyPulse  = 0.0f; // pulso visual do aviso no HUD

    // Condicao de vitoria — boss final NUCLEO KRONOS no KronosNexus
    bool          gameWon             = false;
    float         victoryTimer        = 0.0f;
    bool          finalBossSpawned    = false; // ja foi invocado nesta partida
    bool          finalBossAlive      = false; // esta vivo agora
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

    // Telemetry for achievements
    int   totalKillsEver   = 0;
    int   totalBossesKilled= 0;
    int   totalPortalsClosed=0;
    int   totalCreditsEarned=0;
    int   totalDeaths      = 0;
    float totalPlaytime    = 0.0f;
    int   zonesVisitedSet  = 0;  // bitmask of visited zone ids

    // 3D Models and textures for realistic 3D graphics
    Model m_playerModel;
    Model m_enemyModel;
    Model m_houseModel;
    Model m_turretModel;
    Model m_barracksModel;
    Model m_castleModel; // used for Town Hall / Arca
    Texture2D m_houseTex;
    Texture2D m_turretTex;
    Texture2D m_barracksTex;
    Texture2D m_castleTex;
    bool m_modelsLoaded = false;
};
