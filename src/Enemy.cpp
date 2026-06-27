#include "Enemy.h"
#include "Effects.h"
#include "SpriteGen.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

extern bool g_renderPass3D;

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

static void DrawRotatedRectangle(Vector2 center, float width, float length, float angleRad, Color color, bool wire) {
    Vector2 dir = { std::cos(angleRad), std::sin(angleRad) };
    Vector2 right = { -dir.y, dir.x };

    Vector2 halfDir = { dir.x * length * 0.5f, dir.y * length * 0.5f };
    Vector2 halfRight = { right.x * width * 0.5f, right.y * width * 0.5f };

    Vector2 tl = { center.x - halfDir.x - halfRight.x, center.y - halfDir.y - halfRight.y };
    Vector2 tr = { center.x + halfDir.x - halfRight.x, center.y + halfDir.y - halfRight.y };
    Vector2 br = { center.x + halfDir.x + halfRight.x, center.y + halfDir.y + halfRight.y };
    Vector2 bl = { center.x - halfDir.x + halfRight.x, center.y - halfDir.y + halfRight.y };

    if (wire) {
        DrawLineV(tl, tr, color);
        DrawLineV(tr, br, color);
        DrawLineV(br, bl, color);
        DrawLineV(bl, tl, color);
    } else {
        DrawTriangle(tl, bl, tr, color);
        DrawTriangle(tr, bl, br, color);
    }
}

