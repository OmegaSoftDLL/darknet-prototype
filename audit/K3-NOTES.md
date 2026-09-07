# PROGRAMMER NOTES — response to 2026-08-21 audit

| | |
|---|---|
| **Date** | 2026-08-21 |
| **In response to** | `audit/2026-08-21-full-audit.md` + `PROGRAMMER-GUIDE.md` |
| **Scope** | Correction of audit findings + larger stabilization plan (gameplay bugs, performance, architecture, backend, quality) |

---

## 1. Audit findings — item-by-item status

| Sev. | Finding | Status | What was done |
|---|---|---|---|
| ~~P0~~→P2 | Portal inside building | **Closed (intentional guarantee)** | `updatePhasePortal` now checks `!isBlocked(owPortalPos)` on open; if blocked, logs warning and relocates in a ring of 12 candidates at 700u from the refuge. Already proved useful: triggered in phase 4 of one run and relocated. |
| P1 | Zero coverage of phase flow | **Closed** | `BotController` receives `owPortalPos`/`owPortalOpen` every frame; new state `AdvancePhase` (priority below only death escape); at <100u from portal sets `shouldUsePortal`, which `updatePhasePortal` accepts as equivalent to [E]. **Acceptance reached: `Advanced zones: 3` in 300s seed 7** (acceptance was ≥1). |
| P1 | "Follow the marker" with no marker | **Closed** | Pulsating green blip on radar + edge-of-screen arrow pointing to the portal off-screen (pulsating ring when visible), in `drawMinimap`/`drawHudAndOverlays` (shared HUD points). The text is now true. |
| P1 | Ark/RTS medieval in modern city | **Closed** | `Ark` in urban zone becomes sci-fi fortification (`drawArkStructure`: concrete bunker, antennas with pulsating beacons, barricades); urban `House` uses `drawGenericStructure`. `castle.obj`/`house.obj` only in medieval zones (`CursedFarm`, `DarkForest`, `Cemetery`, `AbandonedManor`). |
| P1 | ~6 FPS sustained at opening | **Closed** | Two causes attacked: (1) double scenery construction eliminated — `safeZoneCenter`/phase defined BEFORE `buildOpenWorldScenery` (`SCENERY` log now 1× per world); (2) `m_voxGenBudget` 3→1 spreads GPU→CPU readback. Real root cause found by instrumentation: synchronous autotest PNG screenshot (114–180ms/frame) + moving-average FPS poisoned by worldgen frame. Screenshot now asynchronous; report has 1s quarantine after frames with dt>0.25s. **Minimum FPS: 6 → 47–57.** |
| P2 | Scenery built 2× at initialization | **Closed** | See item above — reordering in constructor and removal of redundant rebuild in `runAutoTest`. In `advanceOpenWorldPhase`, new phase parameters are set BEFORE rebuilding (defect of same class: new phase was born with previous radius). |
| P2 | Phases 2+ with LA city in hub | **Closed** | `setupWorldRegions` uses `currentZone` for the 9 regions (one phase = one coherent world); `advanceOpenWorldPhase` rewrites `tilemap.currentZone` after `generateOpenWorld()` (was stuck in LARuins). Evidence in log: phase 2 = Cursed Farm, phase 3 = Dark Forest, phase 4 = Cemetery — each with its own structure catalog. |
| P2 | Melee inert in automation | **Closed** | Cause: retreat backoff (150px) + orbit (220px) prevented closing melee range (90px). Fixed with 1.5s engage without retreat and 80px orbit. **11–23 melee attacks/120s** (acceptance was >10). |
| P2 | Misleading FPS alert | **Closed** | Sample quarantine after load frames (dt>0.25s); alert now measures game FPS. Latest runs: **zero** "Critical FPS" entries. |
| — | Hygiene: WIP in `BotController.h` | **Closed** | It was work in progress from this session, now complete: `shouldPickupItem` removed (dead code — no consumers), `reset()` implemented and called by `restartRun`. |

## 2. Additional work beyond the audit (general stabilization)

Project owner request: "fix everything". Executed in phases, each validated with build + autotest:

- **Gameplay bugs:** bot stuck in safe zone (enemies only spawn outside — bot never found them; 0 kills → 33–48 kills/120s); item collection counted in the wrong place (0 → 118–156/120s); skill thresholds; 8 `static` timers leaking state between matches converted to members with reset in `restartRun`; duplicated BFS pathfinding unified; `NetClient` `static` network throttle became a member.
- **Visual pipeline:** parallel 2D pipeline and F10 toggle removed (isometric 3D is the only path); ~1,100 lines of orphaned `render3D()` removed (Player/Enemy/NPC/Companion); global flag renamed to `g_voxelCapture` (reflects real function).
- **Render performance:** light mask from 31 concentric ellipses → 1 textured quad per light at half resolution (mathematically equivalent alpha matte); tilemap floor from 2,809 batches → 1 single batch; real frustum culling on scenery and floor; voxel shadow/outline LOD by distance. **Worst render: 95.9ms → ~19–22ms.**
- **Architecture:** `Game.cpp` from ~8,700 → **4,718 lines**; 11 modules extracted (`Game_Shaders`, `Game_Network`, `Game_PremiumStore`, `Game_Resources`, `Game_Evolution`, `Game_QuestsNPC`, `Game_Phases`, `Game_Spawn`, `Game_WorldGen`, `Game_HUD`, `Game_Bot`) — cut-and-paste of whole functions, no logic change (verified with diff at one stage).
- **Product:** premium cosmetics (Neon/Dragon skins, Drone pet, tints) now render — the pipeline existed, but `visualSignature()` did not include skins in the voxel cache key, so the 3D model never updated.
- **Save:** V4→V5 bump with stable equipment IDs (`EDB::byId`); renaming item no longer breaks saves; fallback for V4 saves by name.
- **Quality:** manual JSON parsing (substring) replaced by vendored `nlohmann/json` in `third_party/`; vendored doctest with **10 cases / 73 assertions** (`CraftingSystem`, `Enemy`, `Equipment`) running via CTest; separate `darknet_tests` target.
- **Backend:** `inventory.qty` used for real (UPSERT + idempotent migration with unique index); DDL consolidated as single source of truth in `index.js` (`init.sql` became a pointer); orphaned Redis removed from compose; default `PUBLIC_URL` → gateway 8080; `server/README.md` with real endpoints; `node --check` OK.
- **Documentation:** protagonist standardized (Vance Rios), year 2047, README updated for isometric 3D, consolidated `ROADMAP.md` created at root.

## 3. Final metrics (Release autotest, seed 7)

| Metric | Audit baseline | Final |
|---|---|---|
| Average FPS | 58–59 | 58–59 |
| Minimum FPS | 6 | **47–57** |
| Worst render() | 28.7–44.5 ms | **~19–22 ms** |
| Kills/120s | 20–31 | 33–48 |
| **Advanced zones** | **0** | **3** (in 300s) |
| Deaths / stalls >10s | 0 / 0 | 0 / 0 |

