// Game_Spawn.cpp — spawn of enemies, bosses finais and companheiros; screen of victory.
// Extraido of Game.cpp. Same class Game.
#include "Game.h"
#include "SpriteGen.h"
#include "SpriteExtrude.h"
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

// ─── Spawn ───────────────────────────────────────────────────────────────────

void Game::spawnEnemy() {
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    float dist  = (float)GetRandomValue(840, 1120);   // LONGE (outside the screen) — not "nascem of the meu lado"
    Vector2 pos = {
        player.position.x + std::cos(angle) * dist,
        player.position.y + std::sin(angle) * dist
    };
    // Nunca inside the SAFE ZONE (city/refuge): plays the spawn to outside the radius.
    if (inSafeZone(pos)) {
        Vector2 d = { pos.x - safeZoneCenter.x, pos.y - safeZoneCenter.y };
        float l = std::sqrt(d.x*d.x + d.y*d.y); if (l < 1.0f) { d = {1.0f, 0.0f}; l = 1.0f; }
        pos.x = safeZoneCenter.x + d.x / l * (safeZoneRadius + 140.0f);
        pos.y = safeZoneCenter.y + d.y / l * (safeZoneRadius + 140.0f);
    }

    // If the point continue solido after of the tentativas, the enemy nascia DENTRO
    // of um building and ficava presos la to always: with the city densa isso zerou
    // the combat (0 kills in 80s with 30 enemies vivos).
    auto slideToFree = [&](Vector2 q) {
        for (int st = 0; st < 14 && isBlocked(q); ++st) {
            Vector2 d = { player.position.x - q.x, player.position.y - q.y };
            float l = sqrtf(d.x*d.x + d.y*d.y); if (l < 1.0f) break;
            q.x += d.x / l * 70.0f; q.y += d.y / l * 70.0f;
        }
        return q;
    };
    int attempts = 0;
    while (tilemap.isWallAtPosition(pos) && attempts < 10) {
        angle = GetRandomValue(0, 360) * DEG2RAD;
        pos.x = player.position.x + std::cos(angle) * dist;
        pos.y = player.position.y + std::sin(angle) * dist;
        if (inSafeZone(pos)) {
            Vector2 d = { pos.x - safeZoneCenter.x, pos.y - safeZoneCenter.y };
            float l = std::sqrt(d.x*d.x + d.y*d.y); if (l < 1.0f) { d = {1.0f, 0.0f}; l = 1.0f; }
            pos.x = safeZoneCenter.x + d.x / l * (safeZoneRadius + 140.0f);
            pos.y = safeZoneCenter.y + d.y / l * (safeZoneRadius + 140.0f);
        }
        attempts++;
    }

    clampInsideOpenWorldBounds(pos, 120.0f);

    pos = slideToFree(pos);

    ZoneInfo info = getZoneInfo(currentZone);
    int roll = GetRandomValue(0, 100);

    EnemyType type;
    switch (currentZone) {
        case ZoneID::LARuins:
            // 3% OrcCibernetico (roll 97-99), rest shifted down
            if      (roll > 97) type = EnemyType::OrcCibernetico;
            else if (roll > 95) type = EnemyType::Zergling;
            else if (roll > 90) type = EnemyType::Kamikaze;
            else if (roll > 78) type = EnemyType::Shooter;
            else if (roll > 65) type = EnemyType::Tank;
            else if (roll > 55) type = EnemyType::Sniper;
            else                type = EnemyType::Scout;
            break;
        case ZoneID::Bunker:
            // 3% PaladinCorrompido (97-99), 3% OrcCibernetico (94-96)
            if      (roll > 97) type = EnemyType::PaladinCorrompido;
            else if (roll > 94) type = EnemyType::OrcCibernetico;
            else if (roll > 91) type = EnemyType::Hydra;
            else if (roll > 82) type = EnemyType::Zergling;
            else if (roll > 77) type = EnemyType::HunterDrone;
            else if (roll > 65) type = EnemyType::Sniper;
            else if (roll > 51) type = EnemyType::Shooter;
            else if (roll > 41) type = EnemyType::Kamikaze;
            else                type = EnemyType::Scout;
            break;
        case ZoneID::KronosForge:
            // SkynetFactory: 5% UndeadEnforcer, 2% OrcCibernetico, 2% PaladinCorrompido
            if      (roll > 95) type = EnemyType::UndeadEnforcer;
            else if (roll > 93) type = EnemyType::OrcCibernetico;
            else if (roll > 91) type = EnemyType::PaladinCorrompido;
            else if (roll > 90) type = EnemyType::Broodmother;
            else if (roll > 74) type = EnemyType::Zergling;
            else if (roll > 59) type = EnemyType::Hydra;
            else if (roll > 48) type = EnemyType::MorphX;
            else if (roll > 36) type = EnemyType::HunterDrone;
            else if (roll > 24) type = EnemyType::Shooter;
            else if (roll > 12) type = EnemyType::Kamikaze;
            else                type = EnemyType::Tank;
            break;
        case ZoneID::KronosNexus:
            // CoreFacility: 8% UndeadEnforcer, 4% OrcCibernetico, 3% PaladinCorrompido
            if      (roll > 92) type = EnemyType::UndeadEnforcer;
            else if (roll > 88) type = EnemyType::OrcCibernetico;
            else if (roll > 85) type = EnemyType::PaladinCorrompido;
            else if (roll > 82) type = EnemyType::Broodmother;
            else if (roll > 65) type = EnemyType::Zergling;
            else if (roll > 49) type = EnemyType::Hydra;
            else if (roll > 37) type = EnemyType::MorphX;
            else if (roll > 26) type = EnemyType::HunterDrone;
            else if (roll > 16) type = EnemyType::KronosSentry;
            else if (roll > 8)  type = EnemyType::Sniper;
            else                type = EnemyType::Tank;
            break;

        // ── Phases Sombrias — dark zone spawn tables ────────────────────────────
        // Cada phase has your PROPRIA fauna. Antes cemetery/farm,
        // city-ghost/forest and catacomb/manor dividiam the same table: dois
        // worlds different with exatamente the same bichos.
        case ZoneID::CursedFarm:      // RURAL: horde of zumbis, almost nada etereo
            if      (roll > 96) type = EnemyType::ZombieLord;
            else if (roll > 82) type = EnemyType::ZombieRager;
            else if (roll > 74) type = EnemyType::Ghost;
            else if (roll > 44) type = EnemyType::ZombieHorde;
            else                type = EnemyType::Zombie;
            break;
        case ZoneID::Cemetery:        // MORTOS-VIVOS + assombracoes leaving of the covas
            if      (roll > 93) type = EnemyType::ZombieLord;
            else if (roll > 80) type = EnemyType::PoltergeistBoss;
            else if (roll > 62) type = EnemyType::GhostElite;
            else if (roll > 38) type = EnemyType::Ghost;
            else if (roll > 20) type = EnemyType::ZombieRager;
            else                type = EnemyType::Zombie;
            break;
        case ZoneID::DarkForest:      // ESPECTRAL: the that caca in the neblina
            if      (roll > 92) type = EnemyType::BansheeHowler;
            else if (roll > 76) type = EnemyType::ShadowWraith;
            else if (roll > 55) type = EnemyType::Ghost;
            else if (roll > 40) type = EnemyType::GhostElite;
            else if (roll > 22) type = EnemyType::Zergling;
            else                type = EnemyType::ZombieHorde;
            break;
        case ZoneID::GhostCity:       // URBANO: maquinas of KRONOS among the espectros
            if      (roll > 94) type = EnemyType::BansheeHowler;
            else if (roll > 84) type = EnemyType::ShadowWraith;
            else if (roll > 70) type = EnemyType::HunterDrone;
            else if (roll > 56) type = EnemyType::Shooter;
            else if (roll > 44) type = EnemyType::KronosSentry;
            else if (roll > 24) type = EnemyType::Ghost;
            else                type = EnemyType::Zombie;
            break;
        case ZoneID::Catacombs:       // SUBTERRANEO: dead-vivo heavy, without maquina
            if      (roll > 90) type = EnemyType::ZombieLord;
            else if (roll > 78) type = EnemyType::UndeadEnforcer;
            else if (roll > 62) type = EnemyType::ShadowWraith;
            else if (roll > 44) type = EnemyType::ZombieRager;
            else if (roll > 24) type = EnemyType::ZombieHorde;
            else                type = EnemyType::Ghost;
            break;
        case ZoneID::AbandonedManor:  // ASSOMBRACAO: poltergeist and banshee mandam
            if      (roll > 88) type = EnemyType::PoltergeistBoss;
            else if (roll > 72) type = EnemyType::BansheeHowler;
            else if (roll > 54) type = EnemyType::GhostElite;
            else if (roll > 34) type = EnemyType::ShadowWraith;
            else if (roll > 16) type = EnemyType::Ghost;
            else                type = EnemyType::ZombieRager;
            break;

        // ── Zone Inferno — aliens + zumbis in environment vulcanico ──────────────
        case ZoneID::InfernoZone:
            if      (roll > 95) type = EnemyType::AlienBoss;
            else if (roll > 89) type = EnemyType::OmegaBoss;
            else if (roll > 79) type = EnemyType::Ghost;
            else if (roll > 69) type = EnemyType::ZombieHorde;
            else if (roll > 57) type = EnemyType::ZombieRager;
            else if (roll > 44) type = EnemyType::Zombie;
            else if (roll > 33) type = EnemyType::Broodmother;
            else if (roll > 19) type = EnemyType::Hydra;
            else                type = EnemyType::Zergling;
            break;

        default:
            type = EnemyType::Scout;
            break;
    }

    Enemy& and = enemies.emplace_back(pos, type);

    // Zergling swarm — spawn 2 more in formation (StarCraft feel)
    // Done ANTES of the scaling to that all recebam the same tratamento.
    if (type == EnemyType::Zergling) {
        for (int z = 0; z < 2; ++z) {
            float za = angle + (z == 0 ? 0.25f : -0.25f);
            float zd = 360.0f + z * 30.0f;
            Vector2 zp = {
                player.position.x + std::cos(za) * zd,
                player.position.y + std::sin(za) * zd
            };
            enemies.emplace_back(zp, EnemyType::Zergling);
        }
    }

    // Aplica scaling/elite the all the Zerglings recem-criados.
    size_t firstIdx = &and - &enemies[0];
    size_t lastIdx = enemies.size();
    for (size_t idx = firstIdx; idx < lastIdx; ++idx) {
        Enemy& en = enemies[idx];

        // Scale enemy with player level — harder the you get stronger
        if (player.level > 1) {
            float scale = 1.0f + (player.level - 1) * 0.12f;
            en.health    *= scale;
            en.maxHealth *= scale;
            en.damage    *= (1.0f + (player.level - 1) * 0.08f);
            en.xpReward  = (int)(en.xpReward * scale);
        }

        // Global progression scaling — gets harder the kills and wave count rise
        {
            float gs = Enemy::getGlobalScaling(totalKills, anomalySystem.waveNumber);
            if (gs > 1.0f) {
                en.health    *= gs;
                en.maxHealth *= gs;
                en.damage    *= gs;
                en.speed     *= (1.0f + (gs - 1.0f) * 0.35f); // speed scales slower
                en.xpReward  = (int)(en.xpReward * gs);
            }
        }

        // Difficulty scaling
        {
            const DifficultySettings& diff = getDifficulty();
            en.health    *= diff.enemyHPMult;
            en.maxHealth *= diff.enemyHPMult;
            en.damage    *= diff.enemyDmgMult;
            en.speed     *= diff.enemySpeedMult;
        }

        // Gradiente by DISTANCIA of the REFUGE (world centrado in the base): cada anel
        // of 1 zone (2560px) alem of the center endurece and reward more.
        if (openWorldMode) {
            const float ZONE = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize);   // 2560
            const Vector2 wc = safeZoneCenter;   // base/hub, coracao of the phase
            float dx = en.position.x - wc.x, dy = en.position.y - wc.y;
            float ring = std::max(0.0f, (sqrtf(dx*dx + dy*dy) - ZONE) / ZONE);
            if (ring > 0.0f) {
                float hpMul = 1.0f + ring * 0.22f, dmgMul = 1.0f + ring * 0.18f;
                en.health *= hpMul; en.maxHealth *= hpMul; en.damage *= dmgMul;
                en.xpReward = (int)(en.xpReward * (1.0f + ring * 0.20f));
            }
        }

        // Infinite Evolution Engine — Threat Level + mutador active
        {
            float ts = threatStatMult();
            en.health    *= ts * mutatorHPMult();
            en.maxHealth *= ts * mutatorHPMult();
            en.damage    *= ts * mutatorDmgMult();
            en.speed     *= mutatorSpeedMult();
            en.xpReward   = (int)(en.xpReward * ts);
        }

        // Curva by PHASE (world open): the mare inimiga endurece the cada world.
        // Phase 1 (ruins) = normal; worlds seguintes +8% HP and +5% damage by phase.
        if (openWorldMode && owPhase > 0) {
            float phMul = 1.0f + owPhase * 0.08f;
            en.health    *= phMul;
            en.maxHealth *= phMul;
            en.damage    *= (1.0f + owPhase * 0.05f);
        }

        // Chance of ELITE scale with the phase and the radius of the refuge (estilo Risk of Rain):
        // base 12% + 3% by phase + 5% by anel, ceiling 45%.
        if (type != EnemyType::Boss && type != EnemyType::Zergling) {
            int pct = 12;
            if (openWorldMode) {
                const float ZONE = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize);
                float dx = en.position.x - safeZoneCenter.x, dy = en.position.y - safeZoneCenter.y;
                float ring = std::max(0.0f, (sqrtf(dx*dx + dy*dy) - ZONE) / ZONE);
                pct += (int)owPhase * 3 + (int)ring * 5;
            }
            pct = std::min(45, pct);
            if (GetRandomValue(0, 99) < pct)
                en.makeElite(GetRandomValue(0, 4));
            // Armored elite stays more blindado conforme the world (8 + 4 by phase).
            if (en.isElite && en.eliteMod == 1)
                en.armor = 8.0f + (float)owPhase * 4.0f;
        }
    }
}

