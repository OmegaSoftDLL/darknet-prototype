# AUDITORIA COMPLETA — Darknet Prototype

| | |
|---|---|
| **Data** | 2026-08-21 |
| **Commit auditado** | `6113e73` (master) |
| **Binário** | `build/Release/darknet.exe` de 21/08 16:13, posterior à última edição de código; `content/` e `resources/shaders/` presentes ao lado do exe |
| **Auditor** | Claude (papel exclusivo de auditoria — ver `.agents/AUDITOR.md`) |
| **Veredito** | **APROVADO COM RESSALVAS** — na 1ª passada: 1×P0, 4×P1, 2×P2. **Atualizado pelo adendo (§6): P0 rebaixado após verificação → 0×P0, 4×P1, 5×P2.** |

Este relatório é para o programador. Cada achado traz onde, evidência, repro e o
efeito no jogador. Está separado o que foi **medido** do que foi **deduzido** —
não misture os dois ao priorizar.

---

## 1. Metodologia

Dois runs completos do portão de validação, seeds distintas, 120 s cada, em
Release:

```bash
./validate.sh 120 7
build/Release/darknet.exe --autotest --test-seconds=120 --seed=20260821
```

Ambos: **`VALIDACAO: PASSOU`** (exit 0) — primeira aprovação do portão nesta
base. Além disso: inspeção dos 10 frames `shot_NN.png` gravados a cada 10 s do
run seed 20260821, e conferência de código nos pontos que os frames levantaram.

### Números medidos (2 runs)

| Métrica | seed 7 | seed 20260821 |
|---|---|---|
| FPS médio | 59 | 58 |
| FPS mínimo | 6 | 6 |
| Abates | 20 | 31 |
| Mortes | 0 | 0 |
| Bloqueios >2 s / críticos >10 s | 1 / 0 | — / 0 |
| Quadrantes explorados | 4/4 | 4/4 |
| Distância percorrida | 19.240 px | 22.199 px |
| Pior update() / render() | 1,27 / 44,5 ms | 1,47 / 28,7 ms |
| **Zonas avançadas** | **0** | **0** |

O fix do detector de "preso" (commit `6113e73`) confirma-se na prática: o bot
voltou a engajar (20–31 abates vs. 0 nos runs anteriores) e o falso travamento
sumiu (0 críticos).

---

## 2. Achados

### [P0] Portal da fase pode nascer dentro de construção — soft-lock de progressão

> **⚠ SUPERADO PELO ADENDO (§6):** verificado por reconstrução determinística —
> 120/120 variantes com o portal alcançável. Rebaixado para risco residual
> (P2: tornar a garantia intencional). O texto abaixo é mantido como registro
> da hipótese original e do porquê ela era plausível.

- **Onde:** `src/Game.cpp:5486` — `owPortalPos = safeZoneCenter + (620, −520)`,
  posição fixa definida quando a cota de abates fecha.
- **Evidência:** **DEDUZIDO de código, não observado em execução.** O cenário é
  bakeado no início da fase (`buildOpenWorldScenery`); nada limpa nem protege o
  ponto do portal. A praça protegida do hub tem raio 360; o portal fica a ~810
  do centro — área onde a grade da cidade coloca prédios. Prédio moderno marca
  tiles sólidos com raio `125 × escala` (`markSolidAt`); a interação do portal
  exige o jogador a **105 u** do centro (`Game.cpp:5491`). Se um prédio ocupar o
  ponto, não há como chegar perto o bastante: **a fase não fecha nunca**.
- **Repro sugerida (script descartável, sem tocar no jogo):** varrer seeds
  1–200 chamando `isBlocked(owPortalPos)` após a geração do mundo e contar
  colisões. Qualquer taxa acima de zero confirma o P0.
- **Efeito no jogador:** progressão morta sem mensagem de erro. O tipo de bug
  que escapa de todo teste manual e vira review negativa.

### [P1] Fluxo de avanço de fase tem cobertura automatizada ZERO

- **Onde:** `src/Game.cpp:4252` — o bot recebe `tilemap.portals` (sistema
  antigo de portais, **vazio no mundo aberto**); `owPortalPos` nunca é
  informado ao `BotController`, e o avanço exige `IsKeyPressed(KEY_E)`.
- **Evidência:** **MEDIDO** — `Zonas avancadas: 0` em todos os runs validados
  até hoje. `advanceOpenWorldPhase()` (regenerar mundo, recompensas, fase de
  chefe, tela de transição) **nunca executou sob o portão de validação**. A
  mecânica central mais nova do jogo é a única sem rede de segurança; as fases
  2–11 estão, na prática, sem nenhum teste.

