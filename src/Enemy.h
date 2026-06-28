#pragma once
#include <raylib.h>

enum class EnemyType {
    Scout,         // Enforcer leve e rapido
    Tank,          // IRON-VIII pesado
    Shooter,       // Atirador de media distancia
    Boss,          // IRON-VIII Boss
    MorphX,        // Metal liquido - regenera e se divide
    HunterDrone,   // Drone Cacador aereo - orbita e atira
    KronosSentry,  // Torrinha KRONOS - estacionaria, tira rapido
    Kamikaze,      // Corre rapido e explode no contato
    Sniper,        // Longa distancia, recua se jogador se aproxima
    // ── Alienigenas (terceira fracao) ─────────────────────────────────────────
    Zergling,      // Inseto rapido — rush suicida em bando
    Hydra,         // Cobra cuspidora de acido — atirador medio alcance
    Broodmother,   // Tanque organico pesado — spawna Zerglings ao morrer
    AlienBoss,     // Chefe alienigena massivo — multiplos padroes de ataque
    // ── WarCraft Cyberpunk (fracao KRONOS-WarCraft) ───────────────────────────
    OrcCibernetico,    // Orc enorme com implantes KRONOS — carrega no player
    PaladinCorrompido, // Paladin corrompido por nanobots — atirador dourado/roxo
    UndeadEnforcer,    // Endoskeleton necro — ressuscita uma vez ao morrer
    // ── Boss OMEGA (luta hard aleatoria) ──────────────────────────────────────
    OmegaBoss,     // Boss nivel HARD — aparece a cada 50 kills — mistura maquina+alien
    // ── Sobrenaturais — Fases Sombrias ────────────────────────────────────────
    Ghost,           // Fantasma — semi-invisivel, atravessa colisoes
    GhostElite,      // Fantasma Elite — maior, ataque psiconico AoE
    Zombie,          // Zumbi — lento, resistente, infecta
    ZombieRager,     // Zumbi Berserker — entra em rage quando player perto
    ZombieHorde,     // Invocado por ZombieLord — fraco mas em grupo
    PoltergeistBoss, // Boss fantasma — teleporta, invisivel, invoca elites
    ZombieLord,      // Boss zumbi — 3 fases, invoca Horde, regenera
    ShadowWraith,    // Sombra — rapido, drena HP
    BansheeHowler,   // Grito sonico — retarda movimento do player
    // ── Facção Meca-Orgânica (StarCraft-like) ────────────────────────────────
    CorrupterDrone,  // drone que corrompe estruturas
    ReaperMech,      // mech rápido com dual blades
    VoidColossus,    // tanque enorme, 4x tamanho, 2000 HP
    SiegeCrawler,    // modo siege: para e atira projéteis de área
    NeuralParasite,  // controla inimigos fracos próximos
    // ── Facção das Trevas (WarCraft-like) ────────────────────────────────────
    LichKnight,      // conjurador de gelo, congela player 2s
    BloodBerserker,  // mais damage quanto menos HP
    DemonHunter,     // esquiva rápida, ataque duplo
    SoulReaper,      // drena HP ao atingir
    FrostWyrm,       // boss voador, drops area de gelo
    // ── Facção Infernal ──────────────────────────────────────────────────────
    MoltenGolem,     // imune a fogo, vulnerável a gelo
    AcidSpitter,     // projéteis de ácido com poça de dano
    InfernoHerald,   // boss, invoca geysers de lava
    CrimsonBat,      // swarm rápido, pouco HP
    VolcanicTitan,   // mega boss zona inferno, 5000 HP
    // ── Humanoides Corrompidos ───────────────────────────────────────────────
    CyberSamurai,    // combo melee 3-hit rápido
    PlagueDoctor,    // lança bombas de veneno
    Necromancer,     // ressuscita inimigos mortos
    IronGuard,       // alto armor, lento mas tanky
    GhostSniper,     // invisível até atirar, snipe longa distância
    // ── Criaturas Abissais ───────────────────────────────────────────────────
    AbyssalEel,      // emerge sob o player
    VoidStalker,     // teletransporta atrás do player
    DarkMatter,      // split em 3 ao morrer
    ChaosSpawn,      // random: melee/ranged/mage
    Leviathan        // boss final multi-fase, 8000 HP
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

    // ── Combate avançado: telegraph + investida + flanqueio ───────────────────
    float telegraphTimer = 0.0f;  // >0 = "avisando" antes de atacar (render pisca)
    float telegraphMax   = 0.35f; // duração da janela de aviso
    float lungeTimer     = 0.0f;  // >0 = em investida (dash) de melee
    float lungeCooldown  = 0.0f;  // tempo até a próxima investida
    float flankSign      = 1.0f;  // lado do ângulo de aproximação (flanqueio)
    bool  flankInit      = false;
    bool  genericPhase2  = false; // bosses de facção sem lógica própria de fase

