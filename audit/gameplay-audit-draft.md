# Draft - Technical audit: gameplay, combat, balance, enemies, skills and progression

## Bugs / inconsistencies confirmed (with location)

1. **Skill Tree T3 blocked** — `src/SkillTree.cpp:42` `branchSpentReq(tier)=tier*2` and `src/SkillTree.cpp:53-59` `canBuy` requires 4 perks in the branch for tier 3, but each branch only has 3 perks (T1,T1,T2,T3). T3 never purchasable. Affects Immortal Protocol, Predator System, Digital Ghost.

2. **PlasmaCell fuse lost on level-up** — `src/Player.cpp:1078-1079` changes `sk.cooldown`, but `Player::levelUp()` calls `refreshSkillVectors()` (`src/Player.cpp:1213`) which overwrites `skills[i].cooldown` using `baseSkillCool[i]` (never updated). The cooldown reduction effect from fusion is lost after leveling up.

3. **Grenade does not explode on hitting enemy** — `src/Game.cpp:1176` `if (proj.isGrenade) continue;` makes `checkCollisions` ignore grenades; `src/Game.cpp:1355` only explodes on wall/range. Behavior counterintuitive for AoE.

4. **Boss kill slow-mo does not affect gameplay** — `src/Game_Gameplay.cpp:279-284` calculates `effectiveDt` but does not use it in any entity update; reserved only for "future visual effects".

5. **Defense can heal the player** — `src/Player.cpp:1149` `reduced = amount * (1.0f - defense / 100.0f)`. `applyEquipmentStats` limits to 80 (`src/Player.cpp:1327`), but `increaseBaseDefense`/`baseDefense` can exceed 100 via items/quests/fusions, making damage negative (heals when hit).

6. **Combo count in melee resets by timer but does not decrement gradually** — ok mechanic, but `comboTimer` resets to 2.5s every hit; can keep combo high indefinitely if there are enemies.

7. **Hit-stop freezes enemies/projectiles but player/companion already updated** — `src/Game_Gameplay.cpp:148-154` occurs after `player.update(dt)` and `updateCompanions(dt)` (lines 189-190), so hit-stop does not pause player in the same frame.

8. **Aim assist sticks to dead enemy?** — `src/Game_Gameplay.cpp:1579` `if (e.isDead()) continue;`, ok.

9. **Enemy auto-evolve speed multiplier does not revert on some paths** — `src/Enemy.cpp:539-557` permanently multiplies `speed`, `damage`, `shootRate` by tier. If it evolves, it stays stronger forever. May be intentional.

10. **Boss phase transition stacking** — `src/Enemy.cpp:396-414` `checkBossPhases` can be called multiple times if health drops fast (e.g. massive damage at once), but `tgt > bossPhase` prevents. However, `bossInvulnTimer` resets on every transition; ok.

11. **Zergling `emplace_back` after reference `e`** — `src/Game_Spawn.cpp:276-286` creates Zergling and then does `enemies.emplace_back` inside the block `if (type == EnemyType::Zergling)`. Reference `e` from main spawn is used before; extra Zerglings are added after, not invalidating `e` if already done using. But `e.makeElite` happens before. Seems ok.

12. **Elite spawn with `e` reference and then `Zergling` swarm** — same as above.

13. **EnemyDirector `damageTaken` smoothing wrong** — `src/EnemyDirector.cpp:30` `prof.damageTaken = prof.damageTaken * 0.0f + dpm * 1.0f` effectively `= dpm`, no moving average. Probably should be `* 0.6f + dpm * 0.4f` like `killSpeed`.

14. **Companion `wantsHeal`/`wantsCollect` inert before fix** — `src/Companion.cpp:76-81` flags exist; `src/Game_Spawn.cpp:356-389` now reads. Ok apparently.

15. **BotController `k8DirAngles` comment inconsistent** — `src/BotController.cpp:20-30` comment says N,NE,E... but array starts at 0=E, then NE, N, NW... The real order is E,NE,N,NW,W,SW,S,SE. This is only a comment; ok if used consistently.

16. **SaveManager `player.totalKills` overwrite** — `src/SaveManager.cpp:146` `totalKills > 0 ? totalKills : player.totalKills` saves argument or player. In `Game::autoSave`/`Game::startLoadedGame` passes `totalKills` (Game). Ok.

17. **Item affixes not applied on equip** — `src/Item.h:89-100` defines `bonusDamage`/`Health`/etc. and `rollAffixes` fills them, but `Player::applyEquipmentStats` and `Player::equipItem` do not read these `Item` bonuses (only from `Equipment`). Extended items (weapons/armor) drop with affixes but have no mechanical effect. `Equipment` has no affix fields.

18. **Item drop `createRandom` does not use rarity affixes** — `src/Item.cpp:9-54` creates basic items without `rollAffixes`. `createWeaponDrop` calls `rollAffixes` but no code calls `createWeaponDrop`? Grep pending.

19. **AnomalyPortal spawn `totalSpawned` does not limit** — `src/AnomalyPortal.cpp:10-19` resets; `pollSpawn` increments without limit, but portals close on death. ok.

20. **`runAutoTest` screenshots mix old shots** — `src/Game.cpp:496-501` removes old ones. ok.

## Next steps
- Verify if `createWeaponDrop` is called.
- Verify `BuildingSystem` interaction with combat and costs.
- Verify `Game_WorldRender` for hit-flash/aim render.
- Verify `Tilemap` collisions and portal.
- Verify numeric balance (XP curve, scaled enemy damage).
- Verify `SaveManager` correctly loads evolution/perks.