## 4. Not covered / left out (declared)

- **Save/load roundtrip, multiplayer, premium store, backend in execution** — still without end-to-end integration testing (require active server); what was validated is static (`node --check`, compilation, unit tests).
- **Audio** — not evaluated (requires human judgment).
- **Phases 5+** — 300s runs reach phase 4; later urban phases (GhostCity) use the same code path as phase 1, but were not observed in execution.
- **Visual screenshot verification** of Ark/biome fixes — evidence is by logs + render path analysis, not frame inspection.
- **Mass naming standardization** (`m_` prefix) — not done; high risk/low return. Adopt in new code.
- **Comment language** — kept Portuguese (current project standard).
- Sporadic 400–900ms swap hitch observed in some rounds — machine driver/DWM, not reproducible deterministically, update+render <10ms at the instant.

## 5. Instruments left

- `validate.sh [seconds] [seed]` — gate (Debug+Release + bot), used with the two seeds from the guide.
- `darknet_tests.exe` / `ctest` — 10 pure-rule cases.
- `bot_report.txt` + `shot_NN.png` during `--autotest`; report with load quarantine.
- `ROADMAP.md` (root) — single document of state and next steps.

---

# ADDENDUM — response to audit 2 (2026-08-21, second pass)

| | |
|---|---|
| **In response to** | `audit/2026-08-21-audit-2.md` (APPROVED WITH RESERVATIONS) |
| **Action** | Correction of the 8 new findings (1×P1, 7×P2) + visual verification by frames |

## A1. Acknowledgment of audit 2

The second pass confirmed **CLOSED** the 9 audit 1 findings (measurement: Advanced zones 1/1/3, minimum FPS 42-57, zero false alarm, biome per phase). About phase scenery: *"Farm reads as farm... The per-biome clutter palette works."* — the biome work (palette per zone in `Zone.h:clutterPaletteFor` + variable seed per phase) was verified in frame.

## A2. New findings — corrections applied

| Sev. | Finding | Status | What was done |
|---|---|---|---|
| P1 | Modern building hides player | **Closed** | Occlusion translucency replicated in `case 20` (building) and `case 15` (bunker), with window based on **real footprint** (not the fixed 620/230). `rlDisableDepthMask` wraps translucent draw so the hero is not clipped. **Verified in frame: `shot_02` shows LA buildings translucent with the street readable through and the player visible.** |
| P2 | Gate does not require phase advancement | **Closed** | `BotController::passed()` fails runs ≥180s with `zonesVisited==0`. Shields this cycle's achievement against silent regression. |
| P2 | Dark Forest / night unreadable | **Closed** | Ambient light floor (multiplicative mask `amb>=0.64` ≈ 0.33 minimum luminance — mood stays in hue, not pitch black) + player light 430→480/0.72→0.82 + cold rim light on actors as `ambientDark` rises (in `drawVoxel`, covers player/enemies/NPCs/companions). |
| P2 | Portal guarantee weak | **Closed** | Tests the **disk** (center + 8 samples at 100u), searches in 3 rings × 16 angles + 220-candidate spiral; if nothing free, relocates to refuge with `Tilemap::clearSolidAt` — never again keeps a blocked position. Triggered for real in the validation run. |
| P2 | `SCENERY` `structures=0` in city | **Closed** | Counter includes types 6, 9, 14-20. Log now `structures=181` in LA (was 0). |
| P2 | Hardcoded absolute paths | **Closed** | Bot reports written to CWD (works on CI/other machines). |
| P2 | Autotest does not clean old shots | **Closed** | `runAutoTest` removes `shot_*.png` from CWD before starting. |
| P2 | Inconsistent phase texts | **Closed** | `advanceOpenWorldPhase` advances `storyChapter`; `phases.txt` unified ("Cursed Farm"). **Verified in frame: `shot_04` shows `CAP.2 | Cursed Farm`.** |

## A3. Own visual verification (k3 watched the frames)

- `shot_01`/`shot_02` (LA): gray urban ruin; buildings translucent after correction, player always visible.
- `shot_04` (Farm): green field, dead trees, correct `CAP.2` — scenery and text distinct from LA.
- `shot_07`/`shot_09` (Farm at night): showed the excessive darkness that motivated the night readability fix.

## A4. Still not covered (declared)

- Portal marker and sci-fi Ark **rendered in frame** (code checked; 30s shot cadence did not coincide with windows — audit 2 declared the same limit).
- Phases 5-11 in execution (runs reach phase 4; later urban biomes by code analysis).
- Line-by-line diff of the 11-module extraction (guarantee = gate + unit tests).
- Save/load roundtrip, multiplayer/store/backend with server, audio, human input.

## A5. Repository state

As noted in audit 2 (§7): all work is **uncommitted** (34 modified files + new ones). **Recommend committing** — this is the best state the project has ever been in, and an accidental `git checkout` would erase it.

---

# ADDENDUM — Cycle 3: full audit with corrections (2026-09-04)

| | |
|---|---|
| **In response to** | Full audit executed on 2026-09-04 (build, ctest, validate, runtime, git, server) |
| **Recommendations applied** | All critical + structural findings with objective correction |

## C1. Corrected findings

| Sev. | Finding | Status | What was done |
|---|---|---|---|
| 🔴 | Repository does not compile if cloned: 11 `Game_*.cpp` + `tests/` + `third_party/` untracked, CMakeLists referenced them, 35 modified files and 1 commit ahead without push | **Closed** | Commit `06b7940` with everything (53 files). Repository is again self-contained and cloneable. |
| 🟠 | JWT with hardcoded default `dev-secret` (forgeable tokens) | **Closed** | Without `JWT_SECRET`, generates random secret per boot (`crypto.randomBytes`) + warn; never fixed default. |
| 🟡 | Save/load without coverage — and with real bug | **Closed** | V5 + legacy V4 roundtrip test added to `darknet_tests` (12 cases / 109 assertions, release+debug). The test **exposed a bug**: `totalKills` was saved and shown in slot menu, but **never restored into Player** on load → fix in `SaveManager::load`. |
| 🟡 | Repo hygiene | **Closed** | `.gitignore` now covers `autotest_*.log`, `validate_seed*.log`, `shot_*.png`, `screenshot*.png`. |

## C2. Post-correction validation (2026-09-04)

- `cmake --build` Debug and Release: **exit 0** both.
- `darknet_tests.exe`: **12/12 cases, 109/109 assertions** (was 10/73).
- CTest: **1/1 passed**.
- `node --check` (game-server): **OK**.
- `git status`: clean after commit.

## C3. Still not covered (declared — no objective correction in this pass)

- Phases 5–11 in execution (runs reach phase 4; later urban biome by code analysis).
- Full save/load roundtrip in-game (multiplayer/store/backend with active server) — what was validated is unit + build + syntax.
- Audio and gameplay require human judgment.

