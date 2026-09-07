#include "Enemy.h"
#include "Effects.h"
#include "SpriteGen.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

extern bool g_voxelCapture;

Enemy::Enemy(Vector2 startPos, EnemyType t, bool minion)
    : position(startPos), type(t), isMinion(minion) {
    orbitAngle = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
    setupByType();
}

void Enemy::setupByType() {
    switch (type) {
        case EnemyType::Scout:
            speed = 130.0f; health = maxHealth = 35.0f;
            damage = 4.0f; radius = 12.0f; xpReward = 10;
            bodyColor = ORANGE; attackRate = 0.8f;
            break;
        case EnemyType::Tank:
            speed = 50.0f; health = maxHealth = 180.0f;
            damage = 14.0f; radius = 22.0f; xpReward = 30;
            bodyColor = {140, 30, 30, 255}; attackRate = 1.5f;
            break;
        case EnemyType::Shooter:
            speed = 90.0f; health = maxHealth = 60.0f;
            damage = 4.0f; radius = 14.0f; xpReward = 18;
            bodyColor = PURPLE; attackRate = 2.0f;
            shootRate = 1.8f; shootDamage = 12.0f; shootRange = 280.0f;
            shootSpeed = 360.0f; projectileColor = {180, 0, 255, 255};
            break;
        case EnemyType::Boss:
            speed = 60.0f; health = maxHealth = 600.0f;
            damage = 22.0f; radius = 36.0f; xpReward = 250;
            bodyColor = {200, 0, 0, 255}; attackRate = 1.2f;
            shootRate = 2.5f; shootDamage = 18.0f; shootRange = 320.0f;
            shootSpeed = 400.0f; projectileColor = {255, 50, 0, 255};
            break;
        case EnemyType::MorphX:
            if (isMinion) {
                speed = 200.0f; health = maxHealth = 30.0f;
                damage = 6.0f; radius = 9.0f; xpReward = 8;
            } else {
                speed = 160.0f; health = maxHealth = 90.0f;
                damage = 10.0f; radius = 13.0f; xpReward = 40;
            }
            bodyColor = {180, 190, 200, 255}; attackRate = 0.9f;
            regenRate = isMinion ? 0.0f : 3.0f;
            break;
        case EnemyType::HunterDrone:
            speed = 110.0f; health = maxHealth = 70.0f;
            damage = 0.0f; radius = 18.0f; xpReward = 35;
            bodyColor = {0, 200, 220, 255}; attackRate = 99.0f;
            shootRate = 2.2f; shootDamage = 15.0f; shootRange = 999.0f;
            shootSpeed = 420.0f; projectileColor = {0, 230, 255, 255};
            orbitRadius = 190.0f;
            break;
        case EnemyType::KronosSentry:
            speed = 0.0f; health = maxHealth = 45.0f;
            damage = 0.0f; radius = 22.0f; xpReward = 20;
            bodyColor = {200, 80, 0, 255}; attackRate = 99.0f;
            shootRate = 1.0f; shootDamage = 10.0f; shootRange = 320.0f;
            shootSpeed = 450.0f; projectileColor = {255, 140, 0, 255};
            break;
        case EnemyType::Kamikaze:
            speed = 240.0f; health = maxHealth = 25.0f;
            damage = 60.0f; radius = 11.0f; xpReward = 15;
            bodyColor = {255, 30, 0, 255}; attackRate = 99.0f;
            break;
        case EnemyType::Sniper:
            speed = 70.0f; health = maxHealth = 50.0f;
            damage = 0.0f; radius = 12.0f; xpReward = 22;
            bodyColor = {80, 180, 80, 255}; attackRate = 99.0f;
            shootRate = 3.5f; shootDamage = 28.0f; shootRange = 500.0f;
            shootSpeed = 520.0f; projectileColor = {80, 255, 80, 255};
            break;
        // ── Alienigenas ────────────────────────────────────────────────────────
        case EnemyType::Zergling:
            speed = 320.0f; health = maxHealth = 20.0f;
            damage = 12.0f; radius = 10.0f; xpReward = 8;
            bodyColor = {50, 180, 0, 255}; attackRate = 0.6f;
            break;
        case EnemyType::Hydra:
            speed = 90.0f; health = maxHealth = 80.0f;
            damage = 0.0f; radius = 16.0f; xpReward = 35;
            bodyColor = {0, 150, 50, 255}; attackRate = 99.0f;
            shootRate = 2.2f; shootDamage = 18.0f; shootRange = 350.0f;
            shootSpeed = 400.0f; projectileColor = {100, 255, 0, 255};
            break;
        case EnemyType::Broodmother:
            speed = 55.0f; health = maxHealth = 280.0f;
            damage = 18.0f; radius = 28.0f; xpReward = 120;
            bodyColor = {30, 100, 0, 255}; attackRate = 1.5f;
            break;
        case EnemyType::AlienBoss:
            speed = 50.0f; health = maxHealth = 800.0f;
            damage = 25.0f; radius = 40.0f; xpReward = 400;
            bodyColor = {20, 80, 0, 255}; attackRate = 1.2f;
            shootRate = 2.0f; shootDamage = 20.0f; shootRange = 340.0f;
            shootSpeed = 380.0f; projectileColor = {80, 255, 0, 255};
            break;
        case EnemyType::OmegaBoss:
            speed = 42.0f; health = maxHealth = 3200.0f;
            damage = 55.0f; radius = 62.0f; xpReward = 1500;
            bodyColor = {120, 0, 220, 255}; attackRate = 1.0f;
            shootRate = 1.4f; shootDamage = 35.0f; shootRange = 450.0f;
            shootSpeed = 480.0f; projectileColor = {200, 0, 255, 255};
            break;
        // ── WarCraft Cyberpunk ─────────────────────────────────────────────────
        case EnemyType::OrcCibernetico:
            speed = 65.0f; health = maxHealth = 220.0f;
            damage = 18.0f; radius = 26.0f; xpReward = 80;
            bodyColor = {40, 100, 30, 255}; attackRate = 1.4f;
            break;
        case EnemyType::PaladinCorrompido:
            speed = 80.0f; health = maxHealth = 160.0f;
            damage = 0.0f; radius = 18.0f; xpReward = 70;
            bodyColor = {180, 150, 30, 255}; attackRate = 99.0f;
            shootRate = 2.5f; shootDamage = 22.0f; shootRange = 300.0f;
            shootSpeed = 380.0f; projectileColor = {255, 220, 0, 255};
            break;
        case EnemyType::UndeadEnforcer:
            speed = 95.0f; health = maxHealth = 90.0f;
            damage = 12.0f; radius = 14.0f; xpReward = 45;
            bodyColor = {200, 185, 150, 255}; attackRate = 0.8f;
            break;
        // ── Sobrenaturais ──────────────────────────────────────────────────────
        case EnemyType::Ghost:
            speed = 85.0f; health = maxHealth = 55.0f;
            damage = 18.0f; radius = 18.0f; xpReward = 90;
            bodyColor = {160, 200, 255, 180};
            attackRate = 1.2f;
            break;
        case EnemyType::GhostElite:
            speed = 70.0f; health = maxHealth = 130.0f;
            damage = 32.0f; radius = 26.0f; xpReward = 220;
            bodyColor = {100, 150, 255, 160};
            shootRate = 2.5f; shootDamage = 28.0f; shootRange = 200.0f;
            shootSpeed = 220.0f; projectileColor = {80, 0, 200, 200};
            attackRate = 99.0f;
            break;
        case EnemyType::Zombie:
            speed = 38.0f; health = maxHealth = 95.0f;
            damage = 22.0f; radius = 20.0f; xpReward = 65;
            bodyColor = {60, 110, 50, 255};
            attackRate = 1.5f;
            break;
        case EnemyType::ZombieRager:
            speed = 45.0f; health = maxHealth = 70.0f;
            damage = 30.0f; radius = 18.0f; xpReward = 100;
            bodyColor = {120, 40, 40, 255};
            attackRate = 0.9f;
            break;
        case EnemyType::ZombieHorde:
            speed = 55.0f; health = maxHealth = 25.0f;
            damage = 12.0f; radius = 14.0f; xpReward = 20;
            bodyColor = {70, 100, 55, 255};
            attackRate = 1.0f;
            break;
        case EnemyType::PoltergeistBoss:
            speed = 95.0f; health = maxHealth = 1800.0f;
            damage = 40.0f; radius = 45.0f; xpReward = 1200;
            bodyColor = {120, 80, 220, 200};
            shootRate = 1.2f; shootDamage = 32.0f; shootRange = 380.0f;
            shootSpeed = 350.0f; projectileColor = {180, 0, 255, 210};
            attackRate = 99.0f;
            break;
        case EnemyType::ZombieLord:
            speed = 55.0f; health = maxHealth = 2400.0f;
            damage = 48.0f; radius = 55.0f; xpReward = 1400;
            bodyColor = {30, 80, 30, 255};
            shootRate = 1.8f; shootDamage = 25.0f; shootRange = 300.0f;
            shootSpeed = 260.0f; projectileColor = {0, 200, 50, 255};
            attackRate = 99.0f;
            break;
        case EnemyType::ShadowWraith:
            speed = 110.0f; health = maxHealth = 45.0f;
            damage = 28.0f; radius = 16.0f; xpReward = 110;
            bodyColor = {20, 10, 40, 240};
            attackRate = 0.8f;
            break;
        case EnemyType::BansheeHowler:
            speed = 60.0f; health = maxHealth = 80.0f;
            damage = 15.0f; radius = 22.0f; xpReward = 130;
            bodyColor = {200, 180, 255, 200};
            shootRate = 3.5f; shootDamage = 8.0f; shootRange = 250.0f;
            shootSpeed = 300.0f; projectileColor = {255, 240, 200, 200};
            attackRate = 99.0f;
            break;
        // ── Facção Meca-Orgânica ───────────────────────────────────────────────
        case EnemyType::CorrupterDrone:
            speed = 140.0f; health = maxHealth = 55.0f;
            damage = 0.0f; radius = 14.0f; xpReward = 28;
            bodyColor = {0, 180, 100, 255}; attackRate = 99.0f;
            shootRate = 2.0f; shootDamage = 14.0f; shootRange = 300.0f;
            shootSpeed = 380.0f; projectileColor = {0, 255, 150, 255};
            break;
        case EnemyType::ReaperMech:
            speed = 200.0f; health = maxHealth = 90.0f;
            damage = 20.0f; radius = 15.0f; xpReward = 50;
            bodyColor = {160, 20, 60, 255}; attackRate = 0.6f;
            break;
        case EnemyType::VoidColossus:
            speed = 35.0f; health = maxHealth = 2000.0f;
            damage = 40.0f; radius = 55.0f; xpReward = 1200;
            bodyColor = {60, 0, 120, 255}; attackRate = 1.8f;
            shootRate = 2.0f; shootDamage = 30.0f; shootRange = 350.0f;
            shootSpeed = 320.0f; projectileColor = {140, 0, 255, 255};
            break;
        case EnemyType::SiegeCrawler:
            speed = 50.0f; health = maxHealth = 160.0f;
            damage = 0.0f; radius = 22.0f; xpReward = 80;
            bodyColor = {100, 80, 30, 255}; attackRate = 99.0f;
            shootRate = 1.5f; shootDamage = 35.0f; shootRange = 420.0f;
            shootSpeed = 340.0f; projectileColor = {255, 200, 0, 255};
            break;
        case EnemyType::NeuralParasite:
            speed = 120.0f; health = maxHealth = 40.0f;
            damage = 8.0f; radius = 12.0f; xpReward = 35;
            bodyColor = {100, 255, 200, 255}; attackRate = 1.0f;
            break;
        // ── Facção das Trevas ──────────────────────────────────────────────────
        case EnemyType::LichKnight:
            speed = 70.0f; health = maxHealth = 180.0f;
            damage = 0.0f; radius = 20.0f; xpReward = 110;
            bodyColor = {140, 180, 220, 255}; attackRate = 99.0f;
            shootRate = 2.0f; shootDamage = 25.0f; shootRange = 320.0f;
            shootSpeed = 300.0f; projectileColor = {160, 220, 255, 255};
            break;
        case EnemyType::BloodBerserker:
            speed = 100.0f; health = maxHealth = 120.0f;
            damage = 25.0f; radius = 18.0f; xpReward = 75;
            bodyColor = {200, 10, 10, 255}; attackRate = 0.7f;
            break;
        case EnemyType::DemonHunter:
            speed = 180.0f; health = maxHealth = 100.0f;
            damage = 22.0f; radius = 16.0f; xpReward = 65;
            bodyColor = {80, 0, 140, 255}; attackRate = 0.5f;
            break;
        case EnemyType::SoulReaper:
            speed = 90.0f; health = maxHealth = 130.0f;
            damage = 30.0f; radius = 20.0f; xpReward = 90;
            bodyColor = {40, 0, 80, 255}; attackRate = 1.1f;
            break;
        case EnemyType::FrostWyrm:
            speed = 60.0f; health = maxHealth = 1800.0f;
            damage = 35.0f; radius = 50.0f; xpReward = 1100;
            bodyColor = {180, 230, 255, 255}; attackRate = 99.0f;
            shootRate = 1.8f; shootDamage = 28.0f; shootRange = 400.0f;
            shootSpeed = 360.0f; projectileColor = {200, 240, 255, 255};
            break;
        // ── Facção Infernal ────────────────────────────────────────────────────
        case EnemyType::MoltenGolem:
            speed = 45.0f; health = maxHealth = 350.0f;
            damage = 28.0f; radius = 30.0f; xpReward = 140;
            bodyColor = {220, 80, 0, 255}; attackRate = 1.6f;
            break;
        case EnemyType::AcidSpitter:
            speed = 75.0f; health = maxHealth = 65.0f;
            damage = 0.0f; radius = 14.0f; xpReward = 40;
            bodyColor = {80, 180, 30, 255}; attackRate = 99.0f;
            shootRate = 2.5f; shootDamage = 12.0f; shootRange = 280.0f;
            shootSpeed = 280.0f; projectileColor = {120, 240, 0, 255};
            break;
        case EnemyType::InfernoHerald:
            speed = 55.0f; health = maxHealth = 2200.0f;
            damage = 45.0f; radius = 48.0f; xpReward = 1300;
            bodyColor = {255, 60, 0, 255}; attackRate = 99.0f;
            shootRate = 1.5f; shootDamage = 40.0f; shootRange = 380.0f;
            shootSpeed = 400.0f; projectileColor = {255, 120, 0, 255};
            break;
        case EnemyType::CrimsonBat:
            speed = 280.0f; health = maxHealth = 18.0f;
            damage = 8.0f; radius = 9.0f; xpReward = 12;
            bodyColor = {200, 20, 20, 255}; attackRate = 0.5f;
            break;
        case EnemyType::VolcanicTitan:
            speed = 30.0f; health = maxHealth = 5000.0f;
            damage = 70.0f; radius = 70.0f; xpReward = 2500;
            bodyColor = {180, 40, 0, 255}; attackRate = 2.0f;
            shootRate = 1.2f; shootDamage = 55.0f; shootRange = 500.0f;
            shootSpeed = 350.0f; projectileColor = {255, 100, 0, 255};
            break;
        // ── Humanoides Corrompidos ─────────────────────────────────────────────
        case EnemyType::CyberSamurai:
            speed = 150.0f; health = maxHealth = 110.0f;
            damage = 28.0f; radius = 16.0f; xpReward = 70;
            bodyColor = {60, 60, 120, 255}; attackRate = 0.4f;
            break;
        case EnemyType::PlagueDoctor:
            speed = 85.0f; health = maxHealth = 75.0f;
            damage = 0.0f; radius = 15.0f; xpReward = 55;
            bodyColor = {80, 100, 40, 255}; attackRate = 99.0f;
            shootRate = 3.0f; shootDamage = 10.0f; shootRange = 260.0f;
            shootSpeed = 260.0f; projectileColor = {100, 200, 0, 255};
            break;
        case EnemyType::Necromancer:
            speed = 65.0f; health = maxHealth = 95.0f;
            damage = 0.0f; radius = 17.0f; xpReward = 95;
            bodyColor = {80, 40, 120, 255}; attackRate = 99.0f;
            shootRate = 2.8f; shootDamage = 14.0f; shootRange = 300.0f;
            shootSpeed = 290.0f; projectileColor = {160, 0, 220, 255};
            break;
        case EnemyType::IronGuard:
            speed = 40.0f; health = maxHealth = 400.0f;
            damage = 32.0f; radius = 28.0f; xpReward = 100;
            bodyColor = {120, 130, 140, 255}; attackRate = 1.8f;
            break;
        case EnemyType::GhostSniper:
            speed = 60.0f; health = maxHealth = 45.0f;
            damage = 0.0f; radius = 12.0f; xpReward = 80;
            bodyColor = {180, 200, 220, 200}; attackRate = 99.0f;
            shootRate = 3.6f; shootDamage = 42.0f; shootRange = 480.0f;
            shootSpeed = 540.0f; projectileColor = {200, 220, 255, 220};
            break;
        // ── Criaturas Abissais ─────────────────────────────────────────────────
        case EnemyType::AbyssalEel:
            speed = 160.0f; health = maxHealth = 70.0f;
            damage = 24.0f; radius = 13.0f; xpReward = 55;
            bodyColor = {0, 80, 120, 255}; attackRate = 0.9f;
            break;
        case EnemyType::VoidStalker:
            speed = 120.0f; health = maxHealth = 80.0f;
            damage = 35.0f; radius = 15.0f; xpReward = 90;
            bodyColor = {20, 0, 60, 255}; attackRate = 1.2f;
            break;
        case EnemyType::DarkMatter:
            speed = 130.0f; health = maxHealth = 60.0f;
            damage = 18.0f; radius = 16.0f; xpReward = 60;
            bodyColor = {10, 0, 30, 255}; attackRate = 0.8f;
            break;
        case EnemyType::ChaosSpawn:
            speed = 110.0f; health = maxHealth = 85.0f;
            damage = 20.0f; radius = 15.0f; xpReward = 70;
            bodyColor = {120, 60, 180, 255}; attackRate = 1.0f;
            shootRate = 2.5f; shootDamage = 18.0f; shootRange = 280.0f;
            shootSpeed = 320.0f; projectileColor = {180, 100, 255, 255};
            break;
        case EnemyType::Leviathan:
            speed = 25.0f; health = maxHealth = 8000.0f;
            damage = 80.0f; radius = 80.0f; xpReward = 5000;
            bodyColor = {0, 20, 60, 255}; attackRate = 2.5f;
            shootRate = 1.0f; shootDamage = 60.0f; shootRange = 550.0f;
            shootSpeed = 420.0f; projectileColor = {0, 100, 255, 255};
            break;
    }
    // Ritmo de caminhada normalizado — inimigos andavam rapido demais em relacao
    // ao jogador (que tambem foi desacelerado). Mantem a proporcao entre tipos.
    speed *= 0.72f;
}

