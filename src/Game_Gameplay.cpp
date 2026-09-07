// Game_Gameplay.cpp the€” loop of game and input (extraido of Game.cpp).
// Same class Game: Game::update (loop main) and Game::handleInput.
#include "Game.h"
#include "SkillTree.h"
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <algorithm>
#include <string>

void Game::update(float dt) {
    // VICTORY — freezes the world and shows the end-game screen
    if (gameWon) {
        victoryTimer += dt;
        particles.update(dt);
        audio.updateMusic();
        if (playerSpeechTimer > 0.0f) playerSpeechTimer -= dt;
        // Bot: registra the victory UMA vez and encerra the test
        if (botController.active && !victoryReported) {
            victoryReported = true;
            botController.addLog("=== GAME ZERADO! NUCLEO KRONOS DESTRUIDO ===");
            botController.addLog(TextFormat("Level final %d, kills %d", player.level, totalKills));
            botController.writeReport("bot_report_VITORIA.txt");
        }
        // Apos 2s, ENTER returns to the menu main
        if (victoryTimer > 2.0f && IsKeyPressed(KEY_ENTER)) {
            inMainMenu = true;
            gameWon = false;
        }
        return;
    }

    // Dark world scenery update
    darkWorld.update(dt);

    // Inferno zone — lava + geyser damage
    if (infernoZone.active) {
        infernoZone.update(dt, player.position,
                           player.health, player.maxHealth,
                           player.isShielded());
    }

    // Hack Tree — painel open pela key X pausa the simulacao
    if (showSkillTree) {
        updateSkillTreePanel();
        audio.updateMusic();
        return;
    }

    // Level-up screen / evolution — only pauses when the player chose to open it.
    // Ela and opened deliberadamente (key L / K) and closes sozinha when the
    // points run out, ou with ESC (points are kept). The level-up itself
    // NUNCA strength essa screen — only acumula points.
    if (showLevelUpScreen || showEvolutionScreen) {
        levelUpAnimTimer += dt;
        if (playerSpeechTimer > 0.0f) playerSpeechTimer -= dt;
        audio.updateMusic();
        // ESC closes without gastar
        if (IsKeyPressed(KEY_ESCAPE)) {
            showLevelUpScreen = false; showEvolutionScreen = false;
            return;
        }
        if (showEvolutionScreen) {
            if (IsKeyPressed(KEY_LEFT))  evolutionChoice = (evolutionChoice - 1 + 3) % 3;
            if (IsKeyPressed(KEY_RIGHT)) evolutionChoice = (evolutionChoice + 1) % 3;
            int chosen = -1;
            if (IsKeyPressed(KEY_A)) chosen = 0;
            if (IsKeyPressed(KEY_S)) chosen = 1;
            if (IsKeyPressed(KEY_D)) chosen = 2;
            if (IsKeyPressed(KEY_ENTER)) chosen = evolutionChoice;
            if (chosen >= 0) {
                applyEvolutionPath(chosen);
                if (pendingEvolutions > 0) pendingEvolutions--;
                // Closes if there are in the more evolution points
                if (pendingEvolutions <= 0) showEvolutionScreen = false;
            }
        } else {
            if (IsKeyPressed(KEY_LEFT))  levelUpChoice = (levelUpChoice - 1 + 3) % 3;
            if (IsKeyPressed(KEY_RIGHT)) levelUpChoice = (levelUpChoice + 1) % 3;
            int chosen = -1;
            if (IsKeyPressed(KEY_ONE))   chosen = 0;
            if (IsKeyPressed(KEY_TWO))   chosen = 1;
            if (IsKeyPressed(KEY_THREE)) chosen = 2;
            if (IsKeyPressed(KEY_ENTER)) chosen = levelUpChoice;
            if (chosen >= 0) {
                applyLevelUpChoice(chosen);
                if (pendingLevelUps > 0) pendingLevelUps--;
                // If there are still points, generate new options and keep the screen open
                if (pendingLevelUps > 0) {
                    levelUpChoice = 1;
                    generateLevelUpChoices();
                } else {
                    showLevelUpScreen = false;
                }
            }
        }
        return;
    }

    // Visual pulse of the available-points warning
    if (pendingNotifyPulse > 0.0f) pendingNotifyPulse -= dt;

    // Open the Hack Tree whenever the player wants (key X)
    if (IsKeyPressed(KEY_X) && !inMainMenu && !paused &&
        !showLevelUpScreen && !showEvolutionScreen && player.health > 0.0f) {
        if (showSkillTree) { showSkillTree = false; return; }
        shopSystem.close(); craftingSystem.open = false;
        showInventory = false; showEquipment = false; showQuestLog = false;
        perkCursor    = 0;
        showSkillTree = true;
        return;
    }

    // Open the points screen whenever the player wants (without blocking the game on level-up)
    if (IsKeyPressed(KEY_L) && pendingLevelUps > 0) {
        shopSystem.close(); craftingSystem.open = false;
        showInventory = false; showEquipment = false; showQuestLog = false;
        levelUpChoice = 1;
        generateLevelUpChoices();
        showLevelUpScreen   = true;
        showEvolutionScreen = false;
        return;
    }
    if (IsKeyPressed(KEY_K) && pendingEvolutions > 0) {
        shopSystem.close(); craftingSystem.open = false;
        showInventory = false; showEquipment = false; showQuestLog = false;
        evolutionChoice     = 1;
        showEvolutionScreen = true;
        showLevelUpScreen   = false;
        return;
    }

    // Bot: gasta points acumulados automaticamente (escolha 1 / middle)
    if (botController.active) {
        if (pendingLevelUps > 0) {
            generateLevelUpChoices();
            applyLevelUpChoice(1);
            pendingLevelUps--;
        }
        if (pendingEvolutions > 0) {
            applyEvolutionPath(1);
            pendingEvolutions--;
        }
        autoSpendSkillPoints();   // Bot Hack Tree: fixed offensive build order
    }

    // HIT-STOP — freezes the simulation for the few frames on impact (combat juice).
    // Particles continuam animando for the "pancada" stay visible.
    if (hitStopTimer > 0.0f) {
        hitStopTimer -= dt;
        particles.update(dt);
        return;
    }
    // Camera zoom kick decays with gameplay time (freezes during hit-stop)
    camPunch = std::max(0.0f, camPunch - dt * 5.5f);
    // Floor decals (blood/scorched) fade after the short while
    for (auto& d : decals) d.life -= dt;
    decals.erase(std::remove_if(decals.begin(), decals.end(),
        [](const GroundDecal& d){ return d.life <= 0.0f; }), decals.end());

    // F12 toggles the bot
    if (IsKeyPressed(KEY_F12)) botController.toggle();

    // F9 = jump to Inferno Zone (test shortcut)
    if (IsKeyPressed(KEY_F9)) {
        transitionToZone(ZoneID::InfernoZone);
    }

    // P = cycle zones (debug / test) — 11 zones total including InfernoZone
    if (IsKeyPressed(KEY_P) && !shopSystem.open) {   // P in shop = premium tab
        int next = ((int)currentZone + 1) % 11;
        transitionToZone((ZoneID)next);
    }

    shopSystem.update(dt);
    updatePremiumStore(dt);   // premium tab (gems/Stripe) + balance refresh
    updateParty();            // group/alianca (party multiplayer — key O)

    // Cosmetics applied to the player model: common shop tint + premium skins.
    player.hasCosmeticTint = shopSystem.hasCosmeticColor;
    player.cosmeticTint    = shopSystem.playerColor;
    player.skinNeon        = store.ownsItem("skin_neon");
    player.skinDragon      = store.ownsItem("skin_dragon");
    player.petDrone        = store.ownsItem("pet_drone");
    craftingSystem.update(dt);

    handleInput(dt);

    // Tutorial / achievement systems update (after input, before physics/combat)
    tutorial.update(dt);
    if (!tutorial.active && tutorial.currentStep == TutorialStep::Completed && !tutorialRewardGiven) {
        tutorialRewardGiven = true;
        player.xp      += 500;
        player.credits += 100;
    }
    achievements.update(dt);
    totalPlaytime += dt;
    achievements.onPlaytime(totalPlaytime / 60.0f);

    player.update(dt);
    updateCompanions(dt);
    particles.update(dt);
    background.update(dt);
    audio.updateMusic();

    // Light system — dark zone detection and flicker
    {
        bool isDark = true; // pipeline 3D: clima Diablo always
        if (isDark != darkZoneActive) {
            darkZoneActive = isDark;
            lightSystem.setEnabled(isDark);
            if (isDark) {
                lightSystem.clear();
                lightSystem.addPlayerLight(player.position);
                // Torches at fixed scenic positions (spaced around map)
                lightSystem.addTorchLight({350, 420});
                lightSystem.addTorchLight({850, 310});
                lightSystem.addTorchLight({520, 750});
                lightSystem.addTorchLight({1100, 580});
                lightSystem.addTorchLight({200, 700});
                anomalySystem.storm.startAtmospheric();
            } else if (!anomalySystem.waveActive) {
                anomalySystem.storm.stop();
            }
        }
        if (darkZoneActive) {
            // Pipeline 3D: environment lighting per zone always active.
            {
                Color tCol; float tDark;
                switch (currentZone) {
                    // reduced ambientDark: the mask is MULTIPLICATIVE and already came
                    // after the floor fog — the two added together extinguished the scene.
                    // Gloomy climate comes from HUE and contrast, not from extinguishing everything.
                    // CONTRASTE ENTRE ZONAS aumentado: matizes almost neutros faziam
                    // all phase read igual. Inferno = orange-blood, nexus = cyan,
                    // forest = deep green, ghost = blue cold and more dark.
                    case ZoneID::LARuins:       tCol = {228,206,172,255}; tDark = 0.20f; break;
                    case ZoneID::Bunker:        tCol = {150,182,168,255}; tDark = 0.32f; break;
                    case ZoneID::KronosForge:   tCol = {250,150, 90,255}; tDark = 0.26f; break;
                    case ZoneID::KronosNexus:   tCol = {120,220,250,255}; tDark = 0.27f; break;
                    case ZoneID::Cemetery:      tCol = {140,150,220,255}; tDark = 0.40f; break;
                    case ZoneID::CursedFarm:    tCol = {206,190,120,255}; tDark = 0.30f; break;
                    case ZoneID::GhostCity:     tCol = {150,168,210,255}; tDark = 0.42f; break;
                    case ZoneID::DarkForest:    tCol = {120,200,140,255}; tDark = 0.37f; break;
                    case ZoneID::Catacombs:     tCol = {170,130,180,255}; tDark = 0.44f; break;
                    case ZoneID::AbandonedManor:tCol = {180,140,220,255}; tDark = 0.40f; break;
                    case ZoneID::InfernoZone:   tCol = {255,120, 60,255}; tDark = 0.25f; break;
                    default:                    tCol = {206,212,226,255}; tDark = 0.27f; break;
                }
                // Interpolates over ~1.5 s instead of swapping at once: crossing the border
                // of biome becomes the light transition, not the harsh cut to "another world".
                {
                    float k = 1.0f - expf(-GetFrameTime() * 0.8f);
                    m_ambBaseDark += (tDark - m_ambBaseDark) * k;
                    m_ambBaseCol.r = (unsigned char)(m_ambBaseCol.r + (tCol.r - m_ambBaseCol.r) * k);
                    m_ambBaseCol.g = (unsigned char)(m_ambBaseCol.g + (tCol.g - m_ambBaseCol.g) * k);
                    m_ambBaseCol.b = (unsigned char)(m_ambBaseCol.b + (tCol.b - m_ambBaseCol.b) * k);
                    lightSystem.ambientColor = m_ambBaseCol;
                    lightSystem.ambientDark  = m_ambBaseDark;
                }

                // ── CICLO DIA/NOITE (world vivo): night dark/azulada, day clear ──
                worldClock += GetFrameTime() / 420.0f;          // ciclo complete ~7 min
                if (worldClock >= 1.0f) worldClock -= 1.0f;
                float sun = sinf(worldClock * 6.2831853f - 1.5707963f) * 0.5f + 0.5f; // 0=night,1=middle-day
                worldSun = sun;
                lightSystem.ambientDark += (1.0f - sun) * 0.20f; // escurece to the night
                if (lightSystem.ambientDark > 0.52f) lightSystem.ambientDark = 0.52f;  // ceiling: night legivel
                {
                    Color d = lightSystem.ambientColor;
                    Color n = { 104, 132, 196, 255 };            // blue nocturnal (more clear: luar, not breu)
                    lightSystem.ambientColor = {
                        (unsigned char)(n.r + (int)((d.r - n.r) * sun)),
                        (unsigned char)(n.g + (int)((d.g - n.g) * sun)),
                        (unsigned char)(n.b + (int)((d.b - n.b) * sun)), 255 };
                }
                lightSystem.clear();
                lightSystem.addPlayerLight(player.position);
                for (int i = 0; i < 5; ++i) { float the = i * 1.25664f;
                    lightSystem.addTorchLight({ player.position.x + cosf(the)*360.0f, player.position.y + sinf(the)*360.0f }); }
                int lit = 0;
                for (auto& b : buildingSystem.buildings) { if (b.active && lit < 10 && Vector2Distance(b.position, player.position) < 850.0f) { lightSystem.addBuildingLight(b.position); lit++; } }
            }
            lightSystem.updateFlicker(dt);
            lightSystem.updatePlayerPos(player.position);
        }
    }

    // SlowMo timer
    float effectiveDt = dt;
    if (slowMoTimer > 0.0f) {
        slowMoTimer -= dt;
        effectiveDt = dt * 0.25f;
    }
    (void)effectiveDt; // used for visual effects in future

    // Screen shake — sinusoidal decay
    // Session time + story banner timer
    sessionTime += dt;
    if (storyBannerTimer  > 0.0f) storyBannerTimer  -= dt;
    if (playerSpeechTimer > 0.0f) playerSpeechTimer -= dt;

    // Tick down active chats timers and removes expired ones
    for (auto it = activeChats.begin(); it != activeChats.end();) {
        it->second.timer -= dt;
        if (it->second.timer <= 0.0f) {
            it = activeChats.erase(it);
        } else {
            ++it;
        }
    }

    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        float env = std::max(0.0f, shakeTimer / 0.3f);
        float s   = shakeIntensity * env;
        float t   = shakeTimer * 40.0f;
        camera.offset = {screenWidth/2.0f  + std::cos(t * 1.3f) * s,
                         screenHeight/2.0f + std::sin(t)         * s};
    } else {
        camera.offset = {screenWidth/2.0f, screenHeight/2.0f};
    }

    // Smooth camera follow
    float camT = 1.0f - std::exp(-8.0f * dt);
    camera.target.x += (player.position.x - camera.target.x) * camT;
    camera.target.y += (player.position.y - camera.target.y) * camT;
    if (!buildingSystem.buildModeActive && !shopSystem.open && !craftingSystem.open && !showInventory) {
        float wh = GetMouseWheelMove();
        if (wh != 0.0f) { cameraZoom -= wh * 0.06f;
            if (cameraZoom < 0.85f) cameraZoom = 0.85f;
            if (cameraZoom > 1.50f) cameraZoom = 1.50f; }
    }

    // Camera 3D (2.5D) acompanha the player — always ativa.
    updateCamera3D();

    // Infinite world: auto-generates/unloads scenery in chunks around the player.
    updateSceneryChunks(player.position);

    // E.1: amortizes fixed scenery generation in batches per frame.
    streamSceneryBuild();

    // Environmental war events (Ruins of LA / City Fantasma): the cada 20-30s
    // um distant impact crosses the sky — flash on the horizon, estrondo abafado and um
    // micro-tremor. Visual/audio only; does not touch health/damage, entao the autotest segue
    // determinista (the bot harvests/shoots in world coordinates, not from the offset).
    if (openWorldMode &&
        (currentZone == ZoneID::LARuins || currentZone == ZoneID::GhostCity)) {
        if (owWarFlash > 0.0f) {
            owWarFlash -= dt;
            if (owWarFlash <= 0.0f) {
                if (shakeTimer <= 0.01f) triggerShake(1.8f, 0.22f);
                SetSoundVolume(audio.sfxExplosionBig, 0.12f);
                PlaySound(audio.sfxExplosionBig);
                SetSoundVolume(audio.sfxExplosionBig, 1.0f);
            }
        }
        owWarTimer -= dt;
        if (owWarTimer <= 0.0f) {   // schedules the next impact
            auto rf = []() { return (float)GetRandomValue(0, 1000) / 1000.0f; };
            owWarTimer = 20.0f + rf() * 12.0f;
            owWarFlash = 0.9f;
            owWarSeed  = rf() * 6.2832f;
            float d = 1100.0f + rf() * 420.0f;
            owWarPos  = { player.position.x + cosf(owWarSeed) * d,
                          player.position.y + sinf(owWarSeed) * d };
        }
    }

    // Open World region detection (without camera clamp — world is infinite)
    if (openWorldMode) {
        ZoneID newRegion = getRegionAt(player.position);
        if (newRegion != currentRegion) {
            currentRegion       = newRegion;
            currentZone         = newRegion;
            tilemap.currentZone = newRegion;
            spawnInterval       = getZoneInfo(newRegion).spawnInterval / getDifficulty().spawnRateMult;
            audio.setZone(newRegion);

            for (auto& r : worldRegions)
                if (r.zoneType == newRegion && !r.discovered) { r.discovered = true; break; }

            // Single region announcement: the top bar. Before this also triggered
            // zoneNameTimer and, on the FIRST visit, the same nome+descricao saia duas
            // vezes to the same time (bar at the top + giant text in the middle of the screen).
            ZoneInfo zi = getZoneInfo(newRegion);
            showStoryBanner(zi.name.c_str(), zi.description.c_str(), 4.0f);

            switch (newRegion) {
                case ZoneID::Cemetery:
                    triggerPlayerSpeech("Lugar sombrio... almas presas here.", 3.0f); break;
                case ZoneID::CursedFarm:
                    triggerPlayerSpeech("Algo very wrong nessa farm...", 3.0f); break;
                case ZoneID::GhostCity:
                    triggerPlayerSpeech("Uma city whole... silenciada.", 3.5f); break;
                case ZoneID::DarkForest:
                    triggerPlayerSpeech("Visibilidade zero. Cuidado with the fog.", 3.0f); break;
                case ZoneID::KronosForge:
                    triggerPlayerSpeech("Forge KRONOS. Calor extremo detectado.", 3.0f); break;
                case ZoneID::AbandonedManor:
                    triggerPlayerSpeech("Manor abandonada. Presencas sobrenaturais.", 3.5f); break;
                case ZoneID::KronosNexus:
                    triggerPlayerSpeech("KRONOS Core. End of the line.", 4.0f); break;
                case ZoneID::Bunker:
                    triggerPlayerSpeech("Bunker NEXUS. Area aliada.", 2.5f); break;
                default: break;
            }

            bool isDark = lightSystem.isDarkZone((int)newRegion);
            darkZoneActive = isDark;
            lightSystem.setEnabled(isDark);
            if (isDark) {
                lightSystem.clear();
                lightSystem.addPlayerLight(player.position);
                lightSystem.addTorchLight({player.position.x + 350, player.position.y + 200});
                lightSystem.addTorchLight({player.position.x - 280, player.position.y + 310});
                lightSystem.addTorchLight({player.position.x + 180, player.position.y - 300});
                lightSystem.addTorchLight({player.position.x - 400, player.position.y - 180});
                lightSystem.addTorchLight({player.position.x + 120, player.position.y + 450});
                // Rain and wind environment in the zones sombrias
                anomalySystem.storm.startAtmospheric();
            } else if (!anomalySystem.waveActive) {
                // Left of the zone sombria and not ha onda — for the rain
                anomalySystem.storm.stop();
            }

            infernoZone.active = (newRegion == ZoneID::InfernoZone);

            bool hasDarkScenery = ((int)newRegion >= (int)ZoneID::Cemetery &&
                                   newRegion != ZoneID::InfernoZone);
            if (hasDarkScenery) {
                // Seed DERIVADA of the region + phase (deterministica): the same phase
                // always reestrutura the same scenario sombrio to the return.
                unsigned int dseed = 0x343fdu * (unsigned int)((int)newRegion + 1)
                                   + (unsigned int)owPhase * 0x9e3779b9u;
                darkWorld.load((int)newRegion, dseed);
                // Dark scenery is generated at the origin of the old 3x3 grid (hub at (1280,1280)).
                // The world is now CENTERED on the base: translates the decoration for the hub
                // (delta = safeZoneCenter - center old = 0 in the phases atuais, mas
                // explicit in case the base ever moves).
                darkWorld.applyWorldOffset({
                    safeZoneCenter.x - (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f,
                    safeZoneCenter.y - (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f });
            } else {
                darkWorld.active = false;
            }

            setupZoneNPCs(newRegion);

            // FINAL BOSS — to reach the KRONOS Core, triggers the game climax
            if (newRegion == ZoneID::KronosNexus && !finalBossSpawned && !gameWon) {
                spawnFinalBoss();
            }
        }
    }

    // Hit flash timer (player takes damage)
    if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
    if (eliteFlashTimer > 0.0f) eliteFlashTimer -= dt;
    if (hurtDirTimer > 0.0f) hurtDirTimer -= dt;

    // Melee cooldown
    if (meleeCooldown > 0.0f) meleeCooldown -= dt;

    // Combo decay
    if (comboTimer > 0.0f) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) comboCount = 0;
    }

    // Footstep audio
    if (player.isMoving) {
        footstepTimer += dt;
        if (footstepTimer >= 0.30f) {
            audio.playFootstep();
            footstepTimer = 0.0f;
        }
    } else {
        footstepTimer = 0.0f;
    }

    // Ground equipment update + E-to-equip
    int nearEquipIdx = -1;
    float nearEquipDist = 60.0f;
    for (int i = 0; i < (int)groundEquips.size(); ++i) {
        auto& ge = groundEquips[i];
        ge.pulseTimer += dt;
        ge.lifetime   -= dt;
        float d = Vector2Distance(player.position, ge.position);
        if (d < nearEquipDist) { nearEquipIdx = i; nearEquipDist = d; }
    }
    if (nearEquipIdx >= 0 && IsKeyPressed(KEY_E) && !dialogOpen) {
        player.equipItem(groundEquips[nearEquipIdx].equip);
        groundEquips[nearEquipIdx].collected = true;
        audio.playPickup();
        particles.spawnLevelUp(groundEquips[nearEquipIdx].position);
    }
    groundEquips.erase(
        std::remove_if(groundEquips.begin(), groundEquips.end(),
                       [](const GroundEquipment& g){ return g.collected || g.lifetime <= 0.0f; }),
        groundEquips.end());

    // Damage numbers update
    for (auto& dn : damageNumbers) {
        dn.rise += 38.0f * dt;   // goes up in `rise`; in 3D pos.y is the NORTH axis of the floor
        dn.life -= dt;
    }
    damageNumbers.erase(
        std::remove_if(damageNumbers.begin(), damageNumbers.end(),
                       [](const DamageNumber& d){ return d.life <= 0.0f; }),
        damageNumbers.end());
    if (zoneNameTimer > 0.0f) zoneNameTimer -= dt;

    // Warning when CROSSING the safe-zone border (base gate)
    if (openWorldMode) {
        bool nowInSafe = inSafeZone(player.position);
        player.inSafeRefuge = nowInSafe;   // invulnerable in the refuge (in the one kills you in the city)
        if (wasInSafeZone && !nowInSafe) {
            // Leaving the base for danger
            showStoryBanner("!! SAINDO DA SAFE ZONE !!",
                            "Hostile territory ahead. Fique alert.", 3.0f);
            triggerPlayerSpeech("Leaving the base. Combat mode active.", 2.5f);
        } else if (!wasInSafeZone && nowInSafe) {
            // Returning for the base
            showStoryBanner("SAFE ZONE",
                            "You are protected. Recover and prepare.", 2.5f);
            triggerPlayerSpeech("Back to base. Secure.", 2.0f);
        }
        wasInSafeZone = nowInSafe;
    }

    // Infinite Evolution Engine — increases threat and rotates mutators
    updateEvolutionEngine(dt);

    // Natural resource gathering (hold H near the node)
    updateResourceGathering(dt);

    // Multiplayer LAN — sends local state and receives other players
    if (netActive) {
        float vm = std::sqrt(player.velocity.x*player.velocity.x + player.velocity.y*player.velocity.y);
        net.sendState(player.position.x, player.position.y,
                      (int)player.charClass, player.facing, vm > 12.0f);
        net.poll(dt);

        // Process incoming enemy deaths from network
        auto netDeaths = net.drainEnemyDeaths();
        for (uint32_t compId : netDeaths) {
            uint32_t cx = (compId >> 16) & 0xFFFF;
            uint32_t cy = compId & 0xFFFF;
            Vector2 netPos = { cx * 10.0f + 5.0f, cy * 10.0f + 5.0f };

            // Find closest local active enemy within 80 pixels
            Enemy* closest = nullptr;
            float minDist = 80.0f;
            for (auto& and : enemies) {
                if (and.isDead()) continue;
                float d = Vector2Distance(and.position, netPos);
                if (d < minDist) {
                    minDist = d;
                    closest = &and;
                }
            }
            if (closest) {
                closest->health = 0.0f;
                // Marks the SAME enemy (flag moves along on vector reallocation) —
                // avoids use-after-free from storing the pointer in netKilledEnemies.
                closest->netKilled = true;
            }
        }

        // Process incoming chat messages from network
        auto incomingChats = net.drainChats();
        for (const auto& ch : incomingChats) {
            uint32_t senderId = ch.first;
            const std::string& text = ch.second;
            activeChats[senderId] = { text, 4.0f };
        }
    }

    // Animais / health selvagem
    updateAnimals(dt);
    updateCityFolk(dt);   // civis perambulando pela city

    // Spawn enemies — WITH A LIMIT only it does not accumulate endlessly (perf + stability).
    // O cap scale um little with the difficulty; bosses/minions still can somar.
    {
        const int baseCap   = 55;
        const int diffBonus  = (int)difficulty * 12;   // Historia 0 .. Apocalipse +48
        const int enemyCap   = baseCap + diffBonus + threatLevel * 3; // more ameaca = more enemies
        spawnTimer += dt;
        // "Total Invasion" mutator speeds up spawn
        float dayNight = 0.62f + 0.38f * worldSun;   // night: intervalo menor = more enemies
        float effectiveInterval = spawnInterval * mutatorSpawnMult() * dayNight;
        if (spawnTimer >= effectiveInterval) {
            // SAFE ZONE: does not spawn enemies while the player is in the refuge
            if ((int)enemies.size() < enemyCap && !inSafeZone(player.position))
                spawnEnemy();
            spawnTimer = 0.0f;
        }
    }

    // Boss also not surge inside the zone segura.
    // In BOSS PHASE the trigger is the phase quota: kill the quota -> the boss appears
    // -> only then the portal opens. Gives beginning, middle and end to the phase.
    bool bossCue = openWorldMode
        ? (owBossPhase && owPhaseKills >= owPhaseGoal)
        : (enemiesKilled >= bossSpawnThreshold);
    if (bossCue && !bossSpawned && !inSafeZone(player.position)) {
        spawnBoss();
        bossSpawned = true;
        if (openWorldMode)
            showStoryBanner("O BOSS APARECEU", "Defeat it to open the portal.", 4.5f);
    }

    // Auto-save
    saveTimer += dt;
    if (saveTimer >= 30.0f) { autoSave(); saveTimer = 0.0f; }

    // Update enemies and collect shooting requests
    int enemyIdx = 0;
    for (auto& enemy : enemies) {
        Vector2 prevPos = enemy.position;
        // Tactical approach point: far from the player the enemy goes to the flank
        // ou corta the rear; up close, receives the REAL position (otherwise the mira and the
        // telegrafo of attack apontariam for the lugar wrong).
        Vector2 aim = enemy.isBoss()
                    ? player.position
                    : director.approachPoint(enemyIdx++, enemy.position, player.position);
        enemy.update(dt, aim);

        // Collision with walls — enemies in the longer pass through walls.
        // Slides along the wall (axis separation) instead of stopping dead.
        // Flying/supernatural bosses ignore (pass through on purpose).
        bool ghostly = (enemy.type == EnemyType::Ghost ||
                        enemy.type == EnemyType::GhostElite ||
                        enemy.type == EnemyType::ShadowWraith ||
                        enemy.type == EnemyType::BansheeHowler ||
                        enemy.type == EnemyType::PoltergeistBoss);
        auto blockedAt = [&](Vector2 p) {
            // Wall of TILE (grid of the tilemap) OU struct of CHUNK (circle physical
            // of the structures geradas in the infinito). Antes the enemies only barravam
            // on the tile wall: andavam ATRAVESSADOS inside the building of chunk.
            if (tilemap.isWallAtPosition(p)) return true;
            for (const auto& s : m_chunkSolids) {
                float dx = p.x - s.x, dy = p.y - s.y;
                if (dx*dx + dy*dy < s.z * s.z) return true;
            }
            return false;
        };
        if (!ghostly && blockedAt(enemy.position)) {
            Vector2 tryX = {enemy.position.x, prevPos.y};
            Vector2 tryY = {prevPos.x, enemy.position.y};
            if      (!blockedAt(tryX)) enemy.position = tryX;
            else if (!blockedAt(tryY)) enemy.position = tryY;
            else                      enemy.position = prevPos;
        }

        // SAFE ZONE: enemy that enters the refuge is pushed outside (retreats).
        // O player stays safe same if for chased until the base.
        if (inSafeZone(enemy.position)) {
            Vector2 away = { enemy.position.x - safeZoneCenter.x,
                             enemy.position.y - safeZoneCenter.y };
            float len = std::sqrt(away.x*away.x + away.y*away.y);
            if (len < 1.0f) { away = {1.0f, 0.0f}; len = 1.0f; }
            // Empurra strong; if entered MUITO fundo, plays direct to edge (not stays perseguindo).
            float push = enemy.speed * 4.0f * dt + 90.0f * dt;
            enemy.position.x += (away.x / len) * push;
            enemy.position.y += (away.y / len) * push;
            if (len < safeZoneRadius - 200.0f) {
                enemy.position.x = safeZoneCenter.x + (away.x / len) * (safeZoneRadius + 30.0f);
                enemy.position.y = safeZoneCenter.y + (away.y / len) * (safeZoneRadius + 30.0f);
            }
        }

        // Auto-evolution notification
        if (enemy.justEvolved) {
            enemy.justEvolved = false;
            if (Vector2Distance(enemy.position, player.position) < 420.0f) {
                const char* evolMsgs[] = {
                    "Enemy evoluiu — cuidado!",
                    "Ameaca escalando!",
                    "Enemy stayed more strong!",
                    "Evolution detectada — atencao!"
                };
                triggerPlayerSpeech(evolMsgs[GetRandomValue(0, 3)], 2.0f);
                triggerShake(3.0f, 0.18f);
            }
        }

        // Contact damage
        if (!player.isShielded()) {
            float dmg = enemy.attackIfReady(dt, player.position);
            if (dmg > 0.0f) {
                player.takeDamage(dmg);
                audio.playPlayerHurt();
                noteHurtDir(enemy.position);
                // Mutador LUA DE BLOOD: the enemy heals when hitting you
                if (mutatorBloodMoon())
                    enemy.health = std::min(enemy.maxHealth, enemy.health + dmg * 0.5f);
                hitFlashTimer = 0.25f;
                if (enemy.isElite || enemy.isBoss())
                    eliteFlashTimer = std::max(eliteFlashTimer, 0.35f);
                triggerShake(5.0f, 0.18f);
                botController.damageEvents++;
                botController.totalDmgTaken += dmg;
                // Low HP warning speech
                float hpPct = player.health / player.maxHealth;
                if (hpPct < 0.20f && playerSpeechTimer <= 0.0f) {
                    triggerPlayerSpeech("ALERT: Integridade critica. Recuando!", 3.0f);
                } else if (hpPct < 0.40f && playerSpeechTimer <= 0.0f
                           && GetRandomValue(0,3) == 0) {
                    triggerPlayerSpeech("Damage severo detectado.", 2.5f);
                }
            }
        }

        // Enemy shoots
        if (enemy.wantsToShoot) {
            enemyProjectiles.emplace_back(enemy.position, enemy.shootDirection,
                                          enemy.shootDamage, enemy.shootSpeed,
                                          enemy.projectileColor);
        }

        // Kamikaze / Zergling explosion
        if (enemy.wantsToExplode && !enemy.isDead()) {
            bool isZergling = (enemy.type == EnemyType::Zergling);
            float explodeRadius = isZergling ? 60.0f : 110.0f;
            float dist = Vector2Distance(enemy.position, player.position);
            if (dist <= explodeRadius && !player.isShielded()) {
                float falloff = 1.0f - dist / explodeRadius;
                float dmg = isZergling ? 35.0f : enemy.damage;
                audio.playPlayerHurt();
                player.takeDamage(dmg * falloff);
                noteHurtDir(enemy.position);
                hitFlashTimer = 0.35f;
                triggerShake(isZergling ? 5.0f : 9.0f, 0.22f);
            }
            for (auto& other : enemies) {
                if (&other == &enemy) continue;
                if (Vector2Distance(enemy.position, other.position) <= explodeRadius)
                    other.takeDamage(enemy.damage * 0.6f);
            }
            if (isZergling) {
                particles.spawnExplosion(enemy.position, {0,200,50,255}, 18);
                particles.spawnExplosion(enemy.position, {100,255,80,255}, 8);
            } else {
                particles.spawnExplosion(enemy.position, {255,120,0,255}, 25);
                particles.spawnExplosion(enemy.position, {255,220,80,255}, 12);
            }
            enemy.takeDamage(9999.0f);
            enemy.wantsToExplode = false;
        }
    }

    // Separation steering with GRID ESPACIAL — evita O(N^2) in Threat high.
    // So compara enemies in the same celula and in the 8 adjacentes.
    {
        const float CELL = 64.0f;
        std::unordered_map<long long, std::vector<int>> grid;
        grid.reserve(enemies.size() * 2);
        auto key = [](int cx, int cy) {
            return ((long long)cx << 32) ^ (long long)(unsigned int)cy;
        };
        for (int i = 0; i < (int)enemies.size(); ++i)
            grid[key((int)(enemies[i].position.x / CELL),
                     (int)(enemies[i].position.y / CELL))].push_back(i);

        for (int i = 0; i < (int)enemies.size(); ++i) {
            int cx = (int)(enemies[i].position.x / CELL);
            int cy = (int)(enemies[i].position.y / CELL);
            for (int ox = -1; ox <= 1; ++ox)
            for (int oy = -1; oy <= 1; ++oy) {
                auto it = grid.find(key(cx+ox, cy+oy));
                if (it == grid.end()) continue;
                for (int j : it->second) {
                    if (j <= i) continue;   // cada par only uma vez
                    float dx = enemies[i].position.x - enemies[j].position.x;
                    float dy = enemies[i].position.y - enemies[j].position.y;
                    float minDist = enemies[i].radius + enemies[j].radius + 4.0f;
                    float d2 = dx*dx + dy*dy;
                    if (d2 < minDist*minDist && d2 > 0.0001f) {
                        float d = std::sqrt(d2);
                        float push = (minDist - d) * 0.5f;
                        float nx = dx / d, ny = dy / d;
                        enemies[i].position.x += nx * push;
                        enemies[i].position.y += ny * push;
                        enemies[j].position.x -= nx * push;
                        enemies[j].position.y -= ny * push;
                    }
                }
            }
        }
    }

    // Group alert: if any enemy took damage, alert nearby (not-yet-alerted) allies.
    // Pular allies already alertados evita trabalho O(n^2) redundante all frame.
    for (auto& hit : enemies) {
        if (hit.hitFlashTimer > 0.05f) {
            for (auto& ally : enemies) {
                if (&ally != &hit && !ally.alerted) {
                    float d = Vector2Distance(hit.position, ally.position);
                    if (d < 200.0f) ally.alert();
                }
            }
        }
    }

    // Building system update
    {
        std::vector<Enemy*> enemyPtrs;
        enemyPtrs.reserve(enemies.size());
        for (auto& and : enemies) enemyPtrs.push_back(&and);
        buildingSystem.update(dt, player.position, &enemyProjectiles, enemyPtrs);

        // Collect building-generated resources
        int genCredits  = buildingSystem.collectCredits();
        int genMaterials = buildingSystem.collectMaterials();
        if (genCredits  > 0) {
            player.credits += genCredits;
            totalCreditsEarned += genCredits;
            achievements.onCreditsEarned(totalCreditsEarned);
            damageNumbers.push_back({player.position, (float)genCredits,  {255,220,0,255}, 1.2f, "$"});
        }
        if (genMaterials > 0) materialMetal   += genMaterials;

        // Building heals
        buildingSystem.healPlayerIfNear(player.position, player.health, player.maxHealth);

        // Spawn friend projectiles
        for (auto& shot : buildingSystem.pendingShots) {
            projectiles.emplace_back(shot.origin, shot.dir, shot.damage, shot.speed, 300.f, shot.color);
            particles.spawnHit(shot.origin, Color{255,220,120,255}, 5); // muzzle flash
        }
    }

    // ── Anomaly Portal System ─────────────────────────────────────────────────
    if (!anomalySystem.hasActiveWave()) {
        anomalyWaveTimer += dt;
        if (anomalyWaveTimer >= anomalyWaveCooldown) {
            anomalyWaveTimer = 0.0f;
            int mapW = tilemap.width  * Tilemap::tileSize;
            int mapH = tilemap.height * Tilemap::tileSize;
            if (openWorldMode) {
                anomalySystem.spawnWave(mapW, mapH, player.position, safeZoneCenter, owPhaseRadius);
            } else {
                anomalySystem.spawnWave(mapW, mapH, player.position);
            }
            triggerPlayerSpeech("Anomalias detectadas! Feche the portals!", 3.5f);
            showStoryBanner("!! ANOMALIA DETECTADA !!", "Feche all the portals to continue");
        }
    }
    anomalySystem.update(dt, player.position);

    // Spawn enemies from portals
    {
        int portalEnemyType; Vector2 portalSpawnPos;
        auto isFree = [this](Vector2 pos) { return !isBlocked(pos); };
        if (anomalySystem.pollSpawn(portalEnemyType, portalSpawnPos, isFree)) {
            enemies.emplace_back(portalSpawnPos, (EnemyType)portalEnemyType);
        }
    }

    // Player projectiles vs portals
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        if (anomalySystem.checkProjectileHit(proj.position, proj.radius, proj.damage)) {
            proj.active = false;
        }
    }

    // All portals closed — reward
    if (anomalySystem.waveActive && anomalySystem.countOpen() == 0) {
        anomalySystem.waveActive = false;
        totalPortalsClosed++;
        achievements.onPortalClosed(totalPortalsClosed);
        player.credits += 500;
        totalCreditsEarned += 500;
        achievements.onCreditsEarned(totalCreditsEarned);
        player.addXP(2500);
        triggerPlayerSpeech("All anomalias fechadas! Zone segura.", 3.5f);

        // Quest tracking: ClosePortal avanca to the close uma wave of portals.
        for (auto& q : quests) {
            if (!q.completed && q.active && q.type == QuestType::ClosePortal) {
                q.updateProgress(1);
                if (q.isComplete() && !q.rewardGiven) grantQuestRewards(q);
            }
        }
    }
    // ──────────────────────────────────────────────────────────────────────────

    updateProjectiles(dt);
    updateEnemyProjectiles(dt);
    updateItems(dt);
    updateXPOrbs(dt);
    checkCollisions();
    drainLevelUps();      // credita the levels ganhos neste frame (qualquer fonte)
    // A IA aprende with ESTE frame: distance, movimentacao, ritmo of abate and damage
    // sofrido alimentam the diretor, that responde in the spawn and in the tactic.
    director.observe(dt, player.position, player.health, player.maxHealth,
                     enemiesKilled, (int)enemies.size());
    updatePhasePortal(dt);
    // ── PHASE LIMIT ───────────────────────────────────────────────────────
    if (openWorldMode && owFadeTimer <= 0.0f) {
        Vector2 d = { player.position.x - safeZoneCenter.x, player.position.y - safeZoneCenter.y };
        float dl = sqrtf(d.x*d.x + d.y*d.y);
        if (dl > owPhaseRadius) {
            player.position.x = safeZoneCenter.x + d.x / dl * owPhaseRadius;
            player.position.y = safeZoneCenter.y + d.y / dl * owPhaseRadius;
            if (borderWarnTimer <= 0.0f) {
                borderWarnTimer = 2.0f;
                triggerPlayerSpeech("Barrier of KRONOS. Not of the to go alem daqui.", 2.5f);
            }
        }
        if (borderWarnTimer > 0.0f) borderWarnTimer -= dt;
    }
    checkPortalTransition();

    // Grito of "estou morrendo" when the health stays critica (before die)
    {
        float hpPct = player.health / player.maxHealth;
        if (hpPct > 0.0f && hpPct < 0.18f) {
            dyingCryCooldown -= dt;
            if (dyingCryCooldown <= 0.0f) {
                dyingCryCooldown = 4.0f;
                audio.playPlayerDeath();
                triggerPlayerSpeech("ESTOU MORRENDO! ME AJUDE!", 3.0f);
            }
        } else if (hpPct >= 0.30f) {
            dyingCryCooldown = 0.0f; // recuperou — can gritar of new if fall
        }
    }

    // Player death - respawn (in the Arca, if houver uma built)
    if (player.health <= 0.0f) {
        audio.playPlayerDeath();
        triggerPlayerSpeech("NOT... not acabou still!", 3.0f);
        totalDeaths++;
        achievements.onDeathCount(totalDeaths);
        enemies.clear();
        items.clear();
        projectiles.clear();
        enemyProjectiles.clear();
        xpOrbs.clear();
        particles.spawnExplosion(player.position, BLUE, 30);

        Vector2 arkPos;
        if (buildingSystem.getArkPosition(arkPos)) {
            // Renasce in the Arca with more health (75%) — the Arca and your point of return
            player.position = arkPos;
            player.health   = player.maxHealth * 0.75f;
            triggerPlayerSpeech("Renascido in the Arca. De returns the luta.", 3.0f);
        } else {
            // Without Ark: respawns in the SAFE ZONE (initial refuge) with 60%
            player.health  = player.maxHealth * 0.6f;
            player.position = safeZoneCenter;
            triggerPlayerSpeech("De returns the base segura. Recover and prepare.", 3.5f);
        }
    }

    // Process dead enemies
    nearNpcIndex = -1;
    std::vector<Enemy> splitSpawns;
    bool pendingOmega = false;   // spawn of the Omega adiado p/ DEPOIS of the loop (push in the loop invalidaria `it`)

    for (auto it = enemies.begin(); it != enemies.end();) {
        if (it->isDead()) {
            if (it->shouldDropLoot()) {
                it->markLootDropped();

                // Death by sync of network (flag in the enemy) → not rebroadcastar.
                if (it->netKilled) {
                    // already tratada pela network; nada the send
                } else {
                    if (netActive) {
                        uint32_t cx = (uint32_t)(it->position.x / 10.0f) & 0xFFFF;
                        uint32_t cy = (uint32_t)(it->position.y / 10.0f) & 0xFFFF;
                        uint32_t compId = (cx << 16) | cy;
                        net.sendEnemyDeath(compId);
                    }
                }

                playEnemyDeathSound(*it);   // sound of death by faction/type

                // "POP" of death (juice): burst colorido in scale; elite/boss
                // congelam the world and tremem the camera — reward by abate.
                {
                    int burstCount = it->isBoss() ? 46 : it->isElite ? 24 : 12;
                    Color kCol = (it->isElite || it->isBoss()) ? Color{255,200,80,255} : it->bodyColor;
                    particles.spawnExplosion(it->position, kCol, burstCount);
                    particles.spawnHit(it->position, WHITE, it->isBoss() ? 16 : 6);
                    if (it->isBoss() || it->isElite) {
                        hitStopTimer = it->isBoss() ? 0.10f : 0.06f;
                        triggerShake(it->isBoss() ? 7.0f : 4.5f,
                                     it->isBoss() ? 0.30f : 0.14f);
                        camPunch = std::max(camPunch, it->isBoss() ? 0.07f : 0.045f);
                    }
                }
                // Floor decal: blood (organics) or scorched (machines)
                {
                    using ET = EnemyType;
                    bool organic = (it->type==ET::Zergling||it->type==ET::Hydra||it->type==ET::Broodmother||
                                    it->type==ET::Zombie||it->type==ET::ZombieRager||it->type==ET::ZombieHorde||
                                    it->type==ET::AcidSpitter||it->type==ET::AbyssalEel||it->type==ET::MorphX||
                                    it->type==ET::CrimsonBat||it->type==ET::ChaosSpawn||it->type==ET::NeuralParasite);
                    float sz = it->isBoss() ? 28.0f : 12.0f + it->radius * 0.4f;
                    if (organic) addDecal(it->position, Color{120,20,18,255}, 0, sz);
                    else         addDecal(it->position, Color{30,28,26,255}, 1, sz);
                }

                // XP orb — scales by elite/boss status, evolTier, and difficulty
                int xpAmt = it->isElite ? it->xpReward * 2 :
                            (it->type == EnemyType::Boss) ? it->xpReward * 3 : it->xpReward;
                float tierMult = 1.0f + it->evolTier * 0.75f; // Legendary = 3.25x
                xpAmt = (int)(xpAmt * tierMult * getDifficulty().xpMult * mutatorDropMult());
                xpOrbs.emplace_back(it->position, xpAmt);

                // Credits drop — every kill drops some credits
                int credAmt = 0;
                switch (it->type) {
                    case EnemyType::Boss:        credAmt = GetRandomValue(150, 300); break;
                    case EnemyType::Tank:        credAmt = GetRandomValue(30, 60);   break;
                    case EnemyType::Shooter:     credAmt = GetRandomValue(20, 45);   break;
                    case EnemyType::MorphX:       credAmt = GetRandomValue(40, 80);   break;
                    case EnemyType::HunterDrone:     credAmt = GetRandomValue(35, 70);   break;
                    case EnemyType::KronosSentry:credAmt = GetRandomValue(25, 50);   break;
                    case EnemyType::Sniper:      credAmt = GetRandomValue(25, 55);   break;
                    case EnemyType::Kamikaze:    credAmt = GetRandomValue(10, 25);   break;
                    default:                     credAmt = GetRandomValue(8, 20);    break;
                }
                if (it->isElite) credAmt = (int)(credAmt * 2.5f);
                credAmt = (int)(credAmt * getDifficulty().creditMult * mutatorDropMult());
                if (credAmt > 0) {
                    // Scatter credits in the small arc only they're visible
                    int numCoins = std::min(credAmt / 10 + 1, 5);
                    int coinAmt  = credAmt / numCoins;
                    for (int ci = 0; ci < numCoins; ++ci) {
                        float ang = (float)GetRandomValue(0, 628) / 100.0f;
                        float rad = (float)GetRandomValue(20, 55);
                        Vector2 cp = {it->position.x + std::cos(ang)*rad,
                                      it->position.y + std::sin(ang)*rad};
                        Item coin = Item::createCredits(cp, coinAmt);
                        coin.pickupDelay = 0.4f + ci * 0.05f;
                        items.push_back(coin);
                    }
                }

                // ── ABSORCAO DE POWER (estilo V Rising): kill BOSS = buff PERMANENTE ──
                if (it->isBoss()) {
                    bossPowersAbsorbed++;
                    totalBossesKilled++;
                    achievements.onBossKilled(totalBossesKilled);
                    int kind = bossPowersAbsorbed % 4;
                    const char* pname = (kind==0) ? "+10% Health Maxima" : (kind==1) ? "+10% Damage"
                                      : (kind==2) ? "+4% Defense" : "+6% Speed";
                    player.absorbBossEssence(kind);   // buff PERMANENTE (mexe in the base + recalcula)
                    showStoryBanner("POWER ABSORVIDO",
                        TextFormat("Essencia of the boss: %s   (total: %d)", pname, bossPowersAbsorbed), 3.5f);
                    triggerShake(6.0f, 0.4f);
                }

                // Loot drop — elites always drop, others scaled by difficulty drop chance
                int dropRoll = GetRandomValue(0, 100);
                int dropThresh = std::min((int)(55 * getDifficulty().dropChanceMult), 95);
                if (it->isElite || dropRoll < dropThresh) {
                    float ang = (float)GetRandomValue(0, 628) / 100.0f;
                    Vector2 dp = {it->position.x + std::cos(ang)*35.0f,
                                  it->position.y + std::sin(ang)*35.0f};
                    Item drop = Item::createRandom(dp);
                    drop.pickupDelay = 0.5f;
                    bool fromOmega = (it->type == EnemyType::OmegaBoss);
                    drop.rarity = Item::rollRarity(fromOmega);
                    if ((int)difficulty >= (int)DifficultyLevel::Guerreiro) {
                        ItemRarity r2 = Item::rollRarity(fromOmega);
                        if ((int)r2 > (int)drop.rarity) drop.rarity = r2;
                    }
                    drop.applyRarityBonus();
                    drop.isNew = true;
                    drop.dropBeamTimer = (drop.rarity >= ItemRarity::Rare) ? 4.0f : 0.0f;
                    if (drop.rarity >= ItemRarity::Legendary && playerSpeechTimer <= 0.5f)
                        triggerPlayerSpeech("Item LENDARIO detectado!", 3.0f);
                    else if (drop.rarity == ItemRarity::Epic && playerSpeechTimer <= 0.5f)
                        triggerPlayerSpeech("Item Epico found!", 2.0f);
                    items.push_back(drop);
                }
                // Tech chip drop — 20% base, 60% from bosses, scaled by difficulty
                int techChanceBase = (it->type == EnemyType::Boss) ? 60 :
                                     (it->isElite)                 ? 45 : 20;
                int techChance = std::min((int)(techChanceBase * getDifficulty().dropChanceMult), 95);
                if (GetRandomValue(0, 100) < techChance) {
                    float ang = (float)GetRandomValue(0, 628) / 100.0f;
                    Vector2 tp = {it->position.x + std::cos(ang)*45.0f,
                                  it->position.y + std::sin(ang)*45.0f};
                    Item tech = Item::createTech(tp);
                    tech.pickupDelay = 0.6f;
                    items.push_back(tech);
                }

                // ── Material drops for Crafting System ───────────────────────
                auto spawnMaterial = [&](ItemType mtype, const char* mname, Color mcol) {
                    float ang = (float)GetRandomValue(0, 628) / 100.0f;
                    float rad = (float)GetRandomValue(25, 55);
                    Vector2 mp = {it->position.x + std::cos(ang)*rad,
                                  it->position.y + std::sin(ang)*rad};
                    Item mat;
                    mat.position    = mp;
                    mat.type        = mtype;
                    mat.name        = mname;
                    mat.color       = mcol;
                    mat.radius      = 7.f;
                    mat.lifetime    = 40.f;
                    mat.pickupDelay = 0.4f;
                    mat.rarity      = ItemRarity::Uncommon;
                    items.push_back(mat);
                };

                switch (it->type) {
                    case EnemyType::Scout:
                    case EnemyType::Tank:
                    case EnemyType::Shooter:
                    case EnemyType::KronosSentry:
                        // MetalScrap — 40%
                        if (GetRandomValue(0, 99) < 40)
                            spawnMaterial(ItemType::MetalScrap, "Scrap Metal", {180,180,180,255});
                        break;
                    case EnemyType::Zergling:
                    case EnemyType::Hydra:
                    case EnemyType::Broodmother:
                        // AlienCarapace — 50%
                        if (GetRandomValue(0, 99) < 50)
                            spawnMaterial(ItemType::AlienCarapace, "Carapaca Alien", {60,255,80,255});
                        break;
                    case EnemyType::HunterDrone:
                        // PlasmaCore — 35%
                        if (GetRandomValue(0, 99) < 35)
                            spawnMaterial(ItemType::PlasmaCore, "Core Plasma", {0,180,255,255});
                        break;
                    case EnemyType::MorphX:
                        // NanoFiber — 45% (T-1000 analogue)
                        if (GetRandomValue(0, 99) < 45)
                            spawnMaterial(ItemType::NanoFiber, "Fibra Nano", {0,220,200,255});
                        break;
                    case EnemyType::OmegaBoss:
                        // OmegaEssence — 100% garantido
                        spawnMaterial(ItemType::OmegaEssence, "Essencia Omega", {255,215,0,255});
                        break;
                    default:
                        break;
                }
                // ─────────────────────────────────────────────────────────────

                // Elite-exclusive: drop equipment on ground (player must walk to E to equip)
                if (it->isElite && GetRandomValue(0, 100) < 70) {
                    int tier = (it->eliteMod == 1) ? 2 : GetRandomValue(1, 2);
                    Equipment eq;
                    int roll = GetRandomValue(0, 5);
                    switch (tier) {
                        case 1: eq = (roll < 3) ? EDB::submetMilitar() : EDB::chipVel(); break;
                        case 2: eq = (roll < 2) ? EDB::rifleEnergia()  :
                                     (roll < 4) ? EDB::armaduraAvan()  : EDB::neuralLink(); break;
                        default: eq = EDB::canhaoEMP(); break;
                    }
                    if (!eq.isEmpty()) {
                        float ang = (float)GetRandomValue(0, 628) / 100.0f;
                        GroundEquipment ge;
                        ge.position = {it->position.x + std::cos(ang)*50.0f,
                                       it->position.y + std::sin(ang)*50.0f};
                        ge.equip    = eq;
                        groundEquips.push_back(ge);
                    }
                    items.push_back(Item::createEliteDrop(it->position));
                    particles.spawnLevelUp(it->position);
                }

                enemiesKilled++;
                totalKills++;
                botController.killCount++;
                achievements.onKill(totalKills);
                totalKillsEver++;

                // VICTORY — the Core KRONOS went destruido
                if (it->isFinalBoss) {
                    finalBossAlive = false;
                    gameWon        = true;
                    victoryTimer   = 0.0f;
                    triggerShake(20.0f, 1.0f);
                    triggerPlayerSpeech("Acabou... the humanidade is livre.", 6.0f);
                    audio.playLevelUp();
                }

                // Omega Boss trigger every 50 kills (disabled after the victory)
                if (!gameWon && totalKills >= omegaKillThreshold) {
                    omegaKillThreshold += 50;
                    pendingOmega = true;   // spawn DEPOIS of the loop of deaths (push here invalidaria `it`)
                }

                // First kill speech
                if (!firstKillTriggered && playerSpeechTimer <= 0.3f) {
                    firstKillTriggered = true;
                    static const char* fkl[] = { "Primeiro of many.", "NEXUS: 1. KRONOS: 0.", "Isso and by Lyra." };
                    triggerPlayerSpeech(fkl[GetRandomValue(0,2)], 2.5f);
                }
                // Player combat commentary (half-human reactions)
                if (enemiesKilled % 5 == 0 && playerSpeechTimer <= 0.5f) {
                    static const char* killLines[] = {
                        "Unidade neutralizada.", "Target eliminado.",
                        "Sistema of combat eficiente.", "Ameaca suprimida.",
                        "Protocolo of neutralizacao completed.",
                        "Meus sensores detectam more enemies.", "Continuo the quest."
                    };
                    triggerPlayerSpeech(killLines[GetRandomValue(0,6)], 3.0f);
                }
                if (it->type == EnemyType::Boss || it->type == EnemyType::AlienBoss || it->type == EnemyType::OmegaBoss) {
                    static const char* bossLines[] = { "BOSS ABATIDO. Quest cumprida.", "Um the less to humanidade.", "Era isso? Vim prepared.", "KRONOS - your time acabou." };
                    triggerPlayerSpeech(bossLines[GetRandomValue(0,3)], 5.0f);
                }

                // Shake on kill
                triggerShake(4.0f, 0.12f);

                // Quest progress — GENERICO by type (qualquer kill account in the quests
                // of caca; boss account in the of boss). Ensures that the barras enchem.
                bool isBossKill = (it->type == EnemyType::Boss ||
                                   it->type == EnemyType::AlienBoss ||
                                   it->type == EnemyType::OmegaBoss);
                for (auto& q : quests) {
                    if (q.completed || !q.active) continue;
                    if (q.type == QuestType::Kill) {
                        q.updateProgress(1);
                    } else if (q.type == QuestType::KillBoss && isBossKill) {
                        q.updateProgress(1);
                        slowMoTimer = 1.8f; // cinematic slow-mo on boss kill
                    }
                    if (q.isComplete() && !q.rewardGiven) grantQuestRewards(q);
                }

                // MorphX split
                if (it->type == EnemyType::MorphX && !it->hasSplit && !it->isMinion) {
                    it->hasSplit = true;
                    splitSpawns.emplace_back(it->position, EnemyType::MorphX, true);
                    Vector2 p2 = {it->position.x + 30, it->position.y - 20};
                    splitSpawns.emplace_back(p2, EnemyType::MorphX, true);
                }
            }

            // Volatile elite: huge AoE explosion on death
            if (it->isElite && it->eliteMod == 2) {
                float aoe = 120.0f;
                for (auto& other : enemies) {
                    if (&other != &(*it) && Vector2Distance(it->position, other.position) < aoe) {
                        other.takeDamage(it->maxHealth * 0.4f);
                    }
                }
                audio.playPlayerHurt();
                if (Vector2Distance(it->position, player.position) < aoe && !player.isShielded()) {
                    player.takeDamage(30.0f);
                    noteHurtDir(it->position);
                    hitFlashTimer = 0.35f;
                    eliteFlashTimer = std::max(eliteFlashTimer, 0.5f);
                }
                particles.spawnExplosion(it->position, {255, 0, 200, 255}, 28);
                audio.playExplosionBig();
                triggerShake(10.0f, 0.4f);
            } else {
                particles.spawnExplosion(it->position, it->isElite ?
                    Color{255,200,0,255} : it->bodyColor, it->isElite ? 18 : 10);
                audio.playExplosion();
            }

            // UndeadEnforcer ressurreicao — revive with 30% HP uma vez
            if (it->type == EnemyType::UndeadEnforcer && !it->hasRevived) {
                it->hasRevived = true;
                it->health     = it->maxHealth * 0.30f;
                particles.spawnExplosion(it->position, {160, 0, 220, 255}, 20);
                triggerShake(3.0f, 0.15f);
                ++it;
                continue;
            }

            it = enemies.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& s : splitSpawns) enemies.push_back(s);

    // Omega boss adiado: entered after the loop, without invalidar iteradores.
    if (pendingOmega) spawnOmegaBoss();

    // Check NPC proximity
    for (int i = 0; i < (int)npcs.size(); ++i) {
        if (npcs[i].isPlayerNear(player.position)) {
            nearNpcIndex = i;
            break;
        }
    }

    // Quest rewards + conclusao of quests of Zone (to the estar in the zone alvo)
    for (auto& q : quests) {
        if (!q.completed && q.active && q.type == QuestType::Zone &&
            (int)currentZone == q.target && !q.isComplete()) {
            q.updateProgress(q.target);   // chegou in the zone — completa
        }
        if (q.isComplete() && !q.rewardGiven) {
            grantQuestRewards(q);
        }
    }
}