void Enemy::render() const {
    // ── SPRITE PIXEL-ART (substitui o desenho por formas) ────────────────────
    SpriteBank& sb = SpriteBank::get();
    int et = (int)type;
    if (sb.ready && et >= 0 && et < SpriteBank::NUM_ENEMY_TYPES &&
        sb.enemy[et][0].id != 0) {
        float gt    = (float)GetTime();
        int   frame = ((int)(gt * 3.5f + position.x * 0.04f)) % SpriteBank::ENEMY_FRAMES;
        if (frame < 0) frame = 0;
        Texture2D tx = sb.enemy[et][frame];

        // Aura de tier de evolucao
        if (evolTier > 0 && auraColor.a > 0) {
            float ap = 0.85f + std::sin(aliveTimer * 3.0f) * 0.15f;
            float ar = radius * (1.4f + ap * 0.2f);
            DrawCircle((int)position.x, (int)position.y, ar, ColorAlpha(auraColor, 0.22f * ap));
            DrawCircleLines((int)position.x, (int)position.y, ar, ColorAlpha(auraColor, 0.7f * ap));
            if (evolTier >= 3) {
                float orbitR = ar * 1.25f;
                for (int i = 0; i < 4; ++i) {
                    float a = aliveTimer * 1.5f + i * 1.5708f;
                    DrawCircle((int)(position.x + std::cos(a)*orbitR),
                               (int)(position.y + std::sin(a)*orbitR), 4.0f, auraColor);
                }
            }
        }
        if (evolveFlash > 0.0f)
            DrawCircle((int)position.x, (int)position.y, radius*2.5f,
                       ColorAlpha(WHITE, (evolveFlash/0.8f)*0.45f));

        // Boss: aura épica de fase + coroa de pips + partículas de carregamento
        if (isBoss()) renderBossAura();

        // ── DANGER ZONE (Hades/FFXIV style) ──────────────────────────────────
        if (telegraphTimer > 0.0f || (isBoss() && bossAtkTimer > 0.0f && bossAtkTimer <= 0.5f)) {
            float pct = isBoss() ? (1.0f - (bossAtkTimer / 0.5f)) : (1.0f - (telegraphTimer / telegraphMax));
            if (pct < 0.0f) pct = 0.0f;
            if (pct > 1.0f) pct = 1.0f;

            Color warnColor = { 220, 20, 20, 255 }; // Hades Red
            Color fillColor = ColorAlpha(warnColor, 0.12f);
            Color activeFillColor = ColorAlpha(warnColor, 0.28f);
            float tp = 0.5f + 0.5f * std::sin(gt * 24.0f); // pulsating outline
            Color borderCol = ColorAlpha(warnColor, 0.6f + tp * 0.3f);
            float baseAngle = (shootDirection.x == 0.0f && shootDirection.y == 0.0f) ? 0.0f : std::atan2(shootDirection.y, shootDirection.x);

            // Determine shape based on bossPattern or role
            if (isBoss()) {
                // Boss attack shapes
                if (bossPattern == 1) { // LEQUE (Sector)
                    float spread = 0.85f;
                    float startAngle = (baseAngle - spread * 0.5f) * RAD2DEG;
                    float endAngle = (baseAngle + spread * 0.5f) * RAD2DEG;
                    DrawCircleSectorLines(position, shootRange, startAngle, endAngle, 24, borderCol);
                    DrawCircleSector(position, shootRange, startAngle, endAngle, 24, fillColor);
                    DrawCircleSector(position, shootRange * pct, startAngle, endAngle, 24, activeFillColor);
                }
                else if (bossPattern == 2) { // VARREDURA (Sweep)
                    float startAngle = (baseAngle - 0.8f) * RAD2DEG;
                    float endAngle = (baseAngle + 0.8f) * RAD2DEG;
                    DrawCircleSectorLines(position, shootRange, startAngle, endAngle, 24, borderCol);
                    DrawCircleSector(position, shootRange, startAngle, endAngle, 24, fillColor);
                    DrawCircleSector(position, shootRange * pct, startAngle, endAngle, 24, activeFillColor);
                }
                else if (bossPattern == 3) { // METRALHAR (Rectangle)
                    float width = radius * 3.5f;
                    float length = shootRange;
                    DrawRotatedRectangle(position, width, length, baseAngle, borderCol, true);
                    DrawRotatedRectangle(position, width, length, baseAngle, fillColor, false);
                    DrawRotatedRectangle(position, width, length * pct, baseAngle, activeFillColor, false);
                }
                else { // ANEL 360 (Circle)
                    float maxR = shootRange * 0.8f;
                    DrawCircleLines((int)position.x, (int)position.y, maxR, borderCol);
                    DrawCircleV(position, maxR, fillColor);
                    DrawCircleV(position, maxR * pct, activeFillColor);
                }
            }
            else {
                // Non-boss: Brutamonte lunge or regular shooter
                int role = combatRole();
                if (role == 3) { // Brutamonte lunge (Rectangle)
                    float width = radius * 2.8f;
                    float length = speed * 3.0f * 0.32f; // dash distance
                    if (length < 150.0f) length = 150.0f; // min length
                    DrawRotatedRectangle(position, width, length, baseAngle, borderCol, true);
                    DrawRotatedRectangle(position, width, length, baseAngle, fillColor, false);
                    DrawRotatedRectangle(position, width, length * pct, baseAngle, activeFillColor, false);
                }
                else { // Normal circle around enemy
                    float maxR = radius * 1.8f;
                    DrawCircleLines((int)position.x, (int)position.y, maxR, borderCol);
                    DrawCircleV(position, maxR, fillColor);
                    DrawCircleV(position, maxR * pct, activeFillColor);
                }
            }
        }

        // Sombra no chao — só fantasmas/wraiths flutuam; zumbis andam no chao
        bool floaty = isFloating();
        float bobF  = floaty ? std::sin(gt * 2.0f + position.x * 0.05f) * 4.0f : 0.0f;
        if (!g_renderPass3D) {
            DrawEllipse((int)position.x, (int)(position.y + radius * 0.9f),
                        radius * (floaty ? 0.7f : 0.95f), radius * 0.3f,
                        ColorAlpha(BLACK, floaty ? 0.25f : 0.4f));
        }

        // Desenha o sprite (feet ancorados no chao do inimigo)
        float scale = (radius * 3.0f) / (float)tx.height;
        float w = tx.width * scale, h = tx.height * scale;
        float feetY = position.y + radius * 0.9f + bobF;
        Color tint = WHITE;
        if (floaty) tint = ColorAlpha(WHITE, 0.82f);
        // brilho durante windup do telegraph (carregando ataque)
        if (telegraphTimer > 0.0f) {
            float gw = 0.5f + 0.5f * std::sin(gt * 30.0f);
            tint = Color{255, (unsigned char)(200 + (int)(55*gw)), (unsigned char)(120*gw), 255};
        }
        if (hitFlashTimer > 0.0f) tint = Color{255,170,170,255};
        DrawTexturePro(tx, {0,0,(float)tx.width,(float)tx.height},
                       {position.x - w/2, feetY - h, w, h}, {0,0}, 0.0f, tint);

        if (!g_renderPass3D) {
            // Aura/tag de elite
            if (isElite) {
                float puls = 0.5f + 0.5f * std::sin(elitePulse);
                Color eliteCol;
                switch (eliteMod) {
                    case 0:  eliteCol = {255, 60,  0,   255}; break;
                    case 1:  eliteCol = {180, 180, 255, 255}; break;
                    default: eliteCol = {255, 0,   200, 255}; break;
                }
                DrawCircleLines((int)position.x, (int)position.y, radius + 10 + puls * 4,
                                ColorAlpha(eliteCol, 0.6f));
                const char* tag = (eliteMod == 0) ? "BERSERK" : (eliteMod == 1) ? "BLINDADO" : "VOLATIL";
                DrawText(tag, (int)(position.x - MeasureText(tag, 10)/2),
                         (int)(position.y - radius - 32), 10, ColorAlpha(eliteCol, 0.9f));
            }

            // Barra de HP
            float barW  = isBoss() ? 70.0f : (type == EnemyType::Tank ? 48.0f : 36.0f);
            float hpPct = health / maxHealth;
            Color hpCol = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
            DrawHealthBar({position.x, position.y - radius - 16}, hpPct, barW, 5, hpCol);

            // Label de tier
            if (evolTier > 0) {
                const char* tierLabel = evolTier == 1 ? "[VET]" : evolTier == 2 ? "[ELT]" : "[LND]";
                Color tierCol = evolTier == 1 ? Color{0,220,100,255} :
                                evolTier == 2 ? Color{100,180,255,255} : Color{255,160,0,255};
                int tw = MeasureText(tierLabel, 9);
                DrawText(tierLabel, (int)(position.x - tw/2), (int)(position.y - radius - 27), 9, tierCol);
            }
        }
        return;
    }

    switch (type) {
        case EnemyType::MorphX:       renderMorphX();       return;
        case EnemyType::HunterDrone:  renderHunterDrone();  return;
        case EnemyType::KronosSentry: renderKronosSentry(); return;
        case EnemyType::Kamikaze:     renderKamikaze();     return;
        case EnemyType::Sniper:       renderSniper();       return;
        case EnemyType::Zergling:     renderZergling();     return;
        case EnemyType::Hydra:        renderHydra();        return;
        case EnemyType::Broodmother:  renderBroodmother();  return;
        case EnemyType::AlienBoss:         renderAlienBoss();         return;
        case EnemyType::OmegaBoss:         renderOmegaBoss();         return;
        case EnemyType::OrcCibernetico:    renderOrcCibernetico();    return;
        case EnemyType::PaladinCorrompido: renderPaladinCorrompido(); return;
        case EnemyType::UndeadEnforcer:  renderUndeadEnforcer();    return;
        case EnemyType::Ghost:
        case EnemyType::GhostElite:        renderGhost();             return;
        case EnemyType::Zombie:
        case EnemyType::ZombieHorde:       renderZombie();            return;
        case EnemyType::ZombieRager:       renderZombieRager();       return;
        case EnemyType::ZombieLord:        renderZombieLord();        return;
        case EnemyType::PoltergeistBoss:   renderPoltergeistBoss();   return;
        case EnemyType::ShadowWraith:      renderShadowWraith();      return;
        case EnemyType::BansheeHowler:     renderBansheeHowler();     return;
        default: break;
    }

    // ── Evolution tier aura ──────────────────────────────────────────────────
    if (evolTier > 0 && auraColor.a > 0) {
        float ap = 0.85f + std::sin(aliveTimer * 3.0f) * 0.15f;
        float ar = radius * (1.4f + ap * 0.2f);
        DrawCircle((int)position.x, (int)position.y, ar, ColorAlpha(auraColor, 0.22f * ap));
        DrawCircleLines((int)position.x, (int)position.y, ar, ColorAlpha(auraColor, 0.7f * ap));
        if (evolTier >= 3) {
            float orbitR = ar * 1.25f;
            for (int i = 0; i < 4; ++i) {
                float a = aliveTimer * 1.5f + i * 1.5708f;
                DrawCircle((int)(position.x + std::cos(a)*orbitR),
                           (int)(position.y + std::sin(a)*orbitR),
                           4.0f, auraColor);
            }
        }
    }
    if (evolveFlash > 0.0f)
        DrawCircle((int)position.x, (int)position.y, radius*2.5f,
                   ColorAlpha(WHITE, (evolveFlash/0.8f)*0.45f));

    // Ground shadow (depth illusion for all types)
    DrawEllipse((int)position.x, (int)(position.y + radius * 0.75f),
                radius * 1.1f, radius * 0.32f, ColorAlpha(BLACK, 0.35f));

    bool  flash   = hitFlashTimer > 0.0f;
    Color flash_c = ColorAlpha(WHITE, 0.85f);
    float leg     = std::sin(walkAnimTimer) * 5.0f;
    Color eyeGlow = {255, 60, 0, 255};
    Color metal   = {60, 60, 70, 255};
    Color metalLt = {90, 90, 105, 255};

    float px = position.x, py = position.y;
    float f  = (float)facing;

    if (type == EnemyType::Boss) {
        // ── IRON-VIII BOSS — Massive Enforcer ───────────────────────────────
        // Phase 2 rage aura (extra rings + electrical crackle)
        if (bossPhase == 2) {
            float rage = 0.5f + 0.5f * std::sin(walkAnimTimer * 8.0f);
            DrawCircleV(position, 90, ColorAlpha({255,80,0,255}, 0.06f + rage * 0.06f));
            DrawCircleV(position, 70, ColorAlpha({255,30,0,255}, 0.10f + rage * 0.08f));
            DrawCircleLines((int)px, (int)py, 68.0f + rage * 6.0f, ColorAlpha({255,60,0,255}, 0.5f + rage * 0.4f));
            // Arc sparks radiating out
            for (int i = 0; i < 6; ++i) {
                float ang = walkAnimTimer * 3.0f + i * 1.047f;
                float r1 = 40.0f + rage * 8.0f;
                float r2 = 60.0f + rage * 14.0f;
                DrawLineEx({px + std::cos(ang)*r1, py + std::sin(ang)*r1},
                           {px + std::cos(ang)*r2, py + std::sin(ang)*r2},
                           2.2f, ColorAlpha({255,150,0,255}, 0.7f + rage * 0.3f));
            }
            // Phase 2 label
            const char* p2tag = "!FASE 2!";
            int p2w = MeasureText(p2tag, 11);
            DrawText(p2tag, (int)(px - p2w/2), (int)(py - radius - 50), 11,
                     ColorAlpha({255,100,0,255}, 0.7f + rage * 0.3f));
        }
        // Heat aura
        DrawCircleV(position, 58, ColorAlpha({200,0,0,255}, 0.07f));
        DrawCircleV(position, 44, ColorAlpha({200,0,0,255}, 0.12f));

        // LEGS — hydraulic heavy struts
        // Left leg
        DrawRectangle((int)(px-17+leg), (int)(py+16), 13, 8,  metal);   // thigh armor
        DrawRectangle((int)(px-16+leg), (int)(py+17), 11, 6,  metalLt);
        DrawCircleV({px-10.5f+leg, py+25}, 7, metal);                   // knee
        DrawCircleV({px-10.5f+leg, py+25}, 3, metalLt);
        DrawRectangle((int)(px-15+leg), (int)(py+31), 10, 18, metal);   // shin
        DrawRectangle((int)(px-14+leg), (int)(py+32), 8, 5,  metalLt);
        DrawRectangle((int)(px-18+leg), (int)(py+48), 16, 5,  metal);   // foot
        DrawLineEx({px-10+leg,py+32},{px-10+leg,py+47},2.0f,ColorAlpha(eyeGlow,0.35f));
        // Right leg
        DrawRectangle((int)(px+4-leg),  (int)(py+16), 13, 8,  metal);
        DrawRectangle((int)(px+5-leg),  (int)(py+17), 11, 6,  metalLt);
        DrawCircleV({px+10.5f-leg, py+25}, 7, metal);
        DrawCircleV({px+10.5f-leg, py+25}, 3, metalLt);
        DrawRectangle((int)(px+5-leg),  (int)(py+31), 10, 18, metal);
        DrawRectangle((int)(px+6-leg),  (int)(py+32), 8, 5,  metalLt);
        DrawRectangle((int)(px+2-leg),  (int)(py+48), 16, 5,  metal);
        DrawLineEx({px+10-leg,py+32},{px+10-leg,py+47},2.0f,ColorAlpha(eyeGlow,0.35f));

        // Pelvis plate
        DrawRectangle((int)(px-16),(int)(py+10),32,8, {60,15,15,255});
        DrawCircleV({px-10,py+14},5,metal); DrawCircleV({px+10,py+14},5,metal);

        // TORSO — massive armored block
        Color torsoBase = (bossPhase==2) ? Color{120,30,0,255} : Color{90,15,15,255};
        Color torsoMain = (bossPhase==2) ? Color{220,60,0,255} : bodyColor;
        DrawRectangle((int)(px-22),(int)(py-18),44,30,torsoBase);
        DrawRectangle((int)(px-20),(int)(py-17),40,28,torsoMain);
        // Chest division
        DrawRectangle((int)(px- 1),(int)(py-17), 2,28,{50,8,8,255});
        // Ribs / internal skeleton lines
        for (int i=0;i<4;++i){
            float ry=py-12+i*7.0f;
            Color ribCol = (bossPhase==2) ? ColorAlpha({255,120,0,255},0.5f) : ColorAlpha(metalLt,0.4f);
            DrawLineEx({px-18,ry},{px-2,ry},1.5f,ribCol);
            DrawLineEx({px+ 2,ry},{px+18,ry},1.5f,ribCol);
        }
        // Spine
        DrawRectangle((int)(px-2),(int)(py-17),4,28,{45,10,10,255});
        // Red energy core — pulses harder in phase 2
        float coreSize = (bossPhase==2) ? 8.0f + 3.0f * std::sin(walkAnimTimer * 10.0f) : 6.0f;
        Color coreCol  = (bossPhase==2) ? Color{255,80,0,255} : eyeGlow;
        DrawRectangle((int)(px-5),(int)(py-5),10,10,{30,5,5,255});
        DrawGlowCircle({px,py}, coreSize, coreCol, coreSize - 1.0f);
        DrawCircleV({px,py},3, (bossPhase==2) ? Color{255,200,0,255} : RED);

        // SHOULDERS — heavy pauldrons
        DrawRectangle((int)(px-36),(int)(py-18),16,26,metal);
        DrawRectangle((int)(px-35),(int)(py-17),14,8,metalLt);
        DrawCircleV({px-28,py-16},7,metal);
        DrawRectangle((int)(px+20),(int)(py-18),16,26,metal);
        DrawRectangle((int)(px+21),(int)(py-17),14,8,metalLt);
        DrawCircleV({px+28,py-16},7,metal);

        // ARMS
        DrawRectangle((int)(px-34),(int)(py- 8),10,18,{80,15,15,255});
        DrawCircleV({px-29,py+12},5,metal);
        DrawRectangle((int)(px-33),(int)(py+12),8,12,metal);
        // Weapon arm — plasma cannon
        DrawRectangle((int)(px+24),(int)(py- 8),10,18,{80,15,15,255});
        DrawCircleV({px+29,py+12},5,metal);
        // Cannon barrel
        DrawRectangle((int)(px+f*22),(int)(py-10),(int)(f*26),12,metal);
        DrawRectangle((int)(px+f*30),(int)(py- 9),(int)(f*20),10,{45,45,55,255});
        DrawGlowCircle({px+f*52,py-4},7.0f,eyeGlow,5.0f);
        DrawCircleV({px+f*52,py-4},4,RED);

        // HEAD — IRON-VIII skull
        DrawRectangle((int)(px-16),(int)(py-46),32,30,{30,8,8,255});
        DrawRectangle((int)(px-14),(int)(py-44),28,28,metal);
        // Cheek plates
        DrawRectangle((int)(px-14),(int)(py-44),6,22,{55,55,65,255});
        DrawRectangle((int)(px+ 8),(int)(py-44),6,22,{55,55,65,255});
        // Cranium ridge
        DrawRectangle((int)(px-12),(int)(py-47),24,5,metalLt);
        // T-VISOR — iconic red (Phase 2 = blazing orange double-scan)
        DrawRectangle((int)(px-12),(int)(py-38),24,8,{15,0,0,255});
        if (bossPhase == 2) {
            float scanPulse = 0.6f + 0.4f * std::sin(walkAnimTimer * 12.0f);
            Color scanCol = {255, (unsigned char)(80 + (int)(scanPulse*60)), 0, 255};
            DrawGlowLine({px-11,py-36},{px+11,py-36},3.0f,scanCol);
            DrawGlowLine({px-11,py-32},{px+11,py-32},3.0f,scanCol);
            DrawGlowCircle({px+f*7,py-34},6.0f,scanCol,5.0f);
            DrawCircleV({px+f*7,py-34},3,{255,220,0,255});
        } else {
            DrawGlowLine({px-11,py-34},{px+11,py-34},3.5f,eyeGlow);
            DrawGlowCircle({px+f*7,py-34},4.5f,eyeGlow,4.0f);
            DrawCircleV({px+f*7,py-34},2,RED);
        }
        // Jaw
        DrawRectangle((int)(px-11),(int)(py-18),22,5,{40,40,50,255});
        DrawRectangle((int)(px- 9),(int)(py-17),18,3,{25,25,30,255});

    } else if (type == EnemyType::Tank) {
        // ── IRON-VIII TANK — Heavy Exoskeleton ───────────────────────────────
        // Legs
        DrawRectangle((int)(px-12+leg),(int)(py+12),10,20,metal);
        DrawRectangle((int)(px-11+leg),(int)(py+13), 8, 5,metalLt);
        DrawCircleV({px-7+leg,py+26},5,metal); DrawCircleV({px-7+leg,py+26},2,{20,20,25,255});
        DrawRectangle((int)(px-11+leg),(int)(py+30), 8,14,{50,50,60,255});
        DrawRectangle((int)(px-14+leg),(int)(py+43),14, 5,metal);

        DrawRectangle((int)(px+2-leg), (int)(py+12),10,20,metal);
        DrawRectangle((int)(px+3-leg), (int)(py+13), 8, 5,metalLt);
        DrawCircleV({px+7-leg,py+26},5,metal); DrawCircleV({px+7-leg,py+26},2,{20,20,25,255});
        DrawRectangle((int)(px+3-leg), (int)(py+30), 8,14,{50,50,60,255});
        DrawRectangle((int)(px+0-leg), (int)(py+43),14, 5,metal);

        // Pelvis
        DrawRectangle((int)(px-13),(int)(py+8),26,6,{60,15,15,255});

        // Torso
        DrawRectangle((int)(px-18),(int)(py-16),36,26,bodyColor);
        DrawRectangle((int)(px-16),(int)(py-15),32,24,{130,25,25,255});
        // Chest ribs
        for(int i=0;i<3;++i){
            float ry=py-10+i*7.0f;
            DrawLineEx({px-14,ry},{px-1,ry},1.5f,ColorAlpha(metalLt,0.45f));
            DrawLineEx({px+ 1,ry},{px+14,ry},1.5f,ColorAlpha(metalLt,0.45f));
        }
        DrawRectangle((int)(px-1),(int)(py-15),2,24,{55,10,10,255});
        // Core
        DrawGlowCircle({px,py},4.5f,eyeGlow,4.0f);
        DrawCircleV({px,py},2,RED);

        // Shoulders
        DrawRectangle((int)(px-26),(int)(py-14),10,20,metal);
        DrawCircleV({px-21,py-12},5,metalLt);
        DrawRectangle((int)(px+16),(int)(py-14),10,20,metal);
        DrawCircleV({px+21,py-12},5,metalLt);
        // Arms
        DrawRectangle((int)(px-25),(int)(py- 4),8,14,{80,20,20,255});
        DrawCircleV({px-21,py+12},4,metal);
        DrawRectangle((int)(px-24),(int)(py+10),7,10,metal);
        // Weapon arm
        DrawRectangle((int)(px+17),(int)(py- 4),8,14,{80,20,20,255});
        DrawCircleV({px+21,py+12},4,metal);
        DrawRectangle((int)(px+f*22),(int)(py- 6),(int)(f*16), 8,metal);
        DrawRectangle((int)(px+f*28),(int)(py- 5),(int)(f*12), 6,metalLt);
        DrawGlowCircle({px+f*42,py-2},4.5f,eyeGlow,4.0f);
        DrawCircleV({px+f*42,py-2},2,RED);

        // Head
        DrawRectangle((int)(px-12),(int)(py-36),24,22,metal);
        DrawRectangle((int)(px-10),(int)(py-35),20,20,{60,15,15,255});
        DrawRectangle((int)(px-10),(int)(py-36),20,5,metalLt);
        DrawRectangle((int)(px-11),(int)(py-30),22,8,{15,0,0,255});
        // Eye glow
        DrawGlowLine({px-9,py-26},{px+9,py-26},3.0f,eyeGlow);
        DrawCircleV({px+f*5,py-26},3,RED);
        // Jaw
        DrawRectangle((int)(px- 9),(int)(py-16),18,4,{40,40,50,255});

    } else if (type == EnemyType::Shooter) {
        // ── SHOOTER — Sleek ranged android ───────────────────────────────────
        // Legs (lighter, fast)
        DrawRectangle((int)(px-8+leg), (int)(py+10),7,14,metal);
        DrawCircleV({px-4.5f+leg,py+22},4,metalLt);
        DrawRectangle((int)(px-7+leg), (int)(py+25),5,11,{70,20,80,255});
        DrawRectangle((int)(px-9+leg), (int)(py+35),11, 4,metal);

        DrawRectangle((int)(px+1-leg), (int)(py+10),7,14,metal);
        DrawCircleV({px+4.5f-leg,py+22},4,metalLt);
        DrawRectangle((int)(px+2-leg), (int)(py+25),5,11,{70,20,80,255});
        DrawRectangle((int)(px-2-leg), (int)(py+35),11, 4,metal);

        // Pelvis
        DrawRectangle((int)(px-10),(int)(py+7),20,5,bodyColor);

        // Torso — narrow, agile
        DrawRectangle((int)(px-11),(int)(py-12),22,20,{60,15,80,255});
        DrawRectangle((int)(px- 9),(int)(py-11),18,18,bodyColor);
        // Panel lines
        DrawLineEx({px-7,py-8},{px+7,py-8},1.0f,ColorAlpha(metalLt,0.4f));
        DrawLineEx({px-7,py-1},{px+7,py-1},1.0f,ColorAlpha(metalLt,0.3f));
        DrawRectangle((int)(px-1),(int)(py-11),2,18,{50,10,65,255});
        DrawGlowCircle({px,py-4},3.5f,{200,0,255,255},3.5f);

        // Shoulders
        DrawRectangle((int)(px-18),(int)(py-12),8,16,{50,10,65,255});
        DrawCircleV({px-14,py-10},4,metalLt);
        DrawRectangle((int)(px+10),(int)(py-12),8,16,{50,10,65,255});
        DrawCircleV({px+14,py-10},4,metalLt);

        // Arms
        DrawRectangle((int)(px-17),(int)(py- 4),6,12,{80,20,90,255});
        DrawCircleV({px-14,py+10},3,metalLt);
        DrawRectangle((int)(px-16),(int)(py+9),5,8,metal);
        // Sniper rifle
        DrawRectangle((int)(px+11),(int)(py- 4),6,12,{80,20,90,255});
        DrawCircleV({px+14,py+10},3,metalLt);
        // Barrel (long)
        DrawRectangle((int)(px+f*12),(int)(py-11),(int)(f*6),5,{60,15,75,255});
        DrawRectangle((int)(px+f*17),(int)(py-12),(int)(f*24),5,metal);
        // Scope
        DrawRectangle((int)(px+f*19),(int)(py-16),(int)(f*8),5,{30,8,40,255});
        DrawGlowCircle({px+f*19,py-14},2.5f,{200,0,255,255},2.5f);
        // Muzzle
        DrawGlowCircle({px+f*42,py-10},5.0f,{180,0,255,255},4.0f);
        DrawCircleV({px+f*42,py-10},2.5f,{220,0,255,255});

        // Head
        DrawRectangle((int)(px-8),(int)(py-28),16,18,{50,10,65,255});
        DrawRectangle((int)(px-6),(int)(py-27),12,16,{70,15,85,255});
        DrawRectangle((int)(px-7),(int)(py-29),14,4,metalLt);
        // Visor slit
        DrawRectangle((int)(px-6),(int)(py-22),12,5,{15,0,20,255});
        DrawGlowLine({px-5,py-20},{px+5,py-20},2.5f,{200,0,255,255});
        DrawCircleV({px+f*4,py-20},2.5f,{220,100,255,255});
        // Jaw
        DrawRectangle((int)(px-5),(int)(py-12),10,3,{40,10,50,255});

    } else {
        // ── SCOUT — Light Endoskeleton ────────────────────────────────────────
        // Legs (exposed skeleton)
        DrawRectangle((int)(px-7+leg),(int)(py+8),6,12,metal);
        DrawCircleV({px-4+leg,py+18},3.5f,metalLt);
        DrawRectangle((int)(px-6+leg),(int)(py+20),4,10,{55,55,65,255});
        DrawRectangle((int)(px-7+leg),(int)(py+29),9, 3,metal);
        // Hydraulic line
        DrawLineEx({px-4+leg,py+18},{px-4+leg,py+28},1.5f,ColorAlpha(eyeGlow,0.3f));

        DrawRectangle((int)(px+1-leg),(int)(py+8),6,12,metal);
        DrawCircleV({px+4-leg,py+18},3.5f,metalLt);
        DrawRectangle((int)(px+2-leg),(int)(py+20),4,10,{55,55,65,255});
        DrawRectangle((int)(px-2-leg),(int)(py+29),9, 3,metal);
        DrawLineEx({px+4-leg,py+18},{px+4-leg,py+28},1.5f,ColorAlpha(eyeGlow,0.3f));

        // Pelvis
        DrawRectangle((int)(px-9),(int)(py+6),18,5,bodyColor);

        // Torso — exposed rib cage
        DrawRectangle((int)(px-9),(int)(py-10),18,18,bodyColor);
        DrawRectangle((int)(px-8),(int)(py- 9),16,16,{130,45,0,255});
        // Rib bones
        DrawLineEx({px-7,py-5},{px-1,py-5},1.5f,ColorAlpha(metalLt,0.5f));
        DrawLineEx({px+1,py-5},{px+7,py-5},1.5f,ColorAlpha(metalLt,0.5f));
        DrawLineEx({px-7,py+0},{px-1,py+0},1.5f,ColorAlpha(metalLt,0.4f));
        DrawLineEx({px+1,py+0},{px+7,py+0},1.5f,ColorAlpha(metalLt,0.4f));
        DrawLineEx({px-6,py+5},{px-1,py+5},1.5f,ColorAlpha(metalLt,0.3f));
        DrawLineEx({px+1,py+5},{px+ 6,py+5},1.5f,ColorAlpha(metalLt,0.3f));
        // Spine
        DrawRectangle((int)(px-1),(int)(py-9),2,16,{55,25,0,255});
        // Core light
        DrawGlowCircle({px,py-2},3.0f,eyeGlow,3.0f);

        // Shoulders
        DrawRectangle((int)(px-15),(int)(py-10),7,12,{80,30,0,255});
        DrawCircleV({px-11,py- 8},3.5f,metalLt);
        DrawRectangle((int)(px+ 8),(int)(py-10),7,12,{80,30,0,255});
        DrawCircleV({px+11,py- 8},3.5f,metalLt);
        // Arms
        DrawRectangle((int)(px-13),(int)(py- 2),5,10,metal);
        DrawCircleV({px-10,py+10},3,metalLt);
        // Claw
        DrawRectangle((int)(px+f*10),(int)(py- 4),(int)(f*8),4,metal);
        DrawLineEx({px+f*17,py-5},{px+f*20,py-2},2.0f,metalLt);
        DrawLineEx({px+f*17,py-3},{px+f*20,py+1},2.0f,metalLt);
        DrawLineEx({px+f*17,py-1},{px+f*20,py+4},2.0f,metalLt);

        // Head
        DrawRectangle((int)(px-7),(int)(py-22),14,14,metal);
        DrawRectangle((int)(px-6),(int)(py-21),12,12,{130,50,0,255});
        DrawRectangle((int)(px-6),(int)(py-22),12, 3,metalLt);
        // Eye sensor
        DrawRectangle((int)(px-5),(int)(py-17),10,4,{20,5,0,255});
        DrawGlowLine({px-4,py-15},{px+4,py-15},2.0f,eyeGlow);
        DrawCircleV({px+f*3,py-15},2.5f,RED);
        // Mandible
        DrawRectangle((int)(px-4),(int)(py-10),8,3,{40,20,0,255});
        DrawRectangle((int)(px-3),(int)(py- 9),6,2,metal);
    }

    // Elite aura
    if (isElite) {
        float puls = 0.5f + 0.5f * std::sin(elitePulse);
        Color eliteCol;
        switch (eliteMod) {
            case 0: eliteCol = {255, 60,  0,   255}; break; // Berserker — orange
            case 1: eliteCol = {180, 180, 255, 255}; break; // Armored — steel blue
            default:eliteCol = {255, 0,   200, 255}; break; // Volatile — magenta
        }
        DrawCircleV(position, radius + 12 + puls * 5, ColorAlpha(eliteCol, 0.18f));
        DrawCircleV(position, radius + 7  + puls * 3, ColorAlpha(eliteCol, 0.28f));
        DrawCircleLines((int)position.x, (int)position.y, radius + 10 + puls * 4,
                        ColorAlpha(eliteCol, 0.6f));
        // "ELITE" tag above head
        const char* tag = (eliteMod == 0) ? "BERSERK" : (eliteMod == 1) ? "BLINDADO" : "VOLATIL";
        DrawText(tag, (int)(position.x - MeasureText(tag, 10)/2),
                 (int)(position.y - radius - 32), 10, ColorAlpha(eliteCol, 0.9f));
    }

    // Hit flash overlay
    if (flash) {
        DrawCircleV(position, radius + 4, ColorAlpha(WHITE, 0.5f));
    }

    // HP bar
    float barW = (type == EnemyType::Boss) ? 70.0f : (type == EnemyType::Tank ? 48.0f : 36.0f);
    float hpPct = health / maxHealth;
    Color hpCol = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({position.x, position.y - radius - 16}, hpPct, barW, 5, hpCol);

    // Tier label above HP bar
    if (evolTier > 0) {
        const char* tierLabel = evolTier == 1 ? "[VET]" : evolTier == 2 ? "[ELT]" : "[LND]";
        Color tierCol = evolTier == 1 ? Color{0,220,100,255} :
                        evolTier == 2 ? Color{100,180,255,255} : Color{255,160,0,255};
        int tw = MeasureText(tierLabel, 9);
        DrawText(tierLabel, (int)(position.x - tw/2), (int)(position.y - radius - 27), 9, tierCol);
    }
}