void Game::spawnBoss() {
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    Vector2 pos = {
        player.position.x + std::cos(angle) * 420.0f,
        player.position.y + std::sin(angle) * 420.0f
    };
    for (int i = 0; i < 10 && tilemap.isWallAtPosition(pos); ++i) {
        angle = GetRandomValue(0, 360) * DEG2RAD;
        pos.x = player.position.x + std::cos(angle) * 420.0f;
        pos.y = player.position.y + std::sin(angle) * 420.0f;
    }
    clampInsideOpenWorldBounds(pos, 120.0f);
    enemies.emplace_back(pos, EnemyType::Boss);
    {
        Enemy& boss = enemies.back();
        const DifficultySettings& diff = getDifficulty();
        boss.health    *= diff.enemyHPMult * diff.bossHPMult;
        boss.maxHealth *= diff.enemyHPMult * diff.bossHPMult;
        boss.damage    *= diff.enemyDmgMult;
        boss.speed     *= diff.enemySpeedMult;
    }
}

// ─── Companion System ─────────────────────────────────────────────────────────

void Game::spawnCompanion(CompanionType t) {
    // Spawn slightly offset from player
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    Vector2 spawnPos = {
        player.position.x + std::cos(angle) * 60.f,
        player.position.y + std::sin(angle) * 60.f
    };
    companions.emplace_back(t);
    companions.back().position = spawnPos;
}

