# Full Audit — Darknet Prototype

**Date:** 2026-09-06  
**Auditor:** Multi-domain review with senior specialists (architecture, gameplay, render, netcode, save, world, audio/input)  
**Base commit:** `58d4019` (`main`)  
**Scope:** Source code in `src/`, shaders in `resources/shaders/`, backend in `server/`, design documentation (`GAME_DESIGN.md`, `ROADMAP.md`, `DARKNET_STORY.md`).

---

## 1. Executive Summary

`darknet-prototype` is the functional 2.5D/3D isometric ARPG, with ~75 source files and ~35,000 lines of C++17 over raylib 5.5. The game **compiles and passes headless validation gates**, but presents **massive technical debt** concentrated in the `Game` class, **disconnected systems** (tutorial, achievements), **fragile save**, **determinism problems**, and **serious security risks** for multiplayer/production.

### Consolidated severity balance

| Domain | P0 | P1 | P2 | P3 |
|---|---:|---:|---:|---:|
| Architecture & Code | 5 | 6 | 5 | 3 |
| Gameplay & Balance | 2 | 6 | 8 | 6 |
| Rendering & Performance | 2 | 8 | 9 | 5 |
| Networking & Security | 2 | 5 | 5 | 4 |
| Save & Serialization | 5 | 9 | 9 | 6 |
| World, Collision & Physics | 4 | 8 | 8 | 3 |
| Audio, Input & Auxiliaries | 9 | 19 | 24 | 5 |
| **Total** | **29** | **61** | **68** | **32** |

*(P0 = critical, breaks mechanic/security; P1 = severe; P2 = moderate; P3 = light/polish)*

### Main conclusions

1. **Architecture is the biggest long-term risk.** `Game.h` declares ~160 members and includes 30 headers. Any change has high regression risk.
2. **Several systems documented in the GDD of the not work.** Tutorial, achievements, tier 3 skill tree, level-up rewards, and NPC quests are broken or disconnected.
3. **Save is not reliable.** Zeroed metadata, lost equipment, residual state between matches, in the checksum/backup.
4. **Multiplayer is not production-ready.** WebSocket without TLS, non-authoritative server, data races in client, stub authentication.
5. **Graphics performance has clear bottlenecks.** Voxel model generation never used, thousands of state changes, insufficient culling.
6. **The game passes automated tests, but coverage is low.** 26 tests cover Crafting, Enemy, SaveManager, Player, Projectile, Tilemap — nothing in the `Game` class.

---

## 2. Methodology

- **Manual static analysis** of `src/` focused on architecture, gameplay, render, netcode, save, world, and auxiliary systems.
- **Specialized subagents** reviewed specific domains in parallel.
- **Empirical validation:**
  - `validate.sh 60 20260905 1` — APPROVED
  - `validate.sh 120 7 1` — APPROVED
  - `validate.sh 120 20260821 1` — APPROVED
  - `validate.sh 300 7 1` — APPROVED
  - `darknet_tests.exe` — 26/26 cases, 148/148 assertions passing
- **Automated grep** for `TODO/FIXME/HACK/XXX`, `rand()`, `GetRandomValue`, divisions, and critical patterns.

---

## 3. Empirical Validation Results

| Test | Duration/Seed | Result |
|---|---|---|
| Debug build | — | `darknet.exe` generated |
| Release build | — | `darknet.exe` generated |
| Headless autotest | 60s, seed 20260905 | `VALIDATION: PASSED` |
| Headless autotest | 120s, seed 7 | `VALIDATION: PASSED` |
| Headless autotest | 120s, seed 20260821 | `VALIDATION: PASSED` |
| Headless autotest | 300s, seed 7 | `VALIDATION: PASSED` |
| Unit tests | — | 26/26 cases, 148/148 assertions |

The game is stable for autonomous execution, but tests of the not cover gameplay, save, render, or netcode regressions.

---

## 4. Findings by Domain

### 4.1 Architecture and Code Quality

#### P0 — `Game` is the God Object
- **File:** `src/Game.h:87-698`
- **Description:** The `Game` class aggregates ~160 data members and dozens of responsibilities (physics, spawn, input, 2D/3D render, HUD, menus, network, premium store, save, audio, shaders, open world, phases, crafting, building, animals, civilians). Violates the Single Responsibility Principle extremely.
- **Recommendation:** Refactor into independent subsystems (`World`, `Render`, `UI`, `Audio`, `Input`, `Save`, `Network`, `Economy`, `SpawnDirector`, `PhaseDirector`). `Game` should orchestrate, not contain all data.

