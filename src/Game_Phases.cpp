// Game_Phases.cpp — phases of the world open: table of phases, portal and transition of zone.
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

// ─── Zone Transition ─────────────────────────────────────────────────────────

void Game::checkPortalTransition() {
    ZoneID dest;
    if (tilemap.isPortalAtPosition(player.position, dest)) {
        transitionToZone(dest);
    }
}

// Reads content/phases.txt: "zone | kills | boss | radius | title". Formato of
// line simple of purpose: without dependency of JSON and editavel by qualquer um.
void Game::loadPhaseDefs() {
    phaseDefs.clear();
    static const struct { const char* n; ZoneID z; } NAMES[] = {
        {"LARuins", ZoneID::LARuins}, {"CursedFarm", ZoneID::CursedFarm},
        {"DarkForest", ZoneID::DarkForest}, {"Cemetery", ZoneID::Cemetery},
        {"GhostCity", ZoneID::GhostCity}, {"Bunker", ZoneID::Bunker},
        {"Catacombs", ZoneID::Catacombs}, {"AbandonedManor", ZoneID::AbandonedManor},
        {"KronosForge", ZoneID::KronosForge}, {"InfernoZone", ZoneID::InfernoZone},
        {"KronosNexus", ZoneID::KronosNexus},
    };
    const char* path = FileExists("content/phases.txt") ? "content/phases.txt"
                     : (FileExists("../../content/phases.txt") ? "../../content/phases.txt" : nullptr);
    if (path) {
        char* txt = LoadFileText(path);
        if (txt) {
            std::stringstream ss(txt);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.empty() || line[0] == '#') continue;
                std::vector<std::string> col;
                size_t start = 0;
                while (true) {
                    size_t bar = line.find('|', start);
                    std::string part = (bar == std::string::npos) ? line.substr(start)
                                                                  : line.substr(start, bar - start);
                    while (!part.empty() && (part.front() == ' ' || part.front() == '\t')) part.erase(part.begin());
                    while (!part.empty() && (part.back() == ' ' || part.back() == '\r' || part.back() == '\t')) part.pop_back();
                    col.push_back(part);
                    if (bar == std::string::npos) break;
                    start = bar + 1;
                }
                if (col.size() < 4) continue;
                PhaseDef d;
                d.zone = ZoneID::LARuins;
                for (const auto& nz : NAMES) if (col[0] == nz.n) { d.zone = nz.z; break; }
                d.goal   = atoi(col[1].c_str());
                d.boss   = atoi(col[2].c_str()) != 0;
                d.radius = (float)atof(col[3].c_str());
                d.title  = (col.size() > 4) ? col[4] : getZoneInfo(d.zone).name;
                if (d.goal   < 1)     d.goal   = 1;
                if (d.radius < 900.0f) d.radius = 900.0f;   // minimum playable phase
                phaseDefs.push_back(d);
            }
            UnloadFileText(txt);
        }
    }
    if (phaseDefs.empty()) {
        TraceLog(LOG_WARNING, "PHASES: content/phases.txt missing/empty - using embedded table");
        for (const auto& nz : NAMES) {
            PhaseDef d; d.zone = nz.z; d.title = getZoneInfo(nz.z).name;
            d.goal = 20 + (int)phaseDefs.size() * 8;
            d.boss = ((int)phaseDefs.size() + 1) % 3 == 0;
            d.radius = 3000.0f + phaseDefs.size() * 160.0f;
            phaseDefs.push_back(d);
        }
    }
    TraceLog(LOG_INFO, "PHASES: %d loaded", (int)phaseDefs.size());
}

const Game::PhaseDef& Game::phaseDef(int phase) const {
    static PhaseDef fallback;
    if (phaseDefs.empty()) return fallback;
    if (phase < 0) phase = 0;
    return phaseDefs[phase % (int)phaseDefs.size()];
}

// ── Open-world PHASES ───────────────────────────────────────────────────────
ZoneID Game::phaseZone(int phase) {   // compat: order now comes from the table
    static const ZoneID ORDER[] = {
        ZoneID::LARuins, ZoneID::CursedFarm, ZoneID::DarkForest, ZoneID::Cemetery,
        ZoneID::GhostCity, ZoneID::Bunker, ZoneID::Catacombs, ZoneID::AbandonedManor,
        ZoneID::KronosForge, ZoneID::InfernoZone, ZoneID::KronosNexus
    };
    const int N = (int)(sizeof(ORDER) / sizeof(ORDER[0]));
    if (phase < 0) phase = 0;
    return ORDER[phase % N];
}

