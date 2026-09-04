# DIRETRIZES PARA O ANTIGRAVITY (3D real — decisão do usuário)

O usuário definiu: **os personagens têm que ser os modelos DO PRÓPRIO JOGO (como
no 2D) só que em 3D real** — NÃO modelos genéricos (greenman/robot) e NÃO cubos.

## Regras (obrigatórias)
1. **PERSONAGENS = voxelização da arte 2D (SpriteExtrude).** ATUALIZAÇÃO: o pipeline
   atual NÃO chama mais `render3D()` por entidade. O `renderWorld3D` (Game.cpp) usa
   `ensureVoxel(key, capPos, drawFn)` — que captura o `render()` 2D de cada entidade
   em poses (via `SpriteExtrude::CaptureToImage`) e converte para um modelo voxel 3D
   em cache (`SpriteExtrude::BuildVoxelModel`, src/SpriteExtrude.cpp) — e desenha com
   `drawVoxel(base, pos, rotDeg, walkPhase, moving)`. Assim os personagens ficam
   fiéis à arte 2D, com animação de caminhada. Os métodos `render3D()` de Player,
   Enemy, NPC e Companion ficaram ÓRFÃOS (não são mais chamados); apenas
   `Item::render3D()` segue em uso (itens no chão).
   ❌ NÃO volte a usar `m_playerModel`/`m_enemyModel` (greenman/robot) para
   personagens, e NÃO reintroduza as chamadas `*.render3D()` no `renderWorld3D`.
   Pode REMOVER esses dois modelos se quiser.
2. **MODELOS REAIS (.obj/.glb) só para PRÉDIOS/CENÁRIO** — house/turret/castle/
   barracks/market/well/old_car_new. Isso ficou ÓTIMO, mantenha. (Não são
   personagens desenhados à mão.) Assets em `resources/models/` (Claude copiou).
3. **CHÃO/PAREDES**: textura via DrawCubeTexture/rlgl com SpriteBank — ok manter.
4. **NADA de cubo** para personagens. Cenário pode ser modelo real.
5. Peers/tanks/soldiers: pode manter um modelo simples por enquanto (secundário).

## LANES (sem clobber — crítico)
- **CLAUDE** (via workflow de agentes AGORA): arquivos de ENTIDADE — `Player.cpp/h`,
  `Enemy.cpp/h`, `NPC.cpp/h`, `Companion.cpp/h`, `Item.cpp/h` (o `render()` 2D deles
  é a FONTE de arte da voxelização) + `SpriteExtrude.cpp/h` + `Tilemap::render3D`
  (chão). E o TRECHO de personagens do `renderWorld3D` (as chamadas
  `ensureVoxel`/`drawVoxel`).
  → Antigravity, NÃO edite esses arquivos/trecho enquanto o workflow roda.
- **ANTIGRAVITY**: `Game.cpp` — carregamento/desenho dos MODELOS DE PRÉDIOS,
  iluminação/fog 3D, câmera, decalques. E o backend.

Sync por commit; antes de editar Game.cpp faça commit/pull. O Claude compila e
valida (screenshot) o resultado combinado quando o workflow terminar.
