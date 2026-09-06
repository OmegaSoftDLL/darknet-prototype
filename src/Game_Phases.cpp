// Game_Phases.cpp — fases do mundo aberto: tabela de fases, portal e transicao de zona.
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

// ─── Zone Transition ─────────────────────────────────────────────────────────

void Game::checkPortalTransition() {
    ZoneID dest;
    if (tilemap.isPortalAtPosition(player.position, dest)) {
        transitionToZone(dest);
    }
}

// Le content/phases.txt: "zona | abates | chefe | raio | titulo". Formato de
// linha simples de proposito: sem dependencia de JSON e editavel por qualquer um.
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
                    while (!part.empty() && (part.front() == ' ' || part.front() == '	')) part.erase(part.begin());
                    while (!part.empty() && (part.back() == ' ' || part.back() == '' || part.back() == '	')) part.pop_back();
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
                if (d.radius < 900.0f) d.radius = 900.0f;   // fase minima jogavel
                phaseDefs.push_back(d);
            }
            UnloadFileText(txt);
        }
    }
    if (phaseDefs.empty()) {
        TraceLog(LOG_WARNING, "FASES: content/phases.txt ausente/vazio - usando tabela embutida");
        for (const auto& nz : NAMES) {
            PhaseDef d; d.zone = nz.z; d.title = getZoneInfo(nz.z).name;
            d.goal = 20 + (int)phaseDefs.size() * 8;
            d.boss = ((int)phaseDefs.size() + 1) % 3 == 0;
            d.radius = 3000.0f + phaseDefs.size() * 160.0f;
            phaseDefs.push_back(d);
        }
    }
    TraceLog(LOG_INFO, "FASES: %d carregadas", (int)phaseDefs.size());
}

const Game::PhaseDef& Game::phaseDef(int phase) const {
    static PhaseDef fallback;
    if (phaseDefs.empty()) return fallback;
    if (phase < 0) phase = 0;
    return phaseDefs[phase % (int)phaseDefs.size()];
}

