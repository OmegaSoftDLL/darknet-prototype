#pragma once
#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <string>

enum class BuildingType {
    Ark,           // Base main — heal player nearby, spawn point
    House,         // Generates credits passivamente
    Barracks,      // Generates soldados aliados (infantry AI)
    TankFactory,   // Produz tanques amigos that atacam enemies
    Turret,        // Torre automatic that atira in the enemies
    ResourceNode,  // Generates materiais of crafting
    Wall,          // Barrier that bloqueia enemies
    MedBay,        // Healing player and companions around
};

struct FriendlyTank {
    Vector2 position   = {0, 0};
    Vector2 velocity   = {0, 0};
    float   health     = 150.f;
    float   maxHealth  = 150.f;
    float   speed      = 70.f;
    float   damage     = 25.f;
    float   shootRange = 200.f;
    float   shootCooldown = 0.f;
    float   shootRate  = 2.5f;
    float   orbitAngle = 0.f;
    bool    active     = true;
    Vector2 factoryPos = {0, 0};

    bool    wantsToShoot = false;
    Vector2 shootDir     = {1, 0};

    // RTS — selecao and ordem of move
    bool    selected     = false;
    bool    hasMoveOrder = false;
    Vector2 moveOrder    = {0, 0};

    void update(float dt, const std::vector<class Enemy*>& enemies);
    void render() const;
    bool isDead() const { return health <= 0.f; }
};

struct FriendlySoldier {
    Vector2 position   = {0, 0};
    float   health     = 60.f;
    float   maxHealth  = 60.f;
    float   speed      = 100.f;
    float   shootRange = 180.f;
    float   shootCooldown = 0.f;
    float   shootRate  = 1.8f;
    bool    active     = true;
    Vector2 barracksPos = {0, 0};

    bool    wantsToShoot = false;
    Vector2 shootDir     = {1, 0};

    // RTS — selecao and ordem of move
    bool    selected     = false;
    bool    hasMoveOrder = false;
    Vector2 moveOrder    = {0, 0};

    void update(float dt, Vector2 playerPos, const std::vector<class Enemy*>& enemies);
    void render() const;
    bool isDead() const { return health <= 0.f; }
};

struct Building {
    Vector2      position     = {0, 0};
    BuildingType type         = BuildingType::House;
    float        health       = 200.f;
    float        maxHealth    = 200.f;
    bool         active       = true;

    // Evolution of the building — level 1..3, cada level melhora the stats
    int          level        = 1;
    static const int MAX_LESPEED = 3;

    // Production timers
    float        buildTimer   = 0.f;    // construction progress 0→1
    bool         built        = false;
    float        builtAt      = 0.f;    // GetTime() when the structure ended (effect of materializacao)
    float        productionTimer = 0.f;
    float        productionRate  = 15.f; // seconds per unit/credit cycle

    // Queue of production (Quartel/Fabrica) — unidades NOT nascem in the hour.
    // Cada click/hand-of-obra enfileira; the unit leaves ready only after spawnTime.
    int          spawnQueue   = 0;    // unidades enfileiradas (inclui the current)
    float        spawnTimer   = 0.f;  // progress of the unit current (0→spawnTime)
    float        spawnTime    = 5.f;  // seconds of production by unit

    // Resource generation
    float        genTimer     = 0.f;
    float        genRate      = 8.f;    // seconds between credit/material ticks

    // Turret specific
    float        shootCooldown   = 0.f;
    float        shootRate       = 1.5f;
    float        shootRange      = 220.f;
    bool         wantsToShoot    = false;
    Vector2      shootDir        = {1, 0};
    float        shootDamage     = 20.f;

    // MedBay / Ark healing
    float        healTimer    = 0.f;
    float        healRate     = 3.f;    // seconds between heals
    float        healRadius   = 120.f;
    float        healAmount   = 15.f;

    // Visual animation
    float        animTimer    = 0.f;
    Color        tintColor;

    // Wall HP separate from main health for balance
    bool         isWall       = false;

    Building(Vector2 pos, BuildingType t);
};

struct BuildingCost {
    int credits;
    int metalScrap;
    int alienCarapace;
    const char* name;
    const char* desc;
};

class BuildingSystem {
public:
    BuildingSystem();