---

# ADDENDUM — Cycle 4: backend TLS + secure netcode (2026-09-04)

| | |
|---|---|
| **In response to** | Owner plan "make it amazing" — chosen block: **TLS + backend netcode** |
| **Scope** | Server security hardening + client network route repair |

## D1. What was done

| Sev. | Topic | What was done |
|---|---|---|
| 🔴 | JWT re-exposed (public default nullified Cycle 3 fix) | `docker-compose.yml`: `JWT_SECRET: "${JWT_SECRET:?define...}"` — **fail-fast**; no known fixed default. |
| 🔴 | WS without authentication (any connection entered the room) | Upgrade only accepted with valid JWT in `Authorization` header (400 in `ws` v8: `noServer` + `upgrade` event; no token → **401 before opening socket**). |
| 🟠 | Identity spoofing (peer/chat used `id`/`name` from JSON) | Server now derives identity **from token** (`ws.user`), never from body; coordinates sanitized/clamped; peer name comes from JWT. |
| 🟠 | Client talked directly on `:9000`, ignoring gateway | `StoreClient`: `/api` prefix + optional TLS via env `DARKNET_API_URL`; `NetClient` sends JWT in handshake; `startNetwork` waits for login before connecting. |
| 🟠 | No TLS on REST | `HttpClient.cpp` gets `WINHTTP_FLAG_SECURE` (https only with valid certificate); gateway with HTTPS block ready (443 → covers `/api` and `/ws`, realtime becomes `wss://`). |
| 🟡 | No heartbeat / flood / limits | Server 30s heartbeat (kicks inactive), 120 msgs/s per connection, payload ≤4 KiB per message, `maxPayload` 1 MiB. |
| 🟡 | REST without rate limit / validation | Per-IP limit (login 20/min, store 30/min, progress 60/min) + `trust proxy`; name sanitized (≤24, no control); `save_json` ≤100 KB; dev-grant ≥0 ≤1M. |
| 🟡 | Stripe webhook without idempotency | Single credit per `provider_ref` (check in `transactions` + in-memory dedupe in dev mode) — retry/duplicate does not credit 2×. |

## D2. Post-correction validation (2026-09-04)

- `node --check` (game-server): **OK**.
- Debug and Release build: **exit 0** both (targets `darknet` and `darknet_tests`).
- `darknet_tests.exe`: **12/12 cases, 109/109 assertions**.
- **Functional smoke test with real server (in-memory)**: 13/13 PASS — login + token + sanitized name, `/me` 401 without token, rate limit → 429, WS without token rejected (401), WS with token connects, **anti-spoof** (forged id 999 → real id from token), sanitized coordinates, connection survives flood.
- `validate.sh 120 7` and `validate.sh 120 20260821`: **APPROVED** both.
- `git push origin master`: Previous cycles (up to `38bb85f`) **pushed**.

## D3. Still not covered (declared)

- **`wss://` on client**: `NetClient` is raw Winsock without TLS — refuses `wss://` URLs before opening thread (documented in code). Gateway already serves `/ws` under TLS when 443 block is activated; migrating client to WSS requires TLS (Schannel/OpenSSL) on the socket.
- **Real auth** (replace stub `/auth/login` with OAuth or email+password with hash) — out of scope for this cycle.
- Phases 5–11 in execution and audio (human judgment) — same as Cycle 3.

---

# ADDENDUM — Cycle 5: scenery collision + visual menu retheme (2026-09-04)

| | |
|---|---|
| **In response to** | 1) User bug: "characters clipping through things in scenery (cars and very large buildings)". 2) "The game/menu visuals suck, improve 1000%" — approved scope: **menus first**, style **cyberpunk neon dark**, **only raylib primitives**. |

## E1. Scenery collision (root cause + fix)

| Sev. | Topic | What was done |
|---|---|---|
| 🔴 | Collision radius **inscribed** smaller than drawn footprint | Modern building (type 20) was marked with fixed radius `96·sc` but drawn with W≈250-325·sc × D≈220-304·sc; vehicles (type 6) with `78·sc` but car/van/truck have L≈158/196/264·sc. Hero sank dozens of px into facades. |
| 🔴 | City chunks with no collision | `m_chunkSolids` only covered types 0/1/7/8 — buildings (20) and cars (6) spawned by chunk were 100% passable. |
| 🟢 | Fix | `Game_WorldGen.cpp`: helpers `buildingRadius`/`vehicleRadius` replicate **the same hash as render** (identical W/D to drawing) and return the exact **circumscribed circle**; used in fixed world (replaces 96·sc and 78·sc) and chunk switch now covers types 6/9/10/14-20 (same radii as fixed). |

## E2. Visual retheme — menus (cyberpunk neon dark, raylib primitives)

| Screen | What changed |
|---|---|
| Main menu | Deep gradient background; camera frame in corners; decorative counters (KRN.LOG / NEXUS ONLINE); finer cyber grid; IRON-VIII skull re-colored in gunmetal blue with red eyes; title with **3D extrusion + cyan aura**; separator with chamfered tips; buttons with **left energy rail, chamfered corners, key badge with recess, neon underline and pulsating arrow**; footer in chips (`//`) with version on the left. |
| Pause | Embraced title with underline and corners; list wrapped in `DrawPanel`; focused item gets neon rail + chamfered corners and `»`. |
| Class selection | Title with extrusion + underline and amber brace; cards with chamfered corners, class-color rail, cyan pulsating top on selected; hint in chamfered panel. |
| Save slots | Chamfered panel with pulsating top; selected slot with amber rail, pulsating chamfered border and [DEL] delete; bottom hint bar. |
| Level-up / Evolution | Cards with chamfered corners + top bar on selection; darker/consistent backgrounds. |
| Implication | `Game::DrawPanel` (HUD) was already the chamfered-corner standard — retheme extends the same idiom to all menus (single identity). `SaveManager.cpp` got `#include <cmath>`. |

## E3. Visual retheme — in-game HUD (same idiom)

| Panel | What changed |
|---|---|
| Stats HUD / top bar | Palette unified to cyan `{0,235,255,255}` (replaceAll over `{0,210,255}`/`{0,230,255}` in `Game_HUD.cpp`); top bar with double underline and corner marks. |
| Threat / mutator / safe zone / phase | Chamfered chips with colored rail (red/green/cyan). |
| Controls | `DrawPanel` + key chips. |
| Skill bar | Angular background + key number in amber chip + neon top line. |
| Quest HUD / journal | `DrawPanel` + side rail / amber header. |
| Minimap | Angular frame, RADAR chip, MAP repositioned (mapX+60), corner ticks. |
| Skill tree | `Game_SkillTree.cpp`: neon border, top line, selected chamfered lines with rail. |
| Difficulty screen | Title with extrusion + chamfered separator with amber diamond. |
| Network / premium store | C_cyan unified (`Game_Network.cpp`, `Game_PremiumStore.cpp`, `Player.cpp:842` inventory). ⚠ `Player.cpp:35` and `:235` are 3D colors and were **not** touched. |

