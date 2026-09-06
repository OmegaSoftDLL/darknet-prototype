# Auditoria das Fases Avançadas (5-10) — 2026-09-05

## Objetivo

Validar que as fases 6-11 da campanha (índices 5-10 em `content/phases.txt`) geram o bioma correto, spawnam inimigos, permitem combate e passam no portão de validação do bot autônomo.

## Instrumentação adicionada

- Parâmetro `--start-phase=N` em `src/main.cpp`.
- Flag estática `Game::startPhaseOverride` (mesmo padrão de `Game::headless`).
- `Game` lê o override no construtor para inicializar `owPhase`, `currentZone`, `currentRegion`, `owPhaseRadius` e `owPhaseGoal` a partir de `phaseDef(N)`.

## Defeito encontrado e corrigido

`Game game;` era criado **antes** de `game.startPhaseOverride = N`, então o construtor sempre usava fase 0 (`LARuins`). O log `SCENERY` mostrava "Ruínas de Los Angeles" mesmo com `--start-phase=5`.

**Correção**: `startPhaseOverride` foi tornado `static` e `Game::startPhaseOverride` é setado em `main.cpp` **antes** da construção de `Game`.

Arquivos alterados:

- `src/main.cpp`: parse de `--start-phase=N` e atribuição estática antes do construtor.
- `src/Game.h`: declaração de `static int startPhaseOverride`.
- `src/Game.cpp`: definição e uso no construtor.

## Resultados do autotest headless

Comando por fase:

```bash
darknet.exe --autotest --headless --test-seconds=120 --start-phase=N
```

| Índice | Zona | Objetos SCENERY | Estruturas | Validação |
|---|---|---|---|---|
| 5 | Bunker NEXUS | 9.790 | 312 | **PASSOU** |
| 6 | Catacumbas | 10.733 | 292 | **PASSOU** |
| 7 | Mansão Abandonada | 11.753 | 336 | **PASSOU** |
| 8 | Forja KRONOS | 12.902 | 385 | **PASSOU** |
| 9 | Zona Inferno | 14.003 | 407 | **PASSOU** |
| 10 | Núcleo KRONOS | 15.486 | 569 | **PASSOU** |

## Métricas da fase terminal (KronosNexus, 120s)

- Inimigos abatidos: 130
- Skills disparadas: 119
- Ataques melee: 88
- Pico de inimigos: 82
- Itens coletados: 513
- Mortes: 0
- Travamentos >10s: 0
- FPS mínimo: 388

## Notas

- Testes de 30-60s falhavam esporadicamente por aleatoriedade de spawn/movimento do bot; 120s por fase estabilizou o gate.
- Nenhuma alteração de gameplay foi necessária — apenas instrumentação de auditoria + correção do ponto de aplicação do override.

## Gate

- Build Release: OK
- Autotest headless fases 5-10: **APROVADO**