void Enemy::renderMorphX() const {
    DrawEllipse((int)position.x, (int)(position.y + radius * 0.7f),
                radius * 1.0f, radius * 0.28f, ColorAlpha(BLACK, 0.32f));
    float pulse = std::sin(walkAnimTimer * 3.0f) * 2.0f;
    Color silver = {(unsigned char)(180 + (int)(pulse*3)), (unsigned char)(190 + (int)(pulse*3)), (unsigned char)(200 + (int)(pulse*3)), 255};
    Color silverDark = {120, 130, 140, 255};

    if (isMinion) {
        // Mini MorphX - blob
        DrawCircleV(position, radius + pulse * 0.3f, silver);
        DrawCircleV({position.x + 3, position.y - 3}, radius * 0.5f, WHITE);
    } else {
        // Corpo liquido deformado
        DrawEllipse((int)position.x, (int)position.y, 14.0f + pulse, 18.0f, silver);
        // Pseudo-cabeca
        DrawEllipse((int)position.x + facing*2, (int)position.y - 20, (int)10, (int)12, silverDark);
        // Olhos brancos
        DrawCircleV({position.x + facing*4, position.y - 22}, 3, WHITE);
        DrawCircleV({position.x + facing*4, position.y - 22}, 1.5f, BLACK);
        // Bracos-lanca
        DrawLineEx({position.x, position.y - 4},
                   {position.x + facing * 20, position.y - 8 + pulse}, 4, silver);
    }

    float barW = isMinion ? 20.0f : 32.0f;
    float hpPct = health / maxHealth;
    DrawRectangleRec({position.x - barW/2, position.y - radius - 14, barW, 4}, BLACK);
    DrawRectangleRec({position.x - barW/2, position.y - radius - 14, barW * hpPct, 4}, {0, 220, 220, 255});
}

void Enemy::renderHunterDrone() const {
    float hover = std::sin(walkAnimTimer) * 3.0f;
    Vector2 p = {position.x, position.y + hover};

    // Corpo central
    DrawCircleV(p, 16, bodyColor);
    DrawCircleV(p, 10, {0, 150, 170, 255});

    // Rotores nos 4 cantos
    for (int i = 0; i < 4; ++i) {
        float angle = orbitAngle * 8.0f + i * 1.5708f;
        Vector2 rotorPos = {p.x + std::cos(angle) * 22, p.y + std::sin(angle) * 22};
        DrawCircleV(rotorPos, 5, DARKGRAY);
        DrawCircleLines((int)rotorPos.x, (int)rotorPos.y, 8, ColorAlpha(SKYBLUE, 0.5f));
    }

    // Canhao inferior apontando para baixo/frente
    DrawRectangleV({p.x - 3, p.y + 14}, {6, 12}, DARKGRAY);
    DrawCircleV({p.x, p.y + 26}, 4, {0, 230, 255, 255});

    // Luz de scan
    DrawCircleV({p.x, p.y}, 4, {0, 255, 255, 255});

    float barW = 36.0f;
    float hpPct = health / maxHealth;
    DrawRectangleRec({p.x - barW/2, p.y - radius - 18, barW, 4}, BLACK);
    DrawRectangleRec({p.x - barW/2, p.y - radius - 18, barW * hpPct, 4}, {0, 200, 255, 255});
}

void Enemy::renderKronosSentry() const {
    // Base
    DrawCircleV(position, 20, DARKGRAY);
    DrawCircleV(position, 16, {80, 80, 90, 255});

    // Canhao rotativo
    float bA = barrelAngle + std::atan2(shootDirection.y, shootDirection.x);
    Vector2 barrelEnd = {position.x + std::cos(bA) * 28, position.y + std::sin(bA) * 28};
    DrawLineEx(position, barrelEnd, 6, bodyColor);
    DrawCircleV(barrelEnd, 5, bodyColor);

    // Olho central
    DrawCircleV(position, 7, {255, 80, 0, 255});
    DrawCircleV(position, 4, RED);

    // Raio de alcance (sutil)
    DrawCircleLines((int)position.x, (int)position.y, shootRange, ColorAlpha(RED, 0.06f));

    float barW = 36.0f;
    float hpPct = health / maxHealth;
    DrawRectangleRec({position.x - barW/2, position.y - radius - 18, barW, 4}, BLACK);
    DrawRectangleRec({position.x - barW/2, position.y - radius - 18, barW * hpPct, 4}, {255, 140, 0, 255});
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
        case 1: // Armored — triple health, slower
            health *= 3.0f; maxHealth = health;
            speed  *= 0.75f;
            break;
        case 2: // Volatile — normal stats, explodes on death
            health *= 1.8f; maxHealth = health;
            damage *= 1.4f;
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

void Enemy::renderKamikaze() const {
    float px = position.x, py = position.y;
    float flash = std::sin(walkAnimTimer * 18.0f); // fast red pulse
    float alpha = 0.55f + 0.45f * flash;
    Color core  = {255, 30, 0, 255};
    Color lit   = {255, 120, 0, 255};
    Color dark  = {80, 0, 0, 255};

    // Warning aura — pulsing danger circle
    DrawCircleV(position, radius + 8 + flash*4, ColorAlpha(core, 0.22f));

    // Compact, hunched endoskeleton
    // Legs (short, close)
    DrawRectangle((int)(px-6),(int)(py+6),5,10,dark);
    DrawRectangle((int)(px+1),(int)(py+6),5,10,dark);
    DrawCircleV({px-3.5f,py+14},3,lit);
    DrawCircleV({px+3.5f,py+14},3,lit);

    // Torso — packed explosive core
    DrawRectangle((int)(px-8),(int)(py-10),16,18,dark);
    DrawRectangle((int)(px-6),(int)(py-8),12,14,{120,10,0,255});
    // Explosive canisters
    DrawRectangle((int)(px-5),(int)(py-6),4,10,{60,5,0,255});
    DrawRectangle((int)(px+1),(int)(py-6),4,10,{60,5,0,255});
    DrawGlowCircle({px,py},5.0f,core,4.0f);
    DrawCircleV({px,py},2.5f,ColorAlpha(lit,(unsigned char)(alpha*255)));

    // Arms (reaching forward aggressively)
    DrawRectangle((int)(px-14),(int)(py-8),7,10,dark);
    DrawRectangle((int)(px+7),(int)(py-8),7,10,dark);

    // Head — no neck, low to body
    DrawRectangle((int)(px-5),(int)(py-18),10,10,dark);
    DrawRectangle((int)(px-3),(int)(py-17),6,8,{100,5,0,255});
    // Single large red sensor
    DrawGlowCircle({px,py-13},4.5f,core,4.0f);
    DrawCircleV({px,py-13},2,ColorAlpha(WHITE,(unsigned char)(alpha*255)));

    // Countdown beep visual
    float blink = (flash > 0.0f) ? 1.0f : 0.0f;
    DrawCircleV(position, radius+2, ColorAlpha(core, 0.3f * blink));

    float barW = 24.0f;
    float hpPct = health/maxHealth;
    DrawRectangleRec({px-barW/2, py-radius-14, barW, 4}, BLACK);
    DrawRectangleRec({px-barW/2, py-radius-14, barW*hpPct, 4}, core);

    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius+4, ColorAlpha(WHITE, 0.5f));
}

void Enemy::renderSniper() const {
    float px = position.x, py = position.y;
    float f   = (float)facing;
    Color metal   = {55,70,55,255};
    Color metalLt = {90,120,90,255};
    Color camo    = bodyColor;
    Color scope   = {0,200,80,255};

    // Scope laser line (shows aim)
    if (shootCooldown < 0.5f && shootCooldown > 0.0f) {
        float laserLen = 400.0f;
        DrawLineEx({px+f*10,py-5},
                   {px+f*10+shootDirection.x*laserLen,
                    py-5+shootDirection.y*laserLen},
                   1.0f, ColorAlpha(scope, 0.35f));
    }

    // Legs (prone/crouched pose)
    DrawRectangle((int)(px-7),(int)(py+8),6,12,metal);
    DrawRectangle((int)(px+1),(int)(py+8),6,12,metal);
    DrawCircleV({px-4,py+18},3,metalLt);
    DrawCircleV({px+4,py+18},3,metalLt);
    DrawRectangle((int)(px-6),(int)(py+20),5,9,{40,60,40,255});
    DrawRectangle((int)(px+1),(int)(py+20),5,9,{40,60,40,255});

    // Torso — lean, armored
    DrawRectangle((int)(px-9),(int)(py-12),18,21,camo);
    DrawRectangle((int)(px-7),(int)(py-11),14,18,{50,100,50,255});
    DrawRectangle((int)(px-1),(int)(py-11),2,18,{30,60,30,255});
    // Scope glint on chest
    DrawGlowCircle({px,py-4},3.0f,scope,3.0f);

    // Left shoulder / support arm
    DrawRectangle((int)(px-16),(int)(py-12),8,14,{40,70,40,255});
    DrawCircleV({px-12,py-10},3.5f,metalLt);
    DrawRectangle((int)(px-15),(int)(py- 2),6,10,metal);

    // Right shoulder / sniper arm
    DrawRectangle((int)(px+ 8),(int)(py-12),8,14,{40,70,40,255});
    DrawCircleV({px+12,py-10},3.5f,metalLt);
    // Sniper rifle — very long barrel
    DrawRectangle((int)(px+f*12),(int)(py-9),(int)(f*8),5,metal);
    DrawRectangle((int)(px+f*19),(int)(py-10),(int)(f*30),6,{45,60,45,255});
    DrawRectangle((int)(px+f*48),(int)(py- 9),(int)(f*8),4,metalLt); // barrel tip
    // Scope
    DrawRectangle((int)(px+f*22),(int)(py-14),(int)(f*10),5,{20,40,20,255});
    DrawGlowCircle({px+f*22,py-12},2.5f,scope,2.5f);
    DrawGlowCircle({px+f*58,py-7},3.5f,scope,3.5f); // muzzle glow

    // Head — camouflage helmet
    DrawRectangle((int)(px-7),(int)(py-24),14,14,camo);
    DrawRectangle((int)(px-5),(int)(py-23),10,12,{45,90,45,255});
    DrawRectangle((int)(px-6),(int)(py-24),12,3,metalLt);
    // Visor slit
    DrawRectangle((int)(px-4),(int)(py-19),8,4,{10,20,10,255});
    DrawGlowLine({px-3,py-17},{px+3,py-17},2.0f,scope);
    DrawCircleV({px+f*3,py-17},2.0f,{100,255,100,255});

    float barW = 28.0f;
    float hpPct = health/maxHealth;
    DrawRectangleRec({px-barW/2, py-radius-16, barW, 4}, BLACK);
    DrawRectangleRec({px-barW/2, py-radius-16, barW*hpPct, 4}, scope);

    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius+4, ColorAlpha(WHITE, 0.5f));
}

// ─── Alien update functions ───────────────────────────────────────────────────

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
    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        position.x  += knockback.x * dt;
        position.y  += knockback.y * dt;
        float decay  = std::exp(-8.0f * dt);
        knockback.x *= decay; knockback.y *= decay;
        if (std::abs(knockback.x) < 0.5f) knockback.x = 0.0f;
        if (std::abs(knockback.y) < 0.5f) knockback.y = 0.0f;
    }
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
    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        position.x  += knockback.x * dt;
        position.y  += knockback.y * dt;
        float decay  = std::exp(-8.0f * dt);
        knockback.x *= decay; knockback.y *= decay;
        if (std::abs(knockback.x) < 0.5f) knockback.x = 0.0f;
        if (std::abs(knockback.y) < 0.5f) knockback.y = 0.0f;
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
    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        position.x  += knockback.x * dt;
        position.y  += knockback.y * dt;
        float decay  = std::exp(-6.0f * dt);
        knockback.x *= decay; knockback.y *= decay;
        if (std::abs(knockback.x) < 0.5f) knockback.x = 0.0f;
        if (std::abs(knockback.y) < 0.5f) knockback.y = 0.0f;
    }
}

// ─── Alien render functions ───────────────────────────────────────────────────

void Enemy::renderZergling() const {
    float px = position.x, py = position.y;
    float f  = (float)facing;
    float spd = std::sin(walkAnimTimer * 2.5f);
    Color acidGreen = {50,200,0,255};
    Color darkGreen = {20,90,0,255};

    DrawEllipse((int)px, (int)(py + radius * 0.7f), radius * 1.0f, radius * 0.28f, ColorAlpha(BLACK,0.35f));
    DrawCircleV(position, radius + 8.0f + spd*2.0f, ColorAlpha(acidGreen, 0.15f));

    for (int i = 1; i <= 3; ++i)
        DrawCircleV({px - f*i*6.0f, py + i*1.5f}, radius * 0.4f, ColorAlpha(acidGreen, 0.10f * (4-i)));

    DrawEllipse((int)px, (int)(py+2), radius*1.2f, radius*0.9f, darkGreen);
    DrawEllipse((int)px, (int)(py+1), radius*0.9f, radius*0.65f, bodyColor);

    DrawLineEx({px+f*4,py-1},{px+f*16,py-8},2.5f,acidGreen);
    DrawLineEx({px+f*16,py-8},{px+f*22,py-1},2.0f,acidGreen);
    DrawLineEx({px+f*4,py+3},{px+f*15,py+9},2.5f,acidGreen);
    DrawLineEx({px+f*15,py+9},{px+f*21,py+3},2.0f,acidGreen);
    DrawLineEx({px-f*4,py-1},{px-f*12,py-6},2.0f,darkGreen);
    DrawLineEx({px-f*4,py+2},{px-f*12,py+6},2.0f,darkGreen);

    DrawEllipse((int)(px+f*10), (int)(py-6), 7, 6, darkGreen);
    DrawEllipse((int)(px+f*10), (int)(py-6), 5, 4, bodyColor);
    DrawTriangle({px+f*10,py-15},{px+f*7,py-8},{px+f*13,py-8}, acidGreen);
    DrawCircleV({px+f*8,py-8},2.0f,{255,30,0,255});
    DrawCircleV({px+f*12,py-6},1.5f,{255,80,0,255});
    DrawCircleV({px+f*14,py-4},1.5f,ColorAlpha(acidGreen,0.7f));
    DrawCircleV({px+f*17,py-2},1.0f,ColorAlpha(acidGreen,0.5f));

    float barW = 24.0f, hpPct = health/maxHealth;
    DrawRectangleRec({px-barW/2, py-radius-12, barW, 3}, BLACK);
    DrawRectangleRec({px-barW/2, py-radius-12, barW*hpPct, 3}, acidGreen);

    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius+4, ColorAlpha({100,255,0,255},0.6f));
}

