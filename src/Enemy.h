#pragma once
#include <raylib.h>

enum class EnemyType {
    Scout,         // Enforcer light and fast
    Tank,          // IRON-VIII heavy
    Shooter,       // Shooter of media distance
    Boss,          // IRON-VIII Boss
    MorphX,        // Metal liquido - regenera and if divide
    HunterDrone,   // Drone Hunter aereo - orbita and atira
    KronosSentry,  // Torrinha KRONOS - estacionaria, tira fast
    Kamikaze,      // Corre fast and explode in the contato
    Sniper,        // Longa distance, retreats if player if aproxima
    // ── Alienigenas (third fracao) ─────────────────────────────────────────
    Zergling,      // Inseto fast — rush suicida in bando
    Hydra,         // Cobra cuspidora of acid — shooter medio range
    Broodmother,   // Tanque organic heavy — spawns Zerglings to the die
    AlienBoss,     // Boss alienigena massivo — multiplos padroes of attack
    // ── WarCraft Cyberpunk (fracao KRONOS-WarCraft) ───────────────────────────
    OrcCibernetico,    // Orc enorme with implantes KRONOS — loads in the player
    PaladinCorrompido, // Paladin corrompido by nanobots — shooter golden/purple
    UndeadEnforcer,    // Endoskeleton necro — ressuscita uma vez to the die
    // ── Boss OMEGA (luta hard aleatoria) ──────────────────────────────────────
    OmegaBoss,     // Boss level HARD — aparece the cada 50 kills — mistura maquina+alien
    // ── Sobrenaturais — Phases Sombrias ────────────────────────────────────────
    Ghost,           // Fantasma — semi-invisible, atravessa colisoes
    GhostElite,      // Fantasma Elite — maior, attack psiconico AoE
    Zombie,          // Zombie — slow, resistant, infecta
    ZombieRager,     // Zombie Berserker — enters in rage when player near
    ZombieHorde,     // Invocado by ZombieLord — weak mas in group
    PoltergeistBoss, // Boss ghost — teleporta, invisible, invoca elites
    ZombieLord,      // Boss zombie — 3 phases, invoca Horde, regenera
    ShadowWraith,    // Shadow — fast, drena HP
    BansheeHowler,   // Grito sonico — retarda movement of the player
    // ── Faction Meca-Organica (StarCraft-like) ────────────────────────────────
    CorrupterDrone,  // drone that corrompe estruturas
    ReaperMech,      // mech fast with dual blades
    VoidColossus,    // tank enorme, 4x size, 2000 HP
    SiegeCrawler,    // modo siege: to and atira projectiles of area
    NeuralParasite,  // controla enemies fracos proximos
    // ── Faction of the Trevas (WarCraft-like) ────────────────────────────────────
    LichKnight,      // conjurador of ice, congela player 2s
    BloodBerserker,  // more damage the less HP
    DemonHunter,     // dodge fast, attack double
    SoulReaper,      // drena HP to the atingir
    FrostWyrm,       // boss voador, drops area of ice
    // ── Faction Infernal ──────────────────────────────────────────────────────
    MoltenGolem,     // immune the fire, vulneravel the ice
    AcidSpitter,     // projectiles of acid with poca of damage
    InfernoHerald,   // boss, invoca geysers of lava
    CrimsonBat,      // swarm fast, little HP
    VolcanicTitan,   // mega boss zone inferno, 5000 HP
    // ── Humanoides Corrompidos ───────────────────────────────────────────────
    CyberSamurai,    // combo melee 3-hit fast
    PlagueDoctor,    // lanca bombas of poison
    Necromancer,     // ressuscita enemies mortos
    IronGuard,       // high armor, slow mas tanky
    GhostSniper,     // invisible until shoot, snipe longa distance
    // ── Criaturas Abissais ───────────────────────────────────────────────────
    AbyssalEel,      // emerge sob the player
    VoidStalker,     // teletransporta behind of the player
    DarkMatter,      // split in 3 to the die
    ChaosSpawn,      // random: melee/ranged/mage
    Leviathan        // final boss multi-phase, 8000 HP
};

class Enemy {
public:
    Vector2   position      = {0, 0};
    float     speed         = 80.0f;
    float     health        = 50.0f;
    float     maxHealth     = 50.0f;
    float     damage        = 5.0f;
    float     radius        = 14.0f;
    bool      droppedLoot   = false;
    bool      hasSplit      = false;
    bool      isMinion      = false;
    EnemyType type          = EnemyType::Scout;
    int       xpReward      = 10;
    Color     bodyColor     = RED;

    // Hit flash
    float   hitFlashTimer     = 0.0f;