void Game::updatePhasePortal(float dt) {
    if (!openWorldMode) return;
    if (owFadeTimer > 0.0f) { owFadeTimer -= dt; return; }

    owPhaseKills = enemiesKilled - owKillsAtStart;
    // Phase of boss: kill the cota NOT basta, the boss has that fall. E the that closes
    // the phase as uma phase, with clima and desfecho, instead of uma cota of kills.
    if (owBossPhase && !owBossDown && bossSpawned) {
        bool alive = false;
        for (const auto& and : enemies) if (and.isBoss() && !and.isDead()) { alive = true; break; }
        if (!alive) {
            owBossDown = true;
            showStoryBanner("BOSS ABATIDO", "O path for the next world is livre.", 4.0f);
        }
    }
    bool goalMet = (owPhaseKills >= owPhaseGoal) && (!owBossPhase || owBossDown);
    if (!owPortalOpen && goalMet) {
        owPortalOpen = true;
        tutorial.onPortalFound();
        // portal nasce near the refuge, always in the same rumo (the player finds)
        owPortalPos = { safeZoneCenter.x + 620.0f, safeZoneCenter.y - 520.0f };
        // Guarantee intencional (before era acidental): the point of the portal has that
        // be alcancavel. O test cobre the DISCO of interacao (radius ~105u), not
        // only the point central — um portal with the center livre and the edge murada
        // continuous inalcancavel.
        auto diskBlocked = [this](Vector2 p) {
            if (isBlocked(p)) return true;
            for (int s = 0; s < 8; ++s) {
                float the = s * 0.7853982f;   // amostra the cada 45 graus of the edge
                if (isBlocked({ p.x + std::cos(the) * 100.0f,
                                p.y + std::sin(the) * 100.0f })) return true;
            }
            return false;
        };
        if (diskBlocked(owPortalPos)) {
            TraceLog(LOG_WARNING, "PORTAL: point (%.0f,%.0f) bloqueado - realocando",
                     owPortalPos.x, owPortalPos.y);
            // Aneis crescentes x more angles: cobre well more that the anel only
            // of 700u of the versao previous.
            bool found = false;
            for (int ring = 0; ring < 3 && !found; ++ring) {
                float rad = 700.0f + ring * 200.0f;          // 700 / 900 / 1100
                for (int t = 0; t < 16 && !found; ++t) {
                    float the = t * 0.3926991f;                // 22,5 graus by candidato
                    Vector2 c = { safeZoneCenter.x + std::cos(the) * rad,
                                  safeZoneCenter.y + std::sin(the) * rad };
                    if (!diskBlocked(c)) { owPortalPos = c; found = true; }
                }
            }
            if (!found) {
                // Espiral inside the phase: last attempt of find QUALQUER
                // point with the disco livre before the fallback destrutivo.
                float lim = owPhaseRadius - 200.0f;
                for (int t = 0; t < 220 && !found; ++t) {
                    float the = t * 0.9f, rad = 120.0f + t * 9.0f;   // ~120..2100u
                    if (rad > lim) break;
                    Vector2 c = { safeZoneCenter.x + std::cos(the) * rad,
                                  safeZoneCenter.y + std::sin(the) * rad };
                    if (!diskBlocked(c)) { owPortalPos = c; found = true; }
                }
            }
            if (!found) {
                // NUNCA manter position bloqueada in silencio: the portal goes to the
                // refuge (always alcancavel) and the solidos in returns sao limpos.
                owPortalPos = safeZoneCenter;
                clearBlockingAt(owPortalPos, 140.0f);
                TraceLog(LOG_WARNING, "PORTAL: none point livre - solidos limpos in the refuge");
            }
        }
        showStoryBanner("PORTAL ABERTO", "Va until the portal to advance of world.", 5.0f);
        audio.playLevelUp();
    }
    if (owPortalOpen &&
        Vector2Distance(player.position, owPortalPos) < 105.0f &&
        (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) ||
         (botController.active && botWantsPortal)))
        advanceOpenWorldPhase();
}

