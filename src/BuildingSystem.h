#pragma once
#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <string>

enum class BuildingType {
    Ark,           // Base principal — heal player nearby, spawn point
    House,         // Gera creditos passivamente
    Barracks,      // Gera soldados aliados (infantry AI)
    TankFactory,   // Produz tanques amigos que atacam inimigos
    Turret,        // Torre automatica que atira nos inimigos
    ResourceNode,  // Gera materiais de crafting
    Wall,          // Barreira que bloqueia inimigos
    MedBay,        // Cura player e companions ao redor
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

    // RTS — selecao e ordem de mover
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

    // RTS — selecao e ordem de mover
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

    // Evolucao do predio — nivel 1..3, cada nivel melhora os stats
    int          level        = 1;
    static const int MAX_LEVEL = 3;

    // Production timers
    float        buildTimer   = 0.f;    // construction progress 0→1
    bool         built        = false;
    float        builtAt      = 0.f;    // GetTime() quando a construcao terminou (efeito de materializacao)
    float        productionTimer = 0.f;
    float        productionRate  = 15.f; // seconds per unit/credit cycle

    // Fila de producao (Quartel/Fabrica) — unidades NAO nascem na hora.
    // Cada clique/mao-de-obra enfileira; a unidade saí pronta so depois de spawnTime.
    int          spawnQueue   = 0;    // unidades enfileiradas (inclui a atual)
    float        spawnTimer   = 0.f;  // progresso da unidade atual (0→spawnTime)
    float        spawnTime    = 5.f;  // segundos de producao por unidade

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

    void toggleBuildMode();
    bool tryPlace(Vector2 worldPos, int playerCredits, int playerMetal, int playerCarapace,
                  int& outCreditCost, int& outMetalCost, int& outCarapaceCost);
    // Hit-test do menu de construcao (mouse em coords virtuais). Retorna o indice
    // do predio sob o cursor, ou -1 se o clique NAO foi no painel do menu.
    int  menuCellAt(Vector2 screenMouse, int screenW, int screenH) const;

    void healPlayerIfNear(Vector2 playerPos, float& playerHealth, float playerMaxHealth);
    int  collectCredits();       // returns credits generated this frame
    int  collectMaterials();     // returns metal scraps generated

    // Clique numa fabrica/quartel para produzir uma unidade na hora (custa creditos).
    // Retorna: 0=nao clicou em predio, 1=produziu, 2=sem creditos, 3=limite atingido.
    int  clickProduce(Vector2 worldPos, int& playerCredits);
    // Prompts flutuantes sobre fabricas/quarteis (chamado dentro de BeginMode2D)
    void renderUnitPrompts() const;

    // ── Evolucao de predios ──────────────────────────────────────────────────
    // Evolui o predio mais proximo do jogador (custa creditos crescentes).
    // Retorna: 0=nenhum perto, 1=evoluiu, 2=sem creditos, 3=nivel maximo.
    int  upgradeNearby(Vector2 playerPos, int& playerCredits);
    int  upgradeCostFor(const Building& b) const;   // custo p/ proximo nivel
    void applyLevelStats(Building& b);              // aplica stats conforme nivel
    void renderBuildingInfo(Vector2 playerPos) const; // descricao+nivel sobre predios
    void renderBuilding(const Building& b) const;

    // ── Arca: respawn + aura de bonus ────────────────────────────────────────
    bool getArkPosition(Vector2& out) const;       // true se ha uma Arca construida
    bool isInArkAura(Vector2 pos) const;           // dentro do raio de uma Arca viva

    // ── Controle RTS (selecao por arrasto + ordem de mover) ──────────────────
    int  selectUnitsInBox(Rectangle boxWorld);     // seleciona unidades na caixa; retorna qtd
    void clearSelection();
    void orderMove(Vector2 dest);                  // move as unidades selecionadas
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
