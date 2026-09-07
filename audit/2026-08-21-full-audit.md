# FULL AUDIT — Darknet Prototype

| | |
|---|---|
| **Date** | 2026-08-21 |
| **Audited commit** | `6113e73` (master) |
| **Binary** | `build/Release/darknet.exe` from 21/08 16:13, after the last code edit; `content/` and `resources/shaders/` present next to the exe |
| **Auditor** | Claude (audit-only role — see `.agents/AUDITOR.md`) |
| **Verdict** | **APPROVED WITH RESERVATIONS** — on 1st pass: 1×P0, 4×P1, 2×P2. **Updated by addendum (§6): P0 downgraded after verification → 0×P0, 4×P1, 5×P2.** |

This report is for the programmer. Each finding includes where, evidence, repro, and the
player effect. What was **measured** is separated from what was **deduced** — do not
mix the two when prioritizing.

---

## 1. Methodology

Two full validation-gate runs, distinct seeds, 120 s each, in Release:

```bash
./validate.sh 120 7
build/Release/darknet.exe --autotest --test-seconds=120 --seed=20260821
```

Both: **`VALIDATION: PASSED`** (exit 0) — first gate approval on this base. Additionally: inspection of the 10 `shot_NN.png` frames recorded every 10 s from run seed 20260821, and code checking at points the frames raised.

### Measured numbers (2 runs)

| Metric | seed 7 | seed 20260821 |
|---|---|---|
| Average FPS | 59 | 58 |
| Minimum FPS | 6 | 6 |
| Kills | 20 | 31 |
| Deaths | 0 | 0 |
| Stuck >2 s / critical >10 s | 1 / 0 | — / 0 |
| Quadrants explored | 4/4 | 4/4 |
| Distance traveled | 19,240 px | 22,199 px |
| Worst update() / render() | 1.27 / 44.5 ms | 1.47 / 28.7 ms |
| **Advanced zones** | **0** | **0** |

The stuck-detector fix (commit `6113e73`) is confirmed in practice: the bot
started engaging again (20–31 kills vs. 0 in previous runs) and the false stuck
report disappeared (0 critical).

---

## 2. Findings

### [P0] Phase portal can spawn inside a building — progression soft-lock

> **⚠ SUPERSEDED BY ADDENDUM (§6):** verified by deterministic reconstruction —
> 120/120 variants with reachable portal. Downgraded to residual risk
> (P2: make guarantee intentional). Text below is kept as a record of the
> original hypothesis and why it was plausible.

- **Where:** `src/Game.cpp:5486` — `owPortalPos = safeZoneCenter + (620, −520)`,
  fixed position defined when the kill quota closes.
- **Evidence:** **DEDUCED from code, not observed in execution.** Scenery is
  baked at phase start (`buildOpenWorldScenery`); nothing clears or protects the
  portal point. The hub protected plaza has radius 360; the portal is ~810
  from center — area where the city grid places buildings. Modern building marks
  solid tiles with radius `125 × scale` (`markSolidAt`); portal interaction requires the player to be **105 u** from center (`Game.cpp:5491`). If a building occupies the point, there is no way to get close enough: **the phase never closes**.
- **Suggested repro (disposable script, without touching the game):** sweep seeds
  1–200 calling `isBlocked(owPortalPos)` after world generation and count collisions. Any rate above zero confirms the P0.
- **Player effect:** dead progression with no error message. The kind of bug that
  escapes all manual testing and becomes a negative review.

### [P1] Phase advancement flow has ZERO automated coverage

- **Where:** `src/Game.cpp:4252` — bot receives `tilemap.portals` (old
  portal system, **empty in open world**); `owPortalPos` is never passed to
  `BotController`, and advancement requires `IsKeyPressed(KEY_E)`.
- **Evidence:** **MEASURED** — `Advanced zones: 0` in all validated runs to date.
  `advanceOpenWorldPhase()` (regenerate world, rewards, boss phase,
  transition screen) **never executed under the validation gate**. The newest
  central game mechanic is the only one with no safety net; phases 2–11 are,
  in practice, untested.

### [P1] HUD promises "follow the marker" — the marker does not exist

- **Where:** `src/Game.cpp:8048` (text) vs. all uses of `owPortalPos`
  (lines 5486, 5491, 7077, 7102 — spawn, proximity, draw, light; **no
  directional indicator**).
- **Evidence:** **MEASURED** — frame `shot_09` (seed 20260821, t≈99 s) shows
  "PORTAL OPEN — follow the marker and press [E]" with no arrow, no radar blip,
  nothing pointing the way. The visible pink arrows are off-screen enemy indicators.
- **Effect:** the player has to guess the direction of an unsignaled point in a
  world of radius 3000+. The game's own instruction lies.

### [P1] Player base is a medieval castle in the modern city

- **Where:** `src/Game.cpp:7287` — `BuildingType::Ark` draws `castle.obj`;
  `:7289` — RTS `House` draws `house.obj` (tiled roof).