void Game::updateCompanions(float dt) {
    // Build the list of enemy raw pointers for companion AI
    std::vector<Enemy*> enemyPtrs;
    enemyPtrs.reserve(enemies.size());
    for (auto& and : enemies) {
        if (!and.isDead()) enemyPtrs.push_back(&and);
    }

    for (auto& c : companions) {
        if (!c.active) continue;
        c.update(dt, player.position, enemyPtrs);

        // Handle companion projectile requests (KyleReese)
        if (c.wantsToShoot && !c.isDead()) {
            projectiles.emplace_back(c.position, c.shootDir,
                c.shootDamage, c.shootRange,
                c.shootSpeed,  c.projectileColor);
        }

        // Handle companion AoE stomp (T800Ally skill)
        if (c.wantsAoE && !c.isDead()) {
            particles.spawnExplosion(c.position, {0, 180, 255, 255}, 20);
            triggerShake(4.f, 0.15f);
            for (auto& and : enemies) {
                if (and.isDead()) continue;
                if (Vector2Distance(c.position, and.position) <= c.aoeRadius) {
                    and.takeDamage(c.aoeDamage);
                    particles.spawnHit(and.position, {0, 200, 255, 255}, 5);
                }
            }
        }

        // ── Healer: healing continuous while the player is in the radius ──────────────
        // O Companion only SINALIZA (wantsHeal/healAmount); aplicar and of the Game — and isso
        // never era read, the that deixava Healer and LootDrone completamente inertes.
        if (c.wantsHeal && !c.isDead() && c.healAmount > 0.0f) {
            float before = player.health;
            player.heal(c.healAmount);
            audio.playHeal();
            companionHealAccum += player.health - before;
            if (companionHealAccum >= 5.0f) {   // 1 number the cada ~5 HP (not 1 by frame)
                damageNumbers.push_back({player.position, companionHealAccum,
                                         {0,210,80,255}, 1.0f, "+"});
                companionHealAccum = 0.0f;
            }
        }

        // ── LootDrone: puxa items/orbes proximos DELE until the player ────────────
        // Not coleta direct: empurra to inside the radius of the player and the pickup normal
        // (checkCollisions) resolve — without duplicar the logica of cada type of item.
        if (c.wantsCollect && !c.isDead()) {
            const float PULL = 460.0f;
            for (auto& it : items) {
                if (it.pickedUp || it.pickupDelay > 0.0f) continue;
                if (Vector2Distance(c.position, it.position) > c.collectRadius) continue;
                Vector2 d = Vector2Normalize(Vector2Subtract(player.position, it.position));
                it.position.x += d.x * PULL * dt;
                it.position.y += d.y * PULL * dt;
            }
            for (auto& the : xpOrbs) {
                if (the.pickedUp) continue;
                if (Vector2Distance(c.position, the.position) > c.collectRadius) continue;
                Vector2 d = Vector2Normalize(Vector2Subtract(player.position, the.position));
                the.position.x += d.x * PULL * dt;
                the.position.y += d.y * PULL * dt;
            }
        }

        // Damage companions from enemies (melee contact)
        for (auto& and : enemies) {
            if (and.isDead() || c.isDead()) continue;
            float dist = Vector2Distance(c.position, and.position);
            if (dist <= c.radius + and.radius) {
                // Enemy deals contact damage to companion
                float contactDmg = and.damage * dt * 2.f;
                c.takeDamage(contactDmg);
            }
        }

        // Damage companions from enemy projectiles
        for (auto& ep : enemyProjectiles) {
            if (!ep.active || c.isDead()) continue;
            if (Vector2Distance(c.position, ep.position) <= c.radius + 5.f) {
                c.takeDamage(ep.damage);
                ep.active = false;
            }
        }
    }
}

