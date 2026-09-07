#pragma once
#include <raylib.h>
#include <string>
#include <vector>

// Forward declaration — full definition in Enemy.h
class Enemy;

enum class CompanionType {
    // ── Existentes (ordem PRESERVADA — Game usa F2/F3/F4) ────────────────────
    MarcoVeil,    // humano NEXUS — shooter of precisao (sharpshooter)
    Steel,        // robo reprogramado — tank/guardiao with pisada AoE
    Rex,          // cao robotico — flanqueador veloz body-the-body
    // ── Novos (precisam the Game ligar key p/ invocar; see flags below) ─────
    Guardian,     // shield heavy — stays ENTRE the player and the ameaca (body-block)
    Sniper,       // shooter of elite — range enorme, damage high, recharge slow
    Healer,       // dron medico — regenera the player (usa wantsHeal — Game precisa read)
    LootDrone,    // dron coletor — gets items/loot (usa wantsCollect — Game precisa read)
};

struct Companion {
    Vector2  position     = {0, 0};
    Vector2  velocity     = {0, 0};
    float    health       = 100.f;
    float    maxHealth    = 100.f;
    float    damage       = 15.f;
    float    speed        = 120.f;
    float    radius       = 14.f;
    CompanionType type    = CompanionType::MarcoVeil;
    std::string name;
    bool     active       = false;

    // AI state
    float    attackCooldown = 0.f;
    float    attackRate     = 1.5f;
    float    walkTimer      = 0.f;
    int      facing         = 1;
    float    skillCooldown  = 0.f;
    float    skillRate       = 8.f;
    bool     wantsSkill      = false;
    bool     alerted         = false;   // ha enemies by near
    float    formationPhase  = 0.f;     // angle own in the formation around of the player

    // Shooting
    bool     wantsToShoot   = false;
    Vector2  shootDir       = {1, 0};
    float    shootDamage    = 12.f;
    float    shootSpeed     = 380.f;
    float    shootRange     = 260.f;
    Color    projectileColor= {0, 200, 255, 255};
    float    shootCooldown  = 0.f;
    float    shootRate2     = 1.8f;

    float    hitFlash       = 0.f;

    // Death / revive
    float    deadTimer      = 0.f;          // counts up while dead
    static constexpr float deadDuration = 9.f;  // seconds caido before reviver

    // DogAI rush state
    bool     isRushing      = false;
    float    rushTimer      = 0.f;
    Vector2  rushTarget     = {0, 0};

    // Skill burst (MarcoVeil/Sniper)
    int      burstCount     = 0;
    float    burstTimer     = 0.f;
    static constexpr float burstInterval = 0.1f;

    // AoE stomp flag (Steel/Guardian — checado pelo Game)
    bool     wantsAoE       = false;
    float    aoeRadius      = 80.f;
    float    aoeDamage      = 30.f;

    // ── NOVAS interacoes (Game can LER and aplicar; default = inertes) ────────
    bool     wantsHeal      = false;   // Healer: the Game must heal the player
    float    healAmount     = 0.f;     //  the heal neste frame
    float    healRadius     = 200.f;
    bool     taunting       = false;   // Guardian/Steel in postura of guard
    bool     wantsCollect   = false;   // LootDrone: sinaliza coleta of items proximos
    float    collectRadius  = 160.f;

    // ── Personalidade (emotes) — self-contained, render own ──────────────
    std::string emoteText;
    float       emoteTimer  = 0.f;
    float       idleEmoteCD = 6.f;
    float       skillFlash  = 0.f;     // glow to the usar skill

    explicit Companion(CompanionType t);

    // Reseta the contador of "slots" of formation between partidas (restartRun) —
    // before era um static of .cpp that never voltava the zero.
    static void resetSpawnIndex();

    void update(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies);
    void render() const;
    void takeDamage(float dmg);
    bool isDead() const;
    void revive(Vector2 pos);

private:
    void updateMarcoVeil(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies);
    void updateSteel    (float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies);
    void updateRex      (float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies);
    void updateGuardian (float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies);
    void updateSniper   (float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies);
    void updateHealer   (float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies);
    void updateLootDrone(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies);

    void renderMarcoVeil() const;
    void renderSteel    () const;
    void renderRex      () const;
    void renderGuardian () const;
    void renderSniper   () const;
    void renderDrone    (Color tint) const;   // Healer/LootDrone (drones flutuantes)
    void renderHpBar    () const;
    void renderEmote    () const;
    void renderSkillReady() const;

    // Movement: orbita the player numa "slot" of formation (espalha companheiros)
    void followFormation(float dt, Vector2 playerPos, float desiredDist);
    void retreatTo(float dt, Vector2 playerPos);    // retreats p/ near the player (HP down)
    void emote(const char* txt, float dur = 2.2f);
    Enemy* findNearestEnemy(const std::vector<Enemy*>& enemies, float maxRange) const;
    int    countEnemies(const std::vector<Enemy*>& enemies, Vector2 center, float r) const;
};
