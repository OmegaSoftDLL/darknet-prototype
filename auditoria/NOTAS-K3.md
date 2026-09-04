# NOTAS DO PROGRAMADOR — resposta à auditoria 2026-08-21

| | |
|---|---|
| **Data** | 2026-08-21 |
| **Em resposta a** | `auditoria/2026-08-21-auditoria-completa.md` + `GUIA-DO-PROGRAMADOR.md` |
| **Escopo** | Correção dos achados da auditoria + plano maior de estabilização (bugs de gameplay, performance, arquitetura, backend, qualidade) |

---

## 1. Achados da auditoria — status item a item

| Sev. | Achado | Status | O que foi feito |
|---|---|---|---|
| ~~P0~~→P2 | Portal dentro de construção | **Fechado (garantia intencional)** | `updatePhasePortal` agora verifica `!isBlocked(owPortalPos)` ao abrir; se bloqueado, loga aviso e realoca em anel de 12 candidatos a 700u do refúgio. Já provou utilidade: disparou na fase 4 de um run e realocou. |
| P1 | Cobertura zero do fluxo de fase | **Fechado** | `BotController` recebe `owPortalPos`/`owPortalOpen` a cada frame; novo estado `AdvancePhase` (prioridade abaixo só de fuga de morte); a <100u do portal seta `shouldUsePortal`, que `updatePhasePortal` aceita como equivalente ao [E]. **Aceite atingido: `Zonas avancadas: 3` em 300s seed 7** (aceite era ≥1). |
| P1 | "Siga o marcador" sem marcador | **Fechado** | Blip verde pulsante no radar + seta na borda da tela apontando o portal fora da tela (anel pulsante quando visível), em `drawMinimap`/`drawHudAndOverlays` (pontos compartilhados de HUD). O texto agora é verdadeiro. |
| P1 | Arca/RTS medievais na cidade moderna | **Fechado** | `Ark` em zona urbana vira fortificação sci-fi (`drawArkStructure`: bunker de concreto, antenas com faróis pulsantes, barricadas); `House` urbana usa `drawGenericStructure`. `castle.obj`/`house.obj` só em zonas medievais (`CursedFarm`, `DarkForest`, `Cemetery`, `AbandonedManor`). |
| P1 | ~6 FPS sustentado na abertura | **Fechado** | Duas causas atacadas: (1) construção dupla do cenário eliminada — `safeZoneCenter`/fase definidos ANTES de `buildOpenWorldScenery` (log `SCENERY` agora 1× por mundo); (2) `m_voxGenBudget` 3→1 espalha o readback GPU→CPU. Causa-raiz real encontrada por instrumentação: o screenshot PNG síncrono do autotest (114–180ms/frame) + média móvel de FPS envenenada pelo frame de worldgen. Screenshot agora é assíncrono; o relatório tem quarentena de 1s após frames com dt>0,25s. **FPS mínimo: 6 → 47–57.** |
| P2 | Cenário construído 2× na inicialização | **Fechado** | Ver item acima — reordenação no construtor e remoção da reconstrução redundante em `runAutoTest`. Em `advanceOpenWorldPhase`, parâmetros da fase nova são setados ANTES de reconstruir (defeito da mesma classe: fase nova nascia com raio da anterior). |
| P2 | Fases 2+ com cidade de LA no hub | **Fechado** | `setupWorldRegions` usa `currentZone` para as 9 regiões (uma fase = um mundo coerente); `advanceOpenWorldPhase` regrava `tilemap.currentZone` após `generateOpenWorld()` (estava preso em LARuins). Evidência em log: fase 2 = Fazenda Maldita, fase 3 = Floresta Negra, fase 4 = Cemitério — cada uma com seu catálogo de estruturas. |
| P2 | Melee inerte na automação | **Fechado** | Causa: backoff de recuo (150px) + órbita (220px) impedia fechar o alcance melee (90px). Corrigido com engage de 1,5s sem recuo e órbita de 80px. **11–23 ataques melee/120s** (aceite era >10). |
| P2 | Alerta de FPS enganoso | **Fechado** | Quarentena de amostras após frames de carga (dt>0,25s); o alerta agora mede FPS de jogo. Últimos runs: **zero** entradas "FPS critico". |
| — | Higiene: WIP em `BotController.h` | **Fechado** | Era trabalho em andamento desta sessão, agora completo: `shouldPickupItem` removido (era código morto — nenhum consumidor), `reset()` implementado e chamado por `restartRun`. |