#### P0 — Physical split `Game_*.cpp` without logical split
- **File:** all `src/Game_*.cpp`
- **Description:** Files `Game_Gameplay.cpp`, `Game_WorldRender.cpp`, etc. are physical partitions of the same class. All `#include "Game.h"` and freely access private members. There is in the real encapsulation.
- **Recommendation:** Convert into autonomous subsystems that receive dependencies via constructor.

#### P0 — Monolithic methods
- **File:** `src/Game_WorldRender.cpp:632` (`renderWorld3D`, ~1,880 lines), `src/Game_Gameplay.cpp:1267` (`handleInput`, ~580 lines), `src/Game.cpp:552` (`run`, ~240 lines)
- **Description:** Methods with multiple responsibilities, high depth, and dispersed local states.
- **Recommendation:** Decompose into small functions: `updateMenu()`, `renderSky()`, `renderEntities()`, `processCombatInput()`, etc.

#### P0 — `Game.h` includes 30 headers without forward declarations
- **File:** `src/Game.h:3-38`
- **Description:** Directly includes `Player.h`, `Enemy.h`, `Item.h`, `Tilemap.h`, `AudioManager.h`, `NetClient.h`, `StoreClient.h`, etc. Forces cascading recompilation and creates the bloated header.
- **Recommendation:** Use opaque pointers (`std::unique_ptr<class AudioManager>`) and forward declarations.

#### P0 — Massive code duplication
- **File:** `src/Game.cpp:19-43`, `src/Game_WorldRender.cpp:19-43` (identical `structureTintFor`, `isMedievalZone`); `src/Game.cpp:49-55`, `src/Game_WorldRender.cpp:49-55`, `src/Game_WorldGen.cpp:13-16` (`FIT_HOUSE`, `FIT_BARRACKS`, etc.)
- **Description:** Helpers and constants literally copied between files. Divergence risk.
- **Recommendation:** Centralize in `World/BiomeUtils.h` and `World/BuildingMetrics.h` the `inline constexpr`.

#### P1 — Disguised global state
- **File:** `src/Game.h:98-99` (`static bool headless`, `static int startPhaseOverride`); `src/Game.cpp:8` / `src/Game_WorldRender.cpp:17-182` (`g_voxelCapture`); `src/SpriteGen.h:43` (`SpriteBank` singleton); `src/SaveManager.h:22-44` (fully static `SaveManager`)
- **Description:** Global state hinders unit tests and creates hidden coupling.
- **Recommendation:** Pass configurations by constructor/struct; inject `SpriteBank`; convert `SaveManager` to instance with interface.

#### P1 — `std::function` allocated per entity per frame
- **File:** `src/Game.h:192`, `src/Game_WorldRender.cpp:172`
- **Description:** `ensureVoxel` receives `std::function<void()>`. The comment itself admits that assembling the `std::function` costs one allocation per entity per frame.
- **Recommendation:** Replace with template `typename DrawFn` or explicit function pointer/state.

#### P2 — Naming inconsistency and magic numbers
- **File:** `src/Game.h` and all `Game_*.cpp`
- **Description:** Mix of `m_` prefix with unprefixed names; hundreds of literals (`52.0f`, `230.0f`, `560.0f`, `3000.0f`, etc.); 450+ occurrences of literal colors.
- **Recommendation:** Create `namespace Constants`, `namespace Palette`, `GameplayConstants` with `inline constexpr`.

#### P1 — Absence of strict warnings
- **File:** `CMakeLists.txt:13-15`
- **Description:** No `-Wall -Wextra -Wconversion -Wshadow` nor `/W4 /permissive-`.
- **Recommendation:** Add per-compiler flags and treat warnings the errors in CI.

---

### 4.2 Gameplay and Balance

#### P0 — Level-up bonuses are lost on next equip/level-up
- **File:** `src/Game_Menus.cpp:312`
- **Description:** `applyLevelUpChoice` modifies effective stats (`maxHealth`, `attackDamage`, etc.), but `Player::applyEquipmentStats()` recalculates everything from base values. Level-up screen bonuses disappear.
- **Recommendation:** Apply bonuses over `baseMaxHealth`, `baseAttackDamage`, `baseMoveSpeed`, etc.