    // Elite system
    bool  isElite      = false;
    int   eliteMod     = 0;   // 0=Berserker 1=Armored 2=Volatile
    float elitePulse   = 0.0f;

    // Boss final do jogo (NUCLEO KRONOS) — sua morte vence o jogo
    bool  isFinalBoss  = false;

    // Group alert AI
    bool  alerted      = false;
    float alertTimer   = 0.0f;

    // Boss phase (1..3) + sistema de padrões de ataque épicos
    int   bossPhase    = 1;
    float phaseFlash   = 0.0f;   // flash dramático na transição de fase
    bool  bossInvuln   = false;  // breve invulnerabilidade na troca de fase (Game checa via takeDamage)
    float bossInvulnTimer = 0.0f;
    int   bossPattern   = 0;     // 0=idle 1=leque 2=varredura 3=rajada 4=anel
    float bossAtkTimer  = 2.0f;  // tempo até o próximo grande ataque
    int   burstShots    = 0;     // tiros restantes do padrão atual
    float burstGap      = 0.05f; // intervalo entre tiros do padrão
    float burstTimer    = 0.0f;
    float burstAngle    = 0.0f;  // ângulo atual do leque/varredura (rad)
    float burstStep     = 0.0f;  // passo angular por tiro (rad)

    // Kamikaze: wants to explode (checked by Game)
    bool  wantsToExplode = false;

    // UndeadEnforcer: ressurreicao unica
    bool  hasRevived     = false;

    // Sobrenaturais
    float invisTimer     = 0.0f;  // Ghost: tempo ate proxima mudanca de visibilidade
    bool  isInvisible    = false;  // Ghost: estado atual
    int   summonCount    = 0;      // ZombieLord: quantos invocou
    float infectTimer    = 0.0f;   // Zombie: timer de infeccao
    float phaseTimer     = 0.0f;   // timer generico de fase (bosses sobrenaturais)
    bool  hasRaged       = false;   // ZombieRager: ja entrou em rage

    // ── Auto-evolução por tempo de vida ───────────────────────────────────────
    float aliveTimer     = 0.0f;   // segundos vivo
    int   evolTier       = 0;      // 0=normal 1=veterano 2=elite 3=lendario
    bool  justEvolved    = false;  // flag: evoluiu neste frame (Game consome)
    bool  netKilled      = false;  // morto por sync de rede — nao rebroadcastar a morte
    float evolveFlash    = 0.0f;   // timer do flash branco ao evoluir
    Color auraColor      = {0,0,0,0}; // cor da aura de tier

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
    void  render3D() const;   // modelo 3D low-poly (sem cubos)
    void  takeDamage(float amount);
    void  applyKnockback(Vector2 dir, float force);
    void  makeElite(int mod);   // elevate to elite variant
    void  alert();              // called by nearby allies when hit
    bool  isDead() const;
    bool  shouldDropLoot() const;
    void  markLootDropped();
    bool  canAttackPlayer(Vector2 playerPos) const;
    float attackIfReady(float dt, Vector2 playerPos);

private:
    float attackCooldown  = 0.0f;
    float attackRate      = 1.0f;
    float walkAnimTimer   = 0.0f;
    int   facing          = 1;
    float barrelAngle     = 0.0f; // for KronosSentry
    float roarTimer       = 0.0f; // OrcCibernetico — rugido boost timer
    bool  roarActive      = false;

    void  setupByType();
    // Classe de comportamento de combate p/ os tipos que usam a IA genérica.
    // 0=perseguidor melee  1=atirador kiter  2=flanqueador rápido
    // 3=brutamonte (investida)  4=boss (aproxima + barragem)
    int   combatRole() const;
    // Sistema de padrões de ataque épicos de boss (3 fases) — chamado dos updates de boss.
    void  updateBossPatterns(float dt, Vector2 norm);
    void  checkBossPhases();   // transições de fase com flash/invuln
    void  renderBossAura() const; // aura/coroa de fase (chamado nos renders de boss)
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

    // SOMENTE entidades incorporeas que realmente flutuam no ar. Zumbis (apesar de
    // "sobrenaturais") sao corporeos e devem ANDAR no chao, nao flutuar.
    bool isFloating() const {
        return type == EnemyType::Ghost        || type == EnemyType::GhostElite ||
               type == EnemyType::ShadowWraith || type == EnemyType::BansheeHowler ||
               type == EnemyType::PoltergeistBoss;
    }
};
