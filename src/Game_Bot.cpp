// Game_Bot.cpp — decisoes of the bot/autotest (extraido of Game::handleInput).
// Same class Game.
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
        for (const auto& and : enemies) enemyPos.push_back(and.position);

        std::vector<Vector2> itemPos;
        itemPos.reserve(items.size() + xpOrbs.size());
        for (const auto& it : items)   itemPos.push_back(it.position);
        for (const auto& xp : xpOrbs)  itemPos.push_back(xp.position);

        bool skillsReady[6] = {false,false,false,false,false,false};
        for (int si2 = 0; si2 < 6 && si2 < (int)player.skills.size(); ++si2)
            skillsReady[si2] = player.skills[si2].isReady();

        // Sensores of wall — 8 directions around of the player (alinhado the k8DirAngles)
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

        // ── Diagnostic of FPS: picos of entidades + snapshot in the FPS more down ──
        {
            int ne = (int)enemies.size(), ni = (int)items.size(), in the = (int)xpOrbs.size();
            int np = (int)projectiles.size(), nep = (int)enemyProjectiles.size();
            int nu = (int)buildingSystem.tanks.size() + (int)buildingSystem.soldiers.size();
            if (ne  > botController.peakEnemies)     botController.peakEnemies = ne;
            if (ni  > botController.peakItems)       botController.peakItems = ni;
            if (in the  > botController.peakOrbs)        botController.peakOrbs = in the;
            if (np  > botController.peakProjectiles) botController.peakProjectiles = np;
            if (nep > botController.peakEnemyProj)   botController.peakEnemyProj = nep;
            if (nu  > botController.peakUnits)        botController.peakUnits = nu;
            float fps = headless ? headlessFps : (float)GetFPS();
            if (fps > 0.0f && fps < botController.fpsLowValue &&
                botController.fpsQuarantine <= 0.0f && dt <= 0.25f) {   // ignora window pos-load
                botController.fpsLowValue     = fps;
                botController.fpsLowEnemies   = ne;
                botController.fpsLowProj      = np + nep;
                botController.fpsLowParticles = 0; // preenchido below if available
            }
        }

        // Pathfinding of the bot. A consulta antiga era only `tilemap.isWallAtPosition`:
        // ignorava the estruturas of chunk (m_chunkSolids) E the barrier of the phase, entao
        // the BFS tracava route atravessando building and wall of energy. O bot encostava,
        // era empurrado of returns and repetia - foram the 4 travamentos >10s that the portao
        // of validation acusou.
        if (!botController.wallQuery)
            botController.wallQuery = [this](Vector2 p) {
                // Testa the BODY, not um point: existe fresta with the center of the tile
                // livre by where the character (radius ~22u) not passes. A route mandava
                // atravessar, the collision barrava, and the bot empurrava wall to always.
                const float BR = 24.0f;
                if (isBlocked(p)) return true;
                if (isBlocked({ p.x + BR, p.y })) return true;
                if (isBlocked({ p.x - BR, p.y })) return true;
                if (isBlocked({ p.x, p.y + BR })) return true;
                if (isBlocked({ p.x, p.y - BR })) return true;
                float dx = p.x - safeZoneCenter.x, dy = p.y - safeZoneCenter.y;
                float lim = owPhaseRadius - 60.0f;          // margem: not colar in the barrier
                return openWorldMode && (dx*dx + dy*dy > lim*lim);
            };
        // Center for the escape: the REFUGE of the phase. Antes apontava for the center of the
        // map fixed (12288,12288), outside the barrier - the "escape" jogava the bot contra
        // the wall instead of tirar dele.
        botController.worldCenter = safeZoneCenter;
        botController.worldRadius = owPhaseRadius;   // the bot conhece the size of the phase
        // Zone segura: enemies only existem FORA dela — the bot usa isso to
        // force the exploracao to outside the refuge (era the causa of 0 kills).
        botController.safeZoneCenter = safeZoneCenter;
        botController.safeZoneRadius = safeZoneRadius;
        // Portal of phase of the world open — without isto the bot never avancava of phase:
        // recebia only tilemap.portals (system old, empty in the world open).
        botController.owPortalPos  = owPortalPos;
        botController.owPortalOpen = owPortalOpen;

        // Portals of output of the zone — for the bot advance of phase (AdvancePhase).
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

        // Pedido of avanco of phase of the bot — consumido by updatePhasePortal
        // (that also accepted the key E of the player humano).
        if (dec.shouldUsePortal) botWantsPortal = true;

        if (dec.shouldQuit) {
            botController.writeReport("bot_report_final.txt");   // relativo to the CWD
            quitRequested = true;
            return;
        }

        // Auto-save partial the cada 5 minutes of test
        if (botController.autoTest) {
            botReportSaveTimer += dt;
            if (botReportSaveTimer >= 300.0f) {
                botReportSaveTimer = 0.0f;
                int minElapsed = (int)(botController.testTimer / 60.0f);
                std::string rpath = "bot_report_" +   // relativo to the CWD
                                    std::to_string(minElapsed) + "min.txt";
                botController.writeReport(rpath);
                botController.addLog("Report partial saved (" + std::to_string(minElapsed) + "min)");
            }
        }

        if (dec.shouldMove) {
            // Stuck and pathfinding sao gerenciados DENTRO of the BotController
            // (updateStuckTracking with window of 0.5s + BFS in computePathDir).
            // O tracker duplicado that havia here media deslocamento of 1 frame
            // (moved < 2.5px) and the botFindPath local disputava the route with the BFS
            // of the bot — removidos; the alvo of the bot already comes roteado.
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
        // Contagem of shot AQUI (after the re-verificacao isReady): before the
        // BotController already contava to the DECIDIR, inflando skillsFired same
        // when the skill not saia by cooldown.
        if (dec.shouldUseSkill1 && (int)player.skills.size() > 0 && player.skills[0].isReady()) {
            player.useSkill(0, dec.nearestEnemyPos);
            float dmgL = player.getEffectiveDamage() + player.skills[0].damage;
            projectiles.emplace_back(player.position, botAimDir, dmgL, 550.0f, 620.0f,
                                     Color{0,255,255,255});
            int spread = 1 + SkillTree::statsFor(player.perkMask).laserBeams;
            float baseA = std::atan2(botAimDir.y, botAimDir.x);
            for (int k = 1; k <= spread; ++k) {
                for (int s : {-1, 1}) {
                    float the = baseA + s * 0.12f * (float)k;
                    Vector2 d = {std::cos(the), std::sin(the)};
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
        // Skill 4 — Overload
        if (dec.shouldUseSkill4 && (int)player.skills.size() > 3 && player.skills[3].isReady()) {
            player.useSkill(3, dec.nearestEnemyPos);
            player.overloadTimer = 8.0f + SkillTree::statsFor(player.perkMask).overloadBonus;
            particles.spawnLevelUp(player.position);
            audio.playLevelUp();
            botController.skillsFired++;
            botController.skillUsageCounts[3]++;
        }
        // Skill 5 — Barrier
        if (dec.shouldUseSkill5 && (int)player.skills.size() > 4 && player.skills[4].isReady()) {
            player.useSkill(4, dec.nearestEnemyPos);
            player.shieldTimer = 3.0f;
            particles.spawnLevelUp(player.position);
            botController.skillsFired++;
            botController.skillUsageCounts[4]++;
        }
        // Skill 6 — Burst
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
        // BOT: exercita TODOS the sistemas — convoca aliados, constroi tanques/
        // torres/quarteis, produz unidades and auto-evolui the estruturas.
        // ════════════════════════════════════════════════════════════════════
        {
            // Timers sao MEMBROS of the Game (eram static of function: sobreviviam
            // to the restartRun and the automacao voltava "adiantada" in the new match).
            // Em autoTest ensures um fluxo of resources to manage exercitar
            // structures/evolucoes same in phases iniciais.
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

            // 2) Construir estruturas variadas num anel around of the bot
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
                        botController.addLog(TextFormat("Struct built (type %d)",
                                                        buildingSystem.selectedType));
                        break;
                    }
                }
            }

            // 3) Produzir unidades of the fabricas/quarteis existentes
            botProduceTimer -= dt;
            if (botProduceTimer <= 0.0f && !buildingSystem.buildings.empty()) {
                botProduceTimer = 7.0f;
                for (const auto& b : buildingSystem.buildings) {
                    if (b.type == BuildingType::TankFactory ||
                        b.type == BuildingType::Barracks) {
                        if (buildingSystem.clickProduce(b.position, player.credits) == 1) {
                            botController.addLog("Unidade produced");
                            break;
                        }
                    }
                }
            }

            // 4) Auto-evolution: evolui the struct more next
            botUpgradeTimer -= dt;
            if (botUpgradeTimer <= 0.0f) {
                botUpgradeTimer = 15.0f;
                if (buildingSystem.upgradeNearby(player.position, player.credits) == 1)
                    botController.addLog("Struct evoluida");
            }
        }
    }
}