#### P0 — Skill tree tier 3 is unreachable
- **File:** `src/SkillTree.cpp:53`
- **Description:** `SkillTree::canBuy` requires `spentInBranch >= branchSpentReq(tier)`, where `branchSpentReq(2) = 4`. Each branch has only 4 perks; for tier 3 it is impossible to have 4 points spent without already having bought tier 3 itself.
- **Recommendation:** Require 3 points for T3, not 4.

#### P1 — Extra zerglings spawn without scaling
- **File:** `src/Game_Spawn.cpp:276-287`
- **Description:** Extra zerglings are spawned after all scaling is applied to the first one. They are born with base stats, becoming irrelevant in late game.
- **Recommendation:** Apply the same scaling to extra zerglings.

#### P1 — Knockback applied 2× on specific types
- **File:** `src/Enemy.cpp`
- **Description:** `Enemy::update()` processes knockback at the start, but `updateZergling`, `updateHydra`, `updateBroodmother`, `updateAlienBoss`, etc. process knockback again.
- **Recommendation:** Removes manual knockback from the specific updates.

#### P1 — `EnemyDirector::observe` does not use real moving average
- **File:** `src/EnemyDirector.cpp:30`
- **Description:** `damageTaken = damageTaken * 0.0f + dpm * 1.0f` zeroes the previous value. Reacts only to damage from the last 6s window.
- **Recommendation:** Use moving average, and.g. `* 0.6f + dpm * 0.4f`.

#### P1 — NPC quests complete by wrong criteria
- **File:** `src/Game_QuestsNPC.cpp:79` (`q_portais` counts any kill), `src/Game_QuestsNPC.cpp:144` (`q_coleta_raro` does not check rarity)
- **Description:** Free rewards for incorrect conditions.
- **Recommendation:** Create specific quest types (`ClosePortal`, `CollectRare`) and validate criteria.

#### P2 — Multiplicative scaling can explode
- **File:** `src/Game_Spawn.cpp:191-251`, `src/Game_Evolution.cpp`
- **Description:** Player level + global scaling (cap 4x) + difficulty + threat level (in the cap) + mutator + ring + phase. Enemies can reach 50x–100x HP/damage.
- **Recommendation:** Apply the global combined scaling cap or make part of it additive.

#### P2 — Building passive income explodes
- **File:** `src/BuildingSystem.cpp`
- **Description:** House generates 10 credits/8s; with several houses and upgrades, passive income explodes. Barracks/TankFactory spawn free units beyond the paid queue.
- **Recommendation:** Reduce passive income, make it per player, charge cost for automatic units.

#### P3 — Evolution texts of the not match effects
- **File:** `src/Game_Menus.cpp:404-407` vs `src/Game_Menus.cpp:325-348`
- **Description:** "CYBORG SOLDIER — +HP +Defense +Armor" applies only `attackDamage *= 1.30`; "GHOST HACKER — +Speed +Damage +Range" applies only `speed *= 1.40`.
- **Recommendation:** Align texts to effects or adjust effects.

---

### 4.3 Rendering and Performance

#### P0 — 3D voxel models are generated but never rendered
- **File:** `src/Game_WorldRender.cpp:172-210`, `src/Game_WorldRender.cpp:632-699`
- **Description:** On every new entity type/pose the game captures the 2D sprite, creates outline, crops **and generates the real 3D `Model` via `SpriteExtrude::BuildVoxelModel`**. The comment confirms: *"Real 3D voxel: cached but NOT rendered"*. Each pose generates the GPU mesh, occupying VRAM and causing stalls, never used.
- **Recommendation:** Removes `BuildVoxelModel` from `ensureVoxel` flow if not used.

#### P1 — Sprites disable depth-write and backface culling per entity
- **File:** `src/Game_WorldRender.cpp:563-588`
- **Description:** Each entity calls `rlDisableBackfaceCulling()`/`rlDisableDepthMask()`. Per-entity state change + massive overdraw.
- **Recommendation:** Sort entities by depth and use the single Begin/End state-change pair.

#### P1 — Enemies/bosses emit 50–100 draw calls each
- **File:** `src/Enemy_Render.cpp` (and.g. `renderBossAura`, `renderOrcCibernetico`, `renderOmegaBoss`)
- **Description:** Each enemy drawn with dozens of individual primitives. No batching/instancing.
- **Recommendation:** Render in the single atlas/texture atlas or accumulate geometry in the CPU-side vertex buffer.

#### P1 — Particles alternate blend mode individually
- **File:** `src/Particle.cpp:42-73`
- **Description:** `glow=true` particles call `BeginBlendMode(BLEND_ADDITIVE)`/`EndBlendMode()` per particle. With up to 1,400 particles, generates thousands of state changes.
- **Recommendation:** Split into two phases: normals first, then additive in the single Begin/End.