void Game::advanceOpenWorldPhase() {
    owPhase++;
    // O cabecalho "CAP.N" acompanhava only the system ANTIGO of zones
    // (transitionToZone) and ficava congelado in CAP.1 in the world open.
    storyChapter = owPhase + 1;
    // Metrica of the bot: "Advanced zones" of the report account the avanco REAL of phase
    // (before only contava encostar num portal of the tilemap — system old, empty in the OW).
    if (botController.active) {
        botController.zonesVisited++;
        botController.addLog(TextFormat("ZONE ADVANCED! phase=%d total=%d",
                                        owPhase + 1, botController.zonesVisited));
    }
    const PhaseDef& pd = phaseDef(owPhase);
    ZoneID dest = pd.zone;
    currentZone         = dest;
    currentRegion       = dest;
    tilemap.currentZone = dest;

    // clears the old world entirely (otherwise sobra enemy/loot/building ghost)
    enemies.clear(); items.clear(); projectiles.clear(); enemyProjectiles.clear();
    xpOrbs.clear(); groundEquips.clear(); damageNumbers.clear();
    dialogOpen = false; nearNpcIndex = -1;
    bossSpawned = false; spawnTimer = 0.0f;

    // Parameters of the NOVA phase ANTES of reconstruir the scenario: buildOpenWorldScenery
    // limita the world pela barrier (owPhaseRadius) — after the structure, the phase
    // new nasceu varias vezes with the radius of the phase previous.
    owPortalOpen   = false;
    owKillsAtStart = enemiesKilled;
    owPhaseGoal    = pd.goal;       // tudo comes of content/phases.txt
    owPhaseRadius  = pd.radius;
    owBossPhase    = pd.boss;
    owBossDown     = false;

    tilemap.generateOpenWorld();
    // generateOpenWorld() reseta currentZone p/ LARuins (init of the campaign). Sem
    // regravar the biome of the phase AQUI, the piso, the streets and the biomeAtWorld ficavam
    // presos in LA in TODA phase 2+ — and the scenario by region seguia um biome
    // different of the floor embaixo dele.
    tilemap.currentZone = dest;
    setupWorldRegions();
    buildOpenWorldScenery();
    setupZoneNPCs(dest);
    audio.setZone(dest);
    spawnInterval = getZoneInfo(dest).spawnInterval / getDifficulty().spawnRateMult;

    player.position = safeZoneCenter;

    // REWARD of phase: closing the world has to be worth something, otherwise the
    // portal and only um corredor. Healing cheia + credits + uma peca of equipment.
    player.health   = player.maxHealth;
    int bonus       = 250 + owPhase * 150;
    player.credits += bonus;
    totalCreditsEarned += bonus;
    achievements.onCreditsEarned(totalCreditsEarned);
    player.addXP(200 + owPhase * 120);
    {
        Equipment drop = EDB::randomForTier(1 + owPhase / 2);
        if (!drop.isEmpty()) {
            player.equipBag.push_back(drop);
            triggerPlayerSpeech(TextFormat("Reward: %s", drop.name.c_str()), 3.5f);
        }
    }
    damageNumbers.push_back({ player.position, (float)bonus, {255,210,0,255}, 1.6f, "$" });

    ZoneInfo zi = getZoneInfo(dest);
    owFadeText  = TextFormat("PHASE %d  -  %s", owPhase + 1,
                             pd.title.empty() ? zi.name.c_str() : pd.title.c_str());
    owFadeTimer = 3.0f;
}

// Screen of transition: black entering/leaving with the nome of the phase. Sem isto the troca of
// world era um corte dry and not lia as progress.
void Game::drawPhaseFade() const {
    if (owFadeTimer <= 0.0f) return;
    float t = owFadeTimer / 2.6f;                       // 1 -> 0
    float the = (t > 0.5f) ? (t - 0.5f) * 2.0f : t * 2.0f; // goes up and goes down
    the = 0.30f + the * 0.70f;
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, the));
    // Painel condensado p/ the text not be engolido pelas phases escuras
    const char* obj = owBossPhase
        ? TextFormat("OBJECTIVE: %d kills and defeat the BOSS", owPhaseGoal)
        : TextFormat("OBJECTIVE: %d kills to open the portal", owPhaseGoal);
    int tw = MeasureText(owFadeText.c_str(), 36);
    int ow2 = MeasureText(obj, 16);
    int panW = (tw > ow2 ? tw : ow2) + 56;
    int panX = screenWidth/2 - panW/2;
    int panY = screenHeight/2 - 52;
    float pa = 0.35f + the * 0.55f;
    DrawRectangle(panX, panY, panW, 104, ColorAlpha(BLACK, pa));
    DrawRectangleLinesEx({(float)panX, (float)panY, (float)panW, 104.0f},
                         1.5f, ColorAlpha(Color{0,200,255,255}, 0.4f + the * 0.3f));
    float ta = 0.7f + the * 0.3f;                 // text never stays below of ~70%
    DrawText(owFadeText.c_str(), screenWidth/2 - tw/2 + 2, panY + 5, 36,
             ColorAlpha(BLACK, ta));
    DrawText(owFadeText.c_str(), screenWidth/2 - tw/2, panY + 3, 36,
             ColorAlpha(Color{0,230,255,255}, ta));
    DrawText(obj, screenWidth/2 - ow2/2 + 1, panY + 82, 16,
             ColorAlpha(BLACK, ta));
    DrawText(obj, screenWidth/2 - ow2/2, panY + 81, 16,
             ColorAlpha(WHITE, ta));
}

