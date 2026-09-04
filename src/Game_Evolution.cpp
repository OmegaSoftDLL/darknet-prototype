// Game_Evolution.cpp — motor de evolucao: mutadores de mundo e som de morte de inimigo.
// Extraido de Game.cpp. Mesma classe Game.
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
        case WorldMutator::SwiftEnemies:  return "FRENESI VELOZ";
        case WorldMutator::ArmoredEnemies:return "BLINDAGEM PESADA";
        case WorldMutator::BloodMoon:     return "LUA DE SANGUE";
        case WorldMutator::LootRain:      return "CHUVA DE ESPOLIO";
        case WorldMutator::Frenzy:        return "INVASAO TOTAL";
        case WorldMutator::Berserk:       return "FUROR KRONOS";
        default:                          return "";
    }
}

const char* Game::mutatorDesc(WorldMutator m) const {
    switch (m) {
        case WorldMutator::SwiftEnemies:  return "Inimigos +35% velocidade. Reaja rapido!";
        case WorldMutator::ArmoredEnemies:return "Inimigos +60% vida. Traga poder de fogo.";
        case WorldMutator::BloodMoon:     return "Inimigos se curam ao te atingir.";
        case WorldMutator::LootRain:      return "Espolio +150%. Cace tudo agora!";
        case WorldMutator::Frenzy:        return "Inimigos surgem em dobro.";
        case WorldMutator::Berserk:       return "Inimigos +40% dano. Cuidado extremo.";
        default:                          return "";
    }
}

void Game::rollNewMutator() {
    // Escolhe um mutador diferente do atual (variedade garantida)
    int n = (int)WorldMutator::COUNT - 1; // exclui None
    WorldMutator next = activeMutator;
    for (int tries = 0; tries < 8 && next == activeMutator; ++tries)
        next = (WorldMutator)(1 + GetRandomValue(0, n - 1));
    activeMutator = next;
    mutatorTimer  = 0.0f;
    showStoryBanner(TextFormat("MUTADOR: %s", mutatorName(activeMutator)),
                    mutatorDesc(activeMutator), 4.0f);
    triggerPlayerSpeech("As regras mudaram. Adapte-se.", 3.0f);
}

void Game::updateEvolutionEngine(float dt) {
    if (inSafeZone(player.position)) return; // a base nao escala (refugio)

    // ── Nivel de Ameaca: sobe por TEMPO ou por KILLS — o que vier primeiro ────
    threatTimer += dt;
    bool levelByTime  = threatTimer >= 100.0f;
    bool levelByKills = (totalKills - threatKillMark) >= 40;
    if (levelByTime || levelByKills) {
        threatLevel++;
        threatTimer    = 0.0f;
        threatKillMark = totalKills;
        // Recompensa de marco + anuncio de novidade
        int bonus = 50 * threatLevel;
        player.credits += bonus;
        showStoryBanner(TextFormat("NIVEL DE AMEACA %d", threatLevel),
            TextFormat("KRONOS escala. Inimigos +%.0f%% mais fortes. Bonus: $%d",
                       (threatStatMult()-1.0f)*100.0f, bonus), 4.0f);
        triggerPlayerSpeech("O KRONOS esta evoluindo. Eu tambem vou.", 3.0f);
        audio.playLevelUp();
    }

    // ── Mutadores rotativos: muda o "sabor" do mundo periodicamente ───────────
    mutatorTimer += dt;
    if (activeMutator == WorldMutator::None) {
        // primeiro mutador comeca apos ~60s de jogo
        if (sessionTime > 60.0f) rollNewMutator();
    } else if (mutatorTimer >= mutatorDuration) {
        rollNewMutator();
    }
}

void Game::playEnemyDeathSound(const Enemy& e) {
    // Som de morte por FACCAO/tipo do inimigo
    using ET = EnemyType;
    if (e.isFinalBoss || e.type==ET::Boss || e.type==ET::AlienBoss || e.type==ET::OmegaBoss ||
        e.type==ET::VoidColossus || e.type==ET::FrostWyrm || e.type==ET::InfernoHerald ||
        e.type==ET::VolcanicTitan || e.type==ET::Leviathan || e.type==ET::ZombieLord ||
        e.type==ET::PoltergeistBoss || e.type==ET::Broodmother) {
        audio.playBossRoar();                       // chefes
    } else if (e.type==ET::Zergling || e.type==ET::Hydra || e.type==ET::CorrupterDrone ||
               e.type==ET::AcidSpitter || e.type==ET::NeuralParasite || e.type==ET::AbyssalEel ||
               e.type==ET::MorphX || e.type==ET::ChaosSpawn) {
        audio.playAlienScream();                    // aliens/orgânicos
    } else if (e.type==ET::Ghost || e.type==ET::GhostElite || e.type==ET::ShadowWraith ||
               e.type==ET::BansheeHowler || e.type==ET::GhostSniper || e.type==ET::VoidStalker ||
               e.type==ET::DarkMatter || e.type==ET::SoulReaper) {
        audio.playGhostWail();                      // fantasmas/sombras
    } else if (e.type==ET::Zombie || e.type==ET::ZombieRager || e.type==ET::ZombieHorde ||
               e.type==ET::UndeadEnforcer || e.type==ET::Necromancer || e.type==ET::PlagueDoctor) {
        audio.playGhostWail();                      // mortos-vivos (gemido)
    } else if (e.type==ET::MoltenGolem || e.type==ET::CrimsonBat || e.type==ET::LichKnight ||
               e.type==ET::DemonHunter || e.type==ET::BloodBerserker) {
        audio.playExplosion(false);                 // infernais/demônios
    } else {
        audio.playEnemyDeath(false);                // robôs/mechs/padrão (mecânico)
    }
}