- **Evidence:** **MEASURED** — frame `shot_04`: red-towered castle with tooltip
  "Ark [Lv 1/3] — Base Core" surrounded by asphalt, pedestrian crossing and
  concrete buildings. The coherence cleanup (per-biome catalog, commit `c66d2dc`) reached the **scenery**, but not the **BuildingSystem** constructions — which stay center of attention the whole game (it's where the player respawns).

### [P1] Sustained ~6 FPS window at match opening

- **Evidence (MEASURED):** 38+ consecutive `Critical FPS: 6` samples in
  both runs, with **0 enemies and 0 projectiles** at the minimum instant, and worst
  `update()` of only 1.3–1.5 ms — the cost is not in simulation nor the common frame.
- **Cause (DEDUCED):** voxel model generation at match entry.
  `SpriteExtrude::CaptureToImage` creates a RenderTexture **per capture** and does
  `LoadImageFromTexture` (GPU→CPU readback = pipeline stall). ~40+ models
  (types × 4 walk poses), amortized at 3 per frame (`m_voxGenBudget`) — creates a
  window of several seconds at ~6 FPS right in the game's first impression. The `update()/render()` instrumentation does not seem to cover this pre-pass, which explains why the worst measured render (44 ms) does not match 6 FPS (166 ms/frame).

### [P2] Melee practically inert in automation

- **Evidence:** 1 melee attack in 240 s of bot running, against 120 skill shots. Either the melee distance is misaligned with enemy bodies, or the bot never chooses this action. Today the melee metric in the report measures nothing — and the melee system is effectively without coverage.

### [P2] FPS report alert is misleading

- **Where:** `bot_report.txt` — "PROBLEM: FPS dropped below 40 — optimization needed".
- **Evidence:** the alert fires because of the opening hitch (P1 finding above) and masks that the game **sustains 58–59 FPS** the rest of the time. An alert that cries wolf is an alert the programmer learns to ignore; the right fix is to separate "load hitch" from "game FPS".

---

## 3. What is good (confirmed in frames)

- Cars read as cars: wheels, recessed cabin, fenders (`shot_09`).
- Streets with curb and worn lane give direction and city readability.
- Allied units use the soldier model with green ally ring — end of green "pins".
- Buildings with rows of windows, varied materials and some in ruins.
- Stability: zero crash, zero death, navigation without critical freeze in
  4 min of summed Release.

---

## 4. Not audited (declared)

- **Save/load** (roundtrip of continuing a match).
- **Multiplayer, premium store, backend** (require active server).
- **Audio** — the new chain (filter, reverb, sub, compressor) compiles and runs
  without crash, but the auditor does not listen: requires human judgment.
- **Phases 2–11 in real execution** — no run reached there (direct consequence
  of the zero portal coverage finding).
- Human input (mouse/keyboard), balance beyond phase 1.

---

## 5. Highest-return fixes (in order)

1. **Portal at a guaranteed free point** — validate/relocate `owPortalPos`
   (or clear solids in a radius when opening). Kills the P0; as a bonus, a future
   marker will have a reliable target.
2. **Bot crosses the portal** — inform `owPortalPos` to `BotController` and
   simulate [E]. The central mechanic enters the validation gate and phases
   2+ are exercised automatically. Without this, all content beyond phase 1 stays in the dark.
3. **Ark/RTS without medieval models** — it is the most-looked-at object in the game
   (respawn point) and contradicts the art direction defined for urban zones.

> Risk note: all three touch `Game.cpp` (8,000+ lines concentrating render,
> update, world and UI). Treat any change there as high regression risk and run
> `./validate.sh 120 7` **and** a second seed before committing.

---

## 6. ADDENDUM (2026-08-21, same day) — Verification of portal P0

### Method

Scenery generation **does not use `--seed`**: `buildOpenWorldScenery` runs its own LCG
with fixed constant `0x1234abcd` and zero calls to `GetRandomValue`
(verified in source). Chunks do not generate inside the fixed region (`ox < ORIG`).
So the portal surroundings are a **deterministic fact**, not a probability —
"seed sweep" was the wrong instrument; the right one is to reconstruct the layout.

I ported the region (0,0) generator to a measurement script (scratchpad, outside the
repo) and validated it against the checksum the game itself logs (`SCENERY
total/structures/grass`). The replica reached ~99.3% of the checksum (2992–2999 vs
3014; grass 1439 vs 1440) — the exact MSVC stream was not reproduced (argument
 evaluation order / float32). To shield the conclusion from this residue, I ran
**120 stream variants** (2 evaluation orders × 2 × 30 initial LCG offsets): the grid
geometry (cells, EDGE=350, slots) is fixed; only which lots exist, types and scales vary.

### Result

**120/120 variants: portal interaction disk FREE and REACHABLE on foot from the hub** (BFS on the solid tile grid). Worst case: 12 of the ~13 disk tiles free. Structural reason: the portal (hub + 620,−520 = 4716,3576) falls in the **street corridor** between lot rows (y=3420 and y=3670); the nearest possible lot is ~123u from the portal with maximum footprint of 96u (modern building, scale ≤1.0), and the interaction radius is 105u — no isolated structure covers the disk, and combined coverage never occurred.

### Reclassification

> **P0 "portal can spawn inside building" → DOWNGRADED to residual risk
> (monitor).** Not confirmed in phase 1 under 120 variants. Becomes P0
> again if: portal offset changes, `EDGE`/grid changes, catalogs gain footprint > 105u near the hub, or interaction radius shrinks. Structural protection is accidental — no line of code guarantees the corridor free; a guard comment in code (or an `!isBlocked` assert when opening the portal) would make the guarantee intentional. Kept as P2 recommendation.

### New findings discovered during verification

**[P2] World scenery is built TWICE during initialization**
- **Evidence (MEASURED):** `gt.log` lines 468–469 — two consecutive `SCENERY` logs with
  different counts (`total=1626/grass=749`, then `total=3014/grass=1440`).
- **Deduction:** first build runs with old `safeZoneCenter` (~(1280,1280) — replica with this center approximates 1665/757 ≈ 1626/749); the second, with the correct center (4096,4096), is the one that counts. The first is wasted work and contributes to the opening hitch (P1 finding already reported).

**[P2] Phases 2+ keep LA city in the hub, over another biome's ground**
- **Evidence (DEDUCED from code):** `setupWorldRegions` always rebuilds the same 3×3 map; region (0,0) is always `LARuins`, and the city grid branch uses `structuresFor(r.zoneType)` = urban catalog — in **every** phase. The floor follows `biomeAtWorld` = phase zone. Expected result: in Cemetery phase, the hub will have modern buildings and bunkers over cemetery ground. Not observable today because no run reaches phases 2+ (P1 coverage finding) — the two pending items add up.

### Declared method limit

The replica did not reproduce the binary's exact stream (~0.7% checksum difference). The portal conclusion does not depend on this (120 uncorrelated variants agree), but any future use of the replica for questions *sensitive to the exact position* of a specific object requires closing that gap first.

---

## 7. FINAL CONSOLIDATION (2026-08-21) — state after verification

### Finding scorecard (current)

| Sev. | Finding | Status | Evidence |
|---|---|---|---|
| ~~P0~~→P2 | Portal can spawn inside building | **Downgraded** — 120/120 variants reachable (§6); protection is accidental, recommend `!isBlocked(owPortalPos)` assert on open | Deterministic reconstruction |
| P1 | Phase flow with zero automated coverage | Open | Measured: `Advanced zones: 0` in all runs |
| P1 | "Follow the marker" with no marker | Open | Measured: frame `shot_09` |
| P1 | Ark/RTS medieval in modern city | Open | Measured: frame `shot_04` |
| P1 | ~6 FPS sustained at opening | Open | Measured: 38+ samples, 0 entities at minimum |
| P2 | Scenery built 2× at initialization (1st time with old center, discarded) | Open | Measured: 2 `SCENERY` logs (1626→3014) |
| P2 | Phases 2+ keep LA city in hub over another biome's ground | Open | Deduced from code (invisible until phase coverage exists) |
| P2 | Melee inert in automation (1 attack in 240 s) | Open | Measured |
| P2 | FPS report alert cries wolf (mixes load hitch with game FPS) | Open | Measured |
| P2 | Intentional corridor guarantee assert | New recommendation (§6) | — |

### REVISED programmer priorities (replace §5)

1. **Bot crosses the portal** (inform `owPortalPos` to `BotController` +
   simulate [E]). Became #1: unlocks phase 2–11 coverage, puts `advanceOpenWorldPhase` under the validation gate and would expose in screenshots the "LA city in every phase" finding. Without this, ~90% of game content remains untested.
2. **Portal marker on HUD/radar** — or fix the text that promises a
   nonexistent marker. Along with it, the `!isBlocked(owPortalPos)` assert on open
   (3 lines, turns the accidental §6 protection into a guarantee).
3. **Ark/RTS without medieval models** — most-looked-at object in the game (respawn point), only remaining inconsistency from art direction already resolved in scenery.
4. **Opening hitch** — eliminate double scenery build (free win, measured) and remove GPU→CPU readback from the hot path of voxel generation (pre-generate on title/transition screen, or cache meshes on disk).

### Repository hygiene observation

During the audit a modification was found in the working tree in
`src/BotController.h` of **unknown origin** (not from the auditor and not in
any commit): removes the `shouldPickupItem` field from the decision struct and
declares a `reset()` method referring to a `Game::restartRun` that does **not exist
in current code**. Status: incomplete — removing the field breaks compilation
if anything still reads it, and the declared `reset()` without implementation breaks at link time if called. Programmer is advised to identify the origin (another session/agent/manual edit) before the next build; the auditor did not touch the file.

### Instruments left for reuse

- `validate.sh [seconds] [seed]` — pass/fail gate (builds Debug+Release,
  runs bot, non-zero exit fails with reason).
- Deterministic scenery generator replica (session scratchpad,
  disposable) — reusable for layout questions; declared limitation:
  ~0.7% divergence from exact MSVC stream (§6).
- `bot_report.txt` + `shot_NN.png` every 10 s during `--autotest`.