## 2. Trabalho adicional além da auditoria (estabilização geral)

Pedido do dono do projeto: "corrigir tudo". Executado em fases, cada uma validada com build + autotest:

- **Bugs de gameplay:** bot preso na zona segura (inimigos só spawnam fora — o bot nunca os encontrava; 0 kills → 33–48 kills/120s); coleta de itens contada no lugar errado (0 → 118–156/120s); thresholds de skills; 8 timers `static` que vazavam estado entre partidas convertidos em membros com reset em `restartRun`; pathfinding BFS duplicado unificado; throttle de rede `static` em `NetClient` virou membro.
- **Pipeline visual:** pipeline 2D paralelo e toggle F10 removidos (3D isométrico é o único caminho); ~1.100 linhas de `render3D()` órfãos removidos (Player/Enemy/NPC/Companion); flag global renomeada para `g_voxelCapture` (reflete a função real).
- **Performance de render:** máscara de luz de 31 elipses concêntricas → 1 quad texturizada por luz em meia resolução (perfil de alpha matematicamente equivalente); piso do tilemap de 2.809 batches → 1 batch único; frustum culling real no cenário e no piso; LOD de sombra/contorno voxel por distância. **Pior render: 95,9ms → ~19–22ms.**
- **Arquitetura:** `Game.cpp` de ~8.700 → **4.718 linhas**; 11 módulos extraídos (`Game_Shaders`, `Game_Network`, `Game_PremiumStore`, `Game_Resources`, `Game_Evolution`, `Game_QuestsNPC`, `Game_Phases`, `Game_Spawn`, `Game_WorldGen`, `Game_HUD`, `Game_Bot`) — corte-e-cole de funções inteiras, sem mudança de lógica (verificado com diff em uma das etapas).
- **Produto:** cosméticos premium (skins Neon/Dragão, pet Drone, tintas) agora renderizam — o pipeline existia, mas `visualSignature()` não incluía as skins na chave de cache do voxel, então o modelo 3D nunca atualizava.
- **Save:** bump V4→V5 com IDs estáveis de equipamento (`EDB::byId`); renomear item não quebra mais saves; fallback para saves V4 por nome.
- **Qualidade:** parsing JSON manual (substring) substituído por `nlohmann/json` vendored em `third_party/`; doctest vendored com **10 casos / 73 asserções** (`CraftingSystem`, `Enemy`, `Equipment`) rodando via CTest; target `darknet_tests` separado.
- **Backend:** `inventory.qty` usado de verdade (UPSERT + migração idempotente com unique index); DDL consolidado como fonte única no `index.js` (`init.sql` virou apontador); Redis órfão removido do compose; `PUBLIC_URL` default → gateway 8080; `server/README.md` com endpoints reais; `node --check` OK.
- **Documentação:** protagonista padronizado (Vance Rios), ano 2047, README atualizado para 3D isométrico, `ROADMAP.md` consolidado criado na raiz.

## 3. Métricas finais (autotest Release, seed 7)

| Métrica | Linha de base da auditoria | Final |
|---|---|---|
| FPS médio | 58–59 | 58–59 |
| FPS mínimo | 6 | **47–57** |
| Pior render() | 28,7–44,5 ms | **~19–22 ms** |
| Abates/120s | 20–31 | 33–48 |
| **Zonas avancadas** | **0** | **3** (em 300s) |
| Mortes / travamentos >10s | 0 / 0 | 0 / 0 |

## 4. Não coberto / deixado de fora (declarado)

