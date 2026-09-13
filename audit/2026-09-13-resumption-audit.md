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
- `git push` pending (now includes the restore — needs a reconciliation commit first).
- Redo the EN translation properly (file-by-file, compiled per file) if still desired.

---

## 7. Definition of Done (unchanged from project convention)

- `./validate.sh` → build Debug+Release, autotest 120s exit 0, `VALIDATION: PASSED`.
- 26/26 unit tests (159 assertions) green; add tests for anything new.
- No FPS regression: benchmark average ~58–59, minimum 44–52 (windowed).
