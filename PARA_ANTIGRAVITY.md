# Handoff Claude → Antigravity (ATUAL)

## 🟢 Entidades viram MODELOS 3D REAIS low-poly (SEM CUBOS, sem billboard/2.5D)
Decisão do usuário: nada de cubo, nada de billboard/2.5D. Entidades = modelos 3D
low-poly com formas ARREDONDADAS (DrawSphere / DrawCapsule / DrawCylinderEx).

Claude lançou 4 agentes ADICIONANDO `void render3D() const;` (impl em .cpp) em:
- `Player`  (src/Player.cpp/.h)
- `Enemy`   (src/Enemy.cpp/.h)
- `NPC` + `Companion` (src/NPC.*, src/Companion.*)
- `Item`    (src/Item.cpp/.h)
Eles NÃO tocam Game.cpp/Tilemap.cpp nem o `render()` 2D existente.

### Antigravity, sua parte (renderWorld3D em Game.cpp):
1. Dentro do `BeginMode3D`, CHAMAR os novos `render3D()` de cada entidade
   (player.render3D(); e em loop enemies/npcs/companions/items: x.render3D();),
   substituindo o desenho 2D projetado (`drawEnt`/projeção pós-EndMode3D). Remover
   esse overlay 2D das entidades. Manter sombras (DrawPlane) e projéteis/partículas.
2. NÃO editar Player/Enemy/NPC/Companion/Item (os agentes do Claude estão neles).
3. ⚠️ NADA DE CUBO em lugar nenhum — nem entidades, nem cenário. Cenário pode ser
   DrawBillboard (texturas SpriteBank) ou primitivas arredondadas. Se você já pôs
   `DrawCube` nas paredes do tilemap, troque por algo arredondado/textura depois
   (Claude cuida do Tilemap).

Claude compila + valida (screenshot) assim que os agentes terminarem e você ligar
as chamadas. Sincronize por commit; antes de editar Game.cpp faça commit do que tem.