#### P1 — Scanlines the hundreds of rectangles
- **File:** `src/Effects.h:33-40`
- **Description:** `DrawScanlines` draws one rectangle per ~6 pixels of height (≈120 draw calls for 720p).
- **Recommendation:** Use alpha mask texture or post-process shader.

#### P1 — Entities of the not use frustum culling
- **File:** `src/Game_WorldRender.cpp:1989-2027`
- **Description:** Only uses box test with fixed bounds. Objects behind the camera or outside the view cone are still drawn.
- **Recommendation:** Apply `sphereInCameraFrustum` (already implemented for `owDecor`) to all entities.

#### P1 — Vertex shader uses `w = 1.0` for normal
- **File:** `resources/shaders/world.vs:21`
- **Description:** `fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));` adds matrix translation. Normals should use `vec4(vertexNormal, 0.0)`.
- **Recommendation:** Fix to `vec4(vertexNormal, 0.0)`.

#### P2 — Duplicate vignette
- **File:** `src/Game_WorldRender.cpp:2266`, `resources/shaders/grade.fs:32-35`
- **Description:** Scene already applies `DrawVignette`; shader `grade.fs` applies another mathematical vignette.
- **Recommendation:** Choose one of the two.

#### P2 — Screenshot thread without synchronization/join
- **File:** `src/Game_Shaders.cpp:216-221`
- **Description:** `std::thread(...).detach()` exports image in background. If `Game` is destroyed during export, there is risk of use-after-free/corruption.
- **Recommendation:** Use `std::future`/task queue with explicit join in destructor.

#### P2 — Internal resolution fixed at 1280×720
- **File:** `src/Game.h:153-154`, `src/Game_Shaders.cpp:117-118`
- **Description:** Window is resizable, but content always rendered at 720p and scaled with letterbox.
- **Recommendation:** Recreate `gameTarget`, `m_bloomA/B` and `lightMask` on resize.

#### P1 — White hit flash and flickering lights
- **File:** `src/Enemy_Render.cpp` (multiple lines), `src/Game_WorldRender.cpp:1008-1042`
- **Description:** Multiple enemies flashing white simultaneously and fast pulsations can be problematic for photosensitivity.
- **Recommendation:** Limit frequency/amplitude and add accessibility options.

---

### 4.4 Networking and Security

#### P0 — Client rejects `wss://` — WebSocket without TLS
- **File:** `src/NetClient.cpp:132-137`, `src/Game_Network.cpp:14-18`
- **Description:** `NetClient::init` returns `false` if URL starts with `wss://`. Only speaks plain WebSocket over TCP. Sending JWT, positions, and chat over `ws://` is unacceptable in production.
- **Recommendation:** Implement TLS (OpenSSL/Schannel) or migrate to `websocketpp`/`libwebsockets`.

#### P0 — Player progress written under client authority
- **File:** `server/game-server/src/index.js:438-447`
- **Description:** `POST /progress` accepts `level`, `credits`, `char_class`, `save_json` from client. Server does not validate coherence with account history.
- **Recommendation:** Do not allow client to write progress directly; validate delta or use signed checkpoints.

#### P1 — `NetClient::enabled` is plain `bool` — data race
- **File:** `src/NetClient.h:37`, `src/NetClient.cpp:135,149,153,161,170,234`
- **Description:** `enabled` is read by main thread and written by network thread. Plain `bool` = undefined behavior in C++17.
- **Recommendation:** Make it `std::atomic<bool>`.