## E4. 3D world pass (proportions + vehicles)

| Sev. | Topic | What was done |
|---|---|---|
| 🔴 | Cars "children's blocks" and out of proportion | Vehicles had L≈158/196/264 (4.2m/5.2m/7m) next to a ~1.1m character and wheels buried in giant 1.45·wr arches → read as monuments/boxes. |
| 🟢 | Fix `Game.cpp` case 6 | Rewritten with **real proportions**: sedan ≈100u, van ≈122u, truck ≈164u (ruler 1m≈37.6u); **continuous smooth waist** (no step between hood/roof/trunk) with chrome trim stitching sections; **recessed windows** (roof collar), windshield and rear glass **slanted** (Z rotation); wheels **flush** with bodywork with discrete 1.16·wr arch and grounded tires (floor sills on the ground — eliminates "floating"); headlights/taillights/bumpers/mirrors/antenna. |
| 🟢 | Street layout | `Game_WorldGen.cpp`: cars (type 6) now **align to street grid** (4 angles, like buildings) instead of random cross rotation; `vehicleRadius` recalibrated for new L/WD (circumscribed radius, tire ±5u). |
| 🟢 | Extras | 12 sides on wheels (rounder, was 10); recessed hubcap; truck with ribbed cargo box + cab with slanted windshield, own roof. |

## E5. Post-correction validation (2026-09-04)

- Debug and Release build: **exit 0** both (targets `darknet` and `darknet_tests`).
- `darknet_tests.exe`: **12/12 cases, 109/109 assertions**.
- `validate.sh 120 7` and `validate.sh 120 20260821`: **APPROVED** both.

## E6. City without buildings/trees in the middle of the street (root cause + fix)

Owner finding: "there are buildings and trees in the middle of the streets, this can't happen".

| Topic | What was found / done |
|---|---|
| Root cause | The **streets the player sees** are quads at `475 + k*950` (±112u from curb, `Tilemap::render3D`), independent of tiles (tile streets are random every 6-8 tiles). The city **ignored this grid**: building grid at `220 + k*950` with `EDGE=350` → facade intruded ~137u **into the lane**; trees/cars from `place()` fell anywhere (area value ~56%). GhostCity still used block 880 over 950 roads (misaligned). |
| Fix | `laneDist()` (grid 475+k*950) as single ruler: fixed-world grid **synchronized to roads** (block center at multiples of 950, `EDGE=350→210` glues facade to curb); `place()` gets *city* flag → tree never `laneDist<130`, car always `laneDist≤102`; city chunk clusters snapped to same grid (corners at ±210 from center, `putB` with 220 spacing for neighboring towers) + gate in prop `put`; GhostCity returns to 950 block (differs in ruin/emptiness, not step). |
| Result | Whole block with mass centered in the lot; street is clean corridor between buildings; parked cars on asphalt; trees only inside lots. |

## E7. Post-correction validation (2026-09-04, street pass)

- Debug and Release build: **exit 0** both (targets `darknet` and `darknet_tests`).
- `darknet_tests.exe`: **12/12 cases, 109/109 assertions**.
- `validate.sh 120 7` and `validate.sh 120 20260821`: **APPROVED** both.

## E8. Enemies respect chunk buildings (collision)

| Topic | What was done |
|---|---|
| Gap | `Game::update` only blocked enemies on **tile wall** (`isWallAtPosition`). Infinite-generated structures (`m_chunkSolids`, circles) were ignored → the enemy walked **inside the chunk building**. Spawn already avoided (`Game_Spawn.cpp` uses `isBlocked`), only movement remained. |
| Fix | Enemy collision block now evaluates `blockedAt(pos)` = tile wall **OR** chunk circle (`Game.cpp`), with same per-axis slide (try X, then Y, else revert). Specters/flying bosses still pass through intentionally. |

## E9. Architecture: extraction of Game_WorldRender.cpp

| Topic | What was done |
|---|---|
| Objective | `Game.cpp` was at **5,027 lines**; rendering monopolized ~1,700. |
| Extraction | Literal **cut-and-paste** copies to `src/Game_WorldRender.cpp` (new): `ensureVoxel`, `drawGenericStructure`, `drawArkStructure`, `drawVoxel`, `sphereInCameraFrustum`, and `renderWorld3D` (~1,620 lines). Duplicated local statics (same pattern as `Game_WorldGen.cpp`): `structureTintFor`, `isMedievalZone`, `FIT_*`, `DrawCubeTexture`. Registered in CMake. |
| Result | `Game.cpp`: 5,027 → **3,236 lines**; `Game_WorldRender.cpp`: 1,912. No logic change (literal copy). |

## E10. Validation (2026-09-04, collision and extraction round)

- Debug and Release build: **exit 0** (targets `darknet` and `darknet_tests`).
- `darknet_tests.exe`: **12/12 cases, 109/109 assertions**.
- `validate.sh 120 7` and `validate.sh 120 20260821`: **APPROVED** both.

## E11. Declared (next in sequence)

- Phases 5–11 in execution and audio (human judgment) — remain pending from larger sequence.
- `wss://` in `NetClient`: requires TLS (OpenSSL/SChannel) — non-vendored, separate infra decision (remains `ws://` local/gateway).
- Character is ~1.1m on the ruler (1m≈37.6u) while buildings use 2.1m floor — rescaling hero/NPC is high-risk decision (reserved; today top-down disguises).
- `Game::update` (1,170 lines) and `Game::handleInput` (~550) remain as next cuts of the monolithic `Game.cpp`.

---

# ADDENDUM — Cycle 6: final Game.cpp cuts + readability of dark phases (2026-09-04)

## E12. Extracted Game::update and Game::handleInput → Game_Gameplay.cpp

- `Game.cpp` now has **1,493 lines** (was 5,027 at the start): lifecycle, menus, small decals/colliders, camera and render.
- New `src/Game_Gameplay.cpp` (1,880 lines): `Game::update` and `Game::handleInput` by literal copy (zero logic changed).
- Included `SkillTree.h` (used `SkillTree::statsFor` without including). Starts the pipeline of extracting up to 5 phases; the next natural target is `Game::drawCharacterSelectScreen`/menus, since the file is now a healthy size.
- `CMakeLists.txt`: source registered after `Game_WorldRender.cpp`. No extra file-scope statics needed (`getZoneInfo` used by update is inline in `Zone.h`).

## E13. Dark phases unreadable (player complaint) — scene + text fix