void Enemy::renderHydra() const {
    float px = position.x, py = position.y;
    float f  = (float)facing;
    float sway = std::sin(walkAnimTimer * 2.0f) * 3.0f;
    Color acidGreen = {100,255,0,255};
    Color darkGreen = {0,120,30,255};
    Color spitGlow  = {80,255,80,255};

    DrawEllipse((int)px, (int)(py + radius * 0.7f), radius * 1.0f, radius * 0.28f, ColorAlpha(BLACK,0.35f));
    DrawCircleV(position, radius + 10.0f, ColorAlpha(acidGreen, 0.12f));

    for (int i = 0; i < 3; ++i) {
        float lx = px + (i-1)*8.0f;
        DrawLineEx({lx, py+10},{lx - 5, py+20}, 3.0f, darkGreen);
        DrawLineEx({lx, py+10},{lx + 5, py+20}, 3.0f, darkGreen);
    }

    DrawEllipse((int)px, (int)(py+4), radius*0.7f, radius*1.1f, darkGreen);
    DrawEllipse((int)px, (int)(py+3), radius*0.5f, radius*0.8f, bodyColor);

    for (int i = 0; i < 3; ++i) {
        float ry = py - 2.0f + i*5.0f;
        DrawLineEx({px-8,ry},{px+8,ry},1.0f,ColorAlpha(acidGreen,0.3f));
    }

    float neckLen = 18.0f;
    Vector2 neckTip = {px + f*neckLen, py - 14.0f + sway};
    DrawLineEx({px,py-6},{neckTip.x,neckTip.y}, 5.0f, darkGreen);
    DrawLineEx({px,py-6},{neckTip.x,neckTip.y}, 2.0f, bodyColor);

    if (shootCooldown < 0.4f) {
        DrawTriangle({neckTip.x+f*4,neckTip.y},
                     {neckTip.x-f*8,neckTip.y-10},
                     {neckTip.x-f*8,neckTip.y+8},
                     ColorAlpha(acidGreen,0.45f));
    }

    DrawEllipse((int)neckTip.x, (int)neckTip.y, 9, 7, darkGreen);
    DrawEllipse((int)neckTip.x, (int)neckTip.y, 6, 5, bodyColor);
    DrawTriangle({neckTip.x,neckTip.y-13},{neckTip.x-5,neckTip.y-6},{neckTip.x+5,neckTip.y-6}, acidGreen);
    DrawCircleV({neckTip.x+f*4,neckTip.y-2},2.5f,{255,220,0,255});
    DrawCircleV({neckTip.x+f*4,neckTip.y-2},1.0f,{0,0,0,255});
    if (shootCooldown < 0.35f) {
        DrawGlowCircle({neckTip.x+f*8,neckTip.y+1},5.0f,spitGlow,4.0f);
        DrawCircleV({neckTip.x+f*8,neckTip.y+1},2.5f,acidGreen);
    }

    float barW = 36.0f, hpPct = health/maxHealth;
    DrawRectangleRec({px-barW/2, py-radius-14, barW, 4}, BLACK);
    DrawRectangleRec({px-barW/2, py-radius-14, barW*hpPct, 4}, acidGreen);

    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius+4, ColorAlpha({100,255,0,255},0.6f));
}

void Enemy::renderBroodmother() const {
    float px = position.x, py = position.y;
    float pulse = std::sin(walkAnimTimer * 2.0f);
    Color acidGreen  = {80,220,0,255};
    Color darkGreen  = {20,70,0,255};
    Color phase2Col  = (bossPhase==2) ? Color{60,180,0,255} : bodyColor;

    DrawEllipse((int)px, (int)(py + radius * 0.7f), radius * 1.2f, radius * 0.32f, ColorAlpha(BLACK,0.4f));
    DrawCircleV(position, radius + 14.0f + pulse*4.0f, ColorAlpha(acidGreen, 0.14f));
    DrawCircleV(position, radius +  8.0f + pulse*2.0f, ColorAlpha(acidGreen, 0.20f));

    for (int i = 0; i < 8; ++i) {
        float ang = i * 0.7854f + walkAnimTimer * (i%2==0?0.5f:-0.5f);
        float legR = radius + 20.0f;
        float kx = px + std::cos(ang) * (radius * 0.8f);
        float ky = py + std::sin(ang) * (radius * 0.5f);
        float ex = px + std::cos(ang) * legR;
        float ey = py + std::sin(ang) * legR * 0.7f;
        DrawLineEx({kx,ky},{ex,ey},4.0f,darkGreen);
        DrawLineEx({kx,ky},{ex,ey},2.0f,bodyColor);
        DrawCircleV({ex,ey},3.0f,acidGreen);
    }

    DrawEllipse((int)px, (int)(py+2), radius*1.15f, radius*0.95f, darkGreen);
    DrawEllipse((int)px, (int)(py+1), radius*0.9f,  radius*0.75f, phase2Col);

    for (int i = -2; i <= 2; ++i)
        DrawCircleV({px+i*9.0f, py-radius*0.65f}, 7.0f + (i%2==0?2.0f:0.0f), {40,110,10,255});
    for (int i = -1; i <= 1; ++i)
        DrawCircleV({px+i*7.0f, py-radius*0.8f}, 4.0f, darkGreen);

    DrawLineEx({px-22,py+2},{px-38,py-10},6.0f,darkGreen);
    DrawLineEx({px-38,py-10},{px-44,py-2},4.0f,acidGreen);
    DrawLineEx({px-38,py-10},{px-42,py+5},4.0f,acidGreen);
    DrawLineEx({px+22,py+2},{px+38,py-10},6.0f,darkGreen);
    DrawLineEx({px+38,py-10},{px+44,py-2},4.0f,acidGreen);
    DrawLineEx({px+38,py-10},{px+42,py+5},4.0f,acidGreen);

    float headY = py - radius * 0.72f;
    DrawEllipse((int)px, (int)headY, 18, 14, darkGreen);
    DrawEllipse((int)px, (int)headY, 14, 10, {30,100,5,255});
    DrawCircleV({px-8,headY-2},4.0f,{220,255,0,255});
    DrawCircleV({px-4,headY-5},3.0f,{180,220,0,255});
    DrawCircleV({px+8,headY-2},4.0f,{220,255,0,255});
    DrawCircleV({px+4,headY-5},3.0f,{180,220,0,255});
    DrawCircleV({px-8,headY-2},1.5f,BLACK);
    DrawCircleV({px+8,headY-2},1.5f,BLACK);
    DrawLineEx({px-10,headY+6},{px-18,headY+14},4.0f,darkGreen);
    DrawLineEx({px+10,headY+6},{px+18,headY+14},4.0f,darkGreen);
    DrawCircleV({px-14,headY+16},2.5f,ColorAlpha(acidGreen,0.8f));
    DrawCircleV({px+14,headY+16},2.5f,ColorAlpha(acidGreen,0.8f));

    if (bossPhase == 2)
        DrawCircleLines((int)px,(int)py,radius+18,ColorAlpha(acidGreen,0.4f+pulse*0.3f));

    float barW = 60.0f, hpPct = health/maxHealth;
    DrawRectangleRec({px-barW/2, py-radius-16, barW, 5}, BLACK);
    DrawRectangleRec({px-barW/2, py-radius-16, barW*hpPct, 5},
                     hpPct > 0.5f ? acidGreen : Color{255,180,0,255});
    DrawText("BROODMOTHER", (int)(px-38),(int)(py-radius-28), 9, ColorAlpha(acidGreen,0.8f));

    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius+6, ColorAlpha({150,255,50,255},0.55f));
}

void Enemy::renderAlienBoss() const {
    float px = position.x, py = position.y;
    float pulse = std::sin(walkAnimTimer * 3.0f);
    float f = (float)facing;
    Color acidGreen  = {80,255,0,255};
    Color darkGreen  = {15,55,0,255};
    Color purpleGlow = {200,0,255,255};
    Color boneWhite  = {200,210,180,255};

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

void Enemy::renderOmegaBoss() const {
    float px = position.x, py = position.y;
    float pulse  = std::sin(walkAnimTimer * 2.5f);
    float pulse2 = std::sin(walkAnimTimer * 4.0f + 1.0f);
    float f = (float)facing;
    Color omegaPurple = {160, 0, 255, 255};
    Color omegaRed    = {255, 30, 80, 255};
    Color omegaGold   = {255, 200, 0, 255};
    Color coreDark    = {30, 0, 60, 255};
    Color metalDark   = {40, 40, 55, 255};
    Color boneWhite   = {210, 200, 190, 255};

    // Ground shadow
    DrawEllipse((int)px,(int)(py+radius*0.8f),radius*1.4f,radius*0.38f,ColorAlpha(BLACK,0.5f));

    // Outer aura rings — pulsing
    float auraR = radius + 30.0f + pulse*8.0f;
    DrawCircleV(position, auraR+12, ColorAlpha(omegaPurple, 0.06f + std::fabs(pulse)*0.04f));
    DrawCircleV(position, auraR,    ColorAlpha(omegaRed,    0.10f + std::fabs(pulse2)*0.06f));
    DrawCircleLines((int)px,(int)py,auraR+8.0f, ColorAlpha(omegaGold, 0.35f+std::fabs(pulse)*0.35f));
    DrawCircleLines((int)px,(int)py,auraR,      ColorAlpha(omegaPurple,0.5f+std::fabs(pulse2)*0.4f));

    // Phase 2 rage
    if (bossPhase == 2) {
        for (int r = 0; r < 3; ++r) {
            float rr = radius + 20.0f + r*18.0f + pulse2*5.0f;
            DrawCircleLines((int)px,(int)py,rr, ColorAlpha(omegaRed,0.25f+r*0.05f));
        }
        const char* p2 = "!! OMEGA FASE 2 !!";
        int p2w = MeasureText(p2, 12);
        DrawText(p2,(int)(px-p2w/2),(int)(py-radius-70),12,ColorAlpha(omegaRed,0.8f+std::fabs(pulse2)*0.2f));
    }

    // Body — massive hybrid mech+alien torso
    DrawEllipse((int)px,(int)(py+4),radius*1.0f,radius*0.85f,metalDark);
    DrawEllipse((int)px,(int)(py+3),radius*0.82f,radius*0.70f,bodyColor);

    // Spine cables
    for (int i = -3; i <= 3; ++i) {
        float ry = py - 8.0f + i*9.0f;
        float sway = std::sin(walkAnimTimer*2.5f + i*0.6f)*3.0f;
        DrawLineEx({px+sway-28,ry},{px+sway-8,ry},2.0f,ColorAlpha(omegaPurple,0.4f));
        DrawLineEx({px+sway+8,ry},{px+sway+28,ry},2.0f,ColorAlpha(omegaPurple,0.4f));
    }

    // Central OMEGA core — glowing
    float coreR = 8.0f + pulse*4.0f;
    DrawGlowCircle({px,py}, coreR+6.0f, omegaPurple, coreR+4.0f);
    DrawGlowCircle({px,py}, coreR,       omegaRed,    coreR-1.0f);
    DrawCircleV({px,py}, 4.0f, WHITE);

    // 8 mechanical legs (alien spider)
    for (int i = 0; i < 8; ++i) {
        float ang = i * 0.7854f + walkAnimTimer * (i%2==0 ? 0.6f : -0.6f);
        float kx = px + std::cos(ang)*radius*0.85f;
        float ky = py + std::sin(ang)*radius*0.65f;
        float ex = px + std::cos(ang)*(radius+48.0f);
        float ey = py + std::sin(ang)*(radius+48.0f)*0.70f;
        DrawLineEx({kx,ky},{ex,ey},6.0f,metalDark);
        DrawLineEx({kx,ky},{ex,ey},3.0f,omegaPurple);
        DrawCircleV({ex,ey},4.5f,omegaGold);
    }

    // Heavy shoulder cannons (both sides)
    for (int s = -1; s <= 1; s += 2) {
        float sx = px + s*(radius*0.85f);
        float sy = py - 12.0f;
        DrawRectangle((int)(sx-8),(int)(sy-14),16,20,metalDark);
        DrawRectangle((int)(sx-6),(int)(sy-12),12,16,{60,10,80,255});
        DrawRectangle((int)(sx+(s*8)),(int)(sy-8),(int)(s*24),8,{70,20,100,255});
        DrawGlowCircle({sx+s*30.0f,sy-4.0f},5.0f,omegaPurple,4.0f);
        DrawCircleV({sx+s*30.0f,sy-4.0f},2.5f,WHITE);
    }

    // Mechanical arms
    DrawLineEx({px-radius*0.8f,py-5},{px-radius*0.8f-30,py+15},8.0f,metalDark);
    DrawLineEx({px-radius*0.8f-30,py+15},{px-radius*0.8f-45,py+8},6.0f,omegaPurple);
    DrawCircleV({px-radius*0.8f-45,py+8},8.0f,omegaRed);
    DrawLineEx({px+radius*0.8f,py-5},{px+radius*0.8f+30,py+15},8.0f,metalDark);
    DrawLineEx({px+radius*0.8f+30,py+15},{px+radius*0.8f+45,py+8},6.0f,omegaPurple);
    DrawCircleV({px+radius*0.8f+45,py+8},8.0f,omegaRed);

    // Head — hybrid IRON-VIII skull + alien crest
    float headY = py - radius * 0.80f;
    DrawEllipse((int)px,(int)headY,22.0f,17.0f,metalDark);
    DrawEllipse((int)px,(int)headY,17.0f,13.0f,coreDark);
    // Alien crest
    DrawTriangle({px,headY-36},{px-12,headY-16},{px+12,headY-16},metalDark);
    DrawTriangle({px,headY-30},{px-8,headY-14},{px+8,headY-14},{60,0,90,255});
    // Eyes — dual-threat
    Color eyeL = (bossPhase==2) ? omegaRed    : omegaPurple;
    Color eyeR = (bossPhase==2) ? omegaGold   : omegaRed;
    DrawGlowCircle({px-9,headY-3},7.0f,eyeL,6.0f);
    DrawGlowCircle({px+9,headY-3},7.0f,eyeR,6.0f);
    DrawCircleV({px-9,headY-3},3.5f,WHITE);
    DrawCircleV({px+9,headY-3},3.5f,WHITE);
    DrawCircleV({px-9,headY-3},1.8f,BLACK);
    DrawCircleV({px+9,headY-3},1.8f,BLACK);
    // Mandibles
    DrawLineEx({px-14,headY+8},{px-26,headY+18},5.0f,metalDark);
    DrawLineEx({px-26,headY+18},{px-32,headY+12},3.0f,omegaPurple);
    DrawLineEx({px+14,headY+8},{px+26,headY+18},5.0f,metalDark);
    DrawLineEx({px+26,headY+18},{px+32,headY+12},3.0f,omegaPurple);
    // Antenna pair
    DrawLineEx({px-6,headY-17},{px-10,headY-38},2.5f,omegaGold);
    DrawLineEx({px+6,headY-17},{px+10,headY-38},2.5f,omegaGold);
    DrawCircleV({px-10,headY-38},3.5f,omegaRed);
    DrawCircleV({px+10,headY-38},3.5f,omegaRed);

    // HP bar — thick, tricolor
    float barW = 120.0f, hpPct = health/maxHealth;
    Color hpCol = hpPct>0.6f ? omegaPurple : hpPct>0.3f ? omegaGold : omegaRed;
    DrawRectangleRec({px-barW/2, py-radius-22, barW, 8},  BLACK);
    DrawRectangleRec({px-barW/2, py-radius-22, barW*hpPct, 8}, hpCol);
    DrawRectangleLinesEx({px-barW/2, py-radius-22, barW, 8}, 1.5f, ColorAlpha(WHITE,0.3f));
    const char* tag = "OMEGA BOSS";
    int tw = MeasureText(tag,12);
    DrawText(tag,(int)(px-tw/2),(int)(py-radius-38),12,ColorAlpha(omegaGold,0.95f));

    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius+10, ColorAlpha(WHITE, 0.55f));
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
    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        position.x  += knockback.x * dt; position.y  += knockback.y * dt;
        float decay  = std::exp(-8.0f * dt);
        knockback.x *= decay; knockback.y *= decay;
        if (std::fabs(knockback.x) < 0.5f) knockback.x = 0.0f;
        if (std::fabs(knockback.y) < 0.5f) knockback.y = 0.0f;
    }
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

    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        position.x  += knockback.x * dt; position.y  += knockback.y * dt;
        float decay  = std::exp(-6.0f * dt);
        knockback.x *= decay; knockback.y *= decay;
        if (std::fabs(knockback.x) < 0.5f) knockback.x = 0.0f;
        if (std::fabs(knockback.y) < 0.5f) knockback.y = 0.0f;
    }
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

    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        position.x  += knockback.x * dt; position.y  += knockback.y * dt;
        float decay  = std::exp(-7.0f * dt);
        knockback.x *= decay; knockback.y *= decay;
        if (std::fabs(knockback.x) < 0.5f) knockback.x = 0.0f;
        if (std::fabs(knockback.y) < 0.5f) knockback.y = 0.0f;
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

    if (knockback.x != 0.0f || knockback.y != 0.0f) {
        position.x  += knockback.x * dt; position.y  += knockback.y * dt;
        float decay  = std::exp(-8.0f * dt);
        knockback.x *= decay; knockback.y *= decay;
        if (std::fabs(knockback.x) < 0.5f) knockback.x = 0.0f;
        if (std::fabs(knockback.y) < 0.5f) knockback.y = 0.0f;
    }
}

// ─── WarCraft Cyberpunk render functions ──────────────────────────────────────