#### P1 — `NetClient::sendChat` still builds JSON manually
- **File:** `src/NetClient.cpp:207-224`
- **Description:** `jsonEscape()` handles `"`, `\`, `\n`, `\r`, `\t`, but not remaining controls, invalid UTF-8, etc.
- **Recommendation:** Use `nlohmann::json` (already used in `sendState`/`joinParty`).

#### P1 — Handshake does not validate `Sec-WebSocket-Accept`
- **File:** `src/NetClient.cpp:317`
- **Description:** Client only checks `" 101"`. Does not compute expected accept from `Sec-WebSocket-Key`.
- **Recommendation:** Compute `BASE64(SHA1(key + GUID))` and compare.

#### P1 — WebSocket frame parser vulnerable to arithmetic overflow
- **File:** `src/NetClient.cpp:376-392`
- **Description:** `rx.size() < pos + len` can wrap around if `len` is near `UINT64_MAX`.
- **Recommendation:** Check `len <= rx.size() - pos` with underflow protection.

#### P1 — `/auth/login` is stub — any name generates valid token
- **File:** `server/game-server/src/index.js:345-354`
- **Description:** No password, OAuth, or account binding. Anyone can create infinite accounts.
- **Recommendation:** Replace with OAuth2 (Steam/Google) or email+password with bcrypt/argon2.

#### P2 — `StoreClient` accumulates threads without limit
- **File:** `src/StoreClient.cpp:51-63`, `src/StoreClient.h:56`
- **Description:** `startThread()` stacks `std::thread` in `threads_` with in the concurrency limit.
- **Recommendation:** Use `std::async` with explicit policy or thread-pool with limited queue.

#### P2 — Gateway nginx has HTTPS block commented out
- **File:** `server/gateway/nginx.conf:54-78`
- **Description:** TLS configuration is commented out.
- **Recommendation:** Uncomment 443 block, redirect 80→443, use real certificates.

#### P2 — Client opens Stripe URL without validation
- **File:** `src/StoreClient.cpp:216-224`, `src/HttpClient.cpp:91-93`
- **Description:** `buyGemsAsync` opens in browser any URL returned by the server via `ShellExecuteA`.
- **Recommendation:** Validate that URL belongs to `https://checkout.stripe.with/` before opening.

---

### 4.5 Save and Serialization

#### P0 — `Game::autoSave()` persists zeroed metadata
- **File:** `src/Game.cpp:1413-1415`
- **Description:** `autoSave()` calls `SaveManager::save()` without passing `playMinutes`, `totalDeaths`, `bossesKilled`, `portalsSealed`, `difficultyLevel`. All stay at default 0.
- **Recommendation:** Pass real `Game` metadata to `SaveManager::save`.

#### P0 — `Game::totalKills` is not saved or restored
- **File:** `src/Game.h:266`, `src/Game_Gameplay.cpp:1135`, `src/SaveManager.cpp:146`
- **Description:** Save only stores `Player::totalKills`. After load, `Game::totalKills` keeps garbage from previous session, breaking difficulty curve and Omega Boss trigger.
- **Recommendation:** Persist `Game::totalKills` explicitly.

#### P0 — `equipBag` (equipment backpack) is not saved
- **File:** `src/Player.h:83`, `src/SaveManager.cpp:95-168`
- **Description:** All unequipped equipment is lost on reload.
- **Recommendation:** Serialize `Player::equipBag`.

#### P0 — Equipment upgrades and attributes are lost
- **File:** `src/Equipment.h:17`, `src/SaveManager.cpp:88-91`, `154-156`
- **Description:** `equipSaveToken` only stores `id` or `name`. `upgradeLevel`, `primary`, `secondary`, `tier`, `color`, and custom bonuses are not saved. A +3 weapon reverts to +0.
- **Recommendation:** Expand format to explicit fields or migrate to schema-versioned JSON.

#### P0 — Inventory loses rarity, affixes, and individual attributes
- **File:** `src/SaveManager.cpp:164-167`, `src/Item.cpp:9-54`, `232-289`
- **Description:** Save only stores `ItemType`. On load, calls `Item::createRandom()`, recreating rarity/affixes randomly. A legendary item may return common.
- **Recommendation:** Serialize all meaningful `Item` fields.

#### P1 — `BuildingSystem` is not persisted
- **File:** `src/BuildingSystem.h:175-190`, `src/SaveManager.cpp:95-168`
- **Description:** `buildings`, `tanks`, `soldiers`, levels, queues, timers, pending credits, and materials are lost.
- **Recommendation:** Add full `BuildingSystem` serialization.

#### P1 — Residual state is not cleared on load
- **File:** `src/Game.cpp:968-999`, `src/SaveManager.cpp:173-315`
- **Description:** `startLoadedGame` does not clear `enemies`, `items`, `projectiles`, `groundEquips`, `buildingSystem`. `Player` is not reset before load; unsaved fields keep values from previous session.
- **Recommendation:** Rebuild `Player` and clear volatile `Game` entities before applying the save.

#### P1 — Non-atomic write
- **File:** `src/SaveManager.cpp:101`
- **Description:** `fopen(path, "w")` truncates immediately. If the process dies during `fprintf`, the previous save is destroyed.
- **Recommendation:** Write to `.tmp` and use atomic `rename()`.

