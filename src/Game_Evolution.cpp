// Game_Evolution.cpp — evolution engine: world mutators and enemy death sounds.
// Extracted from Game.cpp. Same class Game.
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

const char* Game::mutatorName(WorldMutator m) const {
    switch (m) {
        case WorldMutator::SwiftEnemies:  return "SWIFT FRENZY";
        case WorldMutator::ArmoredEnemies:return "HEAVY ARMOR";
        case WorldMutator::BloodMoon:     return "BLOOD MOON";
        case WorldMutator::LootRain:      return "LOOT RAIN";
        case WorldMutator::Frenzy:        return "TOTAL INVASION";
        case WorldMutator::Berserk:       return "KRONOS FURY";
        default:                          return "";
    }
}

const char* Game::mutatorDesc(WorldMutator m) const {
    switch (m) {
        case WorldMutator::SwiftEnemies:  return "Enemies +35% speed. React fast!";
        case WorldMutator::ArmoredEnemies:return "Enemies +60% health. Bring firepower.";
        case WorldMutator::BloodMoon:     return "Enemies heal when they hit you.";
        case WorldMutator::LootRain:      return "Loot +150%. Hunt everything now!";
        case WorldMutator::Frenzy:        return "Enemies spawn in double numbers.";
        case WorldMutator::Berserk:       return "Enemies +40% damage. Extreme caution.";
        default:                          return "";
    }
}

void Game::rollNewMutator() {
    // Picks the mutator different from the current one (variety guaranteed)
    int n = (int)WorldMutator::COUNT - 1; // excludes None
    WorldMutator next = activeMutator;
    for (int tries = 0; tries < 8 && next == activeMutator; ++tries)
        next = (WorldMutator)(1 + GetRandomValue(0, n - 1));
    activeMutator = next;
    mutatorTimer  = 0.0f;
    showStoryBanner(TextFormat("MUTATOR: %s", mutatorName(activeMutator)),
                    mutatorDesc(activeMutator), 4.0f);
    triggerPlayerSpeech("The rules have changed. Adapt.", 3.0f);
}

void Game::updateEvolutionEngine(float dt) {
    if (inSafeZone(player.position)) return; // the base doesn't scale (refuge)

    // ── Threat Level: rises by TIME or by KILLS — whichever comes first ────
    threatTimer += dt;
    bool levelByTime  = threatTimer >= 100.0f;
    bool levelByKills = (totalKills - threatKillMark) >= 40;
    if (levelByTime || levelByKills) {
        threatLevel++;
        threatTimer    = 0.0f;
        threatKillMark = totalKills;
        // Milestone reward + novelty announcement
        int bonus = 50 * threatLevel;
        player.credits += bonus;
        totalCreditsEarned += bonus;
        achievements.onCreditsEarned(totalCreditsEarned);
        showStoryBanner(TextFormat("THREAT LESPEED %d", threatLevel),
            TextFormat("KRONOS scales. Enemies +%.0f%% stronger. Bonus: $%d",
                       (threatStatMult()-1.0f)*100.0f, bonus), 4.0f);
        triggerPlayerSpeech("KRONOS is evolving. So will I.", 3.0f);
        audio.playLevelUp();
    }

    // ── Rotating mutators: changes the "flavor" of the world periodically ───────────
    mutatorTimer += dt;
    if (activeMutator == WorldMutator::None) {
        // first mutator starts after ~60s of gameplay
        if (sessionTime > 60.0f) rollNewMutator();
    } else if (mutatorTimer >= mutatorDuration) {
        rollNewMutator();
    }
}

void Game::playEnemyDeathSound(const Enemy& and) {
    // Death sound by enemy FACTION/type
    using ET = EnemyType;
    if (and.isFinalBoss || and.type==ET::Boss || and.type==ET::AlienBoss || and.type==ET::OmegaBoss ||
        and.type==ET::VoidColossus || and.type==ET::FrostWyrm || and.type==ET::InfernoHerald ||
        and.type==ET::VolcanicTitan || and.type==ET::Leviathan || and.type==ET::ZombieLord ||
        and.type==ET::PoltergeistBoss || and.type==ET::Broodmother) {
        audio.playBossRoar();                       // bosses
    } else if (and.type==ET::Zergling || and.type==ET::Hydra || and.type==ET::CorrupterDrone ||
               and.type==ET::AcidSpitter || and.type==ET::NeuralParasite || and.type==ET::AbyssalEel ||
               and.type==ET::MorphX || and.type==ET::ChaosSpawn) {
        audio.playAlienScream();                    // aliens/organics
    } else if (and.type==ET::Ghost || and.type==ET::GhostElite || and.type==ET::ShadowWraith ||
               and.type==ET::BansheeHowler || and.type==ET::GhostSniper || and.type==ET::VoidStalker ||
               and.type==ET::DarkMatter || and.type==ET::SoulReaper) {
        audio.playGhostWail();                      // ghosts/shadows
    } else if (and.type==ET::Zombie || and.type==ET::ZombieRager || and.type==ET::ZombieHorde ||
               and.type==ET::UndeadEnforcer || and.type==ET::Necromancer || and.type==ET::PlagueDoctor) {
        audio.playGhostWail();                      // undead (wail)
    } else if (and.type==ET::MoltenGolem || and.type==ET::CrimsonBat || and.type==ET::LichKnight ||
               and.type==ET::DemonHunter || and.type==ET::BloodBerserker) {
        audio.playExplosion(false);                 // infernals/demons
    } else {
        audio.playEnemyDeath(false);                // robots/mechs/default (mechanical)
    }
}