void Enemy::renderOrcCibernetico() const {
    float px = position.x, py = position.y;
    float f   = (float)facing;
    float leg = std::sin(walkAnimTimer) * 6.0f;

    Color orcGreen  = {40, 100, 30, 255};
    Color orcDark   = {20, 60, 10, 255};
    Color metal     = {80, 85, 95, 255};
    Color metalLt   = {110, 115, 125, 255};
    Color eyeRed    = {255, 30, 0, 255};
    Color kronosCol = {255, 60, 0, 255};

    DrawEllipse((int)px, (int)(py + radius * 0.75f),
                radius * 1.2f, radius * 0.33f, ColorAlpha(BLACK, 0.4f));

    // Roar aura
    if (roarActive) {
        float pulse = 0.5f + 0.5f * std::sin(walkAnimTimer * 8.0f);
        DrawCircleV(position, radius + 20.0f + pulse * 8.0f, ColorAlpha(eyeRed, 0.18f));
        DrawCircleLines((int)px, (int)py, radius + 18.0f + pulse * 6.0f,
                        ColorAlpha(eyeRed, 0.55f));
    }

    // LEGS — thick orc legs
    DrawRectangle((int)(px - 14 + leg), (int)(py + 16), 14, 10, orcDark);
    DrawCircleV({px - 7.0f + leg, py + 26.0f}, 7.0f, orcDark);
    DrawRectangle((int)(px - 13 + leg), (int)(py + 32), 11, 18, orcGreen);
    DrawRectangle((int)(px - 16 + leg), (int)(py + 49), 18, 6, orcDark);

    DrawRectangle((int)(px + 0 - leg), (int)(py + 16), 14, 10, orcDark);
    DrawCircleV({px + 7.0f - leg, py + 26.0f}, 7.0f, orcDark);
    DrawRectangle((int)(px + 2 - leg), (int)(py + 32), 11, 18, orcGreen);
    DrawRectangle((int)(px - 2 - leg), (int)(py + 49), 18, 6, orcDark);

    // TORSO — massive volumous body
    DrawRectangle((int)(px - 24), (int)(py - 20), 48, 38, orcDark);
    DrawRectangle((int)(px - 22), (int)(py - 18), 44, 34, orcGreen);

    // Chest KRONOS plate armor
    DrawRectangle((int)(px - 18), (int)(py - 14), 36, 24, metal);
    DrawRectangle((int)(px - 16), (int)(py - 12), 32, 20, {55, 58, 68, 255});
    // KRONOS logo hint (concentric circles on chest)
    DrawCircleLines((int)px, (int)(py - 2), 8.0f, ColorAlpha(kronosCol, 0.7f));
    DrawCircleLines((int)px, (int)(py - 2), 5.0f, ColorAlpha(kronosCol, 0.5f));
    DrawCircleV({px, py - 2.0f}, 2.5f, kronosCol);

    // SHOULDERS
    DrawRectangle((int)(px - 36), (int)(py - 20), 14, 26, orcDark);
    DrawRectangle((int)(px - 35), (int)(py - 19), 12, 10, {50, 55, 45, 255});
    DrawRectangle((int)(px + 22), (int)(py - 20), 14, 26, orcDark);
    DrawRectangle((int)(px + 23), (int)(py - 19), 12, 10, {50, 55, 45, 255});

    // LEFT ARM — mechanical implant (metal)
    DrawRectangle((int)(px - 34), (int)(py - 8), 11, 20, metal);
    DrawCircleV({px - 28.0f, py + 14.0f}, 6.0f, metal);
    DrawRectangle((int)(px - 33), (int)(py + 13), 10, 14, metalLt);
    // Claw/piston on mechanical arm
    DrawLineEx({px - 28, py + 26}, {px - 34, py + 32}, 3.0f, metalLt);
    DrawLineEx({px - 28, py + 26}, {px - 28, py + 34}, 3.0f, metalLt);
    DrawLineEx({px - 28, py + 26}, {px - 22, py + 32}, 3.0f, metalLt);
    // Hydraulic glow
    DrawGlowCircle({px - 28.0f, py + 14.0f}, 4.0f, kronosCol, 3.0f);

    // RIGHT ARM — organic orc arm
    DrawRectangle((int)(px + 23), (int)(py - 8), 11, 20, orcGreen);
    DrawCircleV({px + 29.0f, py + 14.0f}, 6.0f, orcDark);
    DrawRectangle((int)(px + 24), (int)(py + 12), 9, 12, {30, 80, 15, 255});
    // Fist
    DrawRectangle((int)(px + f * 28), (int)(py + 22), (int)(f * 14), 10, orcDark);

    // HEAD — large orc skull
    DrawRectangle((int)(px - 16), (int)(py - 50), 32, 32, orcDark);
    DrawRectangle((int)(px - 14), (int)(py - 48), 28, 28, orcGreen);
    // Tusks
    DrawRectangle((int)(px - 14), (int)(py - 22), 5, 10, {220, 200, 160, 255});
    DrawRectangle((int)(px + 9),  (int)(py - 22), 5, 10, {220, 200, 160, 255});
    // Left eye = KRONOS sensor (red circle)
    DrawRectangle((int)(px - 12), (int)(py - 42), 10, 7, {15, 5, 0, 255});
    DrawGlowCircle({px - 7.0f, py - 39.0f}, 5.5f, eyeRed, 4.5f);
    DrawCircleV({px - 7.0f, py - 39.0f}, 2.5f, WHITE);
    // Right eye = organic (darker green slit)
    DrawRectangle((int)(px + 2), (int)(py - 42), 10, 7, {10, 30, 5, 255});
    DrawGlowLine({px + 3.0f, py - 38.0f}, {px + 11.0f, py - 38.0f}, 2.5f,
                 {80, 200, 40, 255});

    // Elite / hit flash
    if (isElite) {
        float puls = 0.5f + 0.5f * std::sin(elitePulse);
        Color eliteCol = {255, 60, 0, 255};
        DrawCircleV(position, radius + 12 + puls * 5, ColorAlpha(eliteCol, 0.18f));
        DrawCircleLines((int)px, (int)py, radius + 10 + puls * 4, ColorAlpha(eliteCol, 0.6f));
    }
    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius + 4, ColorAlpha(WHITE, 0.5f));

    float barW = 52.0f, hpPct = health / maxHealth;
    Color hpCol = hpPct > 0.5f ? Color{0, 220, 80, 255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - radius - 18}, hpPct, barW, 5, hpCol);
}

void Enemy::renderPaladinCorrompido() const {
    float px = position.x, py = position.y;
    float f   = (float)facing;
    float leg = std::sin(walkAnimTimer) * 5.0f;

    Color gold      = {180, 150, 30, 255};
    Color goldOx    = {120, 95, 15, 255};  // oxidized gold
    Color purpleGlw = {160, 0, 255, 255};
    Color eyePurple = {200, 80, 255, 255};
    Color swordGlw  = {140, 0, 220, 255};

    DrawEllipse((int)px, (int)(py + radius * 0.75f),
                radius * 1.0f, radius * 0.28f, ColorAlpha(BLACK, 0.35f));

    // Purple corruption aura
    float pulse = 0.5f + 0.5f * std::sin(walkAnimTimer * 3.5f);
    DrawCircleV(position, radius + 14.0f + pulse * 4.0f, ColorAlpha(purpleGlw, 0.12f));

    // LEGS — golden plate greaves
    DrawRectangle((int)(px - 9 + leg),  (int)(py + 10), 9, 16, goldOx);
    DrawRectangle((int)(px - 8 + leg),  (int)(py + 11), 7, 6, gold);
    DrawCircleV({px - 4.5f + leg, py + 24.0f}, 5.0f, goldOx);
    DrawRectangle((int)(px - 8 + leg),  (int)(py + 28), 7, 12, {90, 65, 10, 255});
    DrawRectangle((int)(px - 10 + leg), (int)(py + 39), 13, 4, goldOx);

    DrawRectangle((int)(px + 0 - leg),  (int)(py + 10), 9, 16, goldOx);
    DrawRectangle((int)(px + 1 - leg),  (int)(py + 11), 7, 6, gold);
    DrawCircleV({px + 4.5f - leg, py + 24.0f}, 5.0f, goldOx);
    DrawRectangle((int)(px + 1 - leg),  (int)(py + 28), 7, 12, {90, 65, 10, 255});
    DrawRectangle((int)(px - 3 - leg),  (int)(py + 39), 13, 4, goldOx);

    // TORSO — golden breastplate
    DrawRectangle((int)(px - 14), (int)(py - 14), 28, 26, goldOx);
    DrawRectangle((int)(px - 12), (int)(py - 12), 24, 22, gold);
    // Corruption veins on armor
    DrawLineEx({px - 10, py - 8}, {px - 2, py + 2}, 1.5f, ColorAlpha(purpleGlw, 0.6f));
    DrawLineEx({px + 10, py - 8}, {px + 2, py + 2}, 1.5f, ColorAlpha(purpleGlw, 0.6f));
    DrawGlowCircle({px, py - 2.0f}, 4.0f, purpleGlw, 3.5f);
    DrawCircleV({px, py - 2.0f}, 1.8f, {220, 150, 255, 255});

    // SHOULDERS — pauldrons
    DrawRectangle((int)(px - 22), (int)(py - 14), 10, 18, goldOx);
    DrawCircleV({px - 17.0f, py - 12.0f}, 5.0f, gold);
    DrawRectangle((int)(px + 12), (int)(py - 14), 10, 18, goldOx);
    DrawCircleV({px + 17.0f, py - 12.0f}, 5.0f, gold);

    // LEFT ARM — shield side
    DrawRectangle((int)(px - 20), (int)(py - 4), 8, 14, goldOx);
    DrawCircleV({px - 16.0f, py + 12.0f}, 4.0f, goldOx);
    // Shield (golden rectangle, left side)
    DrawRectangle((int)(px - f * 30 - 10), (int)(py - 14), 18, 26, goldOx);
    DrawRectangle((int)(px - f * 30 - 8),  (int)(py - 12), 14, 22, gold);
    DrawLineEx({px - f * 30 - 1, py - 12}, {px - f * 30 - 1, py + 10}, 2.0f,
               ColorAlpha(purpleGlw, 0.5f));
    DrawGlowCircle({px - f * 30 + 1.0f, py - 1.0f}, 3.5f, purpleGlw, 3.0f);

    // RIGHT ARM — sword side
    DrawRectangle((int)(px + 12), (int)(py - 4), 8, 14, goldOx);
    DrawCircleV({px + 16.0f, py + 12.0f}, 4.0f, goldOx);
    // Sword blade glowing purple
    DrawRectangle((int)(px + f * 20), (int)(py - 18), (int)(f * 6), 28, {70, 40, 10, 255});
    DrawRectangle((int)(px + f * 22), (int)(py - 16), (int)(f * 3), 24, swordGlw);
    DrawGlowLine({px + f * 23, py - 16}, {px + f * 23, py + 8}, 4.0f, swordGlw);
    // Crossguard
    DrawRectangle((int)(px + f * 16), (int)(py - 5), (int)(f * 14), 5, gold);

    // HEAD — helmet with visor
    DrawRectangle((int)(px - 10), (int)(py - 38), 20, 26, goldOx);
    DrawRectangle((int)(px - 8),  (int)(py - 36), 16, 22, gold);
    DrawRectangle((int)(px - 8),  (int)(py - 38), 16, 5, goldOx);
    // Visor slit — purple glowing eyes
    DrawRectangle((int)(px - 8),  (int)(py - 28), 16, 7, {15, 5, 20, 255});
    DrawGlowLine({px - 6, py - 25}, {px + 6, py - 25}, 2.5f, eyePurple);
    DrawGlowCircle({px - 4.0f, py - 25.0f}, 3.0f, eyePurple, 2.5f);
    DrawGlowCircle({px + 4.0f, py - 25.0f}, 3.0f, eyePurple, 2.5f);
    // Plume on top
    DrawRectangle((int)(px - 3), (int)(py - 44), 6, 8, purpleGlw);

    // Shoot charge indicator
    if (shootCooldown < 0.5f && shootCooldown > 0.0f) {
        DrawCircleV(position, radius + 6, ColorAlpha(purpleGlw, 0.3f));
    }

    if (isElite) {
        float puls = 0.5f + 0.5f * std::sin(elitePulse);
        Color eliteCol = {255, 180, 0, 255};
        DrawCircleV(position, radius + 12 + puls * 5, ColorAlpha(eliteCol, 0.18f));
        DrawCircleLines((int)px, (int)py, radius + 10 + puls * 4, ColorAlpha(eliteCol, 0.6f));
    }
    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius + 4, ColorAlpha(WHITE, 0.5f));

    float barW = 40.0f, hpPct = health / maxHealth;
    Color hpCol = hpPct > 0.5f ? Color{0, 220, 80, 255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - radius - 16}, hpPct, barW, 5, hpCol);
}

void Enemy::renderUndeadEnforcer() const {
    float px = position.x, py = position.y;
    float f   = (float)facing;
    float leg = std::sin(walkAnimTimer) * 4.0f;

    Color bone     = {200, 185, 150, 255};
    Color boneRust = {140, 110, 70, 255};
    Color metal    = {70, 65, 55, 255};
    Color purpleNecro = {160, 0, 240, 255};
    Color eyeRed   = {255, 30, 0, 255};

    DrawEllipse((int)px, (int)(py + radius * 0.7f),
                radius * 1.0f, radius * 0.28f, ColorAlpha(BLACK, 0.32f));

    // Necromantic glow (joints)
    float necPulse = 0.4f + 0.6f * std::sin(walkAnimTimer * 4.0f);
    DrawCircleV(position, radius + 8.0f + necPulse * 3.0f, ColorAlpha(purpleNecro, 0.12f));

    // Post-revive stronger aura
    if (hasRevived) {
        DrawCircleV(position, radius + 16.0f, ColorAlpha(purpleNecro, 0.20f));
        DrawCircleLines((int)px, (int)py, radius + 14.0f,
                        ColorAlpha(purpleNecro, 0.55f));
    }

    // LEGS — partially destroyed endoskeleton
    // Left leg
    DrawRectangle((int)(px - 7 + leg), (int)(py + 8), 5, 10, metal);
    DrawCircleV({px - 4.5f + leg, py + 17.0f}, 3.5f, boneRust);
    DrawGlowCircle({px - 4.5f + leg, py + 17.0f}, 3.0f, purpleNecro, 2.0f);
    DrawRectangle((int)(px - 6 + leg), (int)(py + 20), 4, 10, bone);
    DrawRectangle((int)(px - 7 + leg), (int)(py + 29), 9, 3, metal);

    // Right leg (more intact)
    DrawRectangle((int)(px + 2 - leg), (int)(py + 8), 5, 10, metal);
    DrawCircleV({px + 4.5f - leg, py + 17.0f}, 3.5f, boneRust);
    DrawGlowCircle({px + 4.5f - leg, py + 17.0f}, 3.0f, purpleNecro, 2.0f);
    DrawRectangle((int)(px + 3 - leg), (int)(py + 20), 4, 10, bone);
    DrawRectangle((int)(px - 2 - leg), (int)(py + 29), 9, 3, metal);

    // PELVIS — cracked bone plate
    DrawRectangle((int)(px - 9), (int)(py + 5), 18, 5, boneRust);

    // TORSO — exposed ribs + rusted endoskeleton frame
    DrawRectangle((int)(px - 9), (int)(py - 10), 18, 17, metal);
    DrawRectangle((int)(px - 7), (int)(py - 9), 14, 15, {55, 50, 40, 255});
    // Exposed ribs (bone colour)
    DrawLineEx({px - 6, py - 5}, {px - 1, py - 5}, 1.5f, ColorAlpha(bone, 0.7f));
    DrawLineEx({px + 1, py - 5}, {px + 6, py - 5}, 1.5f, ColorAlpha(bone, 0.7f));
    DrawLineEx({px - 6, py + 0}, {px - 1, py + 0}, 1.5f, ColorAlpha(bone, 0.6f));
    DrawLineEx({px + 1, py + 0}, {px + 6, py + 0}, 1.5f, ColorAlpha(bone, 0.6f));
    DrawLineEx({px - 5, py + 5}, {px - 1, py + 5}, 1.5f, ColorAlpha(bone, 0.5f));
    DrawLineEx({px + 1, py + 5}, {px + 5, py + 5}, 1.5f, ColorAlpha(bone, 0.5f));
    // Spine
    DrawRectangle((int)(px - 1), (int)(py - 9), 2, 15, boneRust);
    // Necro core pulse in chest
    DrawGlowCircle({px, py - 1.0f}, 3.5f, purpleNecro, 3.0f);
    DrawCircleV({px, py - 1.0f}, 1.5f, {220, 180, 255, 255});

    // SHOULDERS — half-missing
    DrawRectangle((int)(px - 15), (int)(py - 10), 7, 10, metal);
    DrawCircleV({px - 11.0f, py - 8.0f}, 3.0f, boneRust);
    DrawGlowCircle({px - 11.0f, py - 8.0f}, 2.5f, purpleNecro, 2.0f);
    DrawRectangle((int)(px + 8),  (int)(py - 10), 7, 10, metal);

    // LEFT ARM — missing parts, just a bone strut
    DrawRectangle((int)(px - 13), (int)(py - 2), 4, 8, bone);
    DrawCircleV({px - 11.0f, py + 8.0f}, 2.5f, boneRust);
    DrawGlowCircle({px - 11.0f, py + 8.0f}, 2.0f, purpleNecro, 1.5f);

    // RIGHT ARM — claw
    DrawRectangle((int)(px + 9), (int)(py - 2), 4, 8, metal);
    DrawRectangle((int)(px + f * 10), (int)(py - 4), (int)(f * 7), 3, bone);
    DrawLineEx({px + f * 16, py - 5}, {px + f * 19, py - 2}, 1.5f, bone);
    DrawLineEx({px + f * 16, py - 3}, {px + f * 19, py + 1}, 1.5f, bone);
    DrawLineEx({px + f * 16, py - 1}, {px + f * 19, py + 4}, 1.5f, bone);

    // HEAD — cracked IRON-VIII skull
    DrawRectangle((int)(px - 7), (int)(py - 22), 14, 14, metal);
    DrawRectangle((int)(px - 5), (int)(py - 21), 10, 12, boneRust);
    // Crack lines
    DrawLineEx({px - 2, py - 21}, {px + 1, py - 15}, 1.0f, ColorAlpha(BLACK, 0.7f));
    // Left eye — OFF (dark)
    DrawCircleV({px - 3.5f, py - 16.0f}, 2.5f, {30, 20, 15, 255});
    // Right eye — still active (red sensor)
    DrawGlowCircle({px + 3.5f, py - 16.0f}, 4.0f, eyeRed, 3.5f);
    DrawCircleV({px + 3.5f, py - 16.0f}, 1.8f, WHITE);
    // Jaw — damaged
    DrawRectangle((int)(px - 4), (int)(py - 10), 8, 3, {45, 35, 25, 255});
    DrawRectangle((int)(px - 2), (int)(py - 9), 4, 2, bone);

    if (isElite) {
        float puls = 0.5f + 0.5f * std::sin(elitePulse);
        Color eliteCol = {180, 0, 255, 255};
        DrawCircleV(position, radius + 12 + puls * 5, ColorAlpha(eliteCol, 0.18f));
        DrawCircleLines((int)px, (int)py, radius + 10 + puls * 4, ColorAlpha(eliteCol, 0.6f));
    }
    if (hitFlashTimer > 0.0f)
        DrawCircleV(position, radius + 4, ColorAlpha(purpleNecro, 0.65f));

    float barW = 32.0f, hpPct = health / maxHealth;
    Color hpCol = hpPct > 0.5f ? Color{0, 220, 80, 255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - radius - 14}, hpPct, barW, 4, hpCol);
}