int Enemy::combatRole() const {
    switch (type) {
        // 1 = atirador kiter (mantém distância e atira)
        case EnemyType::CorrupterDrone:
        case EnemyType::SiegeCrawler:
        case EnemyType::AcidSpitter:
        case EnemyType::PlagueDoctor:
        case EnemyType::Necromancer:
        case EnemyType::LichKnight:
        case EnemyType::GhostSniper:
        case EnemyType::ChaosSpawn:
            return 1;
        // 2 = flanqueador rápido (aproxima em ângulo, zig-zag)
        case EnemyType::ReaperMech:
        case EnemyType::CrimsonBat:
        case EnemyType::DemonHunter:
        case EnemyType::AbyssalEel:
        case EnemyType::DarkMatter:
            return 2;
        // 3 = brutamonte com investida (charge/lunge)
        case EnemyType::VoidColossus:
        case EnemyType::MoltenGolem:
        case EnemyType::IronGuard:
        case EnemyType::BloodBerserker:
        case EnemyType::SoulReaper:
        case EnemyType::CyberSamurai:
        case EnemyType::VoidStalker:
        case EnemyType::NeuralParasite:
            return 3;
        // 4 = boss de facção (aproxima + barragem com fases)
        case EnemyType::FrostWyrm:
        case EnemyType::InfernoHerald:
        case EnemyType::VolcanicTitan:
        case EnemyType::Leviathan:
            return 4;
        default:
            return 0; // perseguidor melee genérico (Scout, Tank, etc.)
    }
}

