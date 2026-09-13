# Darknet Prototype — Resumption Audit
**Date:** 2026-09-13 | **Auditor:** Kimi Code CLI | **Scope:** full project state verification + backlog consolidation

---

## 1. Executive Summary

The project is in a **validated, advanced-prototype state** and ready to resume work.
Build, unit tests, and headless bot autotest all pass on a fresh machine, with no
code changes required. The authoritative backlog remains `audit/2026-09-06-full-audit.md`
(**29 P0 / 61 P1 / 68 P2 / 32 P3 = 190 findings**); this document verifies which of
those findings are still live and consolidates the resumption plan.

---

## 2. Verified State (empirical, this session)

| Check | Result |
|---|---|
| Working tree | Clean (`git status` empty) |
| git vs origin | **1 commit ahead** (`854cf18` — needs `git push`) |
| Unit tests | ✅ **26/26 cases, 159/159 assertions** (doctest, Release binary) |
| Headless autotest | ✅ **VALIDATION: PASSED** (120s, seed 20260913, Release) |
| Bot gameplay | 23 kills, 46 items, 4/4 quadrants, **1 zone advanced**, 0 deaths, 0 stuck |
| Phases loading | 11/11 phases loaded from `content/phases.txt` |
| Backend (server/) | Node.js "Cyber Station" — auth, Stripe gems store, WS rooms, Postgres; Docker |

### Notable improvement since last report
The 2026-09-07 bot run flagged **"Bot did not advance zones in 60s"**. Today's 120s
run shows `ZONE ADVANCED! phase=2 total=1` — the bot now navigates to the phase
portal and transitions correctly. Zone progression mechanics are functional.

### Internationalization cleanup — still incomplete
Recent commits translated PT→EN docs/comments, but runtime strings remain in PT:
- Bot log: `Unidade produzida`, `Farm Maldita`, `Ruins de Los Angeles`
- Zone/scenery names in `content/` and code
- Source files still containing PT-EN pidgin: `NetClient.h`, `main.cpp`
- README links contain broken `*.with` domains (translation find-replace artifact)

---

## 3. Codebase Health

- **Stack:** C++17, raylib 5.5 (FetchContent), doctest, nlohmann/json; Node.js backend.
- **Size:** ~35,500 lines across ~75 source files; `Game` remains a God Object
  (~160 members) physically split into `Game_*.cpp` partitions without real encapsulation.
- **Tests:** only 26 cases covering pure rules (Crafting, Enemy, SaveManager, Player,
  Item, Quest, Skill, SkillTree, Projectile, Tilemap). **Zero coverage of `Game`,
  rendering, netcode, or gameplay integration.**
- **Assets:** 8 OBJ + 2 GLB models, 5 GLSL shaders, no audio files (100% procedural).
- **Dead/disconnected systems (P0, confirmed still relevant):**
  - `TutorialSystem` — never instantiated; reward never granted.
  - `AchievementSystem` — initialized but events never reported; rewards never granted.
  - Skill tree tier 3 unreachable (requirement bug).
  - Level-up bonuses lost on re-equip/next level-up.
  - Premium cosmetics purchased are not rendered (incl. Pet Drone spawn).
- **Security/network (P0):** client rejects `wss://` (no TLS); client-authoritative
  progress writes; `/auth/login` stub (any name → valid token); WS frame parser
  arithmetic overflow risk; data race on `NetClient::enabled`.
- **Determinism (P0):** `InfernoZone` uses global `rand()` without seeding —
  breaks the determinism the bot/CI seeds depend on.
- **Hardcoded:** `ws://127.0.0.1:9000/ws`, host `127.0.0.1` in `NetClient.h`,
  `StoreClient.h`, `Game_Network.cpp` — no external config.

---

## 4. Backup Verification (`3 - darknet-prototype-local-backup`)

The backup is **not** a code backup — it preserves exactly the artifacts the
`.gitignore` deliberately excludes from the public repo:

| Category | Contents |
|---|---|
| Coordination docs (PT) | COORDENACAO.md, AUDITORIA_COMPLETA.md (jun/2026, **superseded**), BACKLOG/ROADMAP_EXECUTIVO_CLAUDE.md, PARA_ANTIGRAVITY.md, CLAUDE_CODE_GUIDELINES.md |
| Bot/test logs | autotest_*.log, validate_seed*.log, bot_report*.txt (incl. **bot_report_VITORIA.txt** — full game clear) |
| Visual audit | screenshot000.png, vox_audit_*.png |

**⚠️ Risk:** `AUDITORIA_COMPLETA.md` (jun/2026) predates the big Aug–Sep refactor.
Anyone reading it will believe the God Object / broken build are current state. The
authoritative audit is `audit/2026-09-06-full-audit.md` in the project. Mark old
docs as superseded if touched.

Also preserved locally (by design, gitignored): `obsidian-vault/` (11-note personal
design knowledge base) and `saves/darknet_slot0.txt` (V7 test savegame, 1.4 KB).

---

## 5. Resumption Plan — Recommended Priorities

### Wave 1 — Quick wins / critical fixes (P0 sweep)
1. **Save system hardening:** atomic write, checksum already exists but header
   version accepted blindly; persist `totalKills`, `equipBag`, equipment
   rarity/affixes/upgrades; clear residual state on load.
2. **Wire up dead systems:** instantiate `TutorialSystem`; connect
   `AchievementSystem` events. Low effort, high visible value (features already
   announced in README).