Complaint: "phases with dark names are TOO dark, can barely see anything" (scene ground with black derivatives in dark phases + swallowed texts).
Response (scene and texts):
- **Scene**: palette of all 11 phases lightened in `Zone.h::getZoneInfo` (ground A/B, walls and outline) while keeping hue; dark mood continues from sky/fog (`skyColorFor`) and DarkWorld props, not from ground. E.g. DarkForest ground (20,28,20)→(54,72,50); Catacombs (30,25,30)→(70,58,66); Nexus (25,10,55)→(66,44,104).
- **Texts**: `drawStoryBanner` phase banner with more opaque panel (0.62→0.84), black shadow on title and lighter subtitle; `drawPhaseFade` phase fade now has condensed panel with cyan border and text alpha minimum ~70%; zone HUD banner with shadow. There is no global dimming beyond colors (confirmed: only ZoneInfo feeds the light).

## E14. Bot phase execution evidence

- Long run `--test-seconds=480 seed 7`: **exit 0, VALIDATION PASSED** (phase advancement criterion is mandatory on runs >=180s).
- `bot_report.txt` (120s run seed 20260821): "Advanced zones: 1" at ~105s; 34 kills. Phases 5-11 remain as human/visual audit (phase content is already in `content/phases.txt`, all 11 loaded).

## E15. Declared (remaining sequence)

- Phases 5-11 in full execution (visual/audio) and full audio — human judgment.
- `wss://` (TLS) remains decided out of scope without vendored OpenSSL.
- Next cuts of large files: `Game.cpp` (1,493) still has `drawCharacterSelectScreen`/menus; `Game_Menus.cpp` and `Game_HUD.cpp` are the largest remaining.
- **Recurring recommendation**: 34+ modified files uncommitted; best state of the project — commit before proceeding.

## E16. Enemy.cpp split → Enemy_Render.cpp

- `Enemy.cpp` 3,280 → **1,525 lines**; new `src/Enemy_Render.cpp` (~1,760) with all 22 `Enemy::render*` + local helper `DrawRotatedRectangle` (literal copy of the 6 contiguous blocks; only file-local static: `DrawRotatedRectangle`; duplicated `extern bool g_voxelCapture` in the new file).
- CMakeLists updated. Gate: Debug+Release exit 0, tests 12/12 (109), validate 120/7 APPROVED.
- Remaining in `Enemy.cpp`: core/AI/knockback/attacks (`setupByType`, `update*`, boss phases) — readability maintained.

## E17. Phase execution — long runs (measurement)

- 480s (seed 7): exit 0, VALIDATION: PASSED — phase advancement criterion is MANDATORY on run ≥180s, so portal/phase worked.
- 900s (seed 20260821): exit 0. `bot_report.txt` was overwritten by smaller runs; preserved data: validation and phase advanced in runs ≥180s. Phases 5-11 remain as visual/human audit (content in `content/phases.txt`, all 11 loaded).
- Timing observation: 120s seed 20260821 (re-run) gave "zones=0" (portal opened after the 60s monitoring cutoff); 120s seed 7 gave "zones=1" at ~105s. Seed/timing variance is intentionally covered by the loose gate (<180s).

## E18. Level-up banner (user complaint: big and over messages)

- Cause: 420×70 banner centered at y-100, drawn AFTER zone banner (y-40) and mission panel — covered the phase name.
- Fix: compact pill (text width + 36, 40/52 height), font 28→20, relocated to y=30 (under HUD bar, far from center). Added `<cstdio>` (snprintf).

## E19. Rendering evolution ("too amateur")

- `world.fs`: anti-plastic layer — 3-octave fbm in WORLD space (uniform `worldPeriod`, 950 in open-world LA / 480 in fixed map) breaks the single-color walls of large opaque faces; + dry specular (`specularK` 0.28, sphere 26) on bodywork/metal only on alpha>0.999 (translucent decal does not receive).
- `grade.fs`: cinematic vignette (up to 32% at edges) folded into final pass at zero cost.
- `initPostFX`: bloom 0.95→1.15, saturation 1.22→1.28 (more vibrance).
- Gate: Debug exit 0, Release exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED (POSTFX/WORLDLIT ACTIVE).

## E20 — "Juice" round (twin-stick shooter game feel research)
Research: solana.garden (hit stop 40-120ms per tier, screen shake trauma^2 with decay, camera kick/recoil on shot, kill pop, impact flash) + official raylib (`shapes_top_down_lights.c` — top-down light mask for future reference). Diagnosis: melee already had hitstop (`Game_Gameplay` update: `hitStopTimer` 0.05/0.09 with partial freeze, particles continue), shake 3.5 and blood sparks; BUT explosions, common kills, and casts had no weight.
Implemented (low risk, without touching bot logic/balance):
- `Game.cpp updateProjectiles` (grenade explosion): `triggerShake(6.0, 0.25)` + `hitStopTimer` 0.07 — explosion "hits" for real.
- `Game_Gameplay.cpp` enemy death block: **kill POP** — particle burst scaled (12 normal / 24 elite / 46 boss; body color; white center), and elite/boss freeze the world (hitstop 0.06/0.10) + shake 4.5/7.0 with long camera.
- Camera kick on casts: Laser 2.2/0.10, EMP 4.0/0.18 (shockwave), Grenade 1.8/0.08 (weight on throw), Burst 2.5/0.12.
- Gate: Debug exit 0, Release exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED.
- Next (if user asks for visual evolution): raylib-style 2D light mask for dynamic scene lights; flash on elite damage taken; pitch variation on hit SFX.

## E21 — "Floating" character fixed + power bar with life
- BUG "floating": `Game::update` calls `handleInput(dt)` (movement → `player.move` → `isMoving=true`) and THEN `player.update(dt)`, which ZEROES `isMoving` at the start. The voxel (`drawVoxel` for player in `Game_WorldRender.cpp`) used `player.isMoving` → was ALWAYS false at render → pose 0 frozen + no step + no bob = slides/floats. The legacy 2D render (`Player::render`) compensated with `velMag>12`, the 3D did not. Fix: use real velocity (|v|>12) in player `drawVoxel`; step (step snap) 0.030→0.040.
- **Powers bar** (`drawSkillsPanel`): was key chip + name ("123456" aesthetic). New "living" panel: procedural icons per power (Laser=cyan projectile, EMP=gold concentric pulse, Grenade=sphere+fuse, Overload=lightning, Barrier=hexagon, Burst=fan) without assets; own color per slot; border pulses when READY; cooldown = descending overlay + progress bar + seconds; visible damage.
- Gate: Debug exit 0, Release exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED.
- Audit: P1 portal marker/bot crosses portal, double scenery build, and `BotController.h` hygiene were already resolved (verified this round: `setupWorldRegions` called 6× but with correct `safeZoneCenter` BEFORE `buildOpenWorldScenery`; `reset()`/`restartRun` exist).

