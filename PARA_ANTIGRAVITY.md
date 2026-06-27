# DIRETRIZES PARA O ANTIGRAVITY (3D real — decisão do usuário)

O usuário definiu: **os personagens têm que ser os modelos DO PRÓPRIO JOGO (como
no 2D) só que em 3D real** — NÃO modelos genéricos (greenman/robot) e NÃO cubos.

## Regras (obrigatórias)
1. **PERSONAGENS = `render3D()` do jogo.** Player, Enemy, NPC, Companion, Item já
   têm `render3D()` (modelos 3D fiéis ao design 2D, feitos de esferas/cápsulas/
   cilindros — SEM cubos). O Claude **já trocou** no `renderWorld3D`:
   `player.render3D(); n.render3D(); c.render3D(); e.render3D(); it.render3D();`.
   ❌ NÃO volte a usar `m_playerModel`/`m_enemyModel` (greenman/robot) para
   personagens. Pode REMOVER esses dois modelos se quiser.
2. **MODELOS REAIS (.obj/.glb) só para PRÉDIOS/CENÁRIO** — house/turret/castle/
   barracks/market/well/old_car_new. Isso ficou ÓTIMO, mantenha. (Não são
   personagens desenhados à mão.) Assets em `resources/models/` (Claude copiou).
3. **CHÃO/PAREDES**: textura via DrawCubeTexture/rlgl com SpriteBank — ok manter.
4. **NADA de cubo** para personagens. Cenário pode ser modelo real.
5. Peers/tanks/soldiers: pode manter um modelo simples por enquanto (secundário).

## LANES (sem clobber — crítico)
- **CLAUDE** (via workflow de agentes AGORA): arquivos de ENTIDADE — `Player.cpp/h`,
  `Enemy.cpp/h`, `NPC.cpp/h`, `Companion.cpp/h`, `Item.cpp/h` (refinando os
  `render3D()` p/ ficarem fiéis) + `Tilemap::render3D` (chão). E o TRECHO de
  personagens do `renderWorld3D` (as chamadas `*.render3D()`).
  → Antigravity, NÃO edite esses arquivos/trecho enquanto o workflow roda.
- **ANTIGRAVITY**: `Game.cpp` — carregamento/desenho dos MODELOS DE PRÉDIOS,
  iluminação/fog 3D, câmera, decalques. E o backend.

Sync por commit; antes de editar Game.cpp faça commit/pull. O Claude compila e
valida (screenshot) o resultado combinado quando o workflow terminar.