    // Knockback / strafe AI
    Vector2 knockback    = {0, 0};
    float   strafeDir    = 1.0f;
    float   strafeTimer  = 0.0f;

    // ── Combat avancado: telegraph + investida + flanqueio ───────────────────
    float telegraphTimer = 0.0f;  // >0 = "avisando" before attack (render pisca)
    float telegraphMax   = 0.35f; // duration of the window of warning
    float lungeTimer     = 0.0f;  // >0 = in investida (dash) of melee
    float lungeCooldown  = 0.0f;  // time until the next investida
    float flankSign      = 1.0f;  // lado of the angle of aproximacao (flanqueio)
    bool  flankInit      = false;
    bool  genericPhase2  = false; // bosses of faction without logica own of phase

    // Elite system
    bool  isElite      = false;
    int   eliteMod     = 0;   // 0=Berserker 1=Armored 2=Volatile 3=Shielded 4=KronosRapid
    float elitePulse   = 0.0f;

    // Mitigacao (estilo ARPG): armor reduz damage plano by hit; shield Shielded
    // absorve uma fracao of cada golpe and regenera with the time.
    float armor         = 0.0f;
    float shieldHp      = 0.0f;
    float shieldMax     = 0.0f;

    // Boss final of the game (NUCLEO KRONOS) — your death vence the game
    bool  isFinalBoss  = false;

    // Group alert AI
    bool  alerted      = false;
    float alertTimer   = 0.0f;

    // Boss phase (1..3) + system of padroes of attack epicos
    int   bossPhase    = 1;
    float phaseFlash   = 0.0f;   // flash dramatico in the transition of phase
    bool  bossInvuln   = false;  // breve invulnerabilidade in the troca of phase (Game checks via takeDamage)
    float bossInvulnTimer = 0.0f;
    int   bossPattern   = 0;     // 0=idle 1=leque 2=varredura 3=burst 4=anel
    float bossAtkTimer  = 2.0f;  // time until the next big attack
    int   burstShots    = 0;     // shots restantes of the padrao current
    float burstGap      = 0.05f; // intervalo between shots of the padrao
    float burstTimer    = 0.0f;
    float burstAngle    = 0.0f;  // angle current of the leque/varredura (rad)
    float burstStep     = 0.0f;  // passo angular by shot (rad)

    // Kamikaze: wants to explode (checked by Game)
    bool  wantsToExplode = false;

    // UndeadEnforcer: ressurreicao only
    bool  hasRevived     = false;

    // Sobrenaturais
    float invisTimer     = 0.0f;  // Ghost: time until next change of visibilidade
    bool  isInvisible    = false;  // Ghost: state current
    int   summonCount    = 0;      // ZombieLord: quantos invocou
    float infectTimer    = 0.0f;   // Zombie: timer of infeccao
    float phaseTimer     = 0.0f;   // timer generico of phase (bosses sobrenaturais)
    bool  hasRaged       = false;   // ZombieRager: already entered in rage

    // ── Auto-evolution by time of health ───────────────────────────────────────
    float aliveTimer     = 0.0f;   // seconds vivo
    int   evolTier       = 0;      // 0=normal 1=veterano 2=elite 3=legendary
    bool  justEvolved    = false;  // flag: evoluiu neste frame (Game consome)
    bool  netKilled      = false;  // dead by sync of network — not rebroadcastar the death
    float evolveFlash    = 0.0f;   // timer of the flash white to the evoluir
    Color auraColor      = {0,0,0,0}; // color of the aura of tier

    static float getGlobalScaling(int totalKills, int waveNumber) {
        float killBonus = (totalKills / 50) * 0.05f;
        float waveBonus = waveNumber * 0.08f;
        float total     = 1.0f + killBonus + waveBonus;
        return total > 4.0f ? 4.0f : total; // cap +300%
    }

private:
    // Patrol AI (Scout/Tank when far from player)
    Vector2 patrolTarget  = {0, 0};
    float   patrolTimer   = 0.0f;
    bool    patrolInit    = false;

public:

    // Shooting state (checked by Game each frame)
    bool    wantsToShoot      = false;
    Vector2 shootDirection    = {0, 0};
    float   shootDamage       = 8.0f;
    float   shootSpeed        = 340.0f;
    Color   projectileColor   = {255, 100, 0, 255};
    float   shootRange        = 280.0f;
    float   shootCooldown     = 0.0f;
    float   shootRate         = 2.0f;

    // MorphX regen
    float regenRate = 0.0f;

    // HunterDrone orbit
    float orbitAngle   = 0.0f;
    float orbitRadius  = 200.0f;

    explicit Enemy(Vector2 startPos, EnemyType t = EnemyType::Scout, bool minion = false);