// ─────────────────────────────────────────────────────────────────────────────
// SOBRENATURAIS — UPDATE
// ─────────────────────────────────────────────────────────────────────────────

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

void Enemy::renderGhost() const {
    float px = position.x, py = position.y;
    bool  elite  = (type == EnemyType::GhostElite);
    float r      = elite ? 26.0f : 18.0f;
    float pulse  = 0.5f + 0.5f * std::sin(walkAnimTimer * 3.0f);
    unsigned char bodyAlpha = isInvisible ? 40 : (elite ? 160 : 180);

    Color ghostBlue  = {160, 200, 255, bodyAlpha};
    Color ghostCore  = {200, 230, 255, bodyAlpha};
    Color eyeVoid    = {10,  15,  30,  bodyAlpha};

    // Outer glow aura
    DrawCircleV({px, py}, r + 10.0f + pulse * 6.0f,
                ColorAlpha({120, 160, 255, 255}, isInvisible ? 0.03f : 0.08f + pulse * 0.05f));

    // Body — oval
    DrawEllipse((int)px, (int)py, r * 0.85f, r, ghostBlue);
    DrawEllipse((int)px, (int)(py - 4), r * 0.6f, r * 0.7f, ghostCore);

    // Tail — triangle wisps below
    Vector2 tl = {px - r * 0.5f, py + r * 0.6f};
    Vector2 tr = {px + r * 0.5f, py + r * 0.6f};
    Vector2 tb = {px, py + r * 1.6f};
    DrawTriangle(tr, tl, tb, ColorAlpha({160,200,255,255}, bodyAlpha * 0.5f / 255.0f));

    // Floating wisps
    for (int i = 0; i < 3; ++i) {
        float wa = walkAnimTimer * 1.5f + i * 2.094f;
        float wr = r * 0.4f + pulse * 3.0f;
        DrawCircleV({px + std::cos(wa)*wr, py + r*0.3f + std::sin(wa*1.3f)*4.0f},
                    3.0f + pulse * 1.5f,
                    ColorAlpha({180,220,255,255}, bodyAlpha * 0.6f / 255.0f));
    }

    // Eyes
    DrawCircleV({px - r*0.28f, py - r*0.15f}, r * 0.18f, eyeVoid);
    DrawCircleV({px + r*0.28f, py - r*0.15f}, r * 0.18f, eyeVoid);
    DrawCircleLines((int)(px - r*0.28f), (int)(py - r*0.15f), r * 0.18f,
                    ColorAlpha({80,120,200,255}, (float)bodyAlpha/255.0f));
    DrawCircleLines((int)(px + r*0.28f), (int)(py - r*0.15f), r * 0.18f,
                    ColorAlpha({80,120,200,255}, (float)bodyAlpha/255.0f));

    if (elite) {
        DrawCircleLines((int)px, (int)py, r + 14.0f + pulse*4.0f,
                        ColorAlpha({100,150,255,255}, 0.4f + pulse*0.3f));
    }

    float barW = elite ? 52.0f : 36.0f;
    float hpPct = health / maxHealth;
    Color hpCol = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - r - 14}, hpPct, barW, 4, hpCol);
}

void Enemy::renderZombie() const {
    float px = position.x, py = position.y;
    float f   = (float)facing;
    bool  horde = (type == EnemyType::ZombieHorde);
    float sc  = horde ? 0.75f : 1.0f;
    float leg = std::sin(walkAnimTimer) * 4.0f * sc;

    Color skinGreen = {60, 110, 50, 255};
    Color skinDark  = {40,  80, 30, 255};
    Color boneW     = {210, 210, 190, 255};
    Color bloodRed  = {120, 10, 10, 200};

    // Blood puddle
    DrawEllipse((int)px, (int)(py + 20*sc), 14.0f*sc, 4.0f*sc, ColorAlpha(bloodRed, 0.35f));

    // Shadow
    DrawEllipse((int)px, (int)(py + 18*sc), 16.0f*sc, 4.5f*sc, ColorAlpha(BLACK, 0.30f));

    // Legs — uneven shamble
    DrawRectangle((int)(px - 6*sc),  (int)(py + leg + 8*sc),  (int)(6*sc), (int)(14*sc), skinDark);
    DrawRectangle((int)(px + 1*sc),  (int)(py - leg + 8*sc),  (int)(6*sc), (int)(14*sc), skinDark);

    // Torso — hunched forward (offset px by facing*3)
    DrawRectangle((int)(px - 9*sc + f*3*sc), (int)(py - 8*sc), (int)(18*sc), (int)(18*sc), skinGreen);
    // Rot patches
    DrawRectangle((int)(px - 5*sc + f*3*sc), (int)(py - 5*sc), (int)(6*sc), (int)(5*sc), skinDark);
    DrawRectangle((int)(px + 1*sc + f*3*sc), (int)(py),        (int)(4*sc), (int)(4*sc), skinDark);

    // Arms dragging (low)
    DrawRectangle((int)(px - 14*sc + f*3*sc), (int)(py + 2*sc), (int)(6*sc), (int)(12*sc), skinGreen);
    DrawRectangle((int)(px + 8*sc + f*3*sc),  (int)(py + 2*sc), (int)(6*sc), (int)(12*sc), skinGreen);

    // Head
    DrawRectangle((int)(px - 8*sc + f*2*sc), (int)(py - 26*sc), (int)(16*sc), (int)(18*sc), skinGreen);
    // Eyes white
    DrawCircleV({px - 3*sc + f*2*sc, py - 20*sc}, 3.0f*sc, boneW);
    DrawCircleV({px + 3*sc + f*2*sc, py - 20*sc}, 3.0f*sc, boneW);
    // Iris gone — totally white
    DrawCircleV({px - 3*sc + f*2*sc, py - 20*sc}, 1.5f*sc, {240,240,230,255});
    DrawCircleV({px + 3*sc + f*2*sc, py - 20*sc}, 1.5f*sc, {240,240,230,255});
    // Mouth open, jagged
    DrawRectangle((int)(px - 5*sc + f*2*sc), (int)(py - 13*sc), (int)(10*sc), (int)(4*sc), {20,5,5,255});
    // Teeth
    for (int t = 0; t < 3; ++t)
        DrawRectangle((int)(px - 4*sc + t*3*sc + f*2*sc), (int)(py - 13*sc), (int)(2*sc), (int)(3*sc), boneW);

    if (hitFlashTimer > 0.0f) DrawCircleV(position, radius + 4, ColorAlpha(WHITE, 0.5f));

    float barW = horde ? 22.0f : 34.0f;
    float hpPct = health / maxHealth;
    Color hpCol = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - radius - (horde ? 10.0f : 14.0f)}, hpPct, barW, 4, hpCol);
}

void Enemy::renderZombieRager() const {
    float px = position.x, py = position.y;
    float f   = (float)facing;
    float leg = std::sin(walkAnimTimer) * 6.0f;
    float pulse = 0.5f + 0.5f * std::sin(walkAnimTimer * 6.0f);

    Color skin    = hasRaged ? Color{180, 20, 20, 255} : Color{120, 40, 40, 255};
    Color skinD   = hasRaged ? Color{130, 5,  5,  255} : Color{90,  20, 20, 255};
    Color vein    = {220, 0, 0, 200};
    Color boneW   = {210, 210, 190, 255};

    // Rage aura
    if (hasRaged) {
        DrawCircleV(position, radius + 12 + pulse*5, ColorAlpha({255,0,0,255}, 0.07f + pulse*0.06f));
        DrawCircleLines((int)px, (int)py, radius + 10 + pulse*4, ColorAlpha({255,50,0,255}, 0.4f + pulse*0.3f));
    }

    DrawEllipse((int)px, (int)(py + 20), 16.0f, 4.5f, ColorAlpha(BLACK, 0.32f));

    // Legs
    DrawRectangle((int)(px - 8), (int)(py + leg + 8), 8, 16, skinD);
    DrawRectangle((int)(px + 1), (int)(py - leg + 8), 8, 16, skinD);

    // Torso — bulky
    DrawRectangle((int)(px - 13 + f*3), (int)(py - 10), 26, 20, skin);
    // Veins
    DrawLineEx({px - 8 + f*3, py - 8}, {px - 4 + f*3, py + 2}, 1.2f, ColorAlpha(vein, 0.7f));
    DrawLineEx({px + 5 + f*3, py - 6}, {px + 2 + f*3, py + 4}, 1.2f, ColorAlpha(vein, 0.7f));

    // Arms raised
    DrawRectangle((int)(px - 20 + f*3), (int)(py - 6), 8, 14, skin);
    DrawRectangle((int)(px + 13 + f*3), (int)(py - 6), 8, 14, skin);

    // Head
    DrawRectangle((int)(px - 10 + f*2), (int)(py - 30), 20, 20, skin);
    DrawCircleV({px - 4 + f*2, py - 22}, 3.5f, hasRaged ? Color{255,50,0,255} : boneW);
    DrawCircleV({px + 4 + f*2, py - 22}, 3.5f, hasRaged ? Color{255,50,0,255} : boneW);
    // Mouth screaming
    DrawRectangle((int)(px - 6 + f*2), (int)(py - 14), 12, 6, {20,5,5,255});
    for (int t = 0; t < 4; ++t)
        DrawRectangle((int)(px - 5 + t*3 + f*2), (int)(py - 14), 2, 4, boneW);

    if (hitFlashTimer > 0.0f) DrawCircleV(position, radius + 4, ColorAlpha(WHITE, 0.5f));

    float barW = 36.0f, hpPct = health / maxHealth;
    Color hpCol = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - radius - 16}, hpPct, barW, 4, hpCol);
}

void Enemy::renderZombieLord() const {
    float px = position.x, py = position.y;
    float f   = (float)facing;
    float leg = std::sin(walkAnimTimer) * 5.0f;
    float pulse = 0.5f + 0.5f * std::sin(walkAnimTimer * 2.5f);

    Color skinG  = bossPhase >= 2 ? Color{70, 20, 70, 255} : Color{30, 80, 30, 255};
    Color skinD  = bossPhase >= 2 ? Color{50, 10, 50, 255} : Color{20, 55, 20, 255};
    Color glowG  = bossPhase >= 3 ? Color{180, 0, 220, 255} : Color{0, 200, 50, 255};
    Color eyeCol = bossPhase >= 2 ? Color{220, 0, 255, 255} : Color{0, 255, 60, 255};

    // Toxic aura
    float aurR = radius + 20 + pulse * 10;
    DrawCircleV(position, aurR, ColorAlpha(glowG, 0.06f + pulse*0.04f));
    DrawCircleLines((int)px, (int)py, aurR - 5, ColorAlpha(glowG, 0.25f + pulse*0.15f));

    DrawEllipse((int)px, (int)(py + 40), 30.0f, 8.0f, ColorAlpha(BLACK, 0.40f));

    // Robe/mantle (triangle behind)
    DrawTriangle({px - 24.0f, py + 30.0f}, {px + 24.0f, py + 30.0f}, {px, py - 48.0f},
                 ColorAlpha(skinD, 0.85f));

    // Legs
    DrawRectangle((int)(px - 12), (int)(py + leg + 16), 11, 22, skinD);
    DrawRectangle((int)(px + 2),  (int)(py - leg + 16), 11, 22, skinD);

    // Torso — large
    DrawRectangle((int)(px - 18 + f*4), (int)(py - 16), 36, 34, skinG);

    // Arms
    DrawRectangle((int)(px - 28 + f*4), (int)(py - 10), 11, 22, skinG);
    DrawRectangle((int)(px + 18 + f*4), (int)(py - 10), 11, 22, skinG);

    // Head — large
    DrawRectangle((int)(px - 14 + f*3), (int)(py - 44), 28, 28, skinG);

    // Bone crown
    for (int b = 0; b < 8; ++b) {
        float ba = (float)b * 0.7854f + walkAnimTimer * 0.3f;
        float bl = 16.0f + pulse * 4.0f;
        Vector2 bc = {px + std::cos(ba) * bl, py - 30 + std::sin(ba) * 6.0f};
        DrawRectangle((int)(bc.x - 2), (int)(bc.y - 8), 4, 10,
                      bossPhase >= 3 ? Color{200, 180, 255, 255} : Color{200, 195, 170, 255});
    }

    // Eyes
    DrawGlowCircle({px - 5 + f*3, py - 32}, 5.0f, eyeCol, 3.5f);
    DrawGlowCircle({px + 5 + f*3, py - 32}, 5.0f, eyeCol, 3.5f);

    // Phase label
    if (bossPhase >= 2) {
        const char* lbl = bossPhase == 3 ? "FASE 3" : "FASE 2";
        int lw = MeasureText(lbl, 10);
        DrawText(lbl, (int)(px - lw/2), (int)(py - radius - 50), 10,
                 ColorAlpha(eyeCol, 0.8f + pulse*0.2f));
    }

    if (hitFlashTimer > 0.0f) DrawCircleV(position, radius + 6, ColorAlpha(WHITE, 0.5f));

    // Boss HP bar
    float barW = 80.0f, hpPct = health / maxHealth;
    Color c1 = {0, 200, 50, 255}, c2 = {200, 0, 50, 255};
    Color hpc = hpPct > 0.5f ? c1 : hpPct > 0.25f ? YELLOW : c2;
    DrawHealthBar({px, py - radius - 18}, hpPct, barW, 7, hpc);
    DrawText("ZOMBIE LORD", (int)(px - 38), (int)(py - radius - 30), 10, ColorAlpha(eyeCol, 0.9f));
}

void Enemy::renderPoltergeistBoss() const {
    float px = position.x, py = position.y;
    float pulse = 0.5f + 0.5f * std::sin(walkAnimTimer * 4.0f);
    unsigned char bodyA = isInvisible ? 60 : 210;

    Color core   = {140, 80, 240, bodyA};
    Color coreW  = {220, 200, 255, bodyA};
    Color tenCol = {100, 40, 200, bodyA};

    // Outer glow
    DrawCircleV(position, radius + 18 + pulse*10,
                ColorAlpha({100,30,200,255}, isInvisible ? 0.02f : 0.05f + pulse*0.04f));

    // Tentacles of energy
    for (int i = 0; i < 6; ++i) {
        float ta   = walkAnimTimer * 1.8f + i * 1.047f;
        float tlen = 28.0f + pulse * 10.0f;
        Vector2 t1 = {px + std::cos(ta) * (radius * 0.6f), py + std::sin(ta) * (radius * 0.6f)};
        Vector2 t2 = {px + std::cos(ta) * tlen, py + std::sin(ta) * tlen};
        DrawLineEx(t1, t2, 2.5f + pulse, ColorAlpha(tenCol, (float)bodyA/255.0f * 0.8f));
        DrawCircleV(t2, 4.0f + pulse*2.0f, ColorAlpha(coreW, (float)bodyA/255.0f * 0.5f));
    }

    // Body — ghost oval
    DrawEllipse((int)px, (int)py, radius * 0.85f, radius, core);
    DrawEllipse((int)px, (int)(py - 5), radius * 0.55f, radius * 0.65f, coreW);

    // Eyes
    DrawGlowCircle({px - 10, py - 8}, 6.0f, {200, 100, 255, bodyA}, 4.0f);
    DrawGlowCircle({px + 10, py - 8}, 6.0f, {200, 100, 255, bodyA}, 4.0f);

    // Phase 2 ring
    if (bossPhase == 2) {
        DrawCircleLines((int)px, (int)py, radius + 8 + pulse*5,
                        ColorAlpha({180,0,255,255}, 0.5f + pulse*0.4f));
    }

    // Teleport warning (when invisTimer < 0.8s)
    if (invisTimer < 0.8f) {
        float warnA = 1.0f - invisTimer / 0.8f;
        DrawCircleV(position, radius + 20 + warnA*20, ColorAlpha({200,150,255,255}, warnA * 0.3f));
    }

    if (hitFlashTimer > 0.0f) DrawCircleV(position, radius + 5, ColorAlpha(WHITE, 0.5f));

    float barW = 70.0f, hpPct = health / maxHealth;
    Color hpc = hpPct > 0.5f ? Color{100,150,255,255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - radius - 20}, hpPct, barW, 6, hpc);
    DrawText("POLTERGEIST", (int)(px - 36), (int)(py - radius - 32), 10,
             ColorAlpha(coreW, 0.9f));
}

