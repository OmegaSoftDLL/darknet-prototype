# Rascunho - Auditoria técnica: gameplay, combate, balanceamento, inimigos, skills e progressão

## Bugs / inconsistências confirmados (c/ local)

1. **Skill Tree T3 bloqueado** — `src/SkillTree.cpp:42` `branchSpentReq(tier)=tier*2` e `src/SkillTree.cpp:53-59` `canBuy` exige 4 perks na branch para tier 3, mas cada branch só tem 3 perks (T1,T1,T2,T3). T3 nunca comprável. Afeta Protocolo Imortal, Sistema Predador, Fantasma Digital.

2. **Fuse de PlasmaCell perdido no level-up** — `src/Player.cpp:1078-1079` altera `sk.cooldown`, mas `Player::levelUp()` chama `refreshSkillVectors()` (`src/Player.cpp:1213`) que sobrescreve `skills[i].cooldown` usando `baseSkillCool[i]` (nunca atualizado). O efeito de redução de cooldown por fusão é perdido após subir de nível.

3. **Granada não explode ao atingir inimigo** — `src/Game.cpp:1176` `if (proj.isGrenade) continue;` faz checkCollisions ignorar granadas; `src/Game.cpp:1355` explode só em parede/alcance. Comportamento contraintuitivo para AoE.

4. **Slow-mo de boss kill não afeta gameplay** — `src/Game_Gameplay.cpp:279-284` calcula `effectiveDt` mas não o usa em nenhum update de entidades; só reserva para "future visual effects".

5. **Defense pode curar o jogador** — `src/Player.cpp:1149` `reduced = amount * (1.0f - defense / 100.0f)`. `applyEquipmentStats` limita a 80 (`src/Player.cpp:1327`), mas `increaseBaseDefense`/`baseDefense` podem exceder 100 via itens/quests/fusões, tornando dano negativo (cura ao ser atingido).

6. **Combo count no melee reseta via timer mas não decrementa gradualmente** — ok mecânica, mas comboTimer é reiniciado a 2.5s a cada hit; pode manter combo alto indefinidamente se houver inimigos.

7. **Hit-stop congela inimigos/projéteis mas player/companion já atualizaram** — `src/Game_Gameplay.cpp:148-154` ocorre após `player.update(dt)` e `updateCompanions(dt)` (linhas 189-190), então hit-stop não pausa player no mesmo frame.

8. **Aim assist gruda em inimigo morto?** — `src/Game_Gameplay.cpp:1579` `if (e.isDead()) continue;`, ok.

9. **Enemy auto-evolve speed multiplier não reverte em alguns caminhos** — `src/Enemy.cpp:539-557` multiplica `speed`, `damage`, `shootRate` permanentemente por tier. Se evoluir, fica mais forte para sempre. Pode ser intencional.

10. **Boss phase transition stacking** — `src/Enemy.cpp:396-414` `checkBossPhases` pode ser chamado múltiplas vezes se health cair rápido (ex: dano massivo de uma vez), mas `tgt > bossPhase` impede. No entanto, `bossInvulnTimer` é reiniciado a cada transição; ok.

11. **Zergling emplace_back após referência `e`** — `src/Game_Spawn.cpp:276-286` cria Zergling e depois faz `enemies.emplace_back` dentro do bloco `if (type == EnemyType::Zergling)`. A referência `e` do spawn principal é usada antes; os Zerglings extras são adicionados depois, sem invalidar `e` se já terminou de usar. Mas `e.makeElite` ocorre antes. Parece ok.

12. **Elite spawn com `e` referência e depois `Zergling` swarm** — mesmo acima.

13. **EnemyDirector `damageTaken` smoothing errada** — `src/EnemyDirector.cpp:30` `prof.damageTaken = prof.damageTaken * 0.0f + dpm * 1.0f` efetivamente `= dpm`, sem média móvel. Provavelmente deveria ser `* 0.6f + dpm * 0.4f` como killSpeed.

14. **Companion `wantsHeal`/`wantsCollect` inertes antes do fix** — `src/Companion.cpp:76-81` flags existem; `src/Game_Spawn.cpp:356-389` agora lê. Ok aparentemente.

15. **BotController `k8DirAngles` comentário inconsistente** — `src/BotController.cpp:20-30` comentário diz N,NE,E... mas array começa em 0=E, depois NE, N, NW... Isso é apenas comentário; a ordem real é E,NE,N,NW,W,SW,S,SE. Não afeta se usado consistentemente.

16. **SaveManager `player.totalKills` overwrite** — `src/SaveManager.cpp:146` `totalKills > 0 ? totalKills : player.totalKills` salva argumento ou player. Em `Game::autoSave`/`Game::startLoadedGame` passa `totalKills` (Game). Ok.

17. **Item affixes não aplicados ao equipar** — `src/Item.h:89-100` define bonusDamage/Health/etc. e `rollAffixes` preenche, mas `Player::applyEquipmentStats` e `Player::equipItem` não leêm esses bônus de `Item` (só de `Equipment`). Itens estendidos (armas/armaduras) dropam com afixos mas não têm efeito mecânico. `Equipment` não tem campos de afixos.

18. **Item drop `createRandom` não usa raridade afixos** — `src/Item.cpp:9-54` cria itens básicos sem `rollAffixes`. `createWeaponDrop` chama rollAffixes mas nenhum código chama `createWeaponDrop`? Grep pendente.

19. **AnomalyPortal spawn `totalSpawned` não limita** — `src/AnomalyPortal.cpp:10-19` reseta; `pollSpawn` incrementa sem limite, mas portals fecham ao morrer. ok.

20. **`runAutoTest` screenshots misturam old shots** — `src/Game.cpp:496-501` remove antigos. ok.

## Próximos passos
- Verificar se `createWeaponDrop` é chamado.
- Verificar `BuildingSystem` interação com combate e custos.
- Verificar `Game_WorldRender` para render de hit-flash/aim.
- Verificar `Tilemap` colisões e portal.
- Verificar balanceamento numérico (curva XP, dano inimigo escalado).
- Verificar `SaveManager` carrega evolução/perks corretamente.