// ── Sistema épico de bosses: 3 fases + padrões de ataque telegrafados ──────────
void Enemy::checkBossPhases() {
    int tgt = (health < maxHealth * 0.33f) ? 3 :
              (health < maxHealth * 0.66f) ? 2 : 1;
    if (tgt > bossPhase) {
        bossPhase       = tgt;
        phaseFlash      = 1.0f;          // flash dramático
        bossInvuln      = true;
        bossInvulnTimer = 0.8f;          // breve invulnerabilidade na virada
        evolveFlash     = 0.8f;
        // Escalada de poder por fase
        speed       *= 1.16f;
        damage      *= 1.22f;
        shootDamage *= 1.18f;
        shootRate    = std::max(0.25f, shootRate * 0.72f);
        bossAtkTimer = 0.6f;             // parte pro ataque logo após a virada
        // recuo/empurrão visual já tratado pelo flash; mantém genericPhase2 sync
        if (bossPhase >= 2) genericPhase2 = true;
    }
}

void Enemy::updateBossPatterns(float dt, Vector2 norm) {
    if (bossInvulnTimer > 0.0f) { bossInvulnTimer -= dt; if (bossInvulnTimer <= 0.0f) bossInvuln = false; }
    if (phaseFlash > 0.0f) phaseFlash -= dt;

    float baseAngle = std::atan2(norm.y, norm.x);

    // Executando um padrão (sequência de tiros)?
    if (burstShots > 0) {
        burstTimer -= dt;
        if (burstTimer <= 0.0f) {
            burstTimer     = burstGap;
            wantsToShoot   = true;
            shootDirection = { std::cos(burstAngle), std::sin(burstAngle) };
            burstAngle    += burstStep;
            burstShots--;
            if (burstShots == 0) {
                bossPattern = 0; // Volta para cooldown
            }
        }
        return; // ocupado disparando o padrão
    }

    // Durante a invulnerabilidade de virada de fase, não ataca (drama)
    if (bossInvuln) return;

    bossAtkTimer -= dt;
    // Telegrafa nos últimos 0.5s antes do grande ataque (pisca/recolhe)
    if (bossAtkTimer <= 0.5f && bossAtkTimer > 0.0f) {
        telegraphTimer = telegraphMax;
        if (bossPattern == 0) {
            // Seleciona o padrão que vai usar logo no início do telegraph
            int phase = bossPhase;
            int maxPat = (phase >= 3) ? 4 : (phase == 2) ? 3 : 2;
            bossPattern = GetRandomValue(1, maxPat);
        }
    }
    if (bossAtkTimer > 0.0f) return;

    // Se chegou aqui (bossAtkTimer <= 0.0f) e bossPattern ainda é 0 (cooldown acabou sem passar por telegraph), seleciona
    if (bossPattern == 0) {
        int phase = bossPhase;
        int maxPat = (phase >= 3) ? 4 : (phase == 2) ? 3 : 2;
        bossPattern = GetRandomValue(1, maxPat);
    }

    // Inicializa os parâmetros de disparos para o padrão selecionado
    int phase = bossPhase;
    int n; float spread;
    switch (bossPattern) {
        case 1: // LEQUE à frente
            n = 5 + phase * 2; spread = 0.85f;
            burstAngle = baseAngle - spread * 0.5f;
            burstStep  = spread / (float)(n - 1);
            burstShots = n; burstGap = 0.045f;
            break;
        case 2: // VARREDURA giratória
            n = 9 + phase * 3;
            burstAngle = baseAngle - 0.8f;
            burstStep  = 1.6f / (float)n;
            burstShots = n; burstGap = 0.05f;
            break;
        case 3: // METRALHAR reto rápido
            n = 6 + phase * 3;
            burstAngle = baseAngle; burstStep = 0.0f;
            burstShots = n; burstGap = 0.06f;
            break;
        default: // ANEL 360° (fases altas)
            n = 14 + phase * 4;
            burstAngle = 0.0f; burstStep = 6.2831853f / (float)n;
            burstShots = n; burstGap = 0.03f;
            break;
    }
    bossAtkTimer = (phase >= 3) ? 1.7f : (phase == 2) ? 2.3f : 3.0f;
}