    void  update(float dt, Vector2 target);
    void  render() const;
    void  takeDamage(float amount);
    void  applyKnockback(Vector2 dir, float force);
    void  makeElite(int mod);   // elevate to elite variant
    void  alert();              // called by nearby allies when hit
    bool  isDead() const;
    bool  shouldDropLoot() const;
    void  markLootDropped();
    bool  canAttackPlayer(Vector2 playerPos) const;
    float attackIfReady(float dt, Vector2 playerPos);

    // public: the Game reads the phase of the passo to choose the QUADRO of the model 3D and
    // strength phases especificas to the generate the quadros of the caminhada.
    float walkAnimTimer   = 0.0f;

private:
    float attackCooldown  = 0.0f;
    float attackRate      = 1.0f;
    int   facing          = 1;
    float barrelAngle     = 0.0f; // for KronosSentry
    float roarTimer       = 0.0f; // OrcCibernetico — rugido boost timer
    bool  roarActive      = false;

    void  setupByType();
    // Class of behavior of combat p/ the types that usam the IA generica.
    // 0=perseguidor melee  1=shooter kiter  2=flanqueador fast
    // 3=brutamonte (investida)  4=boss (aproxima + barragem)
    int   combatRole() const;
    // Sistema of padroes of attack epicos of boss (3 phases) — called of the updates of boss.
    void  updateBossPatterns(float dt, Vector2 norm);
    void  checkBossPhases();   // transicoes of phase with flash/invuln
    void  renderBossAura() const; // aura/coroa of phase (called in the renders of boss)
    void  updateMorphX(float dt, Vector2 target);
    void  updateHunterDrone(float dt, Vector2 target);
    void  updateKronosSentry(float dt, Vector2 target);
    void  updateKamikaze(float dt, Vector2 target);
    void  updateSniper(float dt, Vector2 target);
    void  updateZergling(float dt, Vector2 target);
    void  updateHydra(float dt, Vector2 target);
    void  updateBroodmother(float dt, Vector2 target);
    void  renderMorphX() const;
    void  renderHunterDrone() const;
    void  renderKronosSentry() const;
    void  renderKamikaze() const;
    void  renderSniper() const;
    void  renderZergling() const;
    void  renderHydra() const;
    void  renderBroodmother() const;
    void  renderAlienBoss() const;
    void  renderOmegaBoss() const;
    void  updateAlienBoss(float dt, Vector2 target);
    void  updateOrcCibernetico(float dt, Vector2 target);
    void  updatePaladinCorrompido(float dt, Vector2 target);
    void  updateUndeadEnforcer(float dt, Vector2 target);
    void  renderOrcCibernetico() const;
    void  renderPaladinCorrompido() const;
    void  renderUndeadEnforcer() const;

    // Sobrenaturais
    void  updateGhost(float dt, Vector2 target);
    void  updateZombie(float dt, Vector2 target);
    void  updateZombieRager(float dt, Vector2 target);
    void  updateZombieLord(float dt, Vector2 target);
    void  updatePoltergeistBoss(float dt, Vector2 target);
    void  updateShadowWraith(float dt, Vector2 target);
    void  updateBansheeHowler(float dt, Vector2 target);
    void  renderGhost() const;
    void  renderZombie() const;
    void  renderZombieRager() const;
    void  renderZombieLord() const;
    void  renderPoltergeistBoss() const;
    void  renderShadowWraith() const;
    void  renderBansheeHowler() const;

public:
    bool isBoss() const {
        return type == EnemyType::Boss            || type == EnemyType::AlienBoss       ||
               type == EnemyType::OmegaBoss       || type == EnemyType::PoltergeistBoss ||
               type == EnemyType::ZombieLord      || type == EnemyType::VoidColossus    ||
               type == EnemyType::FrostWyrm       || type == EnemyType::InfernoHerald   ||
               type == EnemyType::VolcanicTitan   || type == EnemyType::Leviathan;
    }
    bool isSupernatural() const {
        return type == EnemyType::Ghost           || type == EnemyType::GhostElite      ||
               type == EnemyType::Zombie          || type == EnemyType::ZombieRager     ||
               type == EnemyType::ZombieHorde     || type == EnemyType::PoltergeistBoss ||
               type == EnemyType::ZombieLord      || type == EnemyType::ShadowWraith    ||
               type == EnemyType::BansheeHowler;
    }

    // SOMENTE entidades incorporeas that really flutuam in the ar. Zumbis (apesar of
    // "sobrenaturais") sao corporeos and devem ANDAR in the floor, not flutuar.
    bool isFloating() const {
        return type == EnemyType::Ghost        || type == EnemyType::GhostElite ||
               type == EnemyType::ShadowWraith || type == EnemyType::BansheeHowler ||
               type == EnemyType::PoltergeistBoss;
    }
};