- **Save/load roundtrip, multiplayer, loja premium, backend em execução** — seguem sem teste de integração ponta a ponta (exigem servidor ativo); o que foi validado é estático (`node --check`, compilação, unit tests).
- **Áudio** — não avaliado (exige julgamento humano).
- **Fases 5+** — runs de 300s chegam à fase 4; fases urbanas posteriores (GhostCity) usam o mesmo caminho de código da fase 1, mas não foram observadas em execução.
- **Verificação visual por screenshot** das correções de Ark/bioma — evidência é por logs + análise do caminho de render, não por inspeção de frame.
- **Padronização em massa de nomenclatura** (`m_` prefixo) — não feita; alto risco/baixo retorno. Adotar em código novo.
- **Idioma dos comentários** — mantido português (padrão vigente do projeto).
- Hitch esporádico de 400–900ms no swap observado em algumas rodadas — driver/DWM da máquina, não reproduzível deterministicamente, update+render <10ms no instante.

## 5. Instrumentos deixados

- `validate.sh [segundos] [seed]` — portão (Debug+Release + bot), usado nas duas seeds do guia.
- `darknet_tests.exe` / `ctest` — 10 casos de regras puras.
- `bot_report.txt` + `shot_NN.png` durante `--autotest`; relatório com quarentena de carga.
- `ROADMAP.md` (raiz) — documento único de estado e próximos passos.

---

# ADENDO — resposta à auditoria 2 (2026-08-21, segunda passada)

| | |
|---|---|
| **Em resposta a** | `auditoria/2026-08-21-auditoria-2.md` (APROVADO COM RESSALVAS) |
| **Ação** | Correção dos 8 achados novos (1×P1, 7×P2) + verificação visual por frames |

## A1. Reconhecimento da auditoria 2

A segunda passada confirmou **FECHADOS** os 9 achados da auditoria 1 (medição: Zonas avançadas 1/1/3, FPS mínimo 42-57, zero falso-alarme, bioma por fase). Sobre o cenário das fases: *"Fazenda lê como fazenda... A paleta de clutter por bioma funciona."* — o trabalho de bioma (paleta por zona em `Zone.h:clutterPaletteFor` + seed variável por fase) foi verificado em frame.

## A2. Achados novos — correções aplicadas

| Sev. | Achado | Status | O que foi feito |
|---|---|---|---|
| P1 | Prédio moderno esconde o jogador | **Fechado** | Translucidez de oclusão replicada no `case 20` (prédio) e `case 15` (bunker), com janela baseada na **pegada real** (não a fixa 620/230). `rlDisableDepthMask` envolve o desenho translúcido para o herói não ficar clipado. **Verificado em frame: `shot_02` mostra os prédios de LA translúcidos com a rua legível através e o player visível.** |
| P2 | Portão não exige avanço de fase | **Fechado** | `BotController::passed()` reprova runs ≥180s com `zonesVisited==0`. Blinda a conquista do ciclo contra regressão silenciosa. |
| P2 | Floresta Negra / noite ilegível | **Fechado** | Piso de luz ambiente (máscara multiplicativa `amb>=0.64` ≈ 0.33 luminosidade mínima — o clima fica no matiz, não no breu) + luz do player 430→480/0.72→0.82 + rim light frio nos atores conforme `ambientDark` sobe (em `drawVoxel`, cobre player/inimigos/NPCs/companions). |
| P2 | Garantia do portal fraca | **Fechado** | Testa o **disco** (centro + 8 amostras a 100u), busca em 3 anéis × 16 ângulos + espiral de 220 candidatos; se nada livre, realoca ao refúgio com `Tilemap::clearSolidAt` — nunca mais mantém posição bloqueada. Disparou de verdade no run de validação. |
| P2 | SCENERY `estruturas=0` em cidade | **Fechado** | Contador inclui tipos 6, 9, 14-20. Log agora `estruturas=181` em LA (era 0). |
| P2 | Caminhos absolutos hardcoded | **Fechado** | Relatórios do bot gravados no CWD (funciona em CI/outras máquinas). |
| P2 | Autotest não limpa shots antigas | **Fechado** | `runAutoTest` remove `shot_*.png` do CWD antes de começar. |
| P2 | Textos de fase inconsistentes | **Fechado** | `advanceOpenWorldPhase` avança `storyChapter`; `phases.txt` unificado ("Fazenda Maldita"). **Verificado em frame: `shot_04` mostra `CAP.2 | Fazenda Maldita`.** |