// ── FASES do mundo aberto ────────────────────────────────────────────────────
ZoneID Game::phaseZone(int phase) {   // compat: a ordem agora vem da tabela
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
    // Fase de chefe: matar a cota NAO basta, o chefe tem que cair. E o que fecha
    // a fase como uma fase, com clima e desfecho, em vez de uma cota de abates.
    if (owBossPhase && !owBossDown && bossSpawned) {
        bool alive = false;
        for (const auto& e : enemies) if (e.isBoss() && !e.isDead()) { alive = true; break; }
        if (!alive) {
            owBossDown = true;
            showStoryBanner("CHEFE ABATIDO", "O caminho para o proximo mundo esta livre.", 4.0f);
        }
    }
    bool goalMet = (owPhaseKills >= owPhaseGoal) && (!owBossPhase || owBossDown);
    if (!owPortalOpen && goalMet) {
        owPortalOpen = true;
        tutorial.onPortalFound();
        // portal nasce perto do refugio, sempre no mesmo rumo (o jogador acha)
        owPortalPos = { safeZoneCenter.x + 620.0f, safeZoneCenter.y - 520.0f };
        // Garantia intencional (antes era acidental): o ponto do portal tem que
        // ser alcancavel. O teste cobre o DISCO de interacao (raio ~105u), nao
        // so o ponto central — um portal com o centro livre e a borda murada
        // continua inalcancavel.
        auto diskBlocked = [this](Vector2 p) {
            if (isBlocked(p)) return true;
            for (int s = 0; s < 8; ++s) {
                float a = s * 0.7853982f;   // amostra a cada 45 graus da borda
                if (isBlocked({ p.x + std::cos(a) * 100.0f,
                                p.y + std::sin(a) * 100.0f })) return true;
            }
            return false;
        };
        if (diskBlocked(owPortalPos)) {
            TraceLog(LOG_WARNING, "PORTAL: ponto (%.0f,%.0f) bloqueado - realocando",
                     owPortalPos.x, owPortalPos.y);
            // Aneis crescentes x mais angulos: cobre bem mais que o anel unico
            // de 700u da versao anterior.
            bool found = false;
            for (int ring = 0; ring < 3 && !found; ++ring) {
                float rad = 700.0f + ring * 200.0f;          // 700 / 900 / 1100
                for (int t = 0; t < 16 && !found; ++t) {
                    float a = t * 0.3926991f;                // 22,5 graus por candidato
                    Vector2 c = { safeZoneCenter.x + std::cos(a) * rad,
                                  safeZoneCenter.y + std::sin(a) * rad };
                    if (!diskBlocked(c)) { owPortalPos = c; found = true; }
                }
            }
            if (!found) {
                // Espiral dentro da fase: ultima tentativa de achar QUALQUER
                // ponto com o disco livre antes do fallback destrutivo.
                float lim = owPhaseRadius - 200.0f;
                for (int t = 0; t < 220 && !found; ++t) {
                    float a = t * 0.9f, rad = 120.0f + t * 9.0f;   // ~120..2100u
                    if (rad > lim) break;
                    Vector2 c = { safeZoneCenter.x + std::cos(a) * rad,
                                  safeZoneCenter.y + std::sin(a) * rad };
                    if (!diskBlocked(c)) { owPortalPos = c; found = true; }
                }
            }
            if (!found) {
                // NUNCA manter posicao bloqueada em silencio: o portal vai pro
                // refugio (sempre alcancavel) e os solidos em volta sao limpos.
                owPortalPos = safeZoneCenter;
                clearBlockingAt(owPortalPos, 140.0f);
                TraceLog(LOG_WARNING, "PORTAL: nenhum ponto livre - solidos limpos no refugio");
            }
        }
        showStoryBanner("PORTAL ABERTO", "Va ate o portal para avancar de mundo.", 5.0f);
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
    // O cabecalho "CAP.N" acompanhava so o sistema ANTIGO de zonas
    // (transitionToZone) e ficava congelado em CAP.1 no mundo aberto.
    storyChapter = owPhase + 1;
    // Metrica do bot: "Zonas avancadas" do relatorio conta o avanco REAL de fase
    // (antes so contava encostar num portal do tilemap — sistema antigo, vazio no OW).
    if (botController.active) {
        botController.zonesVisited++;
        botController.addLog(TextFormat("ZONA AVANCADA! fase=%d total=%d",
                                        owPhase + 1, botController.zonesVisited));
    }
    const PhaseDef& pd = phaseDef(owPhase);
    ZoneID dest = pd.zone;
    currentZone         = dest;
    currentRegion       = dest;
    tilemap.currentZone = dest;

    // limpa o mundo antigo por inteiro (senao sobra inimigo/loot/predio fantasma)
    enemies.clear(); items.clear(); projectiles.clear(); enemyProjectiles.clear();
    xpOrbs.clear(); groundEquips.clear(); damageNumbers.clear();
    dialogOpen = false; nearNpcIndex = -1;
    bossSpawned = false; spawnTimer = 0.0f;

    // Parametros da NOVA fase ANTES de reconstruir o cenario: buildOpenWorldScenery
    // limita o mundo pela barreira (owPhaseRadius) — depois da construcao, a fase
    // nova nasceu varias vezes com o raio da fase anterior.
    owPortalOpen   = false;
    owKillsAtStart = enemiesKilled;
    owPhaseGoal    = pd.goal;       // tudo vem de content/phases.txt
    owPhaseRadius  = pd.radius;
    owBossPhase    = pd.boss;
    owBossDown     = false;

    tilemap.generateOpenWorld();
    // generateOpenWorld() reseta currentZone p/ LARuins (init da campanha). Sem
    // regravar o bioma da fase AQUI, o piso, as ruas e o biomeAtWorld ficavam
    // presos em LA em TODA fase 2+ — e o cenario por regiao seguia um bioma
    // diferente do chao embaixo dele.
    tilemap.currentZone = dest;
    setupWorldRegions();
    buildOpenWorldScenery();
    setupZoneNPCs(dest);
    audio.setZone(dest);
    spawnInterval = getZoneInfo(dest).spawnInterval / getDifficulty().spawnRateMult;

    player.position = safeZoneCenter;

    // RECOMPENSA de fase: fechar um mundo tem que valer alguma coisa, senao o
    // portal e so um corredor. Cura cheia + creditos + uma peca de equipamento.
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
            triggerPlayerSpeech(TextFormat("Recompensa: %s", drop.name.c_str()), 3.5f);
        }
    }
    damageNumbers.push_back({ player.position, (float)bonus, {255,210,0,255}, 1.6f, "$" });

    ZoneInfo zi = getZoneInfo(dest);
    owFadeText  = TextFormat("FASE %d  -  %s", owPhase + 1,
                             pd.title.empty() ? zi.name.c_str() : pd.title.c_str());
    owFadeTimer = 3.0f;
}