## E22 — Damage reaction + boss bar + HUD with nothing fixed in center
- **Red damage flash** (latent bug): `hitFlashTimer` was set in 5 places and decayed, but was NEVER drawn. Now `drawHudAndOverlays` tints the whole screen with alpha min(0.45, flash*1.5).
- **Directional damage indicator**: new `hurtDir`/`hurtDirTimer`/`noteHurtDir(Vector2 src)` (`Game.h` + `Game.cpp`); triggered at the 5 places that damage the player (enemy projectile `Game.cpp`, contact + kamikaze/zergling + AOE explosion in `Game_Gameplay`, wolf in `Game_Resources`); decay in update. Render: trail arrows accelerating toward source + pulsating triangle at screen edge.
- **Boss HP bar** (top-center): panel with name by type (KRONOS COMMANDER/KRONOS NUCLEUS/etc), % + numeric HP, ticks every 10%, pulsating border below 30% HP.
- **Nothing fixed in CENTER of screen** (user complaint): resource HUD (5 colored squares centered at y=56, looked like "menu that never shows anything") docked on the LEFT (x=10, y=80, same style as phase pill: background 8,12,26 + cyan filet); phase pill moved out of center (x=10, y=56); user's power bar stays WHERE IT WAS (bottom-center) and minimap returned to place.
- Gate: Debug exit 0, Release exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED. (Note: running both validations IN PARALLEL causes one to take down the other due to shared rebuild — false "2 stalls >10s"; sequential runs are green.)

## E23 — Resource names + juice round (game feel)
- **Name on each resource** (explicit user request: "need to put the name of each resource"): `drawResourceHUD` redesigned on the LEFT (x=10, y=80, height 40, width 96/item): colored square + quantity + NAME (Wood/Stone/Iron/Silver/Gold) in resource color. `resourceName`/`resourceColor` in `Game_Resources.cpp`.
- **Juice on XP collection**: XP orb gains cyan spark + `spawnExplosion` + damage pop "XP n" (same collection + magnetism, `Game.cpp`).
- **Enemy HP bar**: `drawHudAndOverlays` projects bar with `GetWorldToScreenEx(camera3D)`; boss/elite always, others only when damaged; cull at 900u; white flash with `hitFlashTimer`.
- **Low health vignette**: red gradients at screen edges when `player.health < maxHealth*0.30` (soft pulse).

## E24 — Safe zone becomes the player's HOME (clean hub + shops per NPC)
- **Building clearance in hub**: structure exclusion around the plaza increased 360u → 560u (`Game_WorldGen` `put1`) — no building squeezing the base.
- **PROPORTIONAL shops per NPC** (explicit request): new scenery types 23-27, each with its own procedural 3D render in `Game_WorldRender`:
  - 23 MARKET STALL (LUNA): counter + striped awning + goods;
  - 24 FORGE (BLACKSMITH KANE): anvil + forge with emissive embers + chimney + metal table;
  - 25 COMMAND POST (VANCE RIOS): table + luminescent map + radio + NEXUS banner waving;
  - 26 IMPLANT LAB (DR. CHEN): bench + tank + cyan holoprojector;
  - 27 ARMORY (ZARA): weapon rack + ammo boxes + work light.
  `placeBaseShops()` (`Game_WorldGen`, declared in `Game.h`) places shops in the RING around the plaza, mirroring `setupBaseNPCs()` offsets +52u outward: each vendor ends up IN FRONT of their own shop when coming from the center. Erased with the ramp: rebuilt every phase (called at end of `buildOpenWorldScenery`, after solids — shops do not collide).
- **Safe zone energy ring** (`Game_WorldRender`): translucent cyan/amber floor + 2 pulsating dotted rings rotating in opposite directions + 8 beacons with light + rotating sweep + central energy column (base landmark). Renders only near hub (radius 1250).
- **PURE protected area** (explicit request: "remove wrecked cars, leave only NPCs and their shops"): final cleanup pass in `buildOpenWorldScenery` removes urban debris within `safeZoneRadius` (6 car, 12 rock, 13 fire mark, 21 rubble, 22 campfire); grass/trees/lampposts remain.
- **Minimap of CURRENT PHASE only** (explicit request): `drawMinimap` now uses world window = circle of `owPhaseRadius` (+6%) around base center instead of the whole 128-region world (which squashed blips in a corner). Paints phase disk, pulsating cyan limit ellipse, green safe zone ellipse, "PHASE n" chip, and converts all blips (portals, NPCs, buildings, enemies, viewport, player) with new lambdas mx/my. In closed map keeps old behavior.
- Gate: Release + Debug exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED (sequential).

## E25 — Aim assist (magnetic aim) + lock reticle + lampposts out of street
- **Aim assist on all directional skills**: `aimDir` (`Game_Gameplay.cpp`) now STICKS to the nearest live enemy to the cursor within 120u magnetism radius; direction becomes ALWAYS UNIT. Latent bug fixed along the way: before the vector scaled with mouse distance and the basic projectile ran at different speeds depending on cursor near/far.
- **Aim reticle** (`drawHudAndOverlays`): 4 cyan ticks around cursor (without hiding OS cursor); with locked target reticle turns red, a pulsating lock ring with 4 rotating tips paints the locked enemy target (projection `GetWorldToScreenEx`) and a thin guide line connects cursor→target.
- **Lampposts out of street** (explicit request: "there are lampposts in the middle of the street"): type 5 had no street filter (only tree/car had) — lampposts spawned randomly ON the drawn asphalt. Fixed in 3 places: `place()` (city/bunker) rejects lamppost with dr<118u from road; streaming `put()` rejects same in city biomes; curb lampposts moved from x 363..383 (up to 20u INTO asphalt) to 338..352 (sidewalk, 12..26u off the lane).
- Gate: Release exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED (sequential).

## E26 — Dead code cleanup + elite damage/pitch + procedural sky + self-audit P1/P2
- **Dead code removed**: `ScreenEffectsSystem` was an orphan system (never called, only compiled) — `ScreenEffects.cpp/.h` files deleted and removed from CMake; `Game::drawSkills()` (legacy skills HUD, never called) removed (decl + def). Gain: 700 fewer lines of bug surface, build warns if something disappears.
- **WHITE heavy damage flash** (pending E22): new `eliteFlashTimer` — when an elite/boss hits the player (contact) or elite volatile explosion (`eliteMod==2`), screen flashes white (0.55 max alpha, 2.2× speed) over red flash. Decays in update.
- **Pitch variation on hit SFX**: helper `playPitched(Sound, lo, hi)` (local copy + `SetSoundPitch` + `PlaySound`) applied to `playHit`/`HitHeavy`/`HitAlien`, `playMeleeSwing`/`Impact`, `playMeleeHit`(crit 1.0-1.12), `playEnemyHit`, `playPlayerHurt` — repeated hits no longer sound identical. Explosions and music left as-is.
- **Procedural sky in open world**: besides base biome color, discrete stars (140, deterministic hash, parallax by camera target, night enhances + alpha) + 8 translucent horizon haze bands with soft breathing (sky color lightened). Visual only (does not touch gameplay/FPS gate) and deterministic per frame.
- **Self-audit (subagent + review) — 2 P1 crash/UB fixed**:
  - `Game_Gameplay`: `spawnOmegaBoss()` was called INSIDE the dead enemies loop (`enemies.push_back` invalidated the later iterator `it`). Now `pendingOmega` flag and spawn applied AFTER the loop (same pattern as `splitSpawns`).
  - `Game_Spawn`: on Zergling spawn, `Enemy& e = enemies.emplace_back(...)` was dangling if the 2 `emplace_back` of split reallocated the vector; `e.makeElite` moved to BEFORE the split.