## A3. Verificação visual própria (k3 assistiu os frames)

- `shot_01`/`shot_02` (LA): ruína urbana cinza; prédios translúcidos pós-correção, player sempre visível.
- `shot_04` (Fazenda): campo verde, árvores mortas, `CAP.2` correto — cenário e texto distintos de LA.
- `shot_07`/`shot_09` (Fazenda à noite): evidenciaram a escuridão excessiva que motivou a correção de legibilidade noturna.

## A4. Ainda não coberto (declarado)

- Marcador do portal e Arca sci-fi **renderizados em frame** (código conferido; a cadência de 30s das shots não coincidiu com as janelas — a auditoria 2 declarou o mesmo limite).
- Fases 5-11 em execução (runs chegam à 4ª; biomas urbanos posteriores por análise de código).
- Diff linha a linha da extração dos 11 módulos (garantia = portão + testes unitários).
- Save/load roundtrip, multiplayer/loja/backend com servidor, áudio, entrada humana.

## A5. Estado do repositório

Conforme nota da auditoria 2 (§7): todo o trabalho está **não commitado** (34 arquivos M + novos). **Recomenda-se commitar** — este é o melhor estado que o projeto já teve, e um `git checkout` acidental o apagaria.

---

# ADENDO — Ciclo 3: auditoria completa com correções (2026-09-04)

| | |
|---|---|
| **Em resposta a** | Auditoria completa executada em 2026-09-04 (build, ctest, validate, runtime, git, servidor) |
| **Recomendações aplicadas** | Todos os achados críticos + estruturais com correção objetiva |

## C1. Achados corrigidos

| Sev. | Achado | Status | O que foi feito |
|---|---|---|---|
| 🔴 | Repositório não compila se clonado: 11 `Game_*.cpp` + `tests/` + `third_party/` untracked, CMakeLists os referenciava, 35 arquivos modificados e 1 commit a frente sem push | **Fechado** | Commit `06b7940` com tudo (53 arquivos). Repositório volta a ser auto-contido e clonável. |
| 🟠 | JWT com default hardcoded `dev-secret` (tokens forjáveis) | **Fechado** | Sem `JWT_SECRET`, gera segredo aleatório por boot (`crypto.randomBytes`) + warn; nunca default fixo. |
| 🟡 | Save/load sem cobertura — e com bug real | **Fechado** | Teste roundtrip V5 + legado V4 adicionado ao `darknet_tests` (12 casos / 109 asserções, release+debug). O teste **expôs bug**: `totalKills` era gravado e mostrado no menu de slots, mas **nunca restaurado no Player** no load → correção em `SaveManager::load`. |
| 🟡 | Higiene de repo | **Fechado** | `.gitignore` agora cobre `autotest_*.log`, `validate_seed*.log`, `shot_*.png`, `screenshot*.png`. |

## C2. Validação pós-correção (2026-09-04)

- `cmake --build` Debug e Release: **exit 0** nos dois.
- `darknet_tests.exe`: **12/12 casos, 109/109 asserções** (era 10/73).
- CTest: **1/1 passado**.
- `node --check` (game-server): **OK**.
- `git status`: limpo após commit.

## C3. Ainda não coberto (declarado — sem correção objetiva nesta passada)

- Fases 5–11 em execução (runs chegam à 4ª; bioma urbano posterior por análise de código).
- Save/load roundtrip pleno no jogo (multiplayer/loja/backend com servidor ativo) — o que foi validado é unitário + build + syntax.
- Áudio e gameplay exigem julgamento humano.
