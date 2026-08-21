# GUIA PARA O PROGRAMADOR (k3) — como trabalhar contra a auditoria

Este documento acompanha `2026-08-21-auditoria-completa.md`. O relatório diz **o
que** está errado; este guia diz **o que "corrigido" significa** — o critério
objetivo que a próxima auditoria vai medir. Como implementar é decisão sua; os
critérios abaixo não são.

---

## 0. Regras do ciclo (leia primeiro)

1. **Antes de declarar qualquer entrega pronta, rode:**
   ```bash
   ./validate.sh 120 7
   ./validate.sh 120 20260821
   ```
   As duas precisam terminar em `APROVADO` (exit 0). A auditoria começa por
   aí; se chegar vermelho, o ciclo volta pra você sem análise do resto.
2. **Compile SEMPRE Debug e Release.** O `validate.sh` já faz os dois. O dono
   do projeto joga o **Release** — entrega validada só em Debug já causou um
   dia inteiro de "não vejo nada do que foi feito".
3. **Todo teste com `--seed=N` e `--test-seconds=N`.** Sem seed o resultado é
   anedota; sem test-seconds o relatório do bot nunca é escrito.
4. **Cuidado com `Game.cpp`** (8.000+ linhas, tudo acoplado). Qualquer mudança
   ali é risco alto de regressão — valide com as duas seeds, não com uma.
5. **Não confie em screenshot para bug temporal** (pisca, hitch, travamento).
   Use `bot_report.txt` e os logs.
6. Existe um **WIP seu** não commitado em `src/BotController.h` (remove
   `shouldPickupItem`, declara `reset()` referindo `Game::restartRun` que não
   existe ainda). Está incompleto — termine ou descarte antes do próximo build,
   senão o primeiro item da próxima auditoria vai ser build quebrado.

---

## 1. Critérios de aceite por achado (o que a próxima auditoria vai medir)

### P1 — Cobertura zero do fluxo de fase  ← **PRIORIDADE Nº 1**
- **Onde mexer:** `Game.cpp:4252` (o bot recebe `tilemap.portals`, vazio no
  mundo aberto; precisa conhecer `owPortalPos` quando `owPortalOpen`) e a
  simulação do `[E]` (hoje `IsKeyPressed(KEY_E)` em `updatePhasePortal` —
  o bot não pressiona tecla; dê um caminho para a decisão do bot acionar o
  avanço quando estiver a <105u do portal).
- **Aceite:** rodar `--autotest --test-seconds=300 --seed=7` e o
  `bot_report.txt` mostrar **`Zonas avancadas: >= 1`**. Ou seja:
  `advanceOpenWorldPhase()` executou sob o portão — mundo regenerado,
  recompensa entregue, sem crash, e o teste continua na fase 2.
- **Por que é o nº 1:** destrava a observação das fases 2–11 (hoje 0% de
  cobertura) e vai expor na prática o P2 "cidade de LA em toda fase".

### P1 — "Siga o marcador" sem marcador
- **Onde:** texto em `Game.cpp:8048`; `owPortalPos` não tem nenhum indicador.
- **Aceite:** com o portal aberto, existe indicação direcional visível — seta
  na borda da tela apontando o rumo E/OU blip no radar. Critério do frame:
  em qualquer screenshot com "PORTAL ABERTO" no HUD, o auditor consegue
  apontar o indicador. (Alternativa mínima aceitável: corrigir o texto para
  não prometer marcador — mas aí o achado vira "portal não sinalizado", P2.)
- **Junto (3 linhas):** ao abrir o portal, `assert`/log de
  `!isBlocked(owPortalPos)` — transforma a proteção **acidental** do §6 do
  relatório em garantia intencional. Se um dia falhar, realoque o ponto.

### P1 — Arca/RTS medievais na cidade moderna
- **Onde:** `Game.cpp:7287` (`Ark` → `castle.obj`), `:7289` (`House` do RTS →
  `house.obj`). Já existem no código as primitivas modernas usadas pelo
  cenário (prédio 20, bunker 15, `drawGenericStructure`) — a direção de arte
  das zonas urbanas já foi decidida lá.
- **Aceite:** screenshot do hub sem nenhum telhado de telha/torre de castelo
  vindo do `BuildingSystem` nas zonas urbanas (LARuins/GhostCity). Nas zonas
  onde castelo é coerente (Mansão), pode manter.

