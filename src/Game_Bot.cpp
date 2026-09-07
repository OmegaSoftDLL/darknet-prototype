// Game_Bot.cpp — decisoes do bot/autotest (extraido de Game::handleInput).
// Mesma classe Game.
#include "Game.h"
#include "SkillTree.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

void Game::updateBotControl(float dt) {
    botMeleeRequest = false;
    botWantsPortal  = false;
    if (botController.active) {
        std::vector<Vector2> enemyPos;
        enemyPos.reserve(enemies.size());
        for (const auto& e : enemies) enemyPos.push_back(e.position);

        std::vector<Vector2> itemPos;
        itemPos.reserve(items.size() + xpOrbs.size());
        for (const auto& it : items)   itemPos.push_back(it.position);
        for (const auto& xp : xpOrbs)  itemPos.push_back(xp.position);

        bool skillsReady[6] = {false,false,false,false,false,false};
        for (int si2 = 0; si2 < 6 && si2 < (int)player.skills.size(); ++si2)
            skillsReady[si2] = player.skills[si2].isReady();

        // Sensores de parede — 8 direcoes ao redor do player (alinhado a k8DirAngles)
        {
            static const float k8[8] = {
                0.0f, (float)(PI*0.25), (float)(PI*0.5), (float)(PI*0.75),
                (float)PI, (float)(PI*1.25), (float)(PI*1.5), (float)(PI*1.75)
            };
            for (int d = 0; d < 8; d++) {
                Vector2 probe = {
                    player.position.x + std::cos(k8[d]) * 52.0f,
                    player.position.y + std::sin(k8[d]) * 52.0f
                };
                botController.blockedDir[d] = tilemap.isWallAtPosition(probe);
            }
        }

        // ── Diagnostico de FPS: picos de entidades + snapshot no FPS mais baixo ──
        {
            int ne = (int)enemies.size(), ni = (int)items.size(), no = (int)xpOrbs.size();
            int np = (int)projectiles.size(), nep = (int)enemyProjectiles.size();
            int nu = (int)buildingSystem.tanks.size() + (int)buildingSystem.soldiers.size();
            if (ne  > botController.peakEnemies)     botController.peakEnemies = ne;
            if (ni  > botController.peakItems)       botController.peakItems = ni;
            if (no  > botController.peakOrbs)        botController.peakOrbs = no;
            if (np  > botController.peakProjectiles) botController.peakProjectiles = np;
            if (nep > botController.peakEnemyProj)   botController.peakEnemyProj = nep;
            if (nu  > botController.peakUnits)        botController.peakUnits = nu;
            float fps = headless ? headlessFps : (float)GetFPS();
            if (fps > 0.0f && fps < botController.fpsLowValue &&
                botController.fpsQuarantine <= 0.0f && dt <= 0.25f) {   // ignora janela pos-carga
                botController.fpsLowValue     = fps;
                botController.fpsLowEnemies   = ne;
                botController.fpsLowProj      = np + nep;
                botController.fpsLowParticles = 0; // preenchido abaixo se disponivel
            }
        }

        // Pathfinding do bot. A consulta antiga era so `tilemap.isWallAtPosition`:
        // ignorava as estruturas de chunk (m_chunkSolids) E a barreira da fase, entao
        // o BFS tracava rota atravessando predio e parede de energia. O bot encostava,
        // era empurrado de volta e repetia - foram os 4 travamentos >10s que o portao
        // de validacao acusou.
        if (!botController.wallQuery)
            botController.wallQuery = [this](Vector2 p) {
                // Testa o CORPO, nao um ponto: existe fresta com o centro do tile
                // livre por onde o personagem (raio ~22u) nao passa. A rota mandava
                // atravessar, a colisao barrava, e o bot empurrava parede pra sempre.
                const float BR = 24.0f;
                if (isBlocked(p)) return true;
                if (isBlocked({ p.x + BR, p.y })) return true;
                if (isBlocked({ p.x - BR, p.y })) return true;
                if (isBlocked({ p.x, p.y + BR })) return true;
                if (isBlocked({ p.x, p.y - BR })) return true;
                float dx = p.x - safeZoneCenter.x, dy = p.y - safeZoneCenter.y;
                float lim = owPhaseRadius - 60.0f;          // margem: nao colar na barreira
                return openWorldMode && (dx*dx + dy*dy > lim*lim);
            };
        // Centro para o escape: o REFUGIO da fase. Antes apontava para o centro do
        // mapa fixo (12288,12288), fora da barreira - o "escape" jogava o bot contra
        // a parede em vez de tirar dele.
        botController.worldCenter = safeZoneCenter;
        botController.worldRadius = owPhaseRadius;   // o bot conhece o tamanho da fase
        // Zona segura: inimigos so existem FORA dela — o bot usa isso para
        // forcar a exploracao para fora do refugio (era a causa de 0 abates).
        botController.safeZoneCenter = safeZoneCenter;
        botController.safeZoneRadius = safeZoneRadius;
        // Portal de fase do mundo aberto — sem isto o bot nunca avancava de fase:
        // recebia so tilemap.portals (sistema antigo, vazio no mundo aberto).
        botController.owPortalPos  = owPortalPos;
        botController.owPortalOpen = owPortalOpen;

        // Portais de saida da zona — para o bot avancar de fase (AdvancePhase).
        std::vector<Vector2> portalPos;
        portalPos.reserve(tilemap.portals.size());
        for (const auto& zp : tilemap.portals) portalPos.push_back(zp.position);

        auto dec = botController.update(
            dt,
            player.position, player.attackRange,
            player.health, player.maxHealth,
            player.level, player.credits,
            (headless ? headlessFps : (float)GetFPS()),
            enemyPos, itemPos, skillsReady, portalPos, 0);

        // Pedido de avanco de fase do bot — consumido por updatePhasePortal
        // (que tambem aceita a tecla E do jogador humano).
        if (dec.shouldUsePortal) botWantsPortal = true;

        if (dec.shouldQuit) {
            botController.writeReport("bot_report_final.txt");   // relativo ao CWD
            quitRequested = true;
            return;
        }

        // Auto-save parcial a cada 5 minutos de teste
        if (botController.autoTest) {
            botReportSaveTimer += dt;
            if (botReportSaveTimer >= 300.0f) {
                botReportSaveTimer = 0.0f;
                int minElapsed = (int)(botController.testTimer / 60.0f);
                std::string rpath = "bot_report_" +   // relativo ao CWD
                                    std::to_string(minElapsed) + "min.txt";
                botController.writeReport(rpath);
                botController.addLog("Relatorio parcial salvo (" + std::to_string(minElapsed) + "min)");
            }
        }

        if (dec.shouldMove) {
            // Stuck e pathfinding sao gerenciados DENTRO do BotController
            // (updateStuckTracking com janela de 0.5s + BFS em computePathDir).
            // O tracker duplicado que havia aqui media deslocamento de 1 frame
            // (moved < 2.5px) e o botFindPath local disputava a rota com o BFS
            // do bot — removidos; o alvo do bot ja vem roteado.
            moveTarget = dec.moveTarget;
            hasTarget  = true;
        }
        if (dec.shouldMeleeAttack) {
            botMeleeRequest = true;
        }
        // Aim direction toward nearest enemy for all bot skills
        Vector2 botAimDir = {1.0f, 0.0f};
        if (dec.nearestEnemyIdx >= 0) {
            Vector2 toE = Vector2Subtract(dec.nearestEnemyPos, player.position);
            float   len = std::sqrt(toE.x*toE.x + toE.y*toE.y);
            if (len > 0.001f) botAimDir = {toE.x/len, toE.y/len};
        }
        // Skill 1 — Laser
        // Contagem de disparo AQUI (apos a re-verificacao isReady): antes o
        // BotController ja contava ao DECIDIR, inflando skillsFired mesmo
        // quando a skill nao saia por cooldown.
        if (dec.shouldUseSkill1 && (int)player.skills.size() > 0 && player.skills[0].isReady()) {
            player.useSkill(0, dec.nearestEnemyPos);
            float dmgL = player.getEffectiveDamage() + player.skills[0].damage;
            projectiles.emplace_back(player.position, botAimDir, dmgL, 550.0f, 620.0f,
                                     Color{0,255,255,255});
            int spread = 1 + SkillTree::statsFor(player.perkMask).laserBeams;
            float baseA = std::atan2(botAimDir.y, botAimDir.x);
            for (int k = 1; k <= spread; ++k) {
                for (int s : {-1, 1}) {
                    float a = baseA + s * 0.12f * (float)k;
                    Vector2 d = {std::cos(a), std::sin(a)};
                    projectiles.emplace_back(player.position, d, dmgL * 0.6f, 550.0f, 560.0f,
                                             Color{0,200,255,180});
                }
            }
            particles.spawnHit(player.position, Color{0,255,255,255}, 8);
            audio.playLaser();
            botController.skillsFired++;
            botController.skillUsageCounts[0]++;
        }
        // Skill 2 — EMP
        if (dec.shouldUseSkill2 && (int)player.skills.size() > 1 && player.skills[1].isReady()) {
            player.useSkill(1, dec.nearestEnemyPos);
            float empDmg = player.skills[1].damage * (player.isOverloaded() ? 1.5f : 1.0f);
            for (auto& enemy : enemies)
                if (Vector2Distance(player.position, enemy.position) <= player.skills[1].range)
                    enemy.takeDamage(empDmg);
            particles.spawnExplosion(player.position, YELLOW, 25);
            audio.playEMP();
            botController.skillsFired++;
            botController.skillUsageCounts[1]++;
        }
        // Skill 3 — Plasma Grenade
        if (dec.shouldUseSkill3 && (int)player.skills.size() > 2 && player.skills[2].isReady()) {
            player.useSkill(2, dec.nearestEnemyPos);
            float gDmg = player.skills[2].damage * (player.isOverloaded() ? 1.5f : 1.0f);
            projectiles.emplace_back(player.position, botAimDir, gDmg,
                                     player.skills[2].range, 280.0f,
                                     Color{255,120,0,255}, true);
            audio.playLaser();
            botController.skillsFired++;
            botController.skillUsageCounts[2]++;
        }
        // Skill 4 — Sobrecarga
        if (dec.shouldUseSkill4 && (int)player.skills.size() > 3 && player.skills[3].isReady()) {
            player.useSkill(3, dec.nearestEnemyPos);
            player.overloadTimer = 8.0f + SkillTree::statsFor(player.perkMask).overloadBonus;
            particles.spawnLevelUp(player.position);
            audio.playLevelUp();
            botController.skillsFired++;
            botController.skillUsageCounts[3]++;
        }
        // Skill 5 — Barreira
        if (dec.shouldUseSkill5 && (int)player.skills.size() > 4 && player.skills[4].isReady()) {
            player.useSkill(4, dec.nearestEnemyPos);
            player.shieldTimer = 3.0f;
            particles.spawnLevelUp(player.position);
            botController.skillsFired++;
            botController.skillUsageCounts[4]++;
        }
        // Skill 6 — Rajada
        if (dec.shouldUseSkill6 && (int)player.skills.size() > 5 && player.skills[5].isReady()) {
            player.useSkill(5, dec.nearestEnemyPos);
            float baseAngle6 = std::atan2(botAimDir.y, botAimDir.x);
            float dmg6 = player.skills[5].damage * (player.isOverloaded() ? 1.5f : 1.0f);
            int lo = -3 - SkillTree::statsFor(player.perkMask).burstProj;
            int hi =  4 + SkillTree::statsFor(player.perkMask).burstProj;
            for (int i = lo; i <= hi; ++i) {
                float angle = baseAngle6 + 0.22f * (float)i;
                Vector2 d2 = {std::cos(angle), std::sin(angle)};
                Color col = (std::abs(i) <= 1) ? Color{0,255,100,255} : Color{0,200,80,200};
                projectiles.emplace_back(player.position, d2, dmg6, 420.0f, 660.0f, col);
            }
            audio.playLaser();
            botController.skillsFired++;
            botController.skillUsageCounts[5]++;
        }
        botAimTarget = dec.moveTarget;

        // ════════════════════════════════════════════════════════════════════
        // BOT: exercita TODOS os sistemas — convoca aliados, constroi tanques/
        // torres/quarteis, produz unidades e auto-evolui as estruturas.
        // ════════════════════════════════════════════════════════════════════
        {
            // Timers sao MEMBROS do Game (eram static de funcao: sobreviviam
            // ao restartRun e a automacao voltava "adiantada" na nova partida).
            // Em autoTest garante um fluxo de recursos para conseguir exercitar
            // construcoes/evolucoes mesmo em fases iniciais.
            if (botController.autoTest) {
                botStipendTimer += dt;
                if (botStipendTimer >= 5.0f) {
                    botStipendTimer = 0.0f;
                    player.credits   += 120;
                    materialMetal    += 6;
                    materialCarapace += 4;
                }
            }

            // 1) Convocar aliados (companions MARCO VEIL / STEEL / REX)
            botAllyTimer -= dt;
            if (botAllyTimer <= 0.0f) {
                botAllyTimer = 25.0f;
                if ((int)companions.size() < 3) {
                    CompanionType t = companions.empty() ? CompanionType::MarcoVeil
                                    : ((int)companions.size() == 1 ? CompanionType::Steel
                                                                   : CompanionType::Rex);
                    spawnCompanion(t);
                    botController.addLog("Aliado convocado");
                }
            }

            // 2) Construir estruturas variadas num anel ao redor do bot
            botBuildTimer -= dt;
            if (botBuildTimer <= 0.0f && (int)buildingSystem.buildings.size() < 12) {
                botBuildTimer = 6.0f;
                static const int cycleTypes[] = {
                    (int)BuildingType::Turret,      (int)BuildingType::Barracks,
                    (int)BuildingType::TankFactory, (int)BuildingType::House,
                    (int)BuildingType::ResourceNode,(int)BuildingType::MedBay,
                    (int)BuildingType::Ark,         (int)BuildingType::Wall,
                };
                int nTypes = (int)(sizeof(cycleTypes)/sizeof(cycleTypes[0]));
                buildingSystem.selectedType = cycleTypes[botBuildCycle % nTypes];
                botBuildCycle++;
                for (int attempt = 0; attempt < 6; ++attempt) {
                    float ang = (float)(botBuildCycle * 1.7f + attempt * 1.05f);
                    float rad = 110.0f + attempt * 30.0f;
                    Vector2 spot = { player.position.x + std::cos(ang) * rad,
                                     player.position.y + std::sin(ang) * rad };
                    if (tilemap.isWallAtPosition(spot)) continue;
                    int cc = 0, mc = 0, ac = 0;
                    if (buildingSystem.tryPlace(spot, player.credits, materialMetal,
                                                materialCarapace, cc, mc, ac)) {
                        player.credits   -= cc;
                        materialMetal    -= mc;
                        materialCarapace -= ac;
                        botController.addLog(TextFormat("Estrutura construida (tipo %d)",
                                                        buildingSystem.selectedType));
                        break;
                    }
                }
            }

            // 3) Produzir unidades das fabricas/quarteis existentes
            botProduceTimer -= dt;
            if (botProduceTimer <= 0.0f && !buildingSystem.buildings.empty()) {
                botProduceTimer = 7.0f;
                for (const auto& b : buildingSystem.buildings) {
                    if (b.type == BuildingType::TankFactory ||
                        b.type == BuildingType::Barracks) {
                        if (buildingSystem.clickProduce(b.position, player.credits) == 1) {
                            botController.addLog("Unidade produzida");
                            break;
                        }
                    }
                }
            }

            // 4) Auto-evolucao: evolui a estrutura mais proxima
            botUpgradeTimer -= dt;
            if (botUpgradeTimer <= 0.0f) {
                botUpgradeTimer = 15.0f;
                if (buildingSystem.upgradeNearby(player.position, player.credits) == 1)
                    botController.addLog("Estrutura evoluida");
            }
        }
    }
}
