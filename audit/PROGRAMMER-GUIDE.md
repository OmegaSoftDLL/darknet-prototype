# PROGRAMMER GUIDE (k3) — how to work against the audit

This document accompanies `2026-08-21-full-audit.md`. The report says **what**
is wrong; this guide says **what "fixed" means** — the objective criterion the
next audit will measure. How to implement is your decision; the criteria below
are not.

---

## 0. Cycle rules (read first)

1. **Before declaring any delivery ready, run:**
   ```bash
   ./validate.sh 120 7
   ./validate.sh 120 20260821
   ```
   Both must end in `APPROVED` (exit 0). The audit starts there; if it arrives red,
   the cycle goes back to you without analysis of the rest.
2. **Build Debug and Release.** `validate.sh` already does both. The project owner plays **Release** — the delivery validated only in Debug already cost the full day of "I don't see anything that was done".
3. **Every test with `--seed=N` and `--test-seconds=N`.** Without seed the result is an anecdote; without test-seconds the bot report is never written.
4. **Be careful with `Game.cpp`** (8,000+ lines, everything coupled). Any change there is high regression risk — validate with both seeds, not one.
5. **Do not trust screenshots for temporal bugs** (flicker, hitch, freeze). Use `bot_report.txt` and logs.
6. There is an uncommitted **WIP** of yours in `src/BotController.h` (removes
   `shouldPickupItem`, declares `reset()` referring to `Game::restartRun` that does not
   exist yet). It is incomplete — finish or discard before the next build,
   otherwise the first item of the next audit will be broken build.

---

## 1. Acceptance criteria per finding (what the next audit will measure)

### P1 — Zero coverage of phase flow  ← **PRIORITY #1**
- **Where to change:** `Game.cpp:4252` (bot receives `tilemap.portals`, empty in
  open world; needs to know `owPortalPos` when `owPortalOpen`) and the simulation of `[E]` (today `IsKeyPressed(KEY_E)` in `updatePhasePortal` — the bot does not press keys; give the bot decision the path to trigger advancement when <105u from portal).
- **Acceptance:** run `--autotest --test-seconds=300 --seed=7` and have
  `bot_report.txt` show **`Advanced zones: >= 1`**. That is:
  `advanceOpenWorldPhase()` executed under the gate — world regenerated,
  reward delivered, in the crash, and the test continues in phase 2.
- **Why it is #1:** unlocks observation of phases 2–11 (today 0% coverage) and will
  expose in practice the P2 "LA city in every phase".

### P1 — "Follow the marker" with in the marker
- **Where:** text in `Game.cpp:8048`; `owPortalPos` has in the indicator.
- **Acceptance:** with portal open, there is visible directional indication — edge-of-screen arrow pointing the way AND/OR radar blip. Frame criterion: in any screenshot with "PORTAL OPEN" in the HUD, the auditor can point to the indicator. (Minimum acceptable alternative: fix the text only it does not promise the marker — but then the finding becomes "portal not signaled", P2.)
- **Along with it (3 lines):** when opening the portal, `assert`/log of
  `!isBlocked(owPortalPos)` — turns the **accidental** protection from §6 of the
  report into an intentional guarantee. If it ever fails, relocate the point.

### P1 — Ark/RTS medieval in modern city
- **Where:** `Game.cpp:7287` (`Ark` → `castle.obj`), `:7289` (RTS `House` →
  `house.obj`). Modern primitives already exist in code used by scenery (building 20, bunker 15, `drawGenericStructure`) — the art direction for urban zones was already decided there.
- **Acceptance:** screenshot of the hub with in the clay roof/tower castle from `BuildingSystem` in urban zones (LARuins/GhostCity). In zones where castle is coherent (Manor), it may stay.

### P1 — ~6 FPS sustained at opening
- **Two known causes, attack in cost order:**
  1. **Free:** scenery is built 2× during initialization (2 `SCENERY` logs;
     1st with old `safeZoneCenter` ~(1280,1280) and discarded). Eliminate the first call or delay it until the center is correct.
  2. **Real:** `SpriteExtrude::CaptureToImage` creates the RenderTexture per capture
     and does GPU→CPU readback on the hot path (~40+ models × 4 poses, 3 per
     frame). Take it off the hot path: pre-generate behind the title/
     transition screen, reuse the single RenderTexture, or cache meshes.
- **Acceptance:** `bot_report.txt` with NO `Critical FPS` samples below 30 in the first 30 s (today: 38+ samples at 6), keeping average FPS ≥ 55. `SCENERY` log appearing **once** per world build.

### P2 — Phases 2+ with LA city in hub
- **Where:** `setupWorldRegions` (region (0,0) always `LARuins`) + city grid
  branch in `buildOpenWorldScenery` keyed on `r.zoneType`.
- **Acceptance:** in Cemetery phase (3rd... check `content/phases.txt`), the hub
  uses the biome's catalog (crypts/towers, not modern buildings). Verifiable by screenshot the soon the item #1 grants access to phases 2+.
- **Attention:** the area near the portal was verified free (§6) **for the current grid**. If you change grid/EDGE/catalogs, the portal assert above prevents silent regression.

### P2 — Melee inert in automation
- **Acceptance:** `bot_report.txt` with `Melee attacks > 10` in the 120 s run, or
  documented diagnosis of why the bot does not use melee (then the finding changes nature: metric removed or corrected).

### P2 — Misleading FPS alert
- **Acceptance:** report separates "load hitch" (first N seconds) from
  "game FPS"; the `PROBLEM: FPS` alert only fires for the latter. An alert that cries wolf is one you yourself will learn to ignore.

---

## 2. What does NOT need fixing (of the not waste time)

- Portal inside building: **verified free** (120/120 variants, §6).
  Only the 3-line assert to shield the future.
- Cars, streets, allied units, facades: confirmed good in frames.
- Bot stuck detector: fixed in `6113e73` and confirmed in execution.

## 3. Traps that already cost hours in this project

- **Scale:** ruler measured in code — hero = **66u** tall (log
  `VOXSIZE`), ~37.7u/m, tile 64u. Any new size is justified against the ruler,
  never "by eye" (scale oscillation consumed the whole afternoon).
- **New overlay** goes in `drawHudAndOverlays`/`drawUI` (shared) — never only in one render path (historical root of "fix 1, break 30").
- **Ground shadow/decal:** layer separation in tenths of the unit, not
  0.004 (z-fighting made the whole scenery flicker).
- **2D shadow ellipse** without `g_renderPass3D` guard becomes the pedestal in the voxelized mesh.
- **Bob/animation by translation** makes the character float — use scale.
- The campaign is **data**: `content/phases.txt` (zone | kills | boss | radius |
  title). Phase pacing adjustments of the not require recompiling.

## 4. How the next audit will evaluate your delivery

Each finding from §7 of the report leaves with status **closed / persists /
regressed / new**, compared against the baseline: average FPS 58–59, 0 deaths,
0 critical freezes, 20–31 kills/120 s, seeds 7 and 20260821.
Ideal delivery: both `validate.sh` green, `Advanced zones ≥ 1`, and the paragraph
from you (can be in `audit/K3-NOTES.md`) saying what you attacked and what you
left out — declared coverage is worth more than an impression of completeness.