void Enemy::renderBossAura() const {
    if (!isBoss()) return;
    float t = walkAnimTimer;
    // Aura pulsante por fase
    Color aur = (bossPhase >= 3) ? Color{255,40,40,255}
              : (bossPhase == 2) ? Color{255,140,0,255}
                                 : Color{180,60,255,255};
    float pr = radius * (1.7f + 0.18f * std::sin(t * 3.0f));
    DrawCircleLines((int)position.x, (int)position.y, pr, ColorAlpha(aur, 0.30f));
    DrawCircleLines((int)position.x, (int)position.y, pr * 0.82f, ColorAlpha(aur, 0.18f));
    // Flash branco na virada de fase
    if (phaseFlash > 0.0f)
        DrawCircleV(position, radius * 2.2f, ColorAlpha(WHITE, phaseFlash * 0.35f));
    // Marcadores de fase (coroa de pips acima)
    for (int i = 0; i < bossPhase; ++i) {
        float fx = position.x - (bossPhase - 1) * 5.0f + i * 10.0f;
        DrawCircleV({fx, position.y - radius - 16.0f}, 3.5f, aur);
    }
    // Partículas de carregamento durante o telegraph (anéis convergindo)
    if (telegraphTimer > 0.0f) {
        float k = 1.0f - telegraphTimer / telegraphMax;
        for (int i = 0; i < 6; ++i) {
            float a = t * 4.0f + i * 1.047f;
            float rr = radius * (2.2f - k * 1.4f);
            DrawCircleV({position.x + std::cos(a) * rr, position.y + std::sin(a) * rr},
                        2.5f, ColorAlpha(Color{255,230,120,255}, 0.8f));
        }
    }
}