void Game::drawCompanions() const {
    for (const auto& c : companions) {
        if (c.active) c.render();
    }
}

void Game::spawnOmegaBoss() {
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    Vector2 pos = {
        player.position.x + std::cos(angle) * 480.0f,
        player.position.y + std::sin(angle) * 480.0f
    };
    for (int i = 0; i < 12 && tilemap.isWallAtPosition(pos); ++i) {
        angle = GetRandomValue(0, 360) * DEG2RAD;
        pos.x = player.position.x + std::cos(angle) * 480.0f;
        pos.y = player.position.y + std::sin(angle) * 480.0f;
    }
    clampInsideOpenWorldBounds(pos, 120.0f);
    Enemy omega(pos, EnemyType::OmegaBoss);
    // Scale with player level
    float lvlScale = 1.0f + (player.level - 1) * 0.2f;
    omega.health    *= lvlScale;
    omega.maxHealth *= lvlScale;
    omega.damage    *= (1.0f + (player.level - 1) * 0.12f);
    // Difficulty scaling for omega boss
    {
        const DifficultySettings& diff = getDifficulty();
        omega.health    *= diff.enemyHPMult * diff.bossHPMult;
        omega.maxHealth *= diff.enemyHPMult * diff.bossHPMult;
        omega.damage    *= diff.enemyDmgMult;
        omega.speed     *= diff.enemySpeedMult;
    }
    enemies.push_back(omega);
    showStoryBanner("!! OMEGA BOSS !!", "Uma ameaca of level extincao detectada. BOA SORTE.", 4.0f);
    triggerPlayerSpeech("PERIGO EXTREMO. Protocolo of sobrevivencia enabled!", 4.5f);
    audio.playBossRoar();
    triggerShake(12.0f, 0.5f);
}