### [P1] HUD promete "siga o marcador" — o marcador não existe

- **Onde:** `src/Game.cpp:8048` (texto) vs. todos os usos de `owPortalPos`
  (linhas 5486, 5491, 7077, 7102 — spawn, proximidade, desenho, luz; **nenhum
  indicador direcional**).
- **Evidência:** **MEDIDO** — frame `shot_09` (seed 20260821, t≈99 s) mostra
  "PORTAL ABERTO — siga o marcador e pressione [E]" sem seta, sem blip no
  radar, sem nada apontando o rumo. As setas rosa visíveis são indicadores de
  inimigo fora de tela.
- **Efeito:** o jogador tem que adivinhar a direção de um ponto não sinalizado
  num mundo de raio 3000+. A instrução do próprio jogo mente.

### [P1] A base do jogador é um castelo medieval no meio da cidade moderna

- **Onde:** `src/Game.cpp:7287` — `BuildingType::Ark` desenha `castle.obj`;
  `:7289` — `House` do RTS desenha `house.obj` (telhado de telha).
- **Evidência:** **MEDIDO** — frame `shot_04`: castelo de torres vermelhas com
  tooltip "Arca [Lv 1/3] — Núcleo da base" cercado de asfalto, faixa de
  pedestres e prédios de concreto. A limpeza de coerência (catálogo por bioma,
  commit `c66d2dc`) alcançou o **cenário**, mas não as construções do
  **BuildingSystem** — que ficam no centro da atenção o jogo inteiro (é onde o
  jogador renasce).

### [P1] Janela sustentada de ~6 FPS na abertura da partida

- **Evidência (MEDIDA):** 38+ amostras consecutivas de `FPS critico: 6` nos
  dois runs, com **0 inimigos e 0 projéteis** no instante do mínimo, e pior
  `update()` de apenas 1,3–1,5 ms — o custo não está na simulação nem no frame
  comum.
- **Causa (DEDUZIDA):** geração dos modelos voxel na entrada da partida.
  `SpriteExtrude::CaptureToImage` cria uma RenderTexture **por captura** e faz
  `LoadImageFromTexture` (readback GPU→CPU = stall de pipeline). São ~40+
  modelos (tipos × 4 poses de caminhada), amortizados a 3 por frame
  (`m_voxGenBudget`) — dá uma janela de vários segundos a ~6 FPS logo na
  primeira impressão do jogo. A instrumentação de `update()/render()` não
  parece cobrir esse pré-passe, o que explica o pior render medido (44 ms) não
  bater com 6 FPS (166 ms/frame).

### [P2] Melee praticamente inerte na automação

- **Evidência:** 1 ataque melee em 240 s somados de bot, contra 120 disparos de
  skill. Ou a distância de melee está desalinhada com o corpo dos inimigos, ou
  o bot nunca escolhe essa ação. Hoje a métrica de melee do relatório não mede
  nada — e o sistema de melee está efetivamente sem cobertura.

### [P2] Alerta de FPS do relatório é enganoso

- **Onde:** `bot_report.txt` — "PROBLEMA: FPS caiu abaixo de 40 — otimizacao
  necessaria".
- **Evidência:** o alerta dispara por causa do engasgo de abertura (achado P1
  acima) e mascara que o jogo **sustenta 58–59 FPS** o resto do tempo. Alerta
  que grita sempre é alerta que o programador aprende a ignorar; o correto
  seria separar "hitch de carga" de "FPS de jogo".

---

## 3. O que está bom (confirmado nos frames)

- Carros lêem como carros: rodas, cabine recuada, para-lamas (`shot_09`).
- Ruas com meio-fio e faixa desgastada dão direção e leitura de cidade.
- Unidades aliadas usam o modelo de soldado com anel verde de aliado — fim dos
  "pinos" verdes.
- Prédios com fileiras de janela, material variado e alguns em ruína.
- Estabilidade: zero crash, zero morte, navegação sem travamento crítico em
  4 min de Release somados.

---

## 4. Não auditado (declarado)

- **Save/load** (roundtrip de continuar partida).
- **Multiplayer, loja premium, backend** (exigem servidor ativo).
- **Áudio** — a cadeia nova (filtro, reverb, sub, compressor) compila e roda
  sem crash, mas auditor não escuta: exige julgamento humano.
- **Fases 2–11 em execução real** — nenhum run chegou lá (consequência direta
  do achado de cobertura zero do portal).