void Enemy::update(float dt, Vector2 target) {
    wantsToShoot = false;
    shootCooldown -= dt;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
    if (telegraphTimer > 0.0f) telegraphTimer -= dt;
    if (lungeTimer    > 0.0f) lungeTimer    -= dt;
    if (lungeCooldown > 0.0f) lungeCooldown -= dt;
    if (regenRate > 0.0f) {
        health = std::min(maxHealth, health + regenRate * dt);
    }
    if (shieldMax > 0.0f && shieldHp < shieldMax)   // escudo Shielded recarrega devagar
        shieldHp = std::min(shieldMax, shieldHp + shieldMax * 0.10f * dt);
    if (isElite) elitePulse += dt * 4.0f;

    // ── Auto-evolução por tempo de vida ──────────────────────────────────────
    aliveTimer += dt;
    if (evolveFlash > 0.0f) evolveFlash -= dt;
    justEvolved = false;
    if (evolTier == 0 && aliveTimer >= 30.0f) {
        evolTier   = 1;
        maxHealth *= 1.3f; health = std::min(health * 1.3f, maxHealth);
        damage    *= 1.2f; speed *= 1.1f;
        auraColor  = {0, 210, 100, 180};
        evolveFlash = 0.8f; justEvolved = true;
    } else if (evolTier == 1 && aliveTimer >= 75.0f) {
        evolTier   = 2;
        maxHealth *= 1.5f; health = std::min(health * 1.5f, maxHealth);
        damage    *= 1.3f; speed *= 1.15f;
        auraColor  = {0, 100, 255, 210};
        evolveFlash = 0.8f; justEvolved = true;
    } else if (evolTier == 2 && aliveTimer >= 150.0f) {
        evolTier   = 3;
        maxHealth *= 2.0f; health = std::min(health * 2.0f, maxHealth);
        damage    *= 1.5f; speed *= 1.2f; shootRate *= 1.4f;
        auraColor  = {255, 140, 0, 230};
        evolveFlash = 0.8f; justEvolved = true;
    }
    // ─────────────────────────────────────────────────────────────────────────

    // Alert timeout — restore speed
    if (alerted && alertTimer > 0.0f) {
        alertTimer -= dt;
        if (alertTimer <= 0.0f) {
            alerted = false;
            speed  /= 1.35f;
        }
    }

    // Knockback decay
    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        position.x += knockback.x * dt;
        position.y += knockback.y * dt;
        float kd = std::exp(-8.0f * dt);
        knockback.x *= kd;
        knockback.y *= kd;
        if (std::fabs(knockback.x) < 0.5f) knockback.x = 0.0f;
        if (std::fabs(knockback.y) < 0.5f) knockback.y = 0.0f;
    }

    switch (type) {
        case EnemyType::HunterDrone:
            updateHunterDrone(dt, target);
            break;
        case EnemyType::KronosSentry:
            updateKronosSentry(dt, target);
            break;
        case EnemyType::MorphX:
            updateMorphX(dt, target);
            break;
        case EnemyType::Kamikaze:
            updateKamikaze(dt, target);
            break;
        case EnemyType::Sniper:
            updateSniper(dt, target);
            break;
        case EnemyType::Zergling:
            updateZergling(dt, target);
            break;
        case EnemyType::Hydra:
            updateHydra(dt, target);
            break;
        case EnemyType::Broodmother:
            updateBroodmother(dt, target);
            break;
        case EnemyType::AlienBoss:
            updateAlienBoss(dt, target);
            break;
        case EnemyType::OrcCibernetico:
            updateOrcCibernetico(dt, target);
            break;
        case EnemyType::PaladinCorrompido:
            updatePaladinCorrompido(dt, target);
            break;
        case EnemyType::UndeadEnforcer:
            updateUndeadEnforcer(dt, target);
            break;
        case EnemyType::Ghost:
        case EnemyType::GhostElite:
            updateGhost(dt, target);
            break;
        case EnemyType::Zombie:
        case EnemyType::ZombieHorde:
            updateZombie(dt, target);
            break;
        case EnemyType::ZombieRager:
            updateZombieRager(dt, target);
            break;
        case EnemyType::ZombieLord:
            updateZombieLord(dt, target);
            break;
        case EnemyType::PoltergeistBoss:
            updatePoltergeistBoss(dt, target);
            break;
        case EnemyType::ShadowWraith:
            updateShadowWraith(dt, target);
            break;
        case EnemyType::BansheeHowler:
            updateBansheeHowler(dt, target);
            break;
        case EnemyType::OmegaBoss: {
            walkAnimTimer += dt * 2.0f;
            checkBossPhases();                       // 3 fases épicas
            Vector2 dir = {target.x-position.x, target.y-position.y};
            float dist = std::sqrt(dir.x*dir.x+dir.y*dir.y);
            Vector2 ndir = (dist > 0.001f) ? Vector2{dir.x/dist, dir.y/dist} : Vector2{1,0};
            facing = (ndir.x >= 0) ? 1 : -1;
            // Mantém média distância (mais imponente que correr reto)
            if (!bossInvuln && burstShots == 0) {
                float ideal = shootRange * 0.65f;
                if (dist > ideal * 1.1f) { position.x += ndir.x*speed*dt; position.y += ndir.y*speed*dt; }
                else if (dist < ideal * 0.55f) { position.x -= ndir.x*speed*0.6f*dt; position.y -= ndir.y*speed*0.6f*dt; }
            }
            updateBossPatterns(dt, ndir);            // leque/varredura/metralha/anel
            break;
        }
        default: {
            int role = combatRole();

            // ── Bosses: sistema épico de 3 fases (Boss + bosses de facção) ───
            bool bossLike = (type == EnemyType::Boss) || (role == 4);
            if (bossLike) checkBossPhases();

            Vector2 dir = {target.x - position.x, target.y - position.y};
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            float invLen   = (len > 0.001f) ? 1.0f / len : 0.0f;
            Vector2 norm   = {dir.x * invLen, dir.y * invLen};
            Vector2 perp   = {-norm.y, norm.x};
            if (norm.x > 0.1f) facing = 1;
            if (norm.x < -0.1f) facing = -1;

            // ── Patrulha (Scout/Tank longe e não alertados) ──────────────────
            if ((type == EnemyType::Scout || type == EnemyType::Tank)
                && !alerted && len > 280.0f) {
                if (!patrolInit) { patrolInit = true; patrolTarget = position; patrolTimer = 0.0f; }
                patrolTimer -= dt;
                if (patrolTimer <= 0.0f) {
                    patrolTarget = {position.x + (float)GetRandomValue(-100,100),
                                    position.y + (float)GetRandomValue(-100,100)};
                    patrolTimer  = (float)GetRandomValue(300, 500) / 100.0f;
                }
                Vector2 pd = {patrolTarget.x - position.x, patrolTarget.y - position.y};
                float   pl = std::sqrt(pd.x*pd.x + pd.y*pd.y);
                if (pl > 8.0f) {
                    position.x += (pd.x/pl) * speed * 0.4f * dt;
                    position.y += (pd.y/pl) * speed * 0.4f * dt;
                    walkAnimTimer += dt * 6.0f;
                }
                attackCooldown -= dt;
                break;
            }

            bool isRanged = (type == EnemyType::Shooter || attackRate >= 90.0f);

            // ── Movimento por papel de combate ───────────────────────────────
            if (role == 1 || (isRanged && role == 0 && type != EnemyType::Boss)) {
                // ATIRADOR KITER — mantém banda de distância e faz strafe
                float ideal = shootRange * 0.72f;
                if (len < ideal * 0.75f) {            // perto demais → recua
                    position.x -= norm.x * speed * 1.15f * dt;
                    position.y -= norm.y * speed * 1.15f * dt;
                    facing = (norm.x > 0.0f) ? -1 : 1;
                } else if (len > ideal * 1.15f) {     // longe demais → aproxima
                    position.x += norm.x * speed * dt;
                    position.y += norm.y * speed * dt;
                } else {                              // na banda → strafe
                    strafeTimer -= dt;
                    if (strafeTimer <= 0.0f) {
                        strafeDir   = (GetRandomValue(0,1)==0) ? 1.0f : -1.0f;
                        strafeTimer = (float)GetRandomValue(80, 220) / 100.0f;
                    }
                    position.x += perp.x * speed * 0.6f * strafeDir * dt;
                    position.y += perp.y * speed * 0.6f * strafeDir * dt;
                }
                walkAnimTimer += dt * 9.0f;
            }
            else if (role == 2) {
                // FLANQUEADOR RÁPIDO — aproxima em ângulo (zig-zag), não reto
                if (!flankInit) { flankInit = true; flankSign = (GetRandomValue(0,1)?1.0f:-1.0f); }
                strafeTimer -= dt;
                if (strafeTimer <= 0.0f) { flankSign = -flankSign; strafeTimer = (float)GetRandomValue(50,140)/100.0f; }
                float angBlend = (len > 90.0f) ? 0.55f : 0.0f; // só flanqueia de longe
                Vector2 mv = { norm.x + perp.x * flankSign * angBlend,
                               norm.y + perp.y * flankSign * angBlend };
                float ml = std::sqrt(mv.x*mv.x + mv.y*mv.y);
                if (ml > 0.001f) { mv.x/=ml; mv.y/=ml; }
                if (len > radius + 8.0f) {
                    position.x += mv.x * speed * dt;
                    position.y += mv.y * speed * dt;
                }
                walkAnimTimer += dt * 14.0f;
            }
            else if (role == 3) {
                // BRUTAMONTE — aproxima e dá investidas (lunge) com telegraph
                if (lungeCooldown <= 0.0f && lungeTimer <= 0.0f && len > 120.0f && len < 360.0f) {
                    telegraphTimer = 0.4f;   // aviso visual
                    lungeTimer     = 0.72f;  // janela total (0.4 windup + ~0.32 dash)
                    lungeCooldown  = 4.0f;
                }
                if (lungeTimer > 0.0f) {
                    if (telegraphTimer > 0.0f) {
                        // recua levemente preparando a investida (tensão)
                        position.x -= norm.x * speed * 0.25f * dt;
                        position.y -= norm.y * speed * 0.25f * dt;
                    } else {
                        // DASH rápido na direção do player
                        position.x += norm.x * speed * 3.0f * dt;
                        position.y += norm.y * speed * 3.0f * dt;
                    }
                } else if (len > radius + 10.0f) {
                    position.x += norm.x * speed * dt;
                    position.y += norm.y * speed * dt;
                }
                walkAnimTimer += dt * 11.0f;
            }
            else if (role == 4) {
                // BOSS DE FACÇÃO — mantém média distância e bombardeia
                float ideal = shootRange * 0.6f;
                if (len > ideal * 1.1f) {
                    position.x += norm.x * speed * dt;
                    position.y += norm.y * speed * dt;
                } else if (len < ideal * 0.6f) {
                    // estritamente recua um pouco p/ reposicionar
                    position.x -= norm.x * speed * 0.5f * dt;
                    position.y -= norm.y * speed * 0.5f * dt;
                } else {
                    // strafe lento e ameaçador
                    strafeTimer -= dt;
                    if (strafeTimer <= 0.0f) { strafeDir = -strafeDir; strafeTimer = 1.6f; }
                    position.x += perp.x * speed * 0.35f * strafeDir * dt;
                    position.y += perp.y * speed * 0.35f * strafeDir * dt;
                }
                walkAnimTimer += dt * 6.0f;
            }
            else {
                // PERSEGUIDOR MELEE genérico (Scout/Tank/Boss melee/default)
                float stopDist = radius + 10.0f;
                bool retreating = (health < maxHealth * 0.25f) && (len < 220.0f)
                                  && (type != EnemyType::Boss);
                if (retreating) {
                    position.x -= norm.x * speed * 1.3f * dt;
                    position.y -= norm.y * speed * 1.3f * dt;
                    facing = (norm.x > 0.0f) ? -1 : 1;
                } else if (len > stopDist) {
                    position.x += norm.x * speed * dt;
                    position.y += norm.y * speed * dt;
                }
                walkAnimTimer += dt * 10.0f;
            }

            // ── Ataque ───────────────────────────────────────────────────────
            if (bossLike) {
                // BOSS: padrões épicos (leque/varredura/metralha/anel) por fase
                updateBossPatterns(dt, norm);
            } else if (isRanged) {
                // Atirador comum com telegraph
                if (len <= shootRange && shootCooldown > 0.0f && shootCooldown <= telegraphMax)
                    telegraphTimer = telegraphMax;
                if (len <= shootRange && shootCooldown <= 0.0f) {
                    wantsToShoot   = true;
                    shootDirection = norm;
                    shootCooldown  = shootRate;
                }
            }
            attackCooldown -= dt;
            break;
        }
    }
}

void Enemy::updateMorphX(float dt, Vector2 target) {
    Vector2 dir = {target.x - position.x, target.y - position.y};
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    float stopDist = radius + 8.0f;

    // Jitter movement - looks liquid
    float jitter = std::sin(walkAnimTimer * 8.0f) * 12.0f;
    if (len > stopDist) {
        dir.x /= len; dir.y /= len;
        position.x += (dir.x * speed + dir.y * jitter) * dt;
        position.y += (dir.y * speed - dir.x * jitter) * dt;
        if (dir.x > 0.1f) facing = 1;
        if (dir.x < -0.1f) facing = -1;
        walkAnimTimer += dt * 15.0f;
    }
    attackCooldown -= dt;
}