3. **Determinism:** seed `rand()` (or replace with the project's RNG) in
   `InfernoZone`; audit all `rand()` call sites.
4. **Skill tree tier 3 gate** fix; **level-up bonus loss** fix.

### Wave 2 — Player-facing correctness
5. Render premium cosmetics (incl. Pet Drone) in `Player.cpp`.
6. Quest completion criteria fix; double knockback fix; building passive income
   multiplicative explosion rebalance; key rebinding (SHIFT/E overload).
7. Unit tests for pure rules (economy, loot, combat math) — the ROADMAP's own
   short-term item; cheap given doctest is in place.

### Wave 3 — Multiplayer & distribution
8. Real co-op: versioned protocol, server-side validation, authoritative state
   (current WS is a relay), `/auth/login` real implementation, `wss://` support.
9. Stripe/HTTPS production launch prep (LGPD, anti-fraud).
10. Content: Acts 2–5 (only Act 1 exists) — largest scope item; see
    `DARKNET_STORY.md` (5 acts, 3 endings, NG+ boss ARCHON DIMENSION ZERO).

### Housekeeping (this session's findings)
- `git push` the pending commit `854cf18`.
- Finish PT→EN string cleanup in runtime/log output.
- Fix README `*.with` broken links.
- Mark backup docs as superseded or archive them.

---

## 6. Wave 1 — Execution Results (2026-09-13)

### Critical discovery: HEAD did not compile
The four Sep-7 translation commits (`642e959`, `08a6539`, `b69d920`, `854cf18`, all
**unpushed**) mangled ~800 identifier sites across 77 files (`a`→`the`, `e`→`and`,
`do`→`of the`), breaking compilation. The working exe predated them (incremental build
never recompiled the corrupted files). Fix approved by user: **restored `src/` and
`tests/` from `cbe96e1`** (last provably-compiling state). The broken commits remain in
local history; the working tree now differs from HEAD and needs a reconciliation commit.
Also fixed pre-existing corruption: literal newline inside a char constant at
`Game_Phases.cpp:54` (repaired as `\r`, matching the HEAD repair).

### Verified already-fixed (audit stale)
Save system (atomic+checksum+V7), tutorial reward (`tutorialRewardGiven`),
InfernoZone seeded RNG, level-up bonuses (base stats), skill-tier math.

### Fixes applied this session
1. **`Player::onKill()` was dead code** — now called on every enemy kill: activates
   kill-streak counter/speeches, lifesteal perk, and feeds `achievements.onKillStreak`.
2. **Achievement hooks**: `onItemFound` (pickup, with rarity), `onZoneVisited`
   (phase advance + open-world region crossing + legacy zone transition).
3. **AudioManager RNG isolation** — own `std::mt19937`; audio no longer perturbs the
   global `rand()` used by gameplay rolls (evade/drops) between seeded runs.
4. **SkillTree regression tests** — 2 new cases proving tier gating and that all 12
   perks are reachable. Tests: **26→28 cases, 159→194 assertions**. Doc counts
   updated (README, CONTRIBUTING, PR template).

### Validation
- `darknet_tests`: 28/28 cases, 194/194 assertions ✅
- Autotest headless 120s (seed 20260913): **VALIDAÇÃO: PASSOU**, exit 0 — 34 kills,
  93 items, zone advanced, 0 deaths, 0 stuck; damage taken dropped 42→22 (lifesteal live).

### Still open from Wave 1
- Redo the EN translation properly (file-by-file, compiled per file) if still desired.

---

## 6.1 Wave 2 — Execution Results (2026-09-13)

### Verification: all four "player-facing" P1s from the 09-06 audit are STALE
Empirically verified in the restored code (cbe96e1 baseline):
- **Premium cosmetics**: `skinNeon`/`skinDragon`/`petDrone`/`cosmeticTint` ARE rendered
  in `Player.cpp` (lines ~552-596) and included in the voxel cache signature — already fixed.
- **Quest completion criteria**: all five `updateProgress` call sites are correctly
  type-filtered (Kill/KillBoss/Collect/CollectRare/ClosePortal/Zone) — already correct.
- **Double knockback**: `Enemy::applyKnockback` has a single call site
  (`Game_Gameplay.cpp` melee) — already correct.
- **Passive income explosion**: House income scales LINEARLY with level
  (`genRate = 8/m`, m = 1.0/1.5/2.0) — working as designed.
- **SHIFT/E overload**: still real (SHIFT = sprint + RTS select at
  `Game_Gameplay.cpp:1541/1577`; E = equip/dialog/portal) — documented as a known
  design compromise; full key rebinding is a feature, not a fix.

### What Wave 2 actually delivered: unit tests for pure rules (ROADMAP short-term item)
6 new test cases (28 → 34 cases, 194 → 1430 assertions):
- Quest progress clamping / inactive no-op / progress text
- Item economy: credits/tech/elite drop consistency
- Loot table: 600-roll distribution stays within the Common/Uncommon table
- Combat math: `SkillTree::statsFor` multiplier composition (multiplicative stacking),
  evade/lifesteal/revive perk effects, `Player::getEffectiveDamage` with perk mult

### Validation
- `darknet_tests`: 34/34 cases, 1430/1430 assertions ✅
- Game binary unchanged from the Wave-1 validated build (autotest PASSED).

---

## 6.2 Waves 3-4 — Full audit verification (2026-09-13)

Every remaining finding from `audit/2026-09-06-full-audit.md` was re-verified against
the current tree. **~85% were already fixed** (the Sep-7 pre-translation work had
addressed nearly everything). Verified-stale highlights:

| Finding | Verdict |
|---|---|
| NetClient `enabled` data race | Stale — already `std::atomic<bool>` |
| WS frame parser arithmetic overflow | Stale — `len > rx.size() - pos` guard present |
| No Sec-WebSocket-Accept validation | Stale — SHA1+GUID+base64 check present |
| sendChat manual JSON | Stale — uses `nlohmann::json` |
| world.vs normal `w=1.0` | Stale — uses `vec4(normal, 0.0)` |
| EnemyDirector not a moving average | Stale — `*0.6f + dpm*0.4f` |
| sfxPlayerDeath never played | Stale — wired at Game_Gameplay.cpp:929/939 |
| Extra zerglings without scaling | Stale — mechanic refactored, scaling uniform |
| Projectile vs OW boundary | Stale — `isOutsideOpenWorldBounds` + radius overload |
| isWallAtPosition single point | Stale — radius overload exists |
| Enemy spawn beyond barrier | Stale — `clampInsideOpenWorldBounds(pos, 120)` |
| Anomaly portals outside playable disk | Stale — spawnWave takes diskCenter/diskRadius |
| Player no X/Y sliding | Stale — axis-separated slide implemented |
| Anomaly enemies spawn in walls | Stale — `isFree` retry loop (8 tries) |
| Voxel models generated but never rendered | Stale — BuildVoxelModel removed from render flow |
| StoreClient thread accumulation | Stale — finished threads joined in startThread |
| Audio dead code after composeTrack | Stale — bodies removed, direct TrackStyle config |
| CraftingSystem filtered/global index mix | Stale — tryCraft maps via `filtered[selected]` |

### Real fixes applied (Wave 3-4)
1. **`canAutoSave()`** — blocks auto-save while paused/dead/in-dialogue/phase-fade/
   choice-screen/final-boss; applied to the 30s periodic save and zone-transition save.
2. **ShopSystem duplicate cosmetic purchase** — `ownedCosmetics` list blocks re-buy.
3. **Stripe checkout URL validation** — only `https://checkout.stripe.com/` or
   `https://buy.stripe.com/` URLs open in the browser (was: any server-returned URL).

### Still open after Waves 1-4 (verified real)
| Priority | Item | Scope |
|---|---|---|
| P0 | `Game` God Object / subsystem refactor | Large, strategic |
| P0 | `wss://` TLS + server-authoritative progress | Large, launch-blocking |
| P0 | Hardcoded keys → configurable InputMap | Medium-large feature |
| P2 | Multi-slot save UI (`renderSaveSlots` never called) | Small feature gap |
| P1 | `/auth/login` stub (any name → token) | Server, launch-blocking |
| P2 | nginx HTTPS block commented out | Launch |
| P1 | Entity frustum culling, particle blend batching, scanlines batching, per-entity render state | Perf (FPS already 58-59/44-52) |
| P2 | Duplicate vignette; screenshot thread join; fixed 720p internal res; tileZone/biomeAtWorld divergence; A* pathfinding | Polish |
| P2/P3 | Constants consolidation; strict CMake warnings; accessibility options; JSON save schema | Hygiene |
| P1 | SHIFT/E overload | Documented design compromise |

---

## 6.3 Post-audit developments (2026-09-13, same session)

| Commit | Change |
|---|---|
| `6aec273` | **Dual license**: code MIT / content (resources/, content/, DARKNET_STORY.md) CC BY-NC 4.0 © OmegaSoftDLL; trademarks "KRONOSFALL"/"Darknet"/"KRONOS" reserved. Public repo stays commercially viable (Mindustry/Shapez model). |
| `9981c28` | **New Game+**: [N] on victory screen restarts the world keeping the character; NG+ final boss is **Leviathan** (8k-HP multi-phase boss that existed in code but was never spawned — dead content activated). `ngPlus` persisted in save V7 (backward compatible). |
| `8eb65ff` | **Infrastructure spec** (`server/INFRASTRUCTURE.md`): full handoff doc — topology, TLS/WSS, backups, secrets, hardening, observability, LGPD, ~US$10-45/mo, 9 acceptance criteria. |
| `fdbd8d6` | **Steam-first decision** (PO): zero own infra for launch; multiplayer post-launch via Steam SDR; Steam Wallet mandatory for gems on Steam (30%); Stripe only for a possible future standalone build. Spec kept as backlog. |
| `9cbf9d4` | **Rebrand: Darknet Prototype → KRONOSFALL** — tagline "The Darknet is falling. Make Kronos fall." Reason: "Darknet" is an existing Steam game (app 401910). Repo renamed to `OmegaSoftDLL/kronosfall`; in-game title/menu/tutorial/window updated; `DARKNET_SAVE_V` header and `DARKNET_*` env vars preserved for compatibility. |

**Branch protection applied**: `main` requires PR + 1 approval + green CI ("Build + tests + validate (2 seeds)", strict); force push blocked; admin bypass enabled for emergencies.

**Strategic baseline for the next phase**: public repo, dual license, Steam-first, launch is single-player offline. Roadmap: gameplay depth (Act 1) → Act 2 + meta-progression → trailer → Steam page/Next Fest. CI blocked by GitHub account billing lock until 2026-10-05 (account-level, resolved by the owner).

---

## 7. Definition of Done (unchanged from project convention)

- `./validate.sh` → build Debug+Release, autotest 120s exit 0, `VALIDATION: PASSED`.
- 26/26 unit tests (159 assertions) green; add tests for anything new.
- No FPS regression: benchmark average ~58–59, minimum 44–52 (windowed).