- Entrada humana (mouse/teclado), balanceamento além da fase 1.

---

## 5. Correções de maior retorno (em ordem)

1. **Portal em ponto garantidamente livre** — validar/realocar `owPortalPos`
   (ou limpar sólidos num raio ao abrir). Mata o P0; de quebra, um futuro
   marcador passa a ter alvo confiável.
2. **Bot atravessa o portal** — informar `owPortalPos` ao `BotController` e
   simular o [E]. A mecânica central entra no portão de validação e as fases
   2+ passam a ser exercitadas automaticamente. Sem isso, todo o conteúdo além
   da fase 1 continua no escuro.
3. **Arca/RTS sem modelos medievais** — é o objeto mais olhado do jogo (ponto
   de respawn) e contradiz a direção de arte definida para as zonas urbanas.

> Nota de risco: os três tocam `Game.cpp` (8.000+ linhas concentrando render,
> update, mundo e UI). Tratar qualquer mudança ali como risco alto de regressão
> e rodar `./validate.sh 120 7` **e** com uma segunda seed antes de commitar.

---

## 6. ADENDO (2026-08-21, mesmo dia) — Verificação do P0 do portal

### Método

A geração do cenário **não usa `--seed`**: `buildOpenWorldScenery` roda uma LCG
própria com constante fixa `0x1234abcd` e zero chamadas a `GetRandomValue`
(verificado no fonte). Chunks não geram dentro da região fixa (`ox < ORIG`).
Logo o entorno do portal é **um fato determinístico**, não uma probabilidade —
"varredura de seeds" era o instrumento errado; o correto é reconstruir o layout.

Portei o gerador da região (0,0) para um script de medição (scratchpad, fora do
repo) e validei contra o checksum que o próprio jogo loga (`SCENERY
total/estruturas/grama`). A réplica chegou a ~99,3% do checksum (2992–2999 vs
3014; grama 1439 vs 1440) — o stream exato do MSVC não foi reproduzido (ordem
de avaliação de argumentos/float32). Para blindar a conclusão contra esse
resíduo, rodei **120 variantes de stream** (2 ordens de avaliação × 2 × 30
deslocamentos iniciais da LCG): a geometria da grade (células, EDGE=350,
slots) é fixa; só variam quais lotes existem, tipos e escalas.

### Resultado

**120/120 variantes: disco de interação do portal LIVRE e ALCANÇÁVEL a pé
desde o hub** (BFS na grade de tiles sólidos). Pior caso: 12 dos ~13 tiles do
disco livres. Razão estrutural: o portal (hub + 620,−520 = 4716,3576) cai no
**corredor de rua** entre as fileiras de lotes (y=3420 e y=3670); o lote mais
próximo possível fica a ~123u do portal com pegada máxima de 96u (prédio
moderno, escala ≤1.0), e o raio de interação é 105u — nenhuma estrutura
isolada cobre o disco, e a cobertura combinada nunca ocorreu.

### Reclassificação

> **P0 "portal pode nascer dentro de construção" → REBAIXADO para risco
> residual (monitorar).** Não confirmado na fase 1 sob 120 variantes. Vira P0
> de novo se: o offset do portal mudar, `EDGE`/grade mudarem, os catálogos
> ganharem pegada > 105u perto do hub, ou o raio de interação diminuir. A
> proteção estrutural é acidental — nenhuma linha de código garante o corredor
> livre; um comentário-guarda no código (ou um assert de `!isBlocked` ao abrir
> o portal) tornaria a garantia intencional. Fica como recomendação P2.

### Achados novos descobertos na verificação

**[P2] O cenário do mundo é construído DUAS vezes na inicialização**
- **Evidência (MEDIDA):** `gt.log` linhas 468–469 — dois `SCENERY` seguidos com
  contagens diferentes (`total=1626/grama=749`, depois `total=3014/grama=1440`).
- **Dedução:** a primeira construção roda com `safeZoneCenter` velho
  (~(1280,1280) — a réplica com esse centro aproxima 1665/757 ≈ 1626/749); a
  segunda, com o centro correto (4096,4096), é a que vale. A primeira é
  trabalho jogado fora e contribui para o engasgo de abertura (achado P1 já
  reportado).