void Enemy::updateKamikaze(float dt, Vector2 target) {
    wantsToExplode = false;
    Vector2 dir = {target.x - position.x, target.y - position.y};
    float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (len > 0.001f) {
        dir.x /= len; dir.y /= len;
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
        if (dir.x > 0.1f) facing = 1;
        if (dir.x < -0.1f) facing = -1;
    }
    walkAnimTimer += dt * 20.0f; // fast jitter
    // Explode when within contact range
    if (len <= radius + 20.0f) {
        wantsToExplode = true;
    }
}

void Enemy::updateSniper(float dt, Vector2 target) {
    Vector2 dir = {target.x - position.x, target.y - position.y};
    float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    float invLen = (len > 0.001f) ? 1.0f/len : 0.0f;
    Vector2 norm = {dir.x*invLen, dir.y*invLen};

    // Retreat if player too close
    float safeRange = 350.0f;
    if (len < safeRange) {
        position.x -= norm.x * speed * dt;
        position.y -= norm.y * speed * dt;
        walkAnimTimer += dt * 8.0f;
        facing = (norm.x > 0.0f) ? -1 : 1;
    } else {
        walkAnimTimer = 0.0f;
        // Slight lateral drift
        float drift = std::sin(walkAnimTimer * 0.5f) * 30.0f * dt;
        position.x += (-norm.y) * drift;
        position.y += ( norm.x) * drift;
        facing = (norm.x > 0.0f) ? 1 : -1;
    }

    if (len <= shootRange && shootCooldown <= 0.0f) {
        wantsToShoot   = true;
        shootDirection = norm;
        shootCooldown  = shootRate;
    }
}

void Enemy::updateHunterDrone(float dt, Vector2 target) {
    float dist = Vector2Distance(position, target);

    orbitAngle += 1.5f * dt;

    float orbitX = target.x + std::cos(orbitAngle) * orbitRadius;
    float orbitY = target.y + std::sin(orbitAngle) * orbitRadius;

    Vector2 toOrbit = {orbitX - position.x, orbitY - position.y};
    float toOrbitLen = std::sqrt(toOrbit.x * toOrbit.x + toOrbit.y * toOrbit.y);
    if (toOrbitLen > 5.0f) {
        toOrbit.x /= toOrbitLen;
        toOrbit.y /= toOrbitLen;
        position.x += toOrbit.x * speed * dt;
        position.y += toOrbit.y * speed * dt;
    }

    walkAnimTimer += dt * 5.0f;

    if (dist <= shootRange && shootCooldown <= 0.0f) {
        Vector2 dir = {target.x - position.x, target.y - position.y};
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.0f) {
            wantsToShoot = true;
            shootDirection = {dir.x / len, dir.y / len};
            shootCooldown = shootRate;
        }
    }
}

void Enemy::updateKronosSentry(float dt, Vector2 target) {
    Vector2 dir = {target.x - position.x, target.y - position.y};
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);

    barrelAngle += dt * 0.5f;

    if (len <= shootRange && shootCooldown <= 0.0f) {
        if (len > 0.0f) {
            wantsToShoot = true;
            shootDirection = {dir.x / len, dir.y / len};
            shootCooldown = shootRate;
        }
    }
}

void Enemy::makeElite(int mod) {
    isElite  = true;
    eliteMod = mod;
    elitePulse = 0.0f;
    xpReward  = (int)(xpReward * 3.5f);

    switch (mod) {
        case 0: // Berserker — fast + heavy hits
            speed  *= 1.6f;
            damage *= 2.0f;
            health *= 1.5f; maxHealth = health;
            break;
        case 1: // Armored — triple health, slow, mitigates flat damage per hit
            health *= 3.0f; maxHealth = health;
            speed  *= 0.75f;
            armor   = 8.0f;
            break;
        case 2: // Volatile — normal stats, explodes on death
            health *= 1.8f; maxHealth = health;
            damage *= 1.4f;
            break;
        case 3: // Shielded — energy shield that absorbs a hit fraction and recharges
            health *= 1.9f; maxHealth = health;
            damage *= 1.15f;
            shieldMax = maxHealth * 0.45f;
            shieldHp  = shieldMax;
            break;
        case 4: // KronosRapid — fast, self-repairing
            speed  *= 1.35f;
            damage *= 1.3f;
            health *= 1.5f; maxHealth = health;
            regenRate = maxHealth * 0.02f;
            break;
    }
}

void Enemy::alert() {
    if (!alerted) {
        alerted    = true;
        alertTimer = 6.0f;
        speed     *= 1.35f;
    }
}

void Enemy::applyKnockback(Vector2 dir, float force) {
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.001f) {
        knockback.x += dir.x / len * force;
        knockback.y += dir.y / len * force;
    }
}

void Enemy::takeDamage(float amount) {
    // Boss invulnerável durante a breve transição de fase (drama)
    if (bossInvuln) { hitFlashTimer = 0.08f; return; }
    // Escudo Shielded absorve 75% de cada golpe ate esvaziar; recarrega no update.
    if (shieldHp > 0.0f) {
        float absorbed = std::min(shieldHp, amount * 0.75f);
        shieldHp -= absorbed;
        amount   -= absorbed;
    }
    // Armadura reduz dano plano por hit (Arpg: tiro fraco quase nao arranha).
    if (armor > 0.0f)
        amount -= std::min(armor, amount);
    health -= amount;
    if (health < 0.0f) health = 0.0f;
    hitFlashTimer = 0.12f;
}

bool Enemy::isDead() const { return health <= 0.0f; }

bool Enemy::shouldDropLoot() const { return isDead() && !droppedLoot; }

void Enemy::markLootDropped() { droppedLoot = true; }

bool Enemy::canAttackPlayer(Vector2 playerPos) const {
    if (type == EnemyType::HunterDrone || type == EnemyType::KronosSentry) return false;
    if (type == EnemyType::Kamikaze) return false; // uses wantsToExplode
    if (type == EnemyType::Sniper)   return false; // ranged via wantsToShoot
    return Vector2Distance(position, playerPos) <= radius + 22.0f;
}

float Enemy::attackIfReady(float dt, Vector2 playerPos) {
    (void)dt;
    if (canAttackPlayer(playerPos) && attackCooldown <= 0.0f) {
        attackCooldown = attackRate;
        return damage;
    }
    return 0.0f;
}

void Enemy::updateZergling(float dt, Vector2 target) {
    walkAnimTimer += dt * 12.0f;
    Vector2 dir = {target.x - position.x, target.y - position.y};
    float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (len > 0.5f) {
        dir.x /= len; dir.y /= len;
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
        facing = (dir.x >= 0) ? 1 : -1;
    }
    if (len <= radius + 15.0f) wantsToExplode = true;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
}

void Enemy::updateHydra(float dt, Vector2 target) {
    walkAnimTimer += dt * 4.0f;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
    Vector2 dir = {target.x - position.x, target.y - position.y};
    float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;
    if (dist < 110.0f) {
        position.x -= dir.x * speed * 1.4f * dt;
        position.y -= dir.y * speed * 1.4f * dt;
    } else if (dist > 300.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    } else {
        strafeTimer -= dt;
        if (strafeTimer <= 0.0f) { strafeDir = -strafeDir; strafeTimer = (float)GetRandomValue(100,250)/100.0f; }
        position.x += (-dir.y) * strafeDir * speed * 0.7f * dt;
        position.y += ( dir.x) * strafeDir * speed * 0.7f * dt;
    }
    shootCooldown -= dt;
    if (shootCooldown <= 0.0f && dist <= shootRange) {
        shootCooldown  = shootRate;
        wantsToShoot   = true;
        shootDirection = dir;
    } else {
        wantsToShoot = false;
    }
}

void Enemy::updateBroodmother(float dt, Vector2 target) {
    walkAnimTimer += dt * 3.0f;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
    if (bossPhase == 1 && health < maxHealth * 0.5f) {
        bossPhase = 2;
        speed    *= 1.3f;
    }
    Vector2 dir = {target.x - position.x, target.y - position.y};
    float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;
    if (dist < 500.0f && dist > radius + 20.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }
}