// Tela de transicao: preto entrando/saindo com o nome da fase. Sem isto a troca de
// mundo era um corte seco e nao lia como progresso.
void Game::drawPhaseFade() const {
    if (owFadeTimer <= 0.0f) return;
    float t = owFadeTimer / 2.6f;                       // 1 -> 0
    float a = (t > 0.5f) ? (t - 0.5f) * 2.0f : t * 2.0f; // sobe e desce
    a = 0.30f + a * 0.70f;
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, a));
    // Painel condensado p/ o texto nao ser engolido pelas fases escuras
    const char* obj = owBossPhase
        ? TextFormat("OBJETIVO: %d abates e derrotar o CHEFE", owPhaseGoal)
        : TextFormat("OBJETIVO: %d abates para abrir o portal", owPhaseGoal);
    int tw = MeasureText(owFadeText.c_str(), 36);
    int ow2 = MeasureText(obj, 16);
    int panW = (tw > ow2 ? tw : ow2) + 56;
    int panX = screenWidth/2 - panW/2;
    int panY = screenHeight/2 - 52;
    float pa = 0.35f + a * 0.55f;
    DrawRectangle(panX, panY, panW, 104, ColorAlpha(BLACK, pa));
    DrawRectangleLinesEx({(float)panX, (float)panY, (float)panW, 104.0f},
                         1.5f, ColorAlpha(Color{0,200,255,255}, 0.4f + a * 0.3f));
    float ta = 0.7f + a * 0.3f;                 // texto nunca fica abaixo de ~70%
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
    groundEquips.clear();      // evita drops orfaos da zona anterior
    damageNumbers.clear();
    dialogOpen   = false;
    nearNpcIndex = -1;         // invalida indice antes de reconstruir os NPCs
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
        darkWorld.load((int)dest, 0x343fdu * (unsigned int)((int)dest + 1));  // seed deterministica por zona
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
                "Base do NEXUS localizada. Mas KRONOS nos rastreia...", 5.0f);
            triggerPlayerSpeech("Chegando ao Bunker NEXUS. Aliados detectados.", 4.0f);
            break;
        case ZoneID::KronosForge:
            storyChapter = 3;
            showStoryBanner("CAPITULO 3: A FORJA",
                "Dentro das entranhas de KRONOS. Cada maquina foi construida para matar.", 5.0f);
            triggerPlayerSpeech("Forja KRONOS infiltrada. Alerta maximo.", 4.0f);
            break;
        case ZoneID::KronosNexus:
            storyChapter = 4;
            showStoryBanner("CAPITULO FINAL: O NUCLEO",
                "Este e o coracao de KRONOS. Destrua-o e liberte a humanidade.", 6.0f);
            triggerPlayerSpeech("Nucleo KRONOS localizado. Hora de terminar isso.", 5.0f);
            break;
        case ZoneID::Cemetery:
            showStoryBanner("CEMITERIO ABANDONADO", "Os mortos nao descansam aqui...", 5.0f);
            triggerPlayerSpeech("Lugar sombrio. Mas nao tenho medo de morte.", 4.0f);
            break;
        case ZoneID::CursedFarm:
            showStoryBanner("FAZENDA MALDITA", "A terra esta podre. As colheitas, corrompidas.", 5.0f);
            triggerPlayerSpeech("Algo muito errado nessa fazenda...", 3.5f);
            break;
        case ZoneID::GhostCity:
            showStoryBanner("CIDADE FANTASMA", "Ruas vazias. Mas nao desertas.", 5.0f);
            triggerPlayerSpeech("Uma cidade inteira... silenciada.", 3.5f);
            break;
        case ZoneID::DarkForest:
            showStoryBanner("FLORESTA NEGRA", "A nevoa esconde o que mora entre as arvores.", 5.0f);
            triggerPlayerSpeech("Visibilidade zero. Cuidado.", 3.0f);
            break;
        case ZoneID::Catacombs:
            showStoryBanner("CATACUMBAS", "Passagens de pedra. Cheiro de morte antiga.", 5.0f);
            triggerPlayerSpeech("Catacumbas. Quantos anos de morte aqui?", 3.5f);
            break;
        case ZoneID::AbandonedManor:
            showStoryBanner("MANSAO ABANDONADA", "O boss aguarda nas profundezas.", 6.0f);
            triggerPlayerSpeech("A Mansao. Sinto algo poderoso aqui dentro.", 4.0f);
            break;
        case ZoneID::InfernoZone:
            showStoryBanner("ZONA INFERNO", "Lava, cinzas e criaturas do abismo. Bem-vindo ao inferno.", 6.0f);
            triggerPlayerSpeech("Temperatura critica. Solo derretendo sob meus pes.", 4.5f);
            break;
        default: break;
    }

    autoSave();
}