### P1 — ~6 FPS sustentado na abertura
- **Duas causas conhecidas, ataque em ordem de custo:**
  1. **Grátis:** o cenário é construído 2× na inicialização (2 logs `SCENERY`;
     a 1ª com `safeZoneCenter` velho ~(1280,1280) e descartada). Elimine a
     primeira chamada ou adie-a até o centro estar correto.
  2. **Real:** `SpriteExtrude::CaptureToImage` cria RenderTexture por captura
     e faz readback GPU→CPU no caminho quente (~40+ modelos × 4 poses, 3 por
     frame). Tire do caminho quente: pré-gerar atrás da tela de
     título/transição, reusar uma RenderTexture única, ou cachear malhas.
- **Aceite:** `bot_report.txt` sem NENHUMA amostra de `FPS critico` abaixo de
  30 nos primeiros 30 s (hoje: 38+ amostras a 6), mantendo FPS médio ≥ 55.
  Log `SCENERY` aparecendo **uma vez** por construção de mundo.

### P2 — Fases 2+ com cidade de LA no hub
- **Onde:** `setupWorldRegions` (região (0,0) sempre `LARuins`) + ramo de
  grade em `buildOpenWorldScenery` keyado em `r.zoneType`.
- **Aceite:** na fase Cemitério (3ª… conferir `content/phases.txt`), o hub
  usa o catálogo do bioma da fase (criptas/torres, não prédios modernos).
  Verificável por screenshot assim que o item nº 1 der acesso às fases 2+.
- **Atenção:** o layout perto do portal foi verificado como livre (§6) **para
  a grade atual**. Se você mudar grade/EDGE/catálogos, o assert do portal
  (acima) é o que impede regressão silenciosa.

### P2 — Melee inerte na automação
- **Aceite:** `bot_report.txt` com `Ataques melee > 10` num run de 120 s, ou
  diagnóstico documentado de por que o bot não usa melee (aí o achado muda de
  natureza: métrica removida ou corrigida).

### P2 — Alerta de FPS enganoso
- **Aceite:** o relatório separa "hitch de carga" (primeiros N segundos) de
  "FPS de jogo"; o alerta `PROBLEMA: FPS` só dispara pelo segundo. Um alerta
  que grita sempre é alerta que você mesmo vai aprender a ignorar.

---

## 2. O que NÃO precisa de correção (não gaste tempo)

- Portal dentro de construção: **verificado livre** (120/120 variantes, §6).
  Só o assert de 3 linhas para blindar o futuro.
- Carros, ruas, unidades aliadas, fachadas: confirmados bons nos frames.
- Detector de "preso" do bot: corrigido em `6113e73` e confirmado em execução.

## 3. Armadilhas que já custaram horas neste projeto

- **Escala:** régua medida no código — herói = **66u** de altura (log
  `VOXSIZE`), ~37,7u/m, tile 64u. Tamanho novo se justifica contra a régua,
  nunca "no olho" (a oscilação de escala consumiu uma tarde inteira).
- **Overlay novo** vai em `drawHudAndOverlays`/`drawUI` (compartilhados) —
  nunca só num caminho de render (raiz histórica do "conserta 1, quebra 30").
- **Sombra/decal no chão:** separação de camadas em décimos de unidade, não
  0.004 (z-fighting fez o cenário inteiro piscar).
- **Elipse de sombra 2D** sem guard `g_renderPass3D` vira pedestal na malha
  voxelizada.
- **Bob/animação por translação** faz o personagem flutuar — use escala.
- A campanha é **dado**: `content/phases.txt` (zona | abates | chefe | raio |
  título). Ajuste de ritmo de fase não exige recompilar.

## 4. Como a próxima auditoria vai avaliar sua entrega

Cada achado da §7 do relatório sai com status **fechado / persiste /
regrediu / novo**, comparando contra a linha de base: FPS médio 58–59, 0
mortes, 0 travamentos críticos, 20–31 abates/120 s, seeds 7 e 20260821.
Entrega ideal: os dois `validate.sh` verdes, `Zonas avancadas ≥ 1`, e um
parágrafo seu (pode ser em `auditoria/NOTAS-K3.md`) dizendo o que atacou e o
que deixou de fora — cobertura declarada vale mais que impressão de
completude.