// ─── Alien render functions ───────────────────────────────────────────────────

void Enemy::renderAlienBoss() const {
    float px = position.x, py = position.y;
    float pulse = std::sin(walkAnimTimer * 3.0f);
    float f = (float)facing;
    Color acidGreen  = {80,255,0,255};
    Color darkGreen  = {15,55,0,255};
    Color purpleGlow = {200,0,255,255};
    Color boneWhite  = {200,210,180,255};

    if (!g_voxelCapture)   // sombra 2D fora da voxelizacao (viraria pedestal sob os pes)
        DrawEllipse((int)px, (int)(py + radius * 0.75f), radius * 1.3f, radius * 0.36f, ColorAlpha(BLACK,0.45f));

    if (bossPhase == 2) {
        float rage = 0.5f + 0.5f * pulse;
        DrawCircleV(position, radius + 55.0f, ColorAlpha(purpleGlow, 0.06f + rage*0.06f));
        DrawCircleV(position, radius + 35.0f, ColorAlpha(acidGreen,  0.10f + rage*0.08f));
        DrawCircleLines((int)px,(int)py, radius+50.0f+rage*6.0f, ColorAlpha(purpleGlow, 0.4f+rage*0.4f));
        const char* p2tag = "!FASE 2!";
        int p2w = MeasureText(p2tag,11);
        DrawText(p2tag,(int)(px-p2w/2),(int)(py-radius-55),11, ColorAlpha(purpleGlow,0.7f+rage*0.3f));
    } else {
        DrawCircleV(position, radius + 20.0f, ColorAlpha(acidGreen, 0.15f));
    }

    for (int i = 0; i < 8; ++i) {
        float ang = i * 0.7854f + walkAnimTimer * (i%2==0?0.4f:-0.4f);
        float legR = radius + 35.0f;
        float kx = px + std::cos(ang) * radius * 0.85f;
        float ky = py + std::sin(ang) * radius * 0.55f;
        float ex2 = px + std::cos(ang) * legR;
        float ey2 = py + std::sin(ang) * legR * 0.75f;
        DrawLineEx({kx,ky},{ex2,ey2},6.0f,darkGreen);
        DrawLineEx({kx,ky},{ex2,ey2},2.5f,{50,160,0,255});
        DrawCircleV({ex2,ey2},4.0f,acidGreen);
    }

    DrawEllipse((int)px,(int)(py+4),radius*1.2f,radius*1.0f,darkGreen);
    DrawEllipse((int)px,(int)(py+3),radius*0.95f,radius*0.78f,bodyColor);

    for (int i = 0; i < 5; ++i) {
        float ry = py - 8.0f + i * 8.0f;
        DrawLineEx({px-22,ry},{px-4,ry},2.0f,ColorAlpha(boneWhite,0.35f));
        DrawLineEx({px+ 4,ry},{px+22,ry},2.0f,ColorAlpha(boneWhite,0.35f));
        DrawCircleV({px-22,ry},3.0f,ColorAlpha(boneWhite,0.4f));
        DrawCircleV({px+22,ry},3.0f,ColorAlpha(boneWhite,0.4f));
    }

    if (bossPhase == 2) {
        float coreSize = 8.0f + pulse*4.0f;
        DrawGlowCircle({px,py}, coreSize, purpleGlow, coreSize - 1.0f);
        DrawCircleV({px,py},4.0f,{255,255,255,255});
    } else {
        DrawGlowCircle({px,py},5.0f,acidGreen,4.0f);
        DrawCircleV({px,py},2.5f,{150,255,50,255});
    }

    DrawLineEx({px-22,py-4},{px-42,py-20},7.0f,darkGreen);
    DrawLineEx({px-42,py-20},{px-52,py-6},5.0f,acidGreen);
    DrawLineEx({px-42,py-20},{px-48,py-32},5.0f,acidGreen);
    DrawLineEx({px+22,py-4},{px+42,py-20},7.0f,darkGreen);
    DrawLineEx({px+42,py-20},{px+52,py-6},5.0f,acidGreen);
    DrawLineEx({px+42,py-20},{px+48,py-32},5.0f,acidGreen);

    float tentSwing = pulse * 12.0f;
    DrawLineEx({px-18,py+8},{px-35,py+20+tentSwing},4.0f,{40,130,0,255});
    DrawLineEx({px-35,py+20+tentSwing},{px-45,py+35+tentSwing},3.0f,{60,180,0,255});
    DrawLineEx({px+18,py+8},{px+35,py+20-tentSwing},4.0f,{40,130,0,255});
    DrawLineEx({px+35,py+20-tentSwing},{px+45,py+35-tentSwing},3.0f,{60,180,0,255});

    if (bossPhase == 2) {
        float t2 = pulse * 8.0f;
        DrawLineEx({px-10,py-16},{px-28,py-36+t2},3.0f,ColorAlpha(purpleGlow,0.7f));
        DrawLineEx({px+10,py-16},{px+28,py-36-t2},3.0f,ColorAlpha(purpleGlow,0.7f));
    }

    float headY = py - radius * 0.75f;
    DrawEllipse((int)px,(int)headY,24,18,darkGreen);
    DrawEllipse((int)px,(int)headY,18,13,{25,90,5,255});
    DrawTriangle({px,headY-28},{px-14,headY-10},{px+14,headY-10},darkGreen);
    DrawTriangle({px,headY-22},{px-9,headY-8},{px+9,headY-8},{50,140,10,255});
    for (int i = -2; i <= 2; ++i)
        DrawLineEx({px+i*5.0f,headY-16},{px+i*5.0f,headY-6},1.5f,ColorAlpha(boneWhite,0.3f));
    Color eyeCol = (bossPhase==2) ? purpleGlow : Color{200,255,0,255};
    DrawGlowCircle({px-8,headY-2},6.0f,eyeCol,5.0f);
    DrawGlowCircle({px+8,headY-2},6.0f,eyeCol,5.0f);
    DrawCircleV({px-8,headY-2},3.0f,WHITE);
    DrawCircleV({px+8,headY-2},3.0f,WHITE);
    DrawCircleV({px-8,headY-2},1.5f,BLACK);
    DrawCircleV({px+8,headY-2},1.5f,BLACK);
    DrawLineEx({px-14,headY+10},{px-24,headY+22},5.0f,darkGreen);
    DrawLineEx({px+14,headY+10},{px+24,headY+22},5.0f,darkGreen);
    DrawLineEx({px-24,headY+22},{px-30,headY+16},3.0f,acidGreen);
    DrawLineEx({px+24,headY+22},{px+30,headY+16},3.0f,acidGreen);

    float barW = 80.0f, hpPct = health/maxHealth;
    Color hpCol = hpPct>0.5f ? acidGreen : hpPct>0.25f ? Color{255,200,0,255} : Color{255,50,0,255};
    DrawRectangleRec({px-barW/2,py-radius-18,barW,6},BLACK);
    DrawRectangleRec({px-barW/2,py-radius-18,barW*hpPct,6},hpCol);
    DrawText("ALIEN BOSS",(int)(px-30),(int)(py-radius-30),10,ColorAlpha(acidGreen,0.9f));

    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius+8, ColorAlpha({150,255,50,255},0.5f));
}

void Enemy::updateAlienBoss(float dt, Vector2 target) {
    walkAnimTimer += dt * 3.0f;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
    if (bossPhase == 1 && health < maxHealth * 0.45f) {
        bossPhase = 2;
        speed    *= 1.4f;
        damage   *= 1.3f;
        shootRate = shootRate * 0.7f;
    }
    Vector2 dir = {target.x - position.x, target.y - position.y};
    float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;
    if (dist > radius + 20.0f)
        { position.x += dir.x*speed*dt; position.y += dir.y*speed*dt; }
    shootCooldown -= dt;
    if (shootCooldown <= 0.0f && dist <= shootRange) {
        shootCooldown  = shootRate;
        wantsToShoot   = true;
        shootDirection = dir;
    } else wantsToShoot = false;
}

// ─── WarCraft Cyberpunk update functions ──────────────────────────────────────