void Game::spawnFinalBoss() {
    // NUCLEO KRONOS — the final boss. Your death vence the game.
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    Vector2 pos = {
        player.position.x + std::cos(angle) * 520.0f,
        player.position.y + std::sin(angle) * 520.0f
    };
    for (int i = 0; i < 12 && tilemap.isWallAtPosition(pos); ++i) {
        angle = GetRandomValue(0, 360) * DEG2RAD;
        pos.x = player.position.x + std::cos(angle) * 520.0f;
        pos.y = player.position.y + std::sin(angle) * 520.0f;
    }
    clampInsideOpenWorldBounds(pos, 120.0f);
    Enemy core(pos, EnemyType::OmegaBoss);
    core.isFinalBoss = true;
    // Muito more strong that the OmegaBoss normal — and the climax of the game
    float lvlScale = 1.0f + (player.level - 1) * 0.25f;
    core.health    *= lvlScale * 3.0f;
    core.maxHealth *= lvlScale * 3.0f;
    core.damage    *= (1.0f + (player.level - 1) * 0.14f) * 1.4f;
    {
        const DifficultySettings& diff = getDifficulty();
        core.health    *= diff.enemyHPMult * diff.bossHPMult;
        core.maxHealth *= diff.enemyHPMult * diff.bossHPMult;
        core.damage    *= diff.enemyDmgMult;
        core.speed     *= diff.enemySpeedMult;
    }
    enemies.push_back(core);
    finalBossSpawned = true;
    finalBossAlive   = true;
    showStoryBanner("== NUCLEO KRONOS ==", "O coracao of the IA. Destrua-the and liberte the humanidade.", 5.0f);
    triggerPlayerSpeech("KRONOS... and here that tudo ends. Por all in the!", 5.0f);
    audio.playBossRoar();
    triggerShake(16.0f, 0.7f);
}