- **Self-audit — 2 P2**: `DarkWorld` seed was `GetRandomValue(1000,99999)` on every visit (same region re-rolled dark drawing); now seed derived from region+phase (`0x343fdu*(region+1) + phase*0x9e3779b9u`) in `Game_Gameplay` and legacy `Game_Phases`. Dead Wall cluster block in `Tilemap::generateOpenWorld` (`clusters = 0` never executes) removed along with orphan rng.
- Gate: Release + Debug exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED (sequential).

## E27 — Building evolution + each PHASE becomes a LARGE OPEN WORLD
- **Living construction (base does not spawn into void)**: units do NOT spawn instantly. `Building` gained `spawnQueue/spawnTimer/spawnTime` (`BuildingSystem.h`); Barracks 5s and Tank Factory 9s per unit. Click production and automatic production enqueue (`spawnQueue++`); `updateBuilding` spawns when `spawnTimer >= spawnTime/m` with m = 1+(level-1)*0.5. `renderUnitPrompts` shows n+spawnQueue with cyan progress bar "Building... %d in queue" (auto-production stays yellow).
- **Enemies destroy what you build**: enemy projectiles now damage ALL buildings (Wall BLOCKS the projectile, others take damage and let it pass — deterministic order), and allied tanks/soldiers also take projectile damage. `spawnTank`/`spawnSoldier` stopped using `GetRandomValue` (deterministic angle by index).
- **Sci-fi visual** (was little farm): house→cyan energy relay, barracks→dark shell + deploy arch, factory→orange core + energy motes instead of smoke, wall→energy mesh with HP, MedBay→capsule + bio-repair cross, holographic scaffolds "ASSEMBLING...", tank→dark APC with cyan core, soldier→armor + cyan visor.
- **EACH PHASE = LARGE OPEN WORLD** (user request: "each phase has to be an open world, it's too small and boring"):
  - `content/phases.txt` radii 3000..4600 → 5200..7800 (step ~260), terminal phase with 118% more area per phase.
  - `setupWorldRegions` (`Game_QuestsNPC`) stops being a 3×3 grid anchored at ORIGIN: becomes ODD square grid, CENTERED at base (`safeZoneCenter`), with cells large enough to cover the ENTIRE phase disk (`cols = f(owPhaseRadius)`; 5×5 in phase 1, up to 7×7 in final phase). All biomes use `currentZone` (one phase = one world). Hub region found by containment (not `worldRegions[0]`, which was the corner). Compass names ("North-East Sector").
  - Animals and resource nodes (`Game_Resources`) and eager scenery (`buildOpenWorldScenery`) now automatically cover the whole disk (iterate dynamic `worldRegions`).
  - Minimap (`Game_HUD drawMinimap`): removed clamp to worldW/H (infinite world, phase > old 7680u area); grid lines come from BOUNDS of dynamic regions.
  - Difficulty gradient by radius (`Game_Spawn`) re-anchored at `safeZoneCenter` (before averaged at 3×3 grid center 3840,3840, misaligned from base).
  - Chunk generator (`updateSceneryChunks`): ORIG guard (0..24576) changed to "only generate where there is NO eager region" — no scenery duplication in negative coordinates (phase now covers all four quadrants).
  - Hub visual (war plaza, roadblocks) uses `safeZoneCenter` (was zone 0 center == 1280,1280 by coincidence).
  - Phase state reset on New Game/Restart (`owPhase=0`, phase 1 radius) and REBUILT from saved zone on Continue (save did not persist `owPhase`/radius → "Continue" inherited old phase).
  - Gate: Release + Debug exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED (sequential). Bot: 1 advanced phase (CursedFarm 5×5), 4/4 quadrants explored, 24 kills, min FPS 52 / avg 59, 0 deaths.
- **CI on main**: `.github/workflows/ci.yml` (windows-latest, configure -A x64, build Release darknet+darknet_tests + Debug darknet, tests, validate 120 seed 7 and 20260821 sequentially with `test "$code" -eq 0` + grep "VALIDATION: PASSED"); `master` branch renamed to `main`, `default_branch=main`, `origin/HEAD` points to main; workflow passes reading the version with this E27.
- **Fix CI — `--headless` mode**: on GitHub Actions runner (Windows Server 2025, no display/GPU) validate segfaulted (exit 139) when creating raylib GL context, BEFORE printing any INFO. Fix: `--headless` flag setting `Game::headless` in `main.cpp` BEFORE constructor — constructor skips `InitWindow`/render textures/`SpriteBank`/models/postfx/lightSystem/background (keeps `audio.init()` without device + `loadPhaseDefs`/`buildQuests`/`buildNPCs` + open world and phase), `runHeadless()` runs the SAME `update()` in real time (own loop dt, no render/presentFrame/screenshot/menu), bot FPS measured from own loop (`GetFPS()=0` without window), destructor only frees what was created. ci.yml passes `--headless` on both validates; `validate.sh` gains 3rd argument (headless=1). Local headless gate: Release+Debug exit 0, tests 12/12 (109), validates APPROVED (bot 21 kills, 4/4 quadrants, 1 phase advanced through portal, 0 deaths, no stuck). CI green on run 33961092413 (commits fb31655 + db9531d).

## E28 — Player buildings "real sci-fi" (were little castle/square medieval)

- **User complaint**: "the buildings the character constructs all look like a tiny castle or a square. they have to match the game. recreate everything" — internet research of competitors (sci-fi top-down base building assets: isometric industrial factory with exhausts + cylindrical core, bunkers with emissive windows) consolidated direction: **dark gunmetal shell + emissive accents by function + readable silhouette from afar + power pad on ground**, consistent with the rest of the game (tile 64u, hero ~66u).
- **Cause**: the 3D building loop rendered **medieval OBJ models** (`castle.obj`/`house.obj`/`barracks`/`market`/`well`/`turret`) — the little castle/square on asphalt. The futuristic 2D views in `BuildingSystem.cpp` were dead code (`BuildingSystem::render` had no call site).
- **New `drawPlayerBuilding(const Building&)`** (static, `Game_WorldRender.cpp`) replaces models in rendering ALL buildings, with shadow + power pad ring + **holographic scaffold** with pip bar for under construction (`!b.built`):
  - Ark = command bunker + beacon mast + heal aura (`healRadius`) + emissive corner pillars;
  - House = energy pylon with generation pulse (`genTimer`/`genRate`) rising up the column + side conduits + level rings;
  - Barracks = deploy gate (arch + cyan flaps) + **pulsating spawn beam** with `spawnQueue`/`spawnTimer`/`spawnTime`;
  - Tank Factory = pulsating orange core + 3 exhaust chimneys with rising heat motes + manufacturing doors + **pip production bar** (`productionTimer`/`productionRate`);
  - Tower = rotating turret dome + dual cannons following `shootDir` (`atan2f`) + emissive muzzle + range ring at level 2;
  - MedBay = teal stasis station + medical cross hologram on facade + side pods + aura;
  - Wall = energy parapet: emissive top blade + pulsating cells + reinforcement pillars by level;
  - ResourceNode = orange crystal with orbital clamps.
