# DARKNET — Consolidated Roadmap

Single document of project status and direction. Replaces cross-reading
`ROADMAP_EXECUTIVO_CLAUDE.md`, `BACKLOG_IMPLEMENTACAO_CLAUDE.md` and
`obsidian-vault/Roadmap and Backlog.md` (kept the history/detail).

**Current status:** 3D isometric (voxel) ARPG in C++/raylib, playable offline, with
backend Node.js (auth, premium store, realtime WS) in Docker containers.

---

## Completed (audit and technical correction 2026-08)

### Test bot (autotest)
- Fixed exploration outside the safe zone, melee not connecting and collecting loot
  items not accounted for.
- Adjusted skill thresholds; `static` timers converted to reset members.
- Unified BFS pathfinding; dead code removal.

### Consolidated 3D Pipeline
- Removed the parallel 2D pipeline and the F10 toggle; the voxel system is the single pipeline.
- Removed `render3D()` orphans (Player/Enemy/NPC/Companion).
- Flag renamed to `g_voxelCapture`.

### Performance
- Optimized light mask: 31 ellipses → 1 textured quad per light, in half resolution.
- Single batch of the tilemap floor; frustum culling on the stage and floor.
- LOD of voxel shadows; screenshot of the asynchronous autotest.
- **Result:** worst render frame 95.9ms → ~19ms; Minimum FPS 6 → 44–52; Average SPF 58–59.

### Refactoring
- `Game.cpp` of ~8,700 → ~4,700 lines, with 11 extracted modules: `Game_Shaders`,
  `Game_Network`, `Game_PremiumStore`, `Game_Resources`, `Game_Evolution`,
  `Game_QuestsNPC`, `Game_Phases`, `Game_Spawn`, `Game_WorldGen`, `Game_HUD`, `Game_Bot`.

### Documentation
- Standardized protagonist (Vance Rios), year 2047; README updated for isometric 3D.

### Backend (server/)
- Inventory now uses the `qty` column for real: `savePlayer` adds the list of
  items and does UPSERT (`ON CONFLICT (account,item_id) DO UPDATE SET qty`), instead of
  DELETE + re-INSERT with qty=1; `getPlayer` expands `qty` to the flat list.
- Idempotent migration on boot: consolidates legacy duplicates and creates single index
  `(account,item_id)`.
- Consolidated DDL: `index.js` is the single source of truth (`CREATE TABLE IF NOT EXISTS`
  on every boot); `db/init.sql` became just the pointer — end of the divergence.
- Removed Redis from `docker-compose.yml` (was orphaned: matchmaking is `Map` in memory);
  comment logs when to reintroduce (distributed sessions/multiple instances).
- `PUBLIC_URL` fixed default to `http://localhost:8080` (gateway port) —
  `success_url`/`cancel_url` from Stripe Checkout were leaving with the wrong internal port.
- `server/README.md`: complete endpoint table and ports corrected
  (public 8080 gateway, internal 9000 app).

---

## Next steps (short term)

- **Unit tests (doctest)** for pure rules (economy, loot, progression, combat math).
- **Replace manual JSON parsing with nlohmann/json** in `NetClient`/`StoreClient`.
- **Stable IDs in SaveManager**: `resolveEquipByName` uses `strcmp` for display name —
  migrate to immutable internal IDs.
- **Nomenclature standardization**: prefix `m_` in members, without obscure abbreviations.
- **Visually apply purchased premium cosmetics**: skins/paints of the not render
  in `Player.cpp` — check and link premium inventory to player render
  (includes Pet Drone spawn).

---

## Medium/long term
- **Real cooperative multiplayer**: versioned messaging protocol, validation
  server-side of payloads, lobby/matchmaking with ready/start, synchronization
  authoritative state (today it is simple relay per room).
- **Content of acts 2–5**: zones, quests and bosses beyond act 1 (see `DARKNET_STORY.md`).
- **Complete achievements** and **meta-progression** between runs.
- **UX/onboarding**: 3–5 step guided tutorial, HUD with information hierarchy,
  visual feedback of events (mission, loot, level).
- **Open world**: hubs and routes between zones, purposeful POIs, dynamic events
  that change the map.
- **AI**: tactical roles of enemies (tank/ranged/flanker), strategic companions,
  bosses with phases and telegraph.
- **Linux/macOS port** (toolchain and raylib dependencies).
- **Real Steam/EOS**: real integration (today there is only the directory
  `integration/steam`), secure accounts (email+hashed password or OAuth).
- **Launch MVP**: deploy the backend with HTTPS + Stripe in production,
  legal compliance (terms, LGPD), basic anti-fraud.

---

## Validation criteria

Any relevant change must go through:

```bash
# Build
cmake --build build --config Release

# Autotest (bot plays alone for 120s)
./build/Release/darknet.exe --autotest --test-seconds=120
```

- The autotest should exit with **exit 0** and print `VALIDATION: PASSED`.
- No FPS regression (post-optimization benchmark: average ~58–59, minimum 44–52).
- Backend: `node --check server/game-server/src/index.js` should pass.