void Game::drawVictoryScreen() const {
    // Fundo dark with glow golden pulsante
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.86f));
    float t     = victoryTimer;
    float pulse = 0.6f + 0.4f * sinf(t * 2.0f);

    // Lightnings of light dourada leaving of the center
    int cx = screenWidth / 2, cy = screenHeight / 2;
    for (int i = 0; i < 24; i++) {
        float the  = i * (PI / 12.0f) + t * 0.3f;
        float len = 400.0f + 120.0f * sinf(t * 1.5f + i);
        Vector2 and = {cx + cosf(the) * len, cy + sinf(the) * len};
        DrawLineEx({(float)cx, (float)cy}, and, 2.0f,
                   ColorAlpha({255, 215, 80, 255}, 0.06f * pulse));
    }

    // Titulo
    const char* title = "VICTORY";
    int tw = MeasureText(title, 80);
    DrawText(title, cx - tw/2 + 3, cy - 120 + 3, 80, ColorAlpha(BLACK, 0.7f));
    DrawText(title, cx - tw/2,     cy - 120,     80, ColorAlpha({255, 215, 80, 255}, pulse));

    // Subtitulo
    const char* sub = "O NUCLEO KRONOS FOI DESTRUIDO";
    int sw = MeasureText(sub, 24);
    DrawText(sub, cx - sw/2, cy - 20, 24, {220, 220, 255, 255});

    const char* sub2 = "A humanidade is livre. You won DARKNET.";
    int sw2 = MeasureText(sub2, 18);
    DrawText(sub2, cx - sw2/2, cy + 14, 18, ColorAlpha(WHITE, 0.8f));

    // Estatisticas finais
    int sy = cy + 60;
    const char* stats[] = {
        TextFormat("Level alcancado: %d", player.level),
        TextFormat("Enemies eliminados: %d", totalKills),
        TextFormat("Credits acumulados: %d", player.credits),
    };
    for (int i = 0; i < 3; i++) {
        int w = MeasureText(stats[i], 16);
        DrawText(stats[i], cx - w/2, sy + i*24, 16, ColorAlpha({180, 220, 255, 255}, 0.9f));
    }

    // Prompt to continue
    if (victoryTimer > 2.0f && ((int)(t * 2) % 2 == 0)) {
        const char* prompt = "[ENTER] return to the menu";
        int pw = MeasureText(prompt, 18);
        DrawText(prompt, cx - pw/2, sy + 90, 18, ColorAlpha({255, 215, 80, 255}, 0.9f));
    }
}