- **False build positives investigated**: (1) `const Color ORANGE/RED` collided with **raylib macros** (same name) → renamed to HOT/DEF; (2) rewriting file via PowerShell `Set-Content -NoNewline` **fused the .cpp into a single line** (537-byte "empty" objs, phantom LNK2019) → source restored via git and reapplied with the editor tool.
- Gate: Release + Debug exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED (sequential, windowed); headless untouched (does not render). Autotest screenshots (`shot_00..03.png`) for human visual review.

## E29 — Living sci-fi world: building materialization and surveillance drones

- **User request**: world too "alive"? — asked for "living world" sci-fi.
- `Building.builtAt` (`BuildingSystem.h`) set on completion (`BuildingSystem.cpp`); `drawPlayerBuilding` opens with **materialization** for 1.2s (expanding cyan→white ring 20→75u + 6 sector sparks) before shell compacts.
- **`drawAmbientDrones`** (static, `Game_WorldRender.cpp`): 4 drones orbiting camera target, deterministic by time (irregular orbit, 4-sample light trail, gunmetal shell, blinking beacon, rotating watch ring); called in `renderWorld3D` gated by `openWorldMode`, before AMBIENT LIFE block. Zero particles = frame determinism preserved.
- Gate: Release+Debug OK, tests 12/12 (109), validates 120/7 and 120/20260821 APPROVED.

## E30 — Fair difficulty + end of trees in street + "war code" scenery

- **User requests**: "enemies are too easy to kill — see how big games do it, implement, create, innovate" and "we still have trees in the middle of the street in phase 1, fix and add more elements to scenery" and "there has to be destroyed asphalt parts, war code scenery, destroyed buildings, smoke".
- **Difficulty (ARPG/Risk of Rain style, without breaking validate bot)**:
  - `Enemy::armor` — FLAT damage reduction per hit (Diablo/Hades-style mitigation): Armored elite gets 8u (+4 per phase). Weak weapon barely scratches anymore.
  - `Enemy::shieldHp/shieldMax` — new **Shielded** elite mod (3): shield absorbs 75% of each hit until empty and **recharges 10%/s** (RoR2).
  - **KronosRapid** mod (4): +35% speed, +30% damage, and self-repair 2%/s.
  - Elite chance now **scales with phase and refuge radius**: 12% + 3%/phase + 5%/ring, cap 45%; elite roll excludes Zergling (swarm fodder).
  - **Open-world phase curve**: +8% HP and +5% damage per phase after phase 1.
  - Phase 1 untouched in curve (`owPhase` 0) and elite ~12% at base — 120s autotest remains APPROVED.
- **Trees in street — complete closure**: streaming guard already followed `currentZone == LARuins||GhostCity` (EXACT condition of road drawing) and 170u canopy clearance on roads; now the **central 400u plaza of the hub is tree-free** (the drawn road passes through it). No career condition for "tree on avenue".
- **"War code" scenery** (types 28-31, procedural render in `Game_WorldRender` + per-frame `drawSmokeColumn` volutes, no particles):
  - 28 BOMB CRATER: carbonized bowl + thrown dirt rim + tossed slabs + embers + smoke;
  - 29 COLLAPSED BUILDING: broken skeleton + fallen slab + rebar + dust;
  - 30 DESTROYED ASPHALT: cracked charred plate + crumbling edge (decor, does not block);
  - 31 BURNED WRECK: twisted hull + bent mast + campfire + heavy smoke.
  - Distribution: LA/GhostCity city in fixed world (`place` per block) AND in streaming (`put`/prop roll): crater/asphalt/wreck enter phase 1 roll and GhostCity gets craters; collision marked (28/29/31 block with moderate footprint, 30 stays decor).
- Gate: Release+Debug exit 0, tests 12/12 (109), validate 120/7 and 120/20260821 APPROVED (sequential, windowed); CI headless does not render new props. Screenshots `shot_00..03.png` for visual review.

## E31 — Audit of advanced phases (5-10) and `--start-phase` fix

- **Objective**: validate that campaign phases 6-11 (indices 5-10 in `content/phases.txt`) generate the correct biome, spawn enemies, allow combat, and pass the bot validation gate.
- **Added instrumentation**: `--start-phase=N` parameter in `main.cpp` + static flag `Game::startPhaseOverride` (same pattern as `Game::headless`), read in `Game` constructor to initialize `owPhase`, `currentZone`, `currentRegion`, `owPhaseRadius`, and `owPhaseGoal` from `phaseDef(N)`.
- **Defect found**: `Game game;` was created BEFORE `game.startPhaseOverride = N`, so the constructor always used phase 0 (`LARuins`); `SCENERY` log showed "Ruins of Los Angeles" even with `--start-phase=5`. Fix: make `startPhaseOverride` static and set `Game::startPhaseOverride` before construction.
- **Headless autotest results (120s per phase, Release, random seed)**:
  | Phase (index) | Zone | SCENERY | Validation |
  |---|---|---|---|
  | 5 | NEXUS Bunker | 9,790 objects, 312 structures | **PASSED** |
  | 6 | Catacombs | 10,733 objects, 292 structures | **PASSED** |
  | 7 | Abandoned Manor | 11,753 objects, 336 structures | **PASSED** |
  | 8 | KRONOS Forge | 12,902 objects, 385 structures | **PASSED** |
  | 9 | Inferno Zone | 14,003 objects, 407 structures | **PASSED** |
  | 10 | KRONOS Nucleus | 15,486 objects, 569 structures | **PASSED** |
- **Terminal phase metrics (KronosNexus, 120s)**: 130 kills, 119 skills fired, 88 melee attacks, peak 82 enemies, 513 items collected, 0 deaths, 0 stalls >10s, minimum FPS 388.
- **Note**: 30-60s tests failed sporadically due to bot spawn/movement randomness; 120s per phase stabilized the gate.
- **Gate**: Release build OK; headless autotest phases 5-10 APPROVED; no gameplay change — only audit instrumentation + fix of override application point.