    void update(float dt, Vector2 playerPos, std::vector<class EnemyProjectile>* enemyProj,
                const std::vector<class Enemy*>& enemies);
    void render(Vector2 playerPos, Vector2 mouseWorldPos) const;
    void renderBuildMenu(int screenW, int screenH) const;

    // Persistence: serializa/deserializa state in lines of text.
    void save(std::vector<std::string>& out) const;
    bool load(const std::vector<std::string>& in);

    void toggleBuildMode();
    bool tryPlace(Vector2 worldPos, int playerCredits, int playerMetal, int playerCarapace,
                  int& outCreditCost, int& outMetalCost, int& outCarapaceCost);
    // Hit-test of the menu of structure (mouse in coords virtuais). Returns the indice
    // of the building sob the cursor, ou -1 if the click NOT went in the painel of the menu.
    int  menuCellAt(Vector2 screenMouse, int screenW, int screenH) const;

    void healPlayerIfNear(Vector2 playerPos, float& playerHealth, float playerMaxHealth);
    int  collectCredits();       // returns credits generated this frame
    int  collectMaterials();     // returns metal scraps generated

    // Click numa factory/barracks to produzir uma unit in the hour (custa credits).
    // Returns: 0=not clicou in building, 1=produziu, 2=without credits, 3=limit atingido.
    int  clickProduce(Vector2 worldPos, int& playerCredits);
    // Prompts flutuantes about fabricas/quarteis (called inside of BeginMode2D)
    void renderUnitPrompts() const;

    // ── Evolution of buildings ──────────────────────────────────────────────────
    // Evolui the building more next of the player (custa credits crescentes).
    // Returns: 0=none near, 1=evoluiu, 2=without credits, 3=level maximum.
    int  upgradeNearby(Vector2 playerPos, int& playerCredits);
    int  upgradeCostFor(const Building& b) const;   // cost p/ next level
    void applyLevelStats(Building& b);              // aplica stats conforme level
    void renderBuildingInfo(Vector2 playerPos) const; // descricao+level about buildings
    void renderBuilding(const Building& b) const;

    // ── Arca: respawn + aura of bonus ────────────────────────────────────────
    bool getArkPosition(Vector2& out) const;       // true if ha uma Arca built
    bool isInArkAura(Vector2 pos) const;           // inside the radius of uma Arca viva

    // ── Controle RTS (selecao by arrasto + ordem of move) ──────────────────
    int  selectUnitsInBox(Rectangle boxWorld);     // seleciona unidades in the caixa; returns qtd
    void clearSelection();
    void orderMove(Vector2 dest);                  // move the unidades selecionadas
    int  selectedCount() const;
    void renderSelection() const;                  // aneis verdes sob unidades selecionadas

    bool buildModeActive = false;
    int  selectedType    = 0;    // index into BuildingType enum

    std::vector<Building>       buildings;
    std::vector<FriendlyTank>   tanks;
    std::vector<FriendlySoldier> soldiers;

    // Pending projectiles from turrets / tanks — Game reads and spawns these
    struct PendingShot {
        Vector2 origin;
        Vector2 dir;
        float   damage;
        Color   color;
        float   speed;
    };
    std::vector<PendingShot> pendingShots;

    static constexpr int NUM_TYPES = 8;
    static const BuildingCost COSTS[NUM_TYPES];

private:
    int  pendingCredits   = 0;
    int  pendingMaterials = 0;

    void updateBuilding(Building& b, float dt, Vector2 playerPos,
                        const std::vector<class Enemy*>& enemies);
    void renderArkBuilding    (const Building& b) const;
    void renderHouseBuilding  (const Building& b) const;
    void renderBarracks       (const Building& b) const;
    void renderTankFactory    (const Building& b) const;
    void renderTurret         (const Building& b) const;
    void renderResourceNode   (const Building& b) const;
    void renderWall           (const Building& b) const;
    void renderMedBay         (const Building& b) const;

    void spawnTank(Vector2 factoryPos);
    void spawnSoldier(Vector2 barracksPos);

    // Find nearest enemy in range — returns nullptr if none
    const class Enemy* nearestEnemy(Vector2 from, float range,
                                     const std::vector<class Enemy*>& enemies) const;
};