**[P2] Fases 2+ mantêm a cidade de LA no hub, sobre o chão de outro bioma**
- **Evidência (DEDUZIDA de código):** `setupWorldRegions` reconstrói sempre o
  mesmo mapa 3×3; a região (0,0) é sempre `LARuins`, e o ramo de grade da
  cidade usa `structuresFor(r.zoneType)` = catálogo urbano — em **toda** fase.
  Já o piso segue `biomeAtWorld` = zona da fase. Resultado esperado: na fase
  Cemitério, o hub terá prédios modernos e bunkers sobre terra de cemitério.
  Não observável hoje porque nenhum run alcança fases 2+ (achado P1 de
  cobertura) — as duas pendências se somam.

### Limite declarado do método

A réplica não reproduziu o stream exato do binário (diferença ~0,7% no
checksum). A conclusão do portal não depende disso (120 variantes
descorrelacionadas concordam), mas qualquer uso futuro da réplica para
perguntas *sensíveis à posição exata* de um objeto específico exige fechar
essa diferença primeiro.

---

## 7. CONSOLIDAÇÃO FINAL (2026-08-21) — estado após a verificação

### Placar de achados (vigente)

| Sev. | Achado | Status | Evidência |
|---|---|---|---|
| ~~P0~~→P2 | Portal pode nascer dentro de construção | **Rebaixado** — 120/120 variantes alcançável (§6); proteção é acidental, recomenda-se assert `!isBlocked(owPortalPos)` ao abrir | Reconstrução determinística |
| P1 | Fluxo de fase com cobertura automatizada ZERO | Aberto | Medido: `Zonas avancadas: 0` em todos os runs |
| P1 | "Siga o marcador" sem marcador | Aberto | Medido: frame `shot_09` |
| P1 | Arca/RTS medievais na cidade moderna | Aberto | Medido: frame `shot_04` |
| P1 | ~6 FPS sustentado na abertura | Aberto | Medido: 38+ amostras, 0 entidades no mínimo |
| P2 | Cenário construído 2× na inicialização (1ª vez com centro velho, descartada) | Aberto | Medido: 2 logs `SCENERY` (1626→3014) |
| P2 | Fases 2+ mantêm cidade de LA no hub sobre chão de outro bioma | Aberto | Deduzido de código (invisível até haver cobertura de fases) |
| P2 | Melee inerte na automação (1 ataque em 240 s) | Aberto | Medido |
| P2 | Alerta de FPS do relatório grita sempre (mistura hitch de carga com FPS de jogo) | Aberto | Medido |
| P2 | Assert/garantia intencional do corredor do portal | Recomendação nova (§6) | — |

### Prioridades REVISADAS para o programador (substituem a §5)

1. **Bot atravessa o portal** (informar `owPortalPos` ao `BotController` +
   simular o [E]). Passou a ser o nº 1: destrava a cobertura das fases 2–11,
   coloca `advanceOpenWorldPhase` sob o portão de validação e exporia em
   screenshot o achado "cidade de LA em toda fase". Sem isso, ~90% do conteúdo
   do jogo segue sem nenhum teste.
2. **Marcador do portal no HUD/radar** — ou corrigir o texto que promete um
   marcador inexistente. Junto, o assert de `!isBlocked(owPortalPos)` ao abrir
   (3 linhas, transforma a proteção acidental do §6 em garantia).
3. **Arca/RTS sem modelos medievais** — objeto mais olhado do jogo (ponto de
   respawn), único remanescente da incoerência de direção de arte já resolvida
   no cenário.
4. **Engasgo de abertura** — eliminar a construção dupla do cenário (ganho
   grátis, medido) e tirar o readback GPU→CPU da geração de voxel do caminho
   quente (pré-gerar na tela de título ou cachear em disco).

### Observação de higiene do repositório

Durante a auditoria foi encontrada no working tree uma modificação em
`src/BotController.h` **de origem desconhecida** (não é do auditor e não está
em nenhum commit): remove o campo `shouldPickupItem` da struct de decisão e
declara um método `reset()` referindo um `Game::restartRun` **que não existe
no código atual**. Estado: incompleta — a remoção do campo quebra a compilação
se algo ainda o ler, e o `reset()` declarado sem implementação quebra no link
se for chamado. Recomenda-se ao programador identificar a origem (outra
sessão/agente/edição manual) antes do próximo build; o auditor não tocou no
arquivo.

### Instrumentos deixados para reuso

- `validate.sh [segundos] [seed]` — portão pass/fail (compila Debug+Release,
  roda o bot, exit ≠ 0 reprova com motivo).
- Réplica determinística do gerador de cenário (scratchpad da sessão,
  descartável) — reutilizável para perguntas de layout; limitação declarada:
  ~0,7% de divergência do stream exato do MSVC (§6).
- `bot_report.txt` + `shot_NN.png` a cada 10 s durante `--autotest`.