void Enemy::renderShadowWraith() const {
    float px = position.x, py = position.y;
    float pulse = 0.5f + 0.5f * std::sin(walkAnimTimer * 6.0f);

    Color shadowC = {20, 10, 40, 240};
    Color eyeR    = {180, 0, 30, 255};

    // Shadow aura — distortion
    for (int i = 3; i >= 1; --i) {
        DrawEllipse((int)px, (int)(py + 4), radius * 1.2f * i * 0.4f, radius * 0.35f * i * 0.5f,
                    ColorAlpha(BLACK, 0.08f));
    }

    // Body — flattened shadow ellipse
    DrawEllipse((int)px, (int)py, radius * 1.1f, radius * 0.75f, shadowC);
    DrawEllipse((int)px, (int)py, radius * 0.75f, radius * 0.5f,
                {35, 15, 60, 230});

    // Leaking particles (dark)
    for (int i = 0; i < 4; ++i) {
        float a = walkAnimTimer * 2.0f + i * 1.571f;
        float r = radius * 0.5f + pulse * 4.0f;
        DrawCircleV({px + std::cos(a)*r, py + std::sin(a)*r*0.5f},
                    2.5f + pulse, ColorAlpha({10,0,20,255}, 0.7f));
    }

    // Eyes
    DrawGlowCircle({px - radius*0.3f, py - radius*0.15f}, 4.0f, eyeR, 3.0f);
    DrawGlowCircle({px + radius*0.3f, py - radius*0.15f}, 4.0f, eyeR, 3.0f);

    if (hitFlashTimer > 0.0f) DrawCircleV(position, radius + 3, ColorAlpha(WHITE, 0.45f));

    float barW = 28.0f, hpPct = health / maxHealth;
    Color hpc = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - radius - 12}, hpPct, barW, 4, hpc);
}

void Enemy::renderBansheeHowler() const {
    float px = position.x, py = position.y;
    float f   = (float)facing;
    float pulse = 0.5f + 0.5f * std::sin(walkAnimTimer * 4.0f);
    bool  screaming = shootCooldown < 0.5f && shootCooldown > 0.0f;

    Color skinW  = {200, 190, 220, 200};
    Color hairC  = {160, 140, 190, 200};
    Color eyeH   = {255, 240, 200, 255};

    // Sound waves when screaming
    if (screaming) {
        for (int i = 1; i <= 3; ++i) {
            float wa = (float)i * 12.0f + shootCooldown * 40.0f;
            DrawCircleLines((int)px, (int)py, wa + radius, ColorAlpha({255,230,180,255}, 0.4f / i));
        }
    }

    DrawEllipse((int)px, (int)(py + 20), 14.0f, 4.0f, ColorAlpha(BLACK, 0.28f));

    // Legs — skeletal
    DrawRectangle((int)(px - 5), (int)(py + 8), 4, 16, {150, 140, 160, 200});
    DrawRectangle((int)(px + 2), (int)(py + 8), 4, 16, {150, 140, 160, 200});

    // Torso — skeletal, thin
    DrawRectangle((int)(px - 8 + f*2), (int)(py - 8), 16, 18, skinW);
    // Ribs visible
    for (int r = 0; r < 3; ++r)
        DrawLineEx({px - 6 + f*2, py - 4 + r*4}, {px + 6 + f*2, py - 4 + r*4},
                   0.8f, ColorAlpha({180,160,200,255}, 0.5f));

    // Hair flowing (long lines radiating)
    for (int h = 0; h < 6; ++h) {
        float ha  = (float)h * 0.524f - 1.571f + std::sin(walkAnimTimer + h) * 0.3f;
        float hl  = 20.0f + pulse * 8.0f;
        Vector2 hp0 = {px + f*2, py - 26};
        Vector2 hp1 = {px + f*2 + std::cos(ha)*hl, py - 26 + std::sin(ha)*hl};
        DrawLineEx(hp0, hp1, 1.5f, ColorAlpha(hairC, 0.7f + pulse*0.2f));
    }

    // Head
    DrawEllipse((int)(px + f*2), (int)(py - 22), 10.0f, 12.0f, skinW);
    DrawEllipse((int)(px + f*2), (int)(py - 24), 8.0f, 10.0f,
                ColorAlpha({220, 210, 230, 255}, 0.7f));

    // Eyes — glowing
    DrawGlowCircle({px - 4 + f*2, py - 24}, 3.5f, eyeH, 2.5f);
    DrawGlowCircle({px + 4 + f*2, py - 24}, 3.5f, eyeH, 2.5f);

    // Mouth — open wide vertically when screaming
    float mH = screaming ? 10.0f : 5.0f;
    DrawEllipse((int)(px + f*2), (int)(py - 16), 5.0f, mH, {10, 5, 15, 255});

    if (hitFlashTimer > 0.0f) DrawCircleV(position, radius + 4, ColorAlpha(WHITE, 0.5f));

    float barW = 32.0f, hpPct = health / maxHealth;
    Color hpc = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
    DrawHealthBar({px, py - radius - 16}, hpPct, barW, 4, hpc);
}