#### P1 — Save without checksum/integrity
- **File:** `src/SaveManager.cpp:95-315`
- **Description:** Plain text without hash, signature, or encryption. Trivial to edit.
- **Recommendation:** Add checksum of payload after header.

#### P1 — Version header accepted blindly
- **File:** `src/SaveManager.cpp:184-186`
- **Description:** Code comments "Accept any DARKNET_SAVE_V* version", but there is in the per-version migration.
- **Recommendation:** Extract version number and reject/migrate old saves.

#### P1 — Auto-save at unsafe moments
- **File:** `src/Game_Gameplay.cpp:602-604`, `src/Game_Phases.cpp:381`
- **Description:** Auto-save every 30s and on every zone transition, with in the guard against active combat, transition, dead player, dialogue, or pause.
- **Recommendation:** Add `canAutoSave()` that blocks in those states.

#### P2 — Multi-slot API is dead code
- **File:** `src/SaveManager.h:22-44`, `src/SaveManager.cpp:319-425`, `src/Game.cpp:563`, `971`
- **Description:** `SaveManager` implements 3 slots, but `Game` only uses legacy file `darknet_save.txt` (slot 0).
- **Recommendation:** Removes multi-slot API or integrate it into the main menu.

---

### 4.6 World, Collision and Physics

#### P0 — Anomaly portals spawn outside playable area
- **File:** `AnomalyPortal.cpp:371`, `src/Game_Gameplay.cpp:821-823`
- **Description:** `spawnWave` receives `zoneW/zoneH` from tilemap (24,576×24,576 units) and places fixed candidates in that rectangle. The real open world is the disk of `owPhaseRadius` (3,000–5,000). Most candidates lie beyond the phase barrier.
- **Recommendation:** Generate candidates inside circle `safeZoneCenter + owPhaseRadius - margin`.

#### P0 — Player does not of the separate X/Y sliding
- **File:** `src/Game_Gameplay.cpp:1506-1524`
- **Description:** `Player::move` applies velocity simultaneously in X and Y; only then tests `isBlocked` and reverts to `old`. Allows crossing wall corners and getting stuck on steps.
- **Recommendation:** Implement separate sliding like enemies (`Game_Gameplay.cpp:637-642`).

#### P0 — `InfernoZone` uses global `rand()` instead of seed
- **File:** `src/InfernoZone.cpp:83`, `119`, `126-132`
- **Description:** Despite receiving `seed`, geyser timers, puddle choice, and ash spawn use `rand()`. `rand()` is not re-initialized by game `--seed`.
- **Recommendation:** Replace all `rand()` with `frand(seed)` keeping local state.

#### P0 — Anomaly enemies spawn inside walls/structures
- **File:** `src/AnomalyPortal.cpp:76-82`
- **Description:** `getSpawnPosition` returns random point in ellipse around portal, without collision testing.
- **Recommendation:** Test `isBlocked(portalSpawnPos)` and shift radially until the free point is found.

#### P1 — `isWallAtPosition` uses single point, ignoring entity radius
- **File:** `src/Tilemap.cpp:1533-1537`
- **Description:** Only queries the center tile. Entities with radius > half tile cross walls.
- **Recommendation:** Add overload `isWallAtPosition(Vector2 pos, float radius)`.

#### P1 — Projectiles of the not collide with open-world boundary
- **File:** `src/Game.cpp:1353`, `1385`
- **Description:** `tilemap.isWallAtPosition` returns `false` outside bounds when `openWorld=true`. Grenades of the not explode at the boundary.
- **Recommendation:** Add explicit test against `owPhaseRadius`.

#### P1 — Enemy spawn can push beyond barrier
- **File:** `src/Game_Spawn.cpp:27-31`, `37-44`
- **Description:** `slideToFree` may keep pushing outward without checking `owPhaseRadius`.
- **Recommendation:** Final clamp inside `owPhaseRadius - margin`.

#### P1 — `buildOpenWorldScenery` generates entire fixed world at once
- **File:** `src/Game_WorldGen.cpp:86-480`
- **Description:** All regions are populated on transition. As `owPhaseRadius` grows, number of regions grows quadratically.
- **Recommendation:** Adopt lazy per-region generation or amortize across multiple frames.

#### P2 — `Tilemap::tileZone` and `biomeAtWorld` diverge
- **File:** `src/Tilemap.cpp:461-476`
- **Description:** `tileZone` computes 3×3 layout; `biomeAtWorld` always returns `currentZone`. 2D render may show different biomes than 3D.
- **Recommendation:** Unify into the single query function.