void Enemy::updateOrcCibernetico(float dt, Vector2 target) {
    walkAnimTimer += dt * 5.0f;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;

    Vector2 dir = {target.x - position.x, target.y - position.y};
    float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;

    // Roar: activate speed boost when player enters 200 px range
    if (!roarActive && dist < 200.0f) {
        roarActive = true;
        roarTimer  = 3.0f;   // boost lasts 3 seconds
        speed     *= 1.5f;
    }
    if (roarActive) {
        roarTimer -= dt;
        if (roarTimer <= 0.0f) {
            roarActive = false;
            speed     /= 1.5f;
        }
    }

    // Charge directly at player
    if (dist > radius + 10.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }

    attackCooldown -= dt;

}

void Enemy::updatePaladinCorrompido(float dt, Vector2 target) {
    walkAnimTimer += dt * 3.5f;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;

    Vector2 dir = {target.x - position.x, target.y - position.y};
    float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;

    // Maintain medium distance (120–260 px)
    float idealMin = 120.0f, idealMax = 260.0f;
    if (dist < idealMin) {
        // Back away
        position.x -= dir.x * speed * dt;
        position.y -= dir.y * speed * dt;
    } else if (dist > idealMax) {
        // Close in
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    } else {
        // Strafe
        strafeTimer -= dt;
        if (strafeTimer <= 0.0f) {
            strafeDir   = -strafeDir;
            strafeTimer = (float)GetRandomValue(80, 200) / 100.0f;
        }
        position.x += (-dir.y) * strafeDir * speed * 0.6f * dt;
        position.y += ( dir.x) * strafeDir * speed * 0.6f * dt;
    }

    // Projectile attack
    shootCooldown -= dt;
    if (shootCooldown <= 0.0f && dist <= shootRange) {
        shootCooldown  = shootRate;
        wantsToShoot   = true;
        shootDirection = dir;
    } else {
        wantsToShoot = false;
    }

}

void Enemy::updateUndeadEnforcer(float dt, Vector2 target) {
    walkAnimTimer += dt * 10.0f;
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;

    // Resurrection check: triggers when health hits 0 for the first time
    if (!hasRevived && health <= 0.0f) {
        hasRevived = true;
        health     = maxHealth * 0.30f;
        hitFlashTimer = 0.4f; // flash to signal revive
    }

    Vector2 dir = {target.x - position.x, target.y - position.y};
    float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;

    // Rush straight at player like a Scout
    if (dist > radius + 8.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }

    attackCooldown -= dt;

}

// ─── WarCraft Cyberpunk render functions ──────────────────────────────────────

void Enemy::updateGhost(float dt, Vector2 target) {
    walkAnimTimer += dt * 3.0f;
    // Invisibility cycle: visible 3s, invisible 2s
    invisTimer -= dt;
    if (invisTimer <= 0.0f) {
        isInvisible = !isInvisible;
        invisTimer  = isInvisible ? 2.0f : 3.0f;
    }

    Vector2 dir  = {target.x - position.x, target.y - position.y};
    float   dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;

    // Teleport behind player when very close
    if (dist < 40.0f) {
        position.x = target.x - dir.x * 150.0f + (float)GetRandomValue(-30,30);
        position.y = target.y - dir.y * 150.0f + (float)GetRandomValue(-30,30);
    } else if (dist > radius + 12.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }

    // GhostElite ranged attack
    if (type == EnemyType::GhostElite) {
        shootCooldown -= dt;
        if (shootCooldown <= 0.0f && dist <= shootRange) {
            shootCooldown  = shootRate;
            wantsToShoot   = true;
            shootDirection = dir;
        }
    }
}

void Enemy::updateZombie(float dt, Vector2 target) {
    walkAnimTimer += dt * 2.5f;
    Vector2 dir  = {target.x - position.x, target.y - position.y};
    float   dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;
    if (dist > radius + 8.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }
    infectTimer -= dt;
    if (infectTimer <= 0.0f) infectTimer = 5.0f; // tick reset
}

void Enemy::updateZombieRager(float dt, Vector2 target) {
    walkAnimTimer += dt * (hasRaged ? 8.0f : 3.0f);
    Vector2 dir  = {target.x - position.x, target.y - position.y};
    float   dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;

    if (!hasRaged && dist < 200.0f) {
        hasRaged  = true;
        speed     = 190.0f;
        damage   *= 1.5f;
        bodyColor = {200, 20, 20, 255};
    }
    if (dist > radius + 8.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }
}

void Enemy::updateZombieLord(float dt, Vector2 target) {
    walkAnimTimer += dt * 2.0f;
    Vector2 dir  = {target.x - position.x, target.y - position.y};
    float   dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;
    if (dist > radius + 15.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }

    // Phase 2 at 50% HP
    if (bossPhase == 1 && health < maxHealth * 0.50f) {
        bossPhase   = 2;
        speed      *= 1.3f;
        bodyColor   = {80, 20, 80, 255};
    }
    // Phase 3 at 25% HP — regen
    if (bossPhase < 3 && health < maxHealth * 0.25f) {
        bossPhase = 3;
        regenRate = 8.0f;
    }

    // Summon wave every 15s
    phaseTimer += dt;
    if (phaseTimer >= 15.0f) {
        phaseTimer  = 0.0f;
        summonCount += 4;
        wantsToShoot   = true; // abused flag — Game checks summonCount delta to spawn
        shootDirection = {0, -1};
        shootDamage    = 0.0f; // zero damage = summon signal
    }

    // Ranged acid shot
    shootCooldown -= dt;
    if (shootCooldown <= 0.0f && dist <= shootRange) {
        shootCooldown   = shootRate;
        wantsToShoot    = true;
        shootDirection  = dir;
        shootDamage     = 25.0f;
    }
}

void Enemy::updatePoltergeistBoss(float dt, Vector2 target) {
    walkAnimTimer += dt * 4.0f;
    Vector2 dir  = {target.x - position.x, target.y - position.y};
    float   dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;

    // Phase 2 at 40% HP — semi-permanent invisibility
    if (bossPhase == 1 && health < maxHealth * 0.40f) {
        bossPhase = 2;
        speed    *= 1.25f;
    }

    // Teleport every 4s
    invisTimer -= dt;
    if (invisTimer <= 0.0f) {
        invisTimer = 4.0f;
        float ang  = (float)GetRandomValue(0, 628) / 100.0f;
        float r    = (float)GetRandomValue(200, 400);
        position.x = target.x + std::cos(ang) * r;
        position.y = target.y + std::sin(ang) * r;
    }

    // Blink visible/invisible in phase 2
    if (bossPhase == 2) {
        phaseTimer += dt;
        isInvisible = std::fmod(phaseTimer, 2.0f) < 1.0f;
    }

    if (dist > radius + 20.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }

    shootCooldown -= dt;
    if (shootCooldown <= 0.0f && dist <= shootRange) {
        shootCooldown  = shootRate;
        wantsToShoot   = true;
        shootDirection = dir;
    }
}

void Enemy::updateShadowWraith(float dt, Vector2 target) {
    walkAnimTimer += dt * 5.0f;
    Vector2 dir  = {target.x - position.x, target.y - position.y};
    float   dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;
    if (dist > radius + 5.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    }
}

void Enemy::updateBansheeHowler(float dt, Vector2 target) {
    walkAnimTimer += dt * 2.5f;
    Vector2 dir  = {target.x - position.x, target.y - position.y};
    float   dist = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (dist > 0.001f) { dir.x /= dist; dir.y /= dist; }
    facing = (dir.x >= 0) ? 1 : -1;

    // Keep medium distance
    if (dist > 200.0f) {
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
    } else if (dist < 100.0f) {
        position.x -= dir.x * speed * dt;
        position.y -= dir.y * speed * dt;
    }

    shootCooldown -= dt;
    if (shootCooldown <= 0.0f && dist <= shootRange) {
        shootCooldown  = shootRate;
        wantsToShoot   = true;
        shootDirection = dir;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SOBRENATURAIS — RENDER
// ─────────────────────────────────────────────────────────────────────────────