void Game::movePlayerWithSlide(Vector2 direction, float dt) {
    float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len < 0.01f) return;
    direction.x /= len;
    direction.y /= len;

    float currentSpeed = player.speed *
                         (player.isOverloaded() ? 1.3f : 1.0f) *
                         (player.sprinting ? 1.7f : 1.0f);
    float t = 1.0f - std::exp(-24.0f * dt);
    Vector2 targetVel = {
        player.velocity.x + (direction.x * currentSpeed - player.velocity.x) * t,
        player.velocity.y + (direction.y * currentSpeed - player.velocity.y) * t
    };

    player.isMoving = true;
    player.moveRequested = true;

    // Slide X first, then Y, to allow movement along walls.
    player.position.x += targetVel.x * dt;
    if (isBlocked(player.position)) {
        player.position.x -= targetVel.x * dt;
        targetVel.x = 0.0f;
    }
    player.position.y += targetVel.y * dt;
    if (isBlocked(player.position)) {
        player.position.y -= targetVel.y * dt;
        targetVel.y = 0.0f;
    }

    player.velocity = targetVel;
    if (player.velocity.x > 12.0f) player.facing = 1;
    if (player.velocity.x < -12.0f) player.facing = -1;
}

void Game::handleInput(float dt) {
    if (paused || inMainMenu) return;

    // ── Chat absorbs all input when active ─────────────────────────────────────
    if (chatActive) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            chatActive = false;
            chatInput.clear();
            return;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (!chatInput.empty()) {
                if (netActive) {
                    net.sendChat(chatInput);
                }
                triggerPlayerSpeech(chatInput, 4.0f);
            }
            chatActive = false;
            chatInput.clear();
            return;
        }

        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (chatInput.size() < 64)) {
                chatInput.push_back((char)key);
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && !chatInput.empty()) {
            chatInput.pop_back();
        }
        return;
    }

    if (IsKeyPressed(KEY_ENTER) && !craftingSystem.open && !shopSystem.open && !showInventory) {
        chatActive = true;
        chatInput.clear();
        return;
    }

    // ── Crafting system absorbs all input when open ───────────────────────────
    if (craftingSystem.open) {
        if (IsKeyPressed(KEY_ESCAPE)) { craftingSystem.open = false; return; }
        if (IsKeyPressed(KEY_UP))   craftingSystem.selected = (craftingSystem.selected - 1 + (int)craftingSystem.recipes.size()) % (int)craftingSystem.recipes.size();
        if (IsKeyPressed(KEY_DOWN)) craftingSystem.selected = (craftingSystem.selected + 1) % (int)craftingSystem.recipes.size();
        Equipment outEquip;
        Item      outItem;
        bool      gotEquip = false;
        bool      crafted  = false;
        if (IsKeyPressed(KEY_ENTER) && !craftingSystem.crafting)
            crafted = craftingSystem.tryCraft(player.inventory, outEquip, outItem, gotEquip);
        // Mouse: hover/click in the receitas and button CRAFTAR
        {
            Vector2 vm = virtualizeMousePos(GetMousePosition());
            bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            bool closed = false;
            if (craftingSystem.handleMouse(vm, click, player.inventory, screenWidth, screenHeight,
                                           outEquip, outItem, gotEquip, closed))
                crafted = true;
            if (closed) { craftingSystem.open = false; return; }
        }
        if (crafted) {
            if (gotEquip) {
                player.equipItem(outEquip);
                particles.spawnLevelUp(player.position);
                audio.playLevelUp();
            } else {
                outItem.position   = player.position;
                outItem.pickupDelay = 0.0f;
                outItem.lifetime   = 60.0f;
                items.push_back(outItem);
                audio.playPickup();
            }
        }
        return;
    }

    // ── Shop absorbs all input when open ─────────────────────────────────────
    if (shopSystem.open) {
        shopSystem.handleInput();
        Equipment outEquip;
        Item      outItem;
        bool      gotEquip    = false;
        bool      gotCosmetic = false;
        Color     cosmeticCol = WHITE;
        bool      bought      = false;
        // Purchase by TECLADO (ENTER) ...
        if (IsKeyPressed(KEY_ENTER)) {
            bought = shopSystem.tryBuy(player.credits, outEquip, outItem,
                                       gotEquip, gotCosmetic, cosmeticCol);
        }
        // ... ou by MOUSE (hover/click in the botoes)
        {
            Vector2 vm = virtualizeMousePos(GetMousePosition());
            bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            bool closed = false;
            if (shopSystem.handleMouse(vm, click, screenWidth, screenHeight, player.credits,
                                       outEquip, outItem, gotEquip, gotCosmetic, cosmeticCol, closed))
                bought = true;
            if (closed) { shopSystem.close(); return; }
        }
        if (bought) {
            if (gotEquip) {
                player.equipItem(outEquip);
                particles.spawnLevelUp(player.position);
                audio.playLevelUp();
            } else if (gotCosmetic) {
                particles.spawnLevelUp(player.position);
                audio.playPickup();
            } else {
                outItem.position   = player.position;
                outItem.pickupDelay = 0.0f;
                outItem.lifetime   = 60.0f;
                items.push_back(outItem);
                audio.playPickup();
            }
        }
        return;
    }

    // ── Inventory absorbs 1/2/3/U when open ──────────────────────────────────
    if (showInventory) {
        player.handleInventoryInput();
        // Mouse: click selects/equipa/usa; button X closes.
        bool lc = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool rc = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if (lc || rc) {
            Vector2 vm = virtualizeMousePos(GetMousePosition());
            if (player.handleInventoryMouse(vm, lc, rc)) showInventory = false;
        }
        if (IsKeyPressed(KEY_I)) showInventory = false;
        return;
    }

    // Mouse in the world: raycast in the plano of the floor 3D (pipeline 2.5D).
    Vector2 mouseWorld = mouseGround3D();

    // ── Bot controller decisions ──────────────────────────────────────────────
    updateBotControl(dt);
    // shouldQuit of the bot (autotest) pedia return immediate of the handleInput —
    // quitRequested only and setado there inside, entao the early-return and equivalente.
    if (quitRequested) return;

    // ── Click numa factory/barracks = produzir unit in the hour ────────────────
    bool producedThisClick = false;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !dialogOpen &&
        !buildingSystem.buildModeActive) {
        int r = buildingSystem.clickProduce(mouseWorld, player.credits);
        if (r == 1) {
            triggerPlayerSpeech("Unidade in production!", 1.5f);
            audio.playPickup();
            producedThisClick = true;
        } else if (r == 2) {
            triggerPlayerSpeech("Credits insuficientes.", 1.5f);
            producedThisClick = true;
        } else if (r == 3) {
            triggerPlayerSpeech("Limit of unidades atingido.", 1.5f);
            producedThisClick = true;
        }
    }

    // ── Clicar num NPC to conversar (selecao by click) ────────────────────
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !dialogOpen &&
        !buildingSystem.buildModeActive && !producedThisClick) {
        // Testa the click contra the NPC PROJETADO NA SCREEN (the voxel model is
        // high and aparece above of the "feet"; the floor sob the cursor falls behind of the NPC).
        Vector2 cs = virtualizeMousePos(GetMousePosition());
        for (int i = 0; i < (int)npcs.size(); ++i) {
            Vector2 ns = GetWorldToScreenEx({ npcs[i].position.x, 28.0f, npcs[i].position.y },
                                            camera3D, screenWidth, screenHeight);
            bool hit = Vector2Distance(cs, ns) <= 44.0f;   // tolerance in pixels (body)
            if (hit) {
                if (Vector2Distance(player.position, npcs[i].position) <= 160.0f) {
                    nearNpcIndex = i;
                    dialogOpen   = true;
                    dialogLine   = 0;
                    producedThisClick = true; // not move the player neste click
                } else {
                    triggerPlayerSpeech("Preciso chegar more near to conversar.", 2.0f);
                }
                break;
            }
        }
    }

    // ── Selecao RTS by arrasto of the mouse esquerdo ─────────────────────────────
    // ── Selecao RTS: SO with SHIFT segurado (esquerdo sozinho = andar) ─────────
    // Assim hold the esquerdo to CAMINHAR never draws caixa of selecao.
    bool selectMod = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (!buildingSystem.buildModeActive && !dialogOpen) {
        if (selectMod && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !producedThisClick) {
            rtsDragStart = mouseWorld;
            rtsDragCur   = mouseWorld;
            rtsDragging  = true;   // enters in modo selecao imediatamente
        }
        if (rtsDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            rtsDragCur = mouseWorld;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && rtsDragging) {
            Rectangle box = { rtsDragStart.x, rtsDragStart.y,
                              rtsDragCur.x - rtsDragStart.x,
                              rtsDragCur.y - rtsDragStart.y };
            int sel = buildingSystem.selectUnitsInBox(box);
            rtsHasUnits = (sel > 0);
            if (sel > 0) triggerPlayerSpeech(TextFormat("%d unit(s) selected(s)", sel), 1.5f);
            rtsDragging = false;
        }
        // Se release SHIFT in the middle of the arrasto, cancela the selecao (returns the andar)
        if (rtsDragging && !selectMod) rtsDragging = false;

        // Button DIREITO = ordem of move the unidades selecionadas
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && rtsHasUnits) {
            buildingSystem.orderMove(mouseWorld);
        }
    }

    // ── Click-to-move (Diablo) — esquerdo sozinho SEMPRE anda (without marcar) ────
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !dialogOpen && !rtsDragging) {
        moveTarget = mouseWorld;
        hasTarget  = true;
        tutorial.onPlayerMoved();
    }

    // CORRER (hold SHIFT) and PULAR (ESPACO) — pulo cruza obstaculos baixos
    player.sprinting = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (IsKeyPressed(KEY_SPACE)) { player.startJump(); audio.playFootstep(); }
    // During the pulo the collision with wall and relaxada (passes by up)
    bool airborne = player.isJumping && player.jumpZ > 8.0f;

    // WASD also sets move direction (alternative control)
    {
        Vector2 wasd = {0, 0};
        if (IsKeyDown(KEY_W)) wasd.y -= 1.0f;
        if (IsKeyDown(KEY_S)) wasd.y += 1.0f;
        if (IsKeyDown(KEY_A)) wasd.x -= 1.0f;
        if (IsKeyDown(KEY_D)) wasd.x += 1.0f;
        float wlen = std::sqrt(wasd.x*wasd.x + wasd.y*wasd.y);
        if (wlen > 0.0f) {
            if (airborne) {
                player.move(wasd, dt);
            } else {
                movePlayerWithSlide(wasd, dt);
            }
            hasTarget = false; // WASD cancels click target
            tutorial.onPlayerMoved();
        }
    }

    // Move toward click target
    if (hasTarget) {
        Vector2 toTarget = {moveTarget.x - player.position.x,
                            moveTarget.y - player.position.y};
        float dist = std::sqrt(toTarget.x*toTarget.x + toTarget.y*toTarget.y);
        if (dist > 10.0f) {
            if (airborne) {
                player.move({toTarget.x / dist, toTarget.y / dist}, dt);
            } else {
                movePlayerWithSlide({toTarget.x / dist, toTarget.y / dist}, dt);
            }
        } else {
            hasTarget = false;
        }
    }

    // ── Right-click → melee attack (also triggered by bot) ───────────────────
    if ((IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || botMeleeRequest) && !dialogOpen && meleeCooldown <= 0.0f) {
        meleeCooldown = 0.35f;
        tutorial.onPlayerAttacked();
        bool hitAny = false;
        float comboMult = 1.0f + std::min(comboCount, 10) * 0.15f;
        float dmg = player.getEffectiveDamage() * comboMult;
        for (auto& enemy : enemies) {
            float dist = Vector2Distance(player.position, enemy.position);
            if (dist <= player.attackRange) {
                enemy.takeDamage(dmg);

                // Knockback away from player
                Vector2 kb = {enemy.position.x - player.position.x,
                               enemy.position.y - player.position.y};
                enemy.applyKnockback(kb, 220.0f);

                particles.spawnBloodSparks(enemy.position, enemy.bodyColor, 10);
                particles.spawnHit(enemy.position, WHITE, 4);
                audio.playHit();
                hitAny = true;
                comboCount++;
                comboTimer = 2.5f;

                // Floating damage number — bigger/gold for combos
                Color col = comboCount >= 5 ? Color{255,220,0,255} :
                            player.isOverloaded() ? Color{255,200,0,255} : Color{255,80,80,255};
                damageNumbers.push_back({enemy.position, dmg, col, 1.2f});
            }
        }
        if (hitAny) {
            triggerShake(3.5f, 0.12f);
            camPunch = std::max(camPunch, 0.045f);
            // HIT-STOP: micro-congelamento in the impacto (more strong in combos altos)
            hitStopTimer = (comboCount >= 5) ? 0.09f : 0.05f;
        }
    }


    // ── MIRA: the cursor define the direction, mas the aim assist GRUDA in the enemy
    //    more near the cursor (magnetismo of shot). A direction vira always
    //    UNITARIA (before escalava with the distance of the mouse and the projectile corria
    //    in speeds different conforme the distance of the cursor). O alvo
    //    travado is published for the reticulo of the HUD draw the lock.
    Vector2 aimDir = Vector2Subtract(mouseWorld, player.position);
    {
        hudAimLock = { -1.0f, -1.0f };          // reset by frame: trava only with enemy sob the cursor
        const float M = 120.0f;                 // radius of magnetismo around of the cursor
        Vector2 best = { -1.0f, -1.0f };
        float bestD = M * M;
        for (const auto& and : enemies) {
            if (and.isDead()) continue;
            float dx = and.position.x - mouseWorld.x, dy = and.position.y - mouseWorld.y;
            float d = dx * dx + dy * dy;
            if (d < bestD) { bestD = d; best = and.position; }
        }
        if (best.x >= 0.0f) {
            aimDir = Vector2Subtract(best, player.position);
            hudAimLock = best;
        }
        float al = std::sqrt(aimDir.x * aimDir.x + aimDir.y * aimDir.y);
        if (al > 0.5f) { aimDir.x /= al; aimDir.y /= al; }
        else           { aimDir = { 1.0f, 0.0f }; }
    }

    // Skill 1 - Laser (piercing: fires 3 staggered beams; Perfurador adds more)
    if (IsKeyPressed(KEY_ONE) && player.skills[0].isReady()) {
        player.useSkill(0, mouseWorld);
        tutorial.onSkillUsed();
        float dmg = player.getEffectiveDamage() + player.skills[0].damage;
        projectiles.emplace_back(player.position, aimDir, dmg, 550.0f, 620.0f, Color{0,255,255,255});
        int spread = 1 + SkillTree::statsFor(player.perkMask).laserBeams;
        float baseA = std::atan2(aimDir.y, aimDir.x);
        for (int k = 1; k <= spread; ++k) {
            for (int s : {-1, 1}) {
                float the = baseA + s * 0.12f * (float)k;
                Vector2 d = {std::cos(the), std::sin(the)};
                projectiles.emplace_back(player.position, d, dmg * 0.6f, 550.0f, 560.0f,
                                         Color{0,200,255,180});
            }
        }
        particles.spawnHit(player.position, Color{0,255,255,255}, 8);
        audio.playLaser();
        triggerShake(2.2f, 0.10f);   // camera kick in the cast
        camPunch = std::max(camPunch, 0.05f);
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Laser active!", 1.5f);
    }

    // Skill 2 - EMP Area
    if (IsKeyPressed(KEY_TWO) && player.skills[1].isReady()) {
        player.useSkill(1, mouseWorld);
        tutorial.onSkillUsed();
        float empDmg = player.skills[1].damage * (player.isOverloaded() ? 1.5f : 1.0f);
        for (auto& enemy : enemies) {
            if (Vector2Distance(player.position, enemy.position) <= player.skills[1].range) {
                enemy.takeDamage(empDmg);
                particles.spawnHit(enemy.position, YELLOW, 10);
            }
        }
        particles.spawnExplosion(player.position, YELLOW, 25);
        audio.playEMP();
        triggerShake(4.0f, 0.18f);   // onda of choque in the EMP
        camPunch = std::max(camPunch, 0.06f);
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("EMP liberado!", 1.5f);
    }

    // Skill 3 - Plasma Grenade
    if (IsKeyPressed(KEY_THREE) && player.skills[2].isReady()) {
        player.useSkill(2, mouseWorld);
        tutorial.onSkillUsed();
        float gDmg = player.skills[2].damage * (player.isOverloaded() ? 1.5f : 1.0f);
        projectiles.emplace_back(player.position, aimDir, gDmg,
                                 player.skills[2].range, 280.0f,
                                 Color{255,120,0,255}, true);
        audio.playLaser();
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Grenade of plasma!", 1.5f);
        triggerShake(1.8f, 0.08f);   // arremesso sente peso
        camPunch = std::max(camPunch, 0.03f);
    }

    // Skill 4 - Overload
    if (IsKeyPressed(KEY_FOUR) && player.skills[3].isReady()) {
        player.useSkill(3, mouseWorld);
        tutorial.onSkillUsed();
        player.overloadTimer = 8.0f + SkillTree::statsFor(player.perkMask).overloadBonus;
        particles.spawnLevelUp(player.position);
        audio.playLevelUp();
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Overload activated!", 2.0f);
    }

    // Skill 5 - Barrier of Shield
    if (IsKeyPressed(KEY_FIVE) && player.skills[4].isReady()) {
        player.useSkill(4, mouseWorld);
        tutorial.onSkillUsed();
        player.shieldTimer = 3.0f;
        particles.spawnLevelUp(player.position);
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Barrier of shield!", 2.0f);
    }

    // Skill 6 - Burst (8 projetos in leque; Sistema Predador adds more)
    if (IsKeyPressed(KEY_SIX) && player.skills[5].isReady()) {
        player.useSkill(5, mouseWorld);
        tutorial.onSkillUsed();
        float baseAngle = std::atan2(aimDir.y, aimDir.x);
        float spread = 0.22f;
        float dmg = player.skills[5].damage * (player.isOverloaded() ? 1.5f : 1.0f);
        int lo = -3 - SkillTree::statsFor(player.perkMask).burstProj;
        int hi =  4 + SkillTree::statsFor(player.perkMask).burstProj;
        for (int i = lo; i <= hi; ++i) {
            float angle = baseAngle + spread * (float)i;
            Vector2 d = {std::cos(angle), std::sin(angle)};
            Color col = (std::abs(i) <= 1) ? Color{0,255,100,255} : Color{0,200,80,200};
            projectiles.emplace_back(player.position, d, dmg, 420.0f, 660.0f, col);
        }
        particles.spawnHit(player.position, Color{0,255,100,255}, 6);
        audio.playLaser();
        triggerShake(2.5f, 0.12f);   // camera kick in the burst
        camPunch = std::max(camPunch, 0.06f);
        if (playerSpeechTimer <= 0.3f) triggerPlayerSpeech("Burst maxima!", 1.5f);
    }

    // Contextual dialogue — first enemy nearby
    if (!firstCombatTriggered) {
        for (const auto& and : enemies) {
            if (Vector2Distance(player.position, and.position) < 400.0f) {
                firstCombatTriggered = true;
                static const char* lines[] = {
                    "Vou clear essa zone.",
                    "KRONOS... always KRONOS.",
                    "Comes. Not tenho the day all.",
                    "NEXUS never desiste."
                };
                triggerPlayerSpeech(lines[GetRandomValue(0, 3)], 2.5f);
                break;
            }
        }
    }

    // Surrounded by 5+ enemies
    if (surroundedCooldown > 0.0f) surroundedCooldown -= dt;
    if (surroundedCooldown <= 0.0f) {
        int nearby = 0;
        for (const auto& and : enemies) {
            if (Vector2Distance(player.position, and.position) < 220.0f) nearby++;
        }
        if (nearby >= 5) {
            surroundedCooldown = 8.0f;
            if (playerSpeechTimer <= 0.5f) {
                static const char* slines[] = {
                    "Cercado! Hour of the skills.",
                    "Many... mas not impossivel.",
                    "Vou derrubar all!"
                };
                triggerPlayerSpeech(slines[GetRandomValue(0,2)], 2.5f);
            }
        }
    }

    // NPC dialog / Shop
    // E = conversar with the NPC next (TODOS contam your story in baloes).
    // Vendedores also conversam; the shop deles opens with [TAB].
    // [E] ou CLICK ESQUERDO (with dialogo open) avanca the fala; ESC closes.
    bool advanceDialog = IsKeyPressed(KEY_E) || (dialogOpen && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !producedThisClick);
    if (advanceDialog) {
        if (nearNpcIndex >= 0 && nearNpcIndex < (int)npcs.size()) {
            int nLines = (int)npcs[nearNpcIndex].dialogLines.size();
            if (nLines > 0) {
                if (!dialogOpen) { dialogOpen = true; dialogLine = 0; tutorial.onNPCTalked(); }   // inicia the story
                else             { dialogLine = (dialogLine + 1) % nLines; } // avanca line
            }
        }
    }

    // Toggle UI
    // Overlay keys — mutually exclusive: opening one closes all others
    if (IsKeyPressed(KEY_I)) {
        showInventory = !showInventory;
        showEquipment = false; showQuestLog = false;
        shopSystem.close(); craftingSystem.open = false;
    }
    if (IsKeyPressed(KEY_G)) {
        showEquipment = !showEquipment;
        showInventory = false; showQuestLog = false;
        shopSystem.close(); craftingSystem.open = false;
    }
    if (IsKeyPressed(KEY_J)) {
        showQuestLog = !showQuestLog;
        showInventory = false; showEquipment = false;
        shopSystem.close(); craftingSystem.open = false;
    }
    if (IsKeyPressed(KEY_TAB)) {
        if (shopSystem.open) { shopSystem.close(); }
        else {
            shopSystem.buildShop(-1, "NEXUS Supply Terminal");
            shopSystem.open = true;
            tutorial.onShopOpened();
            // Close everything else
            craftingSystem.open = false;
            showInventory = false; showEquipment = false; showQuestLog = false;
        }
    }
    if (IsKeyPressed(KEY_C)) {
        craftingSystem.open = !craftingSystem.open;
        if (craftingSystem.open) {
            shopSystem.close();
            showInventory = false; showEquipment = false; showQuestLog = false;
        }
    }
    if (IsKeyPressed(KEY_F5)) autoSave();

    // Companion spawn keys
    if (IsKeyPressed(KEY_F2)) { if (companions.empty()) spawnCompanion(CompanionType::MarcoVeil); }
    if (IsKeyPressed(KEY_F3)) spawnCompanion(CompanionType::Steel);
    if (IsKeyPressed(KEY_F4)) spawnCompanion(CompanionType::Rex);

    // Power of healing (estilo Diablo 3) — key Q: healing instantanea + regeneracao.
    if (IsKeyPressed(KEY_Q)) {
        if (player.potionReady()) {
            player.usePotion();
            particles.spawnLevelUp(player.position);
            audio.playLevelUp();
            triggerPlayerSpeech("Healing activated!", 1.5f);
        } else {
            triggerPlayerSpeech(TextFormat("Healing recarregando (%.0fs)", player.healCooldown), 1.5f);
        }
    }

    // Building system
    if (IsKeyPressed(KEY_B)) buildingSystem.toggleBuildMode();

    // Key U — evoluir the building more next
    if (IsKeyPressed(KEY_U) && !buildingSystem.buildModeActive) {
        int r = buildingSystem.upgradeNearby(player.position, player.credits);
        if (r == 1)      { triggerPlayerSpeech("Struct evoluida!", 1.8f); audio.playLevelUp(); }
        else if (r == 2) triggerPlayerSpeech("Credits insuficientes to evoluir.", 2.5f);
        else if (r == 3) triggerPlayerSpeech("Is struct already is in the level maximum.", 2.5f);
        else if (r == 0) triggerPlayerSpeech("Chegue near of uma structure to evoluir.", 2.5f);
    }

    if (buildingSystem.buildModeActive) {
        // Scroll wheel changes selected building type
        float wheel = GetMouseWheelMove();
        if (wheel > 0.f) buildingSystem.selectedType = (buildingSystem.selectedType + 1) % BuildingSystem::NUM_TYPES;
        if (wheel < 0.f) buildingSystem.selectedType = (buildingSystem.selectedType + BuildingSystem::NUM_TYPES - 1) % BuildingSystem::NUM_TYPES;

        // Number keys 1-8 select building
        for (int k = 0; k < BuildingSystem::NUM_TYPES; k++) {
            if (IsKeyPressed(KEY_ONE + k)) buildingSystem.selectedType = k;
        }

        // CLICK in the painel of the menu = choose the building (virtualized mouse)
        Vector2 vmouse = virtualizeMousePos(GetMousePosition());
        int menuCell = buildingSystem.menuCellAt(vmouse, screenWidth, screenHeight);

        // Left click: if went in the menu -> seleciona; otherwise -> puts in the map
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && menuCell >= 0) {
            buildingSystem.selectedType = menuCell;
            audio.playPickup();
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && menuCell < 0) {
            int creditCost = 0, metalCost = 0, carapaceCost = 0;
            bool ok = buildingSystem.tryPlace(mouseWorld, player.credits, materialMetal, materialCarapace,
                                              creditCost, metalCost, carapaceCost);
            if (ok) {
                player.credits   -= creditCost;
                materialMetal    -= metalCost;
                materialCarapace -= carapaceCost;
                audio.playPickup();
                triggerPlayerSpeech("Construido!", 1.2f);
            } else {
                // Says EXATAMENTE the that falta to manage construir
                const BuildingCost& c = BuildingSystem::COSTS[buildingSystem.selectedType];
                if (player.credits < c.credits) {
                    triggerPlayerSpeech(TextFormat("Faltam credits: has $%d, precisa $%d. Mate enemies p/ ganhar.",
                                        player.credits, c.credits), 3.5f);
                } else if (materialMetal < c.metalScrap) {
                    triggerPlayerSpeech(TextFormat("Falta Scrap of Metal: has %d, precisa %d. Derrote robos/mecas.",
                                        materialMetal, c.metalScrap), 3.5f);
                } else if (materialCarapace < c.alienCarapace) {
                    triggerPlayerSpeech(TextFormat("Falta Carapaca Alien: has %d, precisa %d. Derrote aliens.",
                                        materialCarapace, c.alienCarapace), 3.5f);
                } else {
                    triggerPlayerSpeech("Not of the to construir here (local bloqueado).", 2.5f);
                }
            }
        }
        // Right click / B again to cancel
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) buildingSystem.buildModeActive = false;
    }
}