// ─────────────────────────────────────────────────────────────────────────────
// render3D() — modelo 3D low-poly (SEM CUBOS). Apenas primitivas arredondadas:
// DrawSphere / DrawSphereEx / DrawCapsule / DrawCylinderEx.
// Mapeamento de mundo: X3D = position.x, Z3D = position.y, Y = altura (base Y=0).
// Tamanho escalado por `radius`. Silhuetas agrupadas por categoria:
//   FLY      — incorporeos/voadores (Ghost, Wraith, Banshee, Bat, FrostWyrm...)
//   DRONE    — voadores mecanicos (HunterDrone, CorrupterDrone)
//   SPIDER   — insectoides/alienigenas (Zergling, Hydra, Broodmother, AlienBoss...)
//   BRUTE    — golens/tanques/bosses pesados (Boss, VoidColossus, Orc, Tank...)
//   TURRET   — torres estacionarias (KronosSentry, SiegeCrawler)
//   HUMANOID — fallback bipede (zumbis, paladinos, samurais, scouts...)
// ─────────────────────────────────────────────────────────────────────────────
void Enemy::render3D() const {
    const float x   = position.x;
    const float z   = position.y;
    const float r   = fmaxf(radius, 15.0f); // piso de tamanho (nao achatar de cima)
    const float t   = aliveTimer;          // anima discretamente (bob/orbita/passada)
    const float TAU = 6.28318530718f;
    const float fx  = (float)facing;       // lado esquerdo/direito (eixo X)

    auto V    = [](float vx, float vy, float vz) -> Vector3 { return Vector3{ vx, vy, vz }; };
    auto sph  = [&](float sx, float sy, float sz, float rad, Color col){ DrawSphereEx(V(sx,sy,sz), rad, 8, 8, col); };
    auto limb = [&](Vector3 a, Vector3 b, float rad, Color col){ DrawCapsule(a, b, rad, 6, 5, col); };

    Color c  = bodyColor;
    Color cd = ColorBrightness(bodyColor, -0.35f);    // tom escuro (membros)
    Color cl = ColorBrightness(bodyColor,  0.28f);    // tom claro  (cabeca/luz)

    // ── Cor dos olhos fiel ao 2D, por familia ─────────────────────────────────
    Color eye = Color{ 255, 70, 50, 255 };            // padrao: sensor vermelho (maquinas)
    switch (type) {
        case EnemyType::Zombie: case EnemyType::ZombieHorde:
            eye = Color{235,235,220,255}; break;       // olhos brancos podres
        case EnemyType::ZombieRager:
            eye = hasRaged ? Color{255,50,0,255} : Color{235,235,220,255}; break;
        case EnemyType::ZombieLord:
            eye = (bossPhase>=2) ? Color{220,0,255,255} : Color{0,255,60,255}; break;
        case EnemyType::Broodmother: case EnemyType::AlienBoss:
            eye = Color{220,255,0,255}; break;         // varios olhos amarelos
        case EnemyType::PaladinCorrompido:
            eye = Color{200,80,255,255}; break;
        case EnemyType::Ghost: case EnemyType::GhostElite:
            eye = Color{20,30,60,255}; break;          // vazios escuros
        case EnemyType::ShadowWraith: case EnemyType::VoidStalker:
            eye = Color{210,0,40,255}; break;
        case EnemyType::BansheeHowler:
            eye = Color{255,240,200,255}; break;
        case EnemyType::Hydra:       case EnemyType::Shooter:
        case EnemyType::Sniper:      case EnemyType::GhostSniper:
        case EnemyType::Necromancer: case EnemyType::LichKnight:
        case EnemyType::PlagueDoctor:case EnemyType::AcidSpitter:
        case EnemyType::ChaosSpawn:  case EnemyType::OmegaBoss:
            eye = projectileColor; break;              // olhos brilham na cor do tiro
        default: break;
    }

    enum Cat { SPECTRAL, WINGED, SERPENT, DRONE, SPIDER, TURRET, BLOB, BRUTE, HUMANOID } cat = HUMANOID;
    switch (type) {
        case EnemyType::Ghost:        case EnemyType::GhostElite:
        case EnemyType::ShadowWraith: case EnemyType::BansheeHowler:
        case EnemyType::PoltergeistBoss:
            cat = SPECTRAL; break;
        case EnemyType::CrimsonBat:   case EnemyType::DarkMatter:
            cat = WINGED; break;
        case EnemyType::Hydra:        case EnemyType::AbyssalEel:
        case EnemyType::FrostWyrm:    case EnemyType::Leviathan:
            cat = SERPENT; break;
        case EnemyType::HunterDrone:  case EnemyType::CorrupterDrone:
            cat = DRONE; break;
        case EnemyType::Zergling:     case EnemyType::Broodmother:
        case EnemyType::AlienBoss:    case EnemyType::OmegaBoss:
        case EnemyType::NeuralParasite:
            cat = SPIDER; break;
        case EnemyType::KronosSentry: case EnemyType::SiegeCrawler:
            cat = TURRET; break;
        case EnemyType::MorphX:
            cat = BLOB; break;
        case EnemyType::Boss:           case EnemyType::Tank:
        case EnemyType::OrcCibernetico: case EnemyType::VoidColossus:
        case EnemyType::MoltenGolem:    case EnemyType::IronGuard:
        case EnemyType::VolcanicTitan:  case EnemyType::InfernoHerald:
            cat = BRUTE; break;
        default:
            cat = HUMANOID; break;
    }

    switch (cat) {

    case SPECTRAL: {
        // Incorporeo: flutua, corpo translucido. Wraith fica rasteiro e achatado.
        float hov = r * (type==EnemyType::ShadowWraith ? 0.6f : 1.35f)
                  + sinf(t * 2.0f) * r * 0.16f;
        Vector3 core = V(x, hov + r * 0.85f, z);
        float a = isInvisible ? 0.20f : 0.62f;         // quase some quando invisivel

        if (type == EnemyType::ShadowWraith) {
            // Sombra achatada (disco) + particulas escuras + olhos vermelhos.
            DrawCylinderEx(V(x,hov-r*0.2f,z), V(x,hov+r*0.35f,z), r*1.1f, r*0.55f, 12,
                           ColorAlpha(c, 0.9f));
            sph(x, hov+r*0.2f, z, r*0.55f, ColorAlpha(cl, 0.85f));
            for (int i=0;i<4;++i){ float wa=t*2.0f+i*1.571f;
                sph(x+cosf(wa)*r*0.75f, hov+r*0.1f, z+sinf(wa)*r*0.75f, r*0.14f, ColorAlpha(cd,0.7f)); }
            sph(x-r*0.28f, hov+r*0.35f, z+r*0.5f, r*0.13f, eye);
            sph(x+r*0.28f, hov+r*0.35f, z+r*0.5f, r*0.13f, eye);
            break;
        }

        // Corpo etereo + cauda afunilando (cone arredondado).
        DrawSphereEx(core, r*0.92f, 8, 8, ColorAlpha(cl, a*0.9f));
        DrawCylinderEx(V(x, hov - r*0.5f, z), core, r*0.10f, r*0.82f, 9, ColorAlpha(c, a*0.75f));

        if (type == EnemyType::BansheeHowler) {
            // Cabelo esvoacante (capsulas radiando do topo) + boca aberta gritando.
            for (int i=0;i<6;++i){ float ha=(i-2.5f)*0.42f;
                limb(V(x, core.y+r*0.6f, z),
                     V(x+sinf(ha)*r*1.1f, core.y+r*1.3f+sinf(t+i)*r*0.15f, z-cosf(ha)*r*0.6f),
                     r*0.06f, ColorAlpha(cd, 0.85f)); }
            sph(x, core.y-r*0.25f, z+r*0.55f, r*0.18f, Color{15,8,18,255});
            if (shootCooldown < 0.5f && shootCooldown > 0.0f)   // ondas sonoras
                DrawCylinderEx(V(x,hov,z), V(x,hov+r*0.05f,z), r*1.8f, r*1.9f, 14,
                               ColorAlpha(Color{255,235,190,255}, 0.18f));
        }
        if (type == EnemyType::PoltergeistBoss) {
            // Tentaculos de energia orbitando.
            for (int i=0;i<6;++i){ float ta=t*1.8f+i*1.047f; float tl=r*(1.2f+0.25f*sinf(t*3.0f+i));
                limb(V(x+cosf(ta)*r*0.5f, core.y, z+sinf(ta)*r*0.5f),
                     V(x+cosf(ta)*tl, core.y+sinf(t*2.0f+i)*r*0.3f, z+sinf(ta)*tl),
                     r*0.08f, ColorAlpha(cd, a)); }
        }
        sph(x-r*0.30f, core.y+r*0.10f, z+r*0.55f, r*0.15f, eye);
        sph(x+r*0.30f, core.y+r*0.10f, z+r*0.55f, r*0.15f, eye);
        break;
    }

    case WINGED: {
        float hov = r*1.7f + sinf(t*2.0f)*r*0.25f;
        Vector3 body = V(x,hov,z);
        if (type == EnemyType::DarkMatter) {
            // Nucleo escuro + 3 fragmentos orbitando (split em 3 ao morrer).
            DrawSphereEx(body, r*0.7f, 8, 8, c);
            for (int i=0;i<3;++i){ float fa=t*2.5f+i*2.094f;
                sph(x+cosf(fa)*r*1.2f, hov+sinf(t*2.0f+i)*r*0.3f, z+sinf(fa)*r*1.2f, r*0.34f, cl); }
            sph(x, hov+r*0.1f, z+r*0.5f, r*0.16f, eye);
            break;
        }
        // Morcego: corpo pequeno + asas batendo.
        float flap = sinf(t*12.0f)*r*0.7f;
        DrawSphereEx(body, r*0.6f, 8, 8, c);
        limb(body, V(x-r*1.7f, hov+flap, z-r*0.2f), r*0.09f, cd);
        limb(body, V(x+r*1.7f, hov+flap, z-r*0.2f), r*0.09f, cd);
        sph(x-r*0.18f, hov+r*0.2f, z+r*0.45f, r*0.10f, eye);
        sph(x+r*0.18f, hov+r*0.2f, z+r*0.45f, r*0.10f, eye);
        break;
    }

    case SERPENT: {
        // Corpo em cadeia de capsulas (S sinuoso) + cabeca a frente.
        int   seg    = isBoss() ? 7 : 5;
        bool  flies  = (type==EnemyType::FrostWyrm);
        bool  emerge = (type==EnemyType::AbyssalEel);
        float baseY  = flies ? r*1.6f : r*0.5f;
        Vector3 prev = V(x, emerge?0.0f:baseY, z - r*1.0f);
        Vector3 headPos = prev;
        for (int i=0;i<seg;++i){
            float u    = (float)i/(float)(seg-1);
            float segR = r*(0.85f - 0.45f*u);                 // afunila ate a cabeca
            float wob  = sinf(t*3.0f + i*0.7f)*r*0.55f;
            float yy   = baseY + sinf(u*3.14159f)*r*1.1f + (emerge ? u*r*1.6f : 0.0f);
            Vector3 cur = V(x+wob, yy, z - r*1.0f + u*r*2.6f);
            if (i>0) DrawCapsule(prev, cur, segR, 8, 6, (i>=seg-2)?cl:c);
            prev = cur; headPos = cur;
        }
        DrawSphereEx(headPos, r*0.6f, 9, 9, cl);
        if (flies) {  // asas de gelo
            limb(headPos, V(headPos.x-r*1.6f, headPos.y+r*0.6f, headPos.z-r*0.8f), r*0.10f, ColorAlpha(c,0.7f));
            limb(headPos, V(headPos.x+r*1.6f, headPos.y+r*0.6f, headPos.z-r*0.8f), r*0.10f, ColorAlpha(c,0.7f));
        }
        sph(headPos.x-r*0.22f, headPos.y+r*0.12f, headPos.z+r*0.32f, r*0.13f, eye);
        sph(headPos.x+r*0.22f, headPos.y+r*0.12f, headPos.z+r*0.32f, r*0.13f, eye);
        break;
    }

    case DRONE: {
        float hov = r * 1.6f + sinf(t * 3.0f) * r * 0.12f;
        Vector3 hull = V(x, hov, z);
        DrawSphereEx(hull, r * 0.62f, 8, 8, c);                       // casco
        DrawCylinderEx(V(x, hov - r*0.10f, z), V(x, hov + r*0.10f, z),
                       r * 1.0f, r * 1.0f, 12, cd);                   // disco/anel
        for (int i = 0; i < 4; ++i) {                                 // rotores
            float a  = (float)i / 4.0f * TAU + t * 5.0f;
            sph(x + cosf(a)*r*1.0f, hov, z + sinf(a)*r*1.0f, r*0.18f, cl);
        }
        // Canhao inferior + sensor frontal.
        DrawCylinderEx(V(x, hov-r*0.1f, z+r*0.2f), V(x, hov-r*0.7f, z+r*0.2f), r*0.16f, r*0.10f, 8, cd);
        sph(x, hov, z + r*0.55f, r*0.20f, eye);
        break;
    }

    case SPIDER: {
        bool  big   = (type == EnemyType::Broodmother || isBoss());
        int   nLegs = big ? 8 : 6;
        float bodyY = r * 0.85f;
        Vector3 body = V(x, bodyY, z);
        DrawSphereEx(body, r * 0.90f, 9, 9, c);                       // abdomen
        Vector3 head = V(x, bodyY + r*0.10f, z + r*0.85f);
        DrawSphereEx(head, r * 0.55f, 8, 8, cd);                      // cefalotorax
        for (int i = 0; i < nLegs; ++i) {                             // pernas radiais bipartidas
            float a   = ((float)i / nLegs) * TAU + 0.3f;
            float dx  = cosf(a), dz = sinf(a);
            float wob = sinf(t * 6.0f + i) * 0.10f;
            Vector3 knee = V(x + dx*r*1.0f, bodyY + r*0.55f + wob*r, z + dz*r*1.0f);
            Vector3 foot = V(x + dx*r*1.7f, 0.0f, z + dz*r*1.7f);
            DrawCapsule(body, knee, r*0.12f, 5, 4, cd);
            DrawCapsule(knee, foot, r*0.10f, 5, 4, cd);
        }
        if (type == EnemyType::Zergling) {
            // Garras-foice projetadas a frente (traco marcante do zergling).
            limb(V(x-r*0.5f, bodyY+r*0.2f, z+r*0.4f), V(x-r*0.8f, bodyY+r*1.1f, z+r*1.6f), r*0.10f, cl);
            limb(V(x+r*0.5f, bodyY+r*0.2f, z+r*0.4f), V(x+r*0.8f, bodyY+r*1.1f, z+r*1.6f), r*0.10f, cl);
        }
        if (big) {  // mandibulas/presas frontais
            limb(head, V(x-r*0.4f, bodyY*0.3f, head.z+r*0.6f), r*0.10f, cl);
            limb(head, V(x+r*0.4f, bodyY*0.3f, head.z+r*0.6f), r*0.10f, cl);
            if (type==EnemyType::AlienBoss || type==EnemyType::OmegaBoss)   // crista de cranio
                DrawCylinderEx(V(head.x, head.y+r*0.3f, head.z),
                               V(head.x, head.y+r*1.4f, head.z-r*0.3f), r*0.30f, r*0.02f, 8, cd);
        }
        if (type == EnemyType::OmegaBoss) {  // canhoes de ombro hibridos
            for (int s=-1;s<=1;s+=2){
                Vector3 shh=V(x+s*r*0.8f, bodyY+r*0.6f, z);
                DrawCylinderEx(shh, V(x+s*r*1.2f, bodyY+r*0.6f, z+r*0.9f), r*0.18f, r*0.12f, 8, cd);
                sph(x+s*r*1.2f, bodyY+r*0.6f, z+r*0.9f, r*0.14f, eye);
            }
        }
        // Olhos (par central + extras nos bosses).
        sph(x - r*0.18f, head.y + r*0.15f, head.z + r*0.30f, r*0.12f, eye);
        sph(x + r*0.18f, head.y + r*0.15f, head.z + r*0.30f, r*0.12f, eye);
        if (big) {
            sph(x - r*0.34f, head.y + r*0.05f, head.z + r*0.22f, r*0.08f, eye);
            sph(x + r*0.34f, head.y + r*0.05f, head.z + r*0.22f, r*0.08f, eye);
        }
        break;
    }

    case TURRET: {
        bool  siege = (type == EnemyType::SiegeCrawler);
        float baseR = siege ? r*1.2f : r*1.0f;
        DrawCylinderEx(V(x, 0, z), V(x, r*0.6f, z), baseR, r*0.7f, 10, cd);   // base conica
        Vector3 dome = V(x, r*0.95f, z);
        DrawSphereEx(dome, r*0.70f, 9, 9, c);                                  // domo
        float barR = siege ? r*0.26f : r*0.16f;
        DrawCylinderEx(dome, V(x, r*0.95f, z + r*(siege?2.0f:1.6f)), barR, barR*0.7f, 8, cd); // cano
        sph(x, r*1.05f, z + r*0.45f, r*0.18f, eye);                            // olho/mira
        break;
    }

    case BLOB: {
        // MorphX metal liquido: corpo deformando + pseudo-cabeca + braco-lanca.
        float wob = sinf(t*5.0f)*r*0.18f;
        sph(x, r*0.9f + wob*0.3f, z, r*0.95f + wob, c);
        sph(x, r*1.8f, z, r*0.55f, cl);
        sph(x-r*0.18f, r*1.9f, z+r*0.4f, r*0.12f, Color{255,255,255,255});
        sph(x+r*0.18f, r*1.9f, z+r*0.4f, r*0.12f, Color{255,255,255,255});
        limb(V(x, r*1.1f, z), V(x+fx*r*1.4f, r*0.9f+wob, z+r*0.3f), r*0.14f, cl);
        break;
    }

    case BRUTE: {
        float hipY = r * 0.9f, shY = hipY + r * 1.3f;
        DrawCapsule(V(x - r*0.45f, 0, z), V(x - r*0.40f, hipY, z), r*0.32f, 7, 6, cd);  // pernas
        DrawCapsule(V(x + r*0.45f, 0, z), V(x + r*0.40f, hipY, z), r*0.32f, 7, 6, cd);
        DrawSphereEx(V(x, hipY + r*0.55f, z), r*0.95f, 9, 9, c);                        // torso
        DrawCapsule(V(x, hipY, z), V(x, shY, z), r*0.70f, 9, 8, c);
        // Nucleo brilhante no peito (golens de lava / void).
        if (type==EnemyType::MoltenGolem || type==EnemyType::VolcanicTitan ||
            type==EnemyType::InfernoHerald)
            sph(x, hipY+r*0.7f, z+r*0.5f, r*0.30f, Color{255,150,0,255});
        else if (type==EnemyType::VoidColossus)
            sph(x, hipY+r*0.7f, z+r*0.5f, r*0.30f, Color{200,0,255,255});
        float sw = sinf(t * 3.0f) * r * 0.15f;
        limb(V(x - r*0.90f, shY, z), V(x - r*1.05f, hipY*0.7f, z + sw), r*0.28f, cd); // braco esq
        DrawSphere(V(x - r*1.05f, hipY*0.7f, z + sw), r*0.32f, c);
        if (type == EnemyType::Boss) {
            // Braco-canhao de plasma apontado a frente.
            DrawCylinderEx(V(x+r*0.9f, shY*0.85f, z), V(x+r*0.9f, shY*0.85f, z+r*1.9f), r*0.30f, r*0.22f, 9, cd);
            sph(x+r*0.9f, shY*0.85f, z+r*2.0f, r*0.20f, eye);
        } else {
            limb(V(x + r*0.90f, shY, z), V(x + r*1.05f, hipY*0.7f, z - sw), r*0.28f, cd);
            DrawSphere(V(x + r*1.05f, hipY*0.7f, z - sw), r*0.32f, c);
        }
        Vector3 head = V(x, shY + r*0.45f, z);
        DrawSphereEx(head, r*0.42f, 8, 8, cl);
        if (type == EnemyType::OrcCibernetico) {  // presas
            sph(x-r*0.16f, head.y-r*0.3f, head.z+r*0.35f, r*0.08f, Color{225,210,170,255});
            sph(x+r*0.16f, head.y-r*0.3f, head.z+r*0.35f, r*0.08f, Color{225,210,170,255});
        }
        if (type == EnemyType::Boss) {  // T-visor horizontal vermelho
            DrawCapsule(V(x-r*0.32f, head.y+r*0.05f, z+r*0.34f),
                        V(x+r*0.32f, head.y+r*0.05f, z+r*0.34f), r*0.09f, 5, 4, eye);
        } else {
            sph(x - r*0.18f, head.y + r*0.05f, head.z + r*0.35f, r*0.10f, eye);
            sph(x + r*0.18f, head.y + r*0.05f, head.z + r*0.35f, r*0.10f, eye);
        }
        break;
    }

    default: { // HUMANOID
        bool zombie = (type==EnemyType::Zombie     || type==EnemyType::ZombieHorde ||
                       type==EnemyType::ZombieRager|| type==EnemyType::ZombieLord);
        float gait = sinf(t * 5.0f) * r * 0.30f;
        float hipY = r * 1.0f, shY = hipY + r * 1.05f;
        float lean = zombie ? r*0.22f : 0.0f;                          // zumbi curvado
        DrawCapsule(V(x - r*0.35f, 0, z - gait), V(x - r*0.30f, hipY, z), r*0.20f, 6, 5, cd);
        DrawCapsule(V(x + r*0.35f, 0, z + gait), V(x + r*0.30f, hipY, z), r*0.20f, 6, 5, cd);
        DrawCapsule(V(x, hipY, z), V(x, shY, z + lean), r*0.50f, 8, 7, c);
        // Bracos (zumbis esticam para frente).
        float aEndY = zombie ? hipY*1.05f : hipY*0.85f;
        float aFwd  = zombie ? r*0.7f : 0.0f;
        limb(V(x - r*0.55f, shY, z+lean), V(x - r*0.50f, aEndY, z + aFwd + gait), r*0.16f, cd);
        bool rightArmBusy = (type==EnemyType::PaladinCorrompido || type==EnemyType::Shooter ||
                             type==EnemyType::Sniper || type==EnemyType::GhostSniper ||
                             type==EnemyType::CyberSamurai);
        if (!rightArmBusy)
            limb(V(x + r*0.55f, shY, z+lean), V(x + r*0.50f, aEndY, z + aFwd - gait), r*0.16f, cd);
        Vector3 head = V(x, shY + r*0.50f, z + lean*1.1f);
        DrawSphereEx(head, r*0.42f, 8, 8, cl);
        if (type == EnemyType::UndeadEnforcer) {  // cranio rachado: 1 olho aceso
            sph(x - r*0.16f, head.y + r*0.05f, head.z + r*0.34f, r*0.07f, Color{35,25,20,255});
            sph(x + r*0.16f, head.y + r*0.05f, head.z + r*0.34f, r*0.10f, eye);
        } else {
            sph(x - r*0.16f, head.y + r*0.05f, head.z + r*0.34f, r*0.09f, eye);
            sph(x + r*0.16f, head.y + r*0.05f, head.z + r*0.34f, r*0.09f, eye);
        }

        // ── Acessorios por tipo (silhueta marcante) ──
        if (type == EnemyType::PaladinCorrompido) {
            // Escudo (disco) a esquerda + espada brilhante + plume.
            DrawCylinderEx(V(x-r*0.85f, shY*0.85f, z+r*0.25f), V(x-r*0.85f, shY*0.85f, z+r*0.45f),
                           r*0.55f, r*0.55f, 10, cd);
            Color sword = (auraColor.a>0) ? auraColor : Color{160,0,255,255};
            limb(V(x+r*0.55f, hipY*0.9f, z+r*0.3f), V(x+r*0.55f, shY+r*1.3f, z+r*0.5f), r*0.07f, sword);
            sph(x, head.y+r*0.6f, z, r*0.10f, sword);
        }
        else if (type==EnemyType::Shooter || type==EnemyType::Sniper || type==EnemyType::GhostSniper) {
            // Rifle/sniper: cano longo a frente + brilho de boca.
            float len = (type==EnemyType::Sniper || type==EnemyType::GhostSniper) ? 2.6f : 1.9f;
            limb(V(x+r*0.5f, shY*0.9f, z+r*0.2f), V(x+r*0.5f, shY*0.9f, z+r*len), r*0.08f, cd);
            sph(x+r*0.5f, shY*0.9f, z+r*len, r*0.10f, projectileColor);
        }
        else if (type==EnemyType::CyberSamurai || type==EnemyType::ReaperMech ||
                 type==EnemyType::DemonHunter) {
            // Lamina(s) brilhante(s).
            Color blade = (auraColor.a>0) ? auraColor : cl;
            limb(V(x+r*0.55f, hipY, z+r*0.3f), V(x+r*0.9f, shY+r*1.4f, z+r*0.8f), r*0.06f, blade);
            if (type != EnemyType::CyberSamurai)
                limb(V(x-r*0.55f, hipY, z+r*0.3f), V(x-r*0.9f, shY+r*1.4f, z+r*0.8f), r*0.06f, blade);
        }
        else if (type==EnemyType::Necromancer || type==EnemyType::LichKnight ||
                 type==EnemyType::PlagueDoctor || type==EnemyType::AcidSpitter) {
            // Cajado com orbe (conjuradores).
            limb(V(x+r*0.6f, 0, z+r*0.2f), V(x+r*0.6f, shY+r*1.4f, z+r*0.2f), r*0.06f, cd);
            sph(x+r*0.6f, shY+r*1.6f, z+r*0.2f, r*0.20f, projectileColor);
        }
        else if (type == EnemyType::Kamikaze) {
            sph(x, hipY+r*0.5f, z+r*0.3f, r*0.30f, Color{255,90,0,255});   // nucleo explosivo
        }
        else if (type == EnemyType::ZombieLord) {
            // Coroa de osso + manto cone.
            for (int b=0;b<8;++b){ float ba=(float)b/8.0f*TAU;
                limb(V(x+cosf(ba)*r*0.4f, head.y+r*0.35f, z+sinf(ba)*r*0.4f),
                     V(x+cosf(ba)*r*0.5f, head.y+r*0.8f,  z+sinf(ba)*r*0.5f), r*0.05f,
                     Color{205,200,175,255}); }
            DrawCylinderEx(V(x,0,z), V(x,hipY*1.1f,z), r*1.0f, r*0.4f, 10, cd);
        }
        break;
    }
    } // switch(cat)

    // ── Bosses: esferas de energia orbitando + nucleo flutuante (imponencia). ──
    if (isBoss()) {
        float topY = r * 3.4f;
        for (int i = 0; i < 4; ++i) {
            float a = ((float)i / 4.0f) * TAU + t * 1.5f;
            DrawSphere(V(x + cosf(a)*r*1.3f, topY + sinf(t*2.0f + i)*r*0.20f, z + sinf(a)*r*1.3f),
                       r*0.22f, ColorAlpha(cl, 0.85f));
        }
        DrawSphereEx(V(x, topY + r*0.4f, z), r*0.30f, 7, 7, ColorAlpha(cl, 0.9f));
    }

    // ── Evolucao por tempo de vida: aura translucida na cor do tier (auraColor). ──
    if (evolTier > 0 && auraColor.a > 0) {
        DrawSphereEx(V(x, r*1.2f, z), r*1.7f, 8, 8, ColorAlpha(auraColor, 0.10f));
        if (evolTier >= 3)
            for (int i = 0; i < 4; ++i) {
                float a = ((float)i / 4.0f) * TAU + t * 1.5f;
                DrawSphere(V(x + cosf(a)*r*1.9f, r*0.2f, z + sinf(a)*r*1.9f), r*0.12f, auraColor);
            }
    }

    // ── Elite: aura translucida + anel de esferas brilhantes no chao. ──
    if (isElite) {
        Color glow = (auraColor.a > 0) ? auraColor : Color{ 255, 215, 0, 255 };
        DrawSphereEx(V(x, r*1.2f, z), r*1.6f, 8, 8, ColorAlpha(glow, 0.12f));
        for (int i = 0; i < 6; ++i) {
            float a = ((float)i / 6.0f) * TAU + t * 2.0f;
            DrawSphere(V(x + cosf(a)*r*1.5f, r*0.15f, z + sinf(a)*r*1.5f), r*0.12f, glow);
        }
    }

    // ── Flash de dano: brilho branco translucido. ──
    if (hitFlashTimer > 0.0f)
        DrawSphereEx(V(x, r*1.3f, z), r*1.15f, 8, 8, ColorAlpha(WHITE, 0.35f));
}