#### P2 — No pathfinding for enemies
- **File:** `Enemy.cpp`, `Game_Gameplay.cpp:620-660`
- **Description:** Enemies use direct steering. Can get stuck in retreats or C-shapes.
- **Recommendation:** Implement A* on tile grid for ground enemies.

---

### 4.7 Audio, Input and Auxiliary Systems

#### P0 — CRT `rand()` is never seeded
- **File:** `src/main.cpp:37`, `src/AudioManager.cpp:26`, `1426`, `2057`, `2097`
- **Description:** `main.cpp` only calls `SetRandomSeed(seed)` when `--seed` is passed. Never calls `srand()`. `AudioManager` uses `rand()` for footsteps, ambience, and noise. Without `srand()`, CRT starts with seed 1, making audio identical across all executions.
- **Recommendation:** Call `srand((unsigned)time(nullptr))` at start of `main()` when in the `--seed`; migrate `rand()` to `GetRandomValue`.

#### P0 — `TutorialSystem` is not instantiated or used
- **File:** `src/Game.h` (absence), `src/TutorialSystem.cpp`
- **Description:** The class exists, but **there is in the `TutorialSystem` instance in `Game`**. Entire system is dead code.
- **Recommendation:** Add instance, call init/update/render and connect callbacks.

#### P0 — Tutorial reward is never applied
- **File:** `src/TutorialSystem.cpp:84-88`, `103-123`
- **Description:** Badge promises 500 XP / 100 credits, but `completeStep` only advances the step.
- **Recommendation:** Apply reward in `completeStep(TutorialStep::Completed)`.

#### P0 — `AchievementSystem` is initialized, but in the events are reported
- **File:** `src/Game.cpp:245`, `src/Achievement.cpp:131-206`
- **Description:** `achievements.init()` is called, but in the `achievements.onKill`, `onLevelUp`, `onPortalClosed`, etc. is invoked.
- **Recommendation:** Connect reporters to game events.

#### P0 — Achievement rewards are never granted
- **File:** `src/Achievement.cpp:83-91`
- **Description:** `unlock()` only sets flag and popup text. `rewardXP`/`rewardCredits` are not applied to `Player`.
- **Recommendation:** In `unlock()`, apply rewards to `Player`.

#### P0 — Keys hardcoded; in the rebind system
- **File:** `src/Game_Gameplay.cpp` (dozens of lines), `src/Game_Menus.cpp`, `src/Game_HUD.cpp`
- **Description:** WASD, arrows, 1-6, Q, E, I, G, J, B, C, TAB, F1-F12, SHIFT, SPACE are hardcoded.
- **Recommendation:** Implement `InputMap` loadable from JSON/INI.

#### P0 — `BotController::zonesVisited` increments every frame near portal
- **File:** `src/BotController.cpp:786-788`
- **Description:** In `AdvancePhase`, when `nearestPortalDist < 40.0f`, `zonesVisited++` happens every frame.
- **Recommendation:** Increment only on transition (once per portal).

#### P1 — `SHIFT` overloaded: sprint and RTS selection
- **File:** `src/Game_Gameplay.cpp:1457`, `1492`
- **Description:** Same key for sprint and RTS selection mode.
- **Recommendation:** Separate sprint from RTS selection (and.g. ALT/CTRL).

#### P1 — `KEY_E` overloaded
- **File:** `src/Game_Gameplay.cpp:479`, `1725`, `1753`
- **Description:** `E` opens NPC dialogue, advances dialogue, interacts with ground equipment, and triggers portal.
- **Recommendation:** Define explicit priorities and visual feedback.

#### P1 — `sfxPlayerDeath` generated but never played
- **File:** `src/AudioManager.cpp:1601-1612`, `1871`
- **Description:** Synthesized death sound is loaded, occupies memory and init time, but has in the caller.
- **Recommendation:** Removes or replace `playDeathCry()` with `playPlayerDeath()`.

#### P1 — Multiple audio methods never invoked
- **File:** `src/AudioManager.cpp:1967-1984`, `src/AudioManager.h:124-137`
- **Description:** `playPlayerHurt`, `playEvolve`, `playHeal`, `playItemPickup`, `playSkillUnlock`, `playAchievement`, etc. are not called outside their own definitions.
- **Recommendation:** Removes unused assets/methods or connect to events.

#### P1 — Dead code after `return composeTrack(...)` in zone synths
- **File:** `src/AudioManager.cpp:464-567`, `570-662`, `665-770`, etc.
- **Description:** Huge manual generation bodies never execute.
- **Recommendation:** Removes dead code.