void Game::transitionToZone(ZoneID dest) {
    currentZone = dest;
    enemies.clear();
    items.clear();
    projectiles.clear();
    enemyProjectiles.clear();
    xpOrbs.clear();
    groundEquips.clear();      // evita drops orfaos of the zone previous
    damageNumbers.clear();
    dialogOpen   = false;
    nearNpcIndex = -1;         // invalid indice before reconstruir the NPCs
    particles.spawnExplosion(player.position, SKYBLUE, 20);

    tilemap.generate(dest);
    setupZoneNPCs(dest);
    background.generate(dest, tilemap.width, tilemap.height, Tilemap::tileSize);

    // Inferno zone — reset when leaving, generate when entering
    if (dest == ZoneID::InfernoZone) {
        unsigned int seed = worldSeed ? (worldSeed * 0x9e3779b9u + (unsigned int)dest * 7919u)
                                      : (unsigned int)GetRandomValue(1000, 99999);
        infernoZone.generate(tilemap.width * Tilemap::tileSize,
                             tilemap.height * Tilemap::tileSize, seed);
    } else {
        infernoZone.reset();
    }

    // Load dark world scenery for sombre zones (Cemetery..AbandonedManor only)
    if ((int)dest >= (int)ZoneID::Cemetery && dest != ZoneID::InfernoZone) {
        darkWorld.load((int)dest, 0x343fdu * (unsigned int)((int)dest + 1));  // seed deterministica by zone
    } else {
        darkWorld.active = false;
    }

    float cx = static_cast<float>(tilemap.width  * Tilemap::tileSize) / 2.0f;
    float cy = static_cast<float>(tilemap.height * Tilemap::tileSize) / 2.0f;
    player.position = {cx, cy};

    bossSpawned        = false;
    spawnTimer         = 0.0f;
    spawnInterval      = getZoneInfo(dest).spawnInterval;
    bossSpawnThreshold = (dest == ZoneID::KronosNexus) ? 10 : 20;
    zoneNameTimer      = 4.0f;
    dialogOpen         = false;
    nearNpcIndex       = -1;

    // Story chapter banners
    switch (dest) {
        case ZoneID::Bunker:
            storyChapter = 2;
            showStoryBanner("CAPITULO 2: O BUNKER NEXUS",
                "Base of the NEXUS localizada. Mas KRONOS in the rastreia...", 5.0f);
            triggerPlayerSpeech("Chegando to the Bunker NEXUS. Aliados detectados.", 4.0f);
            break;
        case ZoneID::KronosForge:
            storyChapter = 3;
            showStoryBanner("CAPITULO 3: A FORGE",
                "Inside of the entranhas of KRONOS. Cada maquina went built to kill.", 5.0f);
            triggerPlayerSpeech("Forge KRONOS infiltrada. Alerta maximum.", 4.0f);
            break;
        case ZoneID::KronosNexus:
            storyChapter = 4;
            showStoryBanner("CAPITULO FINAL: O NUCLEO",
                "Este and the coracao of KRONOS. Destrua-the and liberte the humanidade.", 6.0f);
            triggerPlayerSpeech("Core KRONOS localizado. Hour of finish isso.", 5.0f);
            break;
        case ZoneID::Cemetery:
            showStoryBanner("CEMETERY ABANDONADO", "Os mortos not descansam here...", 5.0f);
            triggerPlayerSpeech("Lugar sombrio. Mas not tenho medo of death.", 4.0f);
            break;
        case ZoneID::CursedFarm:
            showStoryBanner("FAZENDA MALDITA", "A terra is podre. As colheitas, corrompidas.", 5.0f);
            triggerPlayerSpeech("Algo very wrong nessa farm...", 3.5f);
            break;
        case ZoneID::GhostCity:
            showStoryBanner("CITY GHOST", "Ruas vazias. Mas not desertas.", 5.0f);
            triggerPlayerSpeech("Uma city whole... silenciada.", 3.5f);
            break;
        case ZoneID::DarkForest:
            showStoryBanner("FLORESTA NEGRA", "A fog esconde the that mora among the arvores.", 5.0f);
            triggerPlayerSpeech("Visibilidade zero. Cuidado.", 3.0f);
            break;
        case ZoneID::Catacombs:
            showStoryBanner("CATACUMBAS", "Passagens of stone. Cheiro of death antiga.", 5.0f);
            triggerPlayerSpeech("Catacumbas. Quantos anos of death here?", 3.5f);
            break;
        case ZoneID::AbandonedManor:
            showStoryBanner("MANSAO ABANDONADA", "O boss aguarda in the profundezas.", 6.0f);
            triggerPlayerSpeech("A Manor. Sinto algo powerful here inside.", 4.0f);
            break;
        case ZoneID::InfernoZone:
            showStoryBanner("ZONE INFERNO", "Lava, cinzas and criaturas of the abismo. Bem-coming to the inferno.", 6.0f);
            triggerPlayerSpeech("Temperatura critica. Solo derretendo sob meus feet.", 4.5f);
            break;
        default: break;
    }

    autoSave();
}