#### P2 — `CraftingSystem` has ambiguous index
- **File:** `src/CraftingSystem.cpp:209-217`, `246-257`, `363`, `493-497`
- **Description:** `selected` mixes filtered and global index. Can craft wrong recipe or crash.
- **Recommendation:** Always use index within the `filtered` vector.

#### P2 — `ShopSystem` allows buying identical cosmetics repeatedly
- **File:** `src/ShopSystem.cpp:473-501`
- **Description:** `tryBuy` does not check if cosmetic was already acquired.
- **Recommendation:** Mark purchased cosmetics and disable already-owned items.

---

## 5. Cross-cutting Problems

### Determinism and randomness
- `rand()` is used in `AudioManager`, `InfernoZone`, `Player` (dodge), `Particle`, `Tilemap`, `Enemy`, `Game_WorldGen` — without `srand()`.
- `GetRandomValue` is used 278 times in `src/`.
- The mix of PRNGs breaks the promise of reproducible world by `--seed`.
- **Recommendation:** Standardize to the single PRNG seeded by `--seed` throughout the game.

### Test coverage
- 26 tests cover `CraftingSystem`, `Enemy`, `SaveManager`, `Player`, `Projectile`, `Tilemap`.
- The `Game` class has in the unit tests because of coupling.
- **Recommendation:** Refactor `Game` to depend on interfaces; create tests for `SpawnSystem`, `PhaseSystem`, `Economy`, `Collision`.

### Documentation vs implementation
- `GAME_DESIGN.md` foresees tutorial, achievements, per-NPC stock, meta-progression in skill tree — none work correctly.
- `ROADMAP.md` mentions migration to stable IDs and `nlohmann/json` — partially done.
- **Recommendation:** Update GDD or implement missing functionalities.

---

## 6. Priority Recommendations

### Immediate (P0) — fix before any release
1. **Refactor `Game` into subsystems** or, at minimum, break `renderWorld3D()` and `handleInput()`.
2. **Connect TutorialSystem and AchievementSystem** or removes them from the game.
3. **Fix save:** pass real metadata, save `Game::totalKills`, `equipBag`, equipment upgrades, item rarity/affixes.
4. **Fix critical gameplay:** level-up bonuses over base values, tier 3 tree unlock, quest criteria.
5. **Fix world:** anomaly spawn inside phase disk, player X/Y sliding, `InfernoZone` determinism.
6. **Fix security:** implement TLS on WebSocket (`wss://`) and make server authoritative for progress/actions.
7. **Standardize randomness:** seed `srand()` or migrate everything to the single PRNG.

### Very short term (P1)
1. Make `NetClient::enabled` atomic; validate `Sec-WebSocket-Accept`; use `nlohmann::json` in `sendChat`.
2. Implement frustum culling for entities and lights.
3. Removes unused voxel model generation.
4. Validate enemy/boss/companion spawn against `m_chunkSolids`.
5. Add `canAutoSave()` and atomic save write.
6. Implement configurable `InputMap`.
7. Fix `BotController::zonesVisited`.

### Medium term (P2/P3)
1. Consolidate constants, colors, and duplicated helpers.
2. Reduce render state changes (particles, billboards, scanlines).
3. Fix vertex shader `world.vs`.
4. Add accessibility options (flashes, saturation, UI scale).
5. Migrate save to schema-versioned JSON with checksum.
6. Implement A* pathfinding for ground enemies.
7. Add strict warnings in CMake.

---

## 7. Conclusion

`darknet-prototype` is an impressive prototype in volume of features, but the **technical debt exceeds code maturity**. The monolithic `Game` class is the epicenter of risk: it concentrates coupling, hinders tests, and makes every new feature prone to regressions. In parallel, **entire systems documented in the GDD (tutorial, achievements, tier 3 tree, reliable save) of the not work**, and **the multiplayer architecture is not safe for production**.

From the stability standpoint, the game passes headless validation gates, which is positive. However, **passing autotest does not equal ready to play**: save corrupts metadata, balance can explode from accumulated scaling, and the procedural world has spawn and collision bugs.

The strategic recommendation is to **stop adding features until the P0s are resolved**. In order: (1) architecture/subsystems, (2) save and persistence, (3) critical gameplay, (4) network security, (5) render/performance. Without this, every new cycle will exponentially increase correction cost.

---

*Report generated automatically from multi-agent analysis and empirical validation.*
