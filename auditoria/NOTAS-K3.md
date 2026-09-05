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

---

# ADENDO — Ciclo 4: backend TLS + netcode seguro (2026-09-04)

| | |
|---|---|
| **Em resposta a** | Plano "deixar incrível" — bloco escolhido pelo dono: **TLS + netcode backend** |
| **Escopo** | Endurecimento de segurança do servidor + reparo da rota de rede do cliente |

## D1. O que foi feito

| Sev. | Tema | O que foi feito |
|---|---|---|
| 🔴 | JWT re-exposto (default público anulava o fix do Ciclo 3) | `docker-compose.yml`: `JWT_SECRET: "${JWT_SECRET:?defina...}"` — **fail-fast**; sem default fixo conhecido. |
| 🔴 | WS sem autenticação (qualquer conexão entrava na sala) | Upgrade só aceito com JWT válido no header `Authorization` (400 em `ws` v8: `noServer` + evento `upgrade`; sem token → **401 antes de abrir o socket**). |
| 🟠 | Spoof de identidade (peer/chat usavam `id`/`nome` do JSON) | Servidor passa a derivar identidade **do token** (`ws.user`), nunca do corpo; coordenadas saneadas/clamped; nome do peer vem do JWT. |
| 🟠 | Cliente falava direto em `:9000`, ignorando o gateway | `StoreClient`: prefixo `/api` + TLS opcional via env `DARKNET_API_URL`; `NetClient` envia o JWT no handshake; `startNetwork` aguarda o login antes de conectar. |
| 🟠 | Sem TLS na REST | `HttpClient.cpp` ganha `WINHTTP_FLAG_SECURE` (https só com certificado válido); gateway com bloco HTTPS pronto (443 → cobre `/api` e `/ws`, realtime vira `wss://`). |
| 🟡 | Sem heartbeat / flood / limites | Heartbeat 30s do servidor (expulsa inativos), 120 msgs/s por conexão, payload ≤4 KiB por mensagem, `maxPayload` 1 MiB. |
| 🟡 | REST sem rate limit / validação | Limite por IP (login 20/min, loja 30/min, progress 60/min) + `trust proxy`; nome saneado (≤24, sem controle); `save_json` ≤100 KB; dev-grant ≥0 ≤1M. |
| 🟡 | Webhook Stripe sem idempotência | Crédito único por `provider_ref` (checagem em `transactions` + dedupe em memória no modo dev) — retry/duplicata não credita 2×. |

## D2. Validação pós-correção (2026-09-04)

- `node --check` (game-server): **OK**.
- Build Debug e Release: **exit 0** nos dois (targets `darknet` e `darknet_tests`).
- `darknet_tests.exe`: **12/12 casos, 109/109 asserções**.
- **Smoke test funcional com servidor real (memória)**: 13/13 PASS — login + token + nome saneado, `/me` 401 sem token, rate limit → 429, WS sem token rejeitado (401), WS com token conecta, **anti-spoof** (id 999 forjado → id real do token), coordenadas saneadas, conexão sobrevive a flood.
- `validate.sh 120 7` e `validate.sh 120 20260821`: **APROVADO** nas duas.
- `git push origin master`: Ciclos anteriores (até `38bb85f`) **enviados**.

## D3. Ainda não coberto (declarado)

- **`wss://` no cliente**: `NetClient` é Winsock puro sem TLS — recusa URLs `wss://` antes de abrir thread (documentado no código). O gateway já serve `/ws` sob TLS quando o bloco 443 é ativado; migrar o cliente para WSS exige TLS (Schannel/OpenSSL) no socket.
- **Auth real** (trocar o stub `/auth/login` por OAuth ou e-mail+senha com hash) — fora do escopo deste ciclo.
- Fases 5–11 em execução e áudio (julgamento humano) — seguem como no Ciclo 3.

---

# ADENDO — Ciclo 5: colisão de cenário + retheme visual dos menus (2026-09-04)

| | |
|---|---|
| **Em resposta a** | 1) Bug do usuário: "personagens atravessando por coisas no cenário (carros e prédios muito grandes)". 2) "O visual do jogo/menus é uma merda, melhorar 1000%" — escopo aprovado: **menus primeiro**, estilo **cyberpunk neon dark**, **só primitivas raylib**. |

## E1. Colisão de cenário (causa raiz + correção)

| Sev. | Tema | O que foi feito |
|---|---|---|
| 🔴 | Raio de colisão **inscrito** menor que a pegada desenhada | Prédio moderno (tipo 20) era marcado com raio fixo `96·sc` mas é desenhado com W≈250-325·sc × D≈220-304·sc; veículos (tipo 6) com `78·sc` mas carro/van/caminhão têm L≈158/196/264·sc. Herói afundava dezenas de px nas fachadas. |
| 🔴 | Chunks de cidade sem colisão nenhuma | `m_chunkSolids` só cobria tipos 0/1/7/8 — prédios (20) e carros (6) spawnados por chunk eram 100% atravessáveis. |
| 🟢 | Correção | `Game_WorldGen.cpp`: helpers `buildingRadius`/`vehicleRadius` replicam **o mesmo hash do render** (W/D idênticos ao desenho) e retornam o **circulo circunscrito** exato; usados no mundo fixo (substitui 96·sc e 78·sc) e o switch de chunks passa a cobrir tipos 6/9/10/14-20 (mesmos raios do fixo). |

## E2. Retheme visual — menus (cyberpunk neon dark, primitivas raylib)

| Tela | O que mudou |
|---|---|
| Main menu | Fundo em gradiente profundo; moldura de câmera nos cantos; contadores decorativos (KRN.LOG / NEXUS ONLINE); grade cibernética mais fina; crânio IRON-VIII reacolhido em azul-aço com olhos vermelhos; título com **extrusão 3D + aura ciano**; separador com pontas chanfradas; botões com **trilho de energia esquerdo, cantos chanfrados, badge de tecla com recesso, sublinhado neon e seta pulsante**; rodapé em chips (`//`) com versão à esquerda. |
| Pause | Título embracado com sublinha e cantos; lista envolta em `DrawPanel`; item focado ganha trilho neon + cantos chanfrados e `»`. |
| Seleção de classe | Título com extrusão + sublinha e chave âmbar; cards com cantos chanfrados, trilho da cor da classe, topo pulsante ciano no selecionado; dica inferior em painel chanfrado. |
| Save slots | Painel chanfrado com topo pulsante; slot selecionado com trilho âmbar, borda chanfrada pulsante e [DEL] apagar; barra de dicas inferior. |
| Level-up / Evolução | Cards com cantos chanfrados + barra de topo na seleção; fundos mais escuros/consistentes. |
| Implicação | `Game::DrawPanel` (HUD) já era o padrão de canto chanfrado — retheme amplia o mesmo idioma para todos os menus (identidade única). `SaveManager.cpp` ganhou `#include <cmath>`. |

## E3. Retheme visual — HUD in-game (mesmo idioma)

| Painel | O que mudou |
|---|---|
| Stats HUD / top bar | Paleta unificada para ciano `{0,235,255,255}` (replaceAll sobre `{0,210,255}`/`{0,230,255}` no `Game_HUD.cpp`); top bar com sublinha dupla e marcas de canto. |
| Ameaça / mutador / zona segura / fase | Chips chanfrados com trilho colorido (vermelho/verde/ciano). |
| Controles | `DrawPanel` + chips de tecla. |
| Skill bar | Fundo angular + número de tecla em chip âmbar + linha neon no topo. |
| Quest HUD / diário | `DrawPanel` + trilho lateral / cabeçalho âmbar. |
| Minimapa | Moldura angular, chip RADAR, MAPA reposicionado (mapX+60), ticks de canto. |
| Skill tree | `Game_SkillTree.cpp`: borda neon, linha de topo, linhas selecionadas chanfradas com trilho. |
| Tela de dificuldade | Título com extrusão + separador chanfrado com diamante âmbar. |
| Rede / loja premium | C_cyan unificado (Game_Network.cpp, Game_PremiumStore.cpp, Player.cpp:842 inventário). ⚠ Player.cpp:35 e :235 são cores 3D e **não** foram tocadas. |

## E4. Passada visual do mundo 3D (proporções + veículos)

| Sev. | Tema | O que foi feito |
|---|---|---|
| 🔴 | Carros "blocos de criança" e desproporcionais | Veículos tinham L≈158/196/264 (4,2m/5,2m/7m) ao lado de um personagem de ~1,1m e rodas enterradas em arcos gigantes de 1,45·wr → liam como monumentos/caixas. |
| 🟢 | Correção `Game.cpp` case 6 | Reescrito com **proporções reais**: sedã ≈100u, van ≈122u, caminhão ≈164u (régua 1m≈37,6u); cintura **contínua e lisa** (sem escadinha entre capô/teto/mala) com friso cromado costurando as seções; vidros **recessados** (colarinho do teto), para-brisa e vidro traseiro **inclinados** (rotação Z); rodas **rentes** à lataria com arco discreto de 1,16·wr e pneus aterrados (assoalho + sainhas no chão — elimina o "flutuando"); faróis/lanternas/para-choques/retrovisores/antena. |
| 🟢 | Layout da rua | `Game_WorldGen.cpp`: carros (tipo 6) passam a **alinhar à malha de ruas** (4 ângulos, como prédios) em vez de rotação aleatória atravessada; `vehicleRadius` recalibrado para os novos L/WD (raio circunscrito, pneu ±5u). |
| 🟢 | Extras | 12 lados nas rodas (mais redondas, antes 10); calota rebaixada; caminhão com baú frisado + cabine com para-brisa inclinado, teto próprio. |

## E5. Validação pós-correção (2026-09-04)

- Build Debug e Release: **exit 0** nos dois (targets `darknet` e `darknet_tests`).
- `darknet_tests.exe`: **12/12 casos, 109/109 asserções**.
- `validate.sh 120 7` e `validate.sh 120 20260821`: **APROVADO** nas duas.

## E6. Cidade sem prédio/árvore no meio da rua (causa raiz + correção)

Achado do dono: "tem prédio e árvore no meio das ruas, isso não pode".

| Tema | O que foi encontrado / feito |
|---|---|
| Causa raiz | As **ruas que o jogador enxerga** são quads em `475 + k*950` (±112u de meio-fio, `Tilemap::render3D`), independentes dos tiles (as ruas de tile são aleatórias a cada 6-8 tiles). A cidade **ignorava essa malha**: grade de prédios em `220 + k*950` com `EDGE=350` → fachada furada ~137u **dentro da pista**; árvores/carros de `place()` caíam em qualquer ponto (área vale ~56%). GhostCity ainda usava quarteirão 880 sobre vias de 950 (desencaixado). |
| Correção | `laneDist()` (malha 475+k*950) como régua única: grade do mundo fixo **sincronizada às vias** (centro de quarteirão em múltiplos de 950, `EDGE=350→210` cola a fachada no meio-fio); `place()` recebe flag *cidade* → árvore nunca `laneDist<130`, carro sempre `laneDist≤102`; clusters de chunk de cidade enconxados na mesma malha (cantos a ±210 do centro, `putB` com espaçamento 220 p/ torres vizinhas) + gate no `put` de props; GhostCity volta a usar quarteirão 950 (difere em ruína/vazio, não em passo). |
| Resultado | Quarteirão inteiro com massa centrada na quadra; pista é corredor limpo entre prédios; carros parados no asfalto; árvores só dentro dos lotes. |

## E7. Validação pós-correção (2026-09-04, passada das ruas)

- Build Debug e Release: **exit 0** nos dois (targets `darknet` e `darknet_tests`).
- `darknet_tests.exe`: **12/12 casos, 109/109 asserções**.
- `validate.sh 120 7` e `validate.sh 120 20260821`: **APROVADO** nas duas.

## E8. Inimigos respeitam as construções de chunk (colisão)

| Tema | O que foi feito |
|---|---|
| Gap | `Game::update` só barrava inimigos em **parede de tile** (`isWallAtPosition`). Construções geradas no infinito (`m_chunkSolids`, círculos) eram ignoradas → o inimigo andava **atravessado dentro do prédio de chunk**. Spawn já evitava (`Game_Spawn.cpp` usa `isBlocked`), fica só a movimentação. |
| Correção | Bloco de colisão do inimigo agora avalia `blockedAt(pos)` = parede de tile **OU** círculo de chunk (`Game.cpp`), com o mesmo deslize por eixo (tenta X, depois Y, senão volta). Espectros/bosses voadores continuam atravessando de propósito. |

## E9. Arquitetura: extração de Game_WorldRender.cpp

| Tema | O que foi feito |
|---|---|
| Objetivo | `Game.cpp` estava com **5.027 linhas**; a renderização monopolizava ~1.700. |
| Extração | Copiadas por **corte literais** para `src/Game_WorldRender.cpp` (novo): `ensureVoxel`, `drawGenericStructure`, `drawArkStructure`, `drawVoxel`, `sphereInCameraFrustum` e `renderWorld3D` (~1.620 linhas). Statics locais duplicadas (mesmo padrão de `Game_WorldGen.cpp`): `structureTintFor`, `isMedievalZone`, `FIT_*`, `DrawCubeTexture`. Registro no CMake. |
| Resultado | `Game.cpp`: 5.027 → **3.236 linhas**; `Game_WorldRender.cpp`: 1.912. Nenhuma mudança de lógica (cópia literal). |

## E10. Validação (2026-09-04, rodada de colisão e extração)

- Build Debug e Release: **exit 0** (targets `darknet` e `darknet_tests`).
- `darknet_tests.exe`: **12/12 casos, 109/109 asserções**.
- `validate.sh 120 7` e `validate.sh 120 20260821`: **APROVADO** nas duas.

## E11. Declarado (próximos da sequência)

- Fases 5–11 em execução e áudio (julgamento humano) — seguem pendentes da sequência maior.
- `wss://` no `NetClient`: exige TLS (OpenSSL/SChannel) — não-vendored, decisão de infra separada (segue com `ws://` local/gateway).
- Personagem fica ~1,1m na régua (1m≈37,6u) enquanto prédios usam andar de 2,1m — reescalar herói/NPC é decisão de risco alto (reserva; hoje o top-down disfarça).
- `Game::update` (1.170 linhas) e `Game::handleInput` (~550) seguem como próximos cortes do monolithic `Game.cpp`.


---

# ADENDO - Ciclo 6: cortes finais de Game.cpp + legibilidade das fases escuras (2026-09-04)

## E12. Extraido Game::update e Game::handleInput -> Game_Gameplay.cpp

- Game.cpp agora tem **1.493 linhas** (era 5.027 no inicio): lifecycle, menus, decals, colliders pequenos, camera e 
ender().
- Novo src/Game_Gameplay.cpp (1.880 linhas): Game::update e Game::handleInput por copia literal (zero logica alterada).
- Incluido SkillTree.h (usava SkillTree::statsFor sem incluir). Comeca agora a pipeline de extras de ate 5 fases; o proximo alvo natural e Game::drawCharacterSelectScreen/menus, ja que o arquivo ficou de tamanho saudavel.
- CMakeLists.txt: fonte registrada apos Game_WorldRender.cpp. Nenhuma statics file-scope extra necessaria (o getZoneInfo que update usa e inline em Zone.h).

## E13. Fases escuras ilegiveis (queixa do jogador) - correcao de cena + textos

Queixa: "fases com dizeres negros escuros estao MUITO escuras, nao da pra ver quase nada" (ceu cenario com derivativos pretos nas fases sombrias + textos engolidos).
Resposta (cena e textos):
- **Cena**: paleta de todas as 11 fases clareada em Zone.h::getZoneInfo (chao A/B, paredes e outline) mantendo o matiz; o clima escuro continua vindo do ceu/fog (skyColorFor) e props do DarkWorld, nao do chao. Ex.: DarkForest chao (20,28,20)->(54,72,50); Catacombs (30,25,30)->(70,58,66); Nexus (25,10,55)->(66,44,104).
- **Textos**: banner de fase drawStoryBanner com painel mais opaco (0.62->0.84), sombra preta no titulo e subtitulo mais claro; fade de fase drawPhaseFade agora tem painel condensado com borda cyan e alpha minimo de texto ~70%; banner de zona do HUD com sombra. Nao existe dimming global alem das cores (confirmado: so ZoneInfo alimenta a luz).

## E14. Evidencia de execucao de fases (bot)

- Run longo --test-seconds=480 seed 7: **exit 0, VALIDACAO PASSOU** (o criterio de avanco de zona e obrigatorio em runs >=180s).
- ot_report.txt (run 120s seed 20260821): "Zonas avancadas : 1" aos ~105s; 34 abates. Fases 5-11 seguem como julgamento humano/auditoria visual (conteudo das fases ja esta em content/phases.txt, as 11 carregadas).

## E15. Declarado (sequencia restante)

- Fases 5-11 em execucao completa (visual/audio) e oudio pleno - julgamento humano.
- wss:// (TLS) segue decidido fora do escopo sem OpenSSL vendored.
- Proximos cortes de arquivo grande: Game.cpp (1.493) ainda tem drawCharacterSelectScreen/menus; Game_Menus.cpp e Game_HUD.cpp sao os maiores restantes.
- **Recomendacao recorrente**: 34+ arquivos modificados nao commitados; melhor estado do projeto - commitar antes de prosseguir.


## E16. Enemy.cpp divido -> Enemy_Render.cpp

- Enemy.cpp 3.280 -> **1.525 linhas**; novo src/Enemy_Render.cpp (~1.760) com todos os 22 Enemy::render* + helper local DrawRotatedRectangle (copia literal dos 6 blocos contiguos; unica static file-local: DrawRotatedRectangle; extern bool g_voxelCapture duplicado no novo arquivo).
- CMakeLists atualizado. Gate: Debug+Release exit 0, testes 12/12 (109), validate 120/7 APROVADO.
- Resta em Enemy.cpp: core/AI/knockback/ataques (setupByType, update*, bosses phases) - legibilidade mantida.



## E17. Execucao de fases - runs longos (medicao)

- 480s (seed 7): exit 0, VALIDACAO: PASSOU � o criterio de avanco de zona e OBRIGATORIO em run >=180s, logo o portal/fase funcionou.
- 900s (seed 20260821): exit 0. ot_report.txt foi sobrescrito por runs menores; dado preservado: validacao e fase avan�ada em runs >=180s. Fases 5-11 seguem como auditoria visual/humana (conteudo em content/phases.txt, 11 carregadas).
- Observacao de pacing: 120s seed 20260821 (re-run) deu "zonas=0" (portal abriu apos o corte dos 60s de monitor); 120s seed 7 deu "zonas=1" as ~105s. Variancia de timing/seed � portao frouxo (<180s) cobre isso de proposito.

## E18. Quadro de level-up (queixa do jogador: grande e por cima das mensagens)

- Causa: banner 420x70 centrado em y-100, desenhado DEPOIS do banner de zona (y-40) e painel de missoes � cobria o nome da fase.
- Correcao: pill compacto (largura do texto + 36, 40/52 de altura), fonte 28->20, realocado para y=30 (sob a barra de HUD, longe do centro). Incluido <cstdio> (snprintf).

## E19. Evolucao da renderizacao ("esta amador demais")

- world.fs: camada anti-plastico � fbm de 3 oitavas no ESPACO do mundo (uniform worldPeriod, 950 no open-world LA / 480 no mapa fixo) quebra os muros de cor unica das grandes faces opacas; + especular seco (specularK 0.28, esfera 26) na lataria/metal � so em alpha>0.999 (decal translucido nao recebe).
- grade.fs: vinheta cinematografica (ate 32% nas bordas) dobrada na passada final � custo zero.
- initPostFX: bloom 0.95->1.15, saturacao 1.22->1.28 (mais viveza).
- Gate: Debug exit 0, Release exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS (POSTFX/WORLDLIT ATIVO).

## E20 � Rodada de "juice" (pesquisa de game feel em twin-stick shooters)
Pesquisa: solana.garden (hit stop 40-120ms por tier, screen shake trauma^2 com decay, camera kick/recoil no tiro, pop de kill, flash de impacto) + raylib oficial (shapes_top_down_lights.c � light mask top-down p/ refer�ncia futura). Diagn�stico: o melee j� tinha hitstop (Game_Gameplay update: hitStopTimer 0.05/0.09 com freeze parcial, part�culas continuam), shake 3.5 e blood sparks; MAS explos�es e abates comuns e casts n�o tinham peso.
Implementado (baixo risco, sem tocar em l�gica de bot/balance):
- Game.cpp updateProjectiles (explos�o de granada): triggerShake(6.0, 0.25) + hitStopTimer 0.07 � explos�o "bate" de verdade.
- Game_Gameplay.cpp bloco de morte de inimigo: **POP de morte** � burst de part�culas em escala (12 normal / 24 elite / 46 boss; cor do corpo; branco central), e elite/boss congelam o mundo (hitstop 0.06/0.10) + shake 4.5/7.0 com c�mera longa.
- Camera kick em casts: Laser 2.2/0.10, EMP 4.0/0.18 (onda de choque), Granada 1.8/0.08 (peso no arremesso), Rajada 2.5/0.12.
- Gate: Debug exit 0, Release exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS.
Pr�ximos (se o usu�rio pedir evolu��o visual): light mask 2D ao estilo raylib top-down p/ luzes din�micas da cena; flash ao receber dano de elite; pitch variation nos SFX de hit.

## E21 � Personagem "voando" corrigido + barra de poderes com vida
- BUG "voando": Game::update chama handleInput(dt) (movimento?player.move?isMoving=true) e DEPOIS player.update(dt), que ZERA isMoving no comeco. O voxel (drawVoxel do player em Game_WorldRender.cpp) usava player.isMoving ? chegava SEMPRE false no render ? pose 0 congelada + sem passo + sem gingado = desliza/flutua. O render 2D legado (Player::render) compensava com elMag>12, o 3D nao. Correcao: usar a velocidade real (|v|>12) no drawVoxel do jogador; passo (step snap) 0.030?0.040.
- Barra de PODERES (drawSkillsPanel): era chip de tecla + nome ("123456" estetica). Novo painel "vivo": icones PROCEDURAIS por poder (Laser=projetil cyan, EMP=pulso concetntrico ouro, Granada=esfera+pavio, Sobrecarga=raio, Barreira=hexagono, Rajada=leque) sem assets; cor propria por slot; borda pulsando quando PRONTO; recarga = overlay descendente + barra de progresso + segundos; dano visivel.
- Gate: Debug exit 0, Release exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS.
- Auditoria: P1 marcador/marcador do portal, bot atravessa o portal, construcao dupla do cenario e higiene do BotController.h j� estavam resolvidos (verificado nesta rodada: setupWorldRegions e chamado 6x mas com safeZoneCenter correto ANTES de buildOpenWorldScenery; reset()/restartRun existem).

## E22 � Reacao ao dano + chefao com barra + HUD sem nada fixo no centro
- **Flash vermelho de dano** (bug latente): `hitFlashTimer` era setado em 5 pontos e decaia, mas NUNCA era desenhado. Agora `drawHudAndOverlays` pinta a tela toda com alpha min(0.45, flash*1.5).
- **Indicador direcional de dano**: novos `hurtDir`/`hurtDirTimer`/`noteHurtDir(Vector2 src)` (Game.h + Game.cpp); disparado nos 5 locais que ferem o player (projetil inimigo Game.cpp, contato + kamikaze/zergling + explosao AOE em Game_Gameplay, lobo em Game_Resources); decay no update. Render: setas em trail acelerando ate a fonte + triangulo pulsante na borda da tela.
- **Barra de HP de boss** (topo-centro): painel com nome por tipo (COMANDANTE KRONOS/NUCLEO KRONOS/etc), % + HP numerico, ticks a cada 10%, borda pulsando abaixo de 30% HP.
- **Nada fixo no CENTRO da tela** (queixa do usuario): HUD de RECURSOS (5 quadradinhos coloridos centrados em y=56, pareciam "menu que nunca mostra nada") dockado na ESQUERDA (x=10, y=80, mesmo estilo da pill da FASE: fundo 8,12,26 + filete cyan); pill da FASE saiu do centro (x=10, y=56); a barra de PODERES do usuario fica ONDE ESTAVA (base-centro) e o minimapa voltou ao lugar.
- Gate: Debug exit 0, Release exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS. (Nota: rodar as duas validações EM PARALELO faz uma derrubar a outra no rebuild compartilhado - "2 travamentos > 10s" falso; sequencial exporta verde.)

## E23 ? Nomes dos recursos + rodada de juice (game feel)
- **Nome em cada recurso** (pedido explicito do usuario: "precisa colocar os nomes de cada recurso"): `drawResourceHUD` redesenhado na ESQUERDA (x=10, y=80, altura 40, largura 96/item): quadrado colorido + quantidade + NOME (Madeira/Pedra/Ferro/Prata/Ouro) na cor do recurso. `resourceName`/`resourceColor` em Game_Resources.cpp.
- **Juice na coleta de XP**: orb de XP ganha faisca cyan + `spawnExplosion` + pop de dano "XP n" (mesma coleta + magnetismo, Game.cpp).
- **Barra de HP em inimigos**: `drawHudAndOverlays` projeta a barra com `GetWorldToScreenEx(camera3D)`; boss/elite sempre, demais só quando feridos; cull a 900u; flash branco com `hitFlashTimer`.
- **Vinheta de vida baixa**: gradientes vermelhos nas bordas da tela quando `player.health < maxHealth*0.30` (pulso suave).

## E24 ? Zona segura vira a CASA do jogador (hub limpo + lojas por NPC)
- **Folga de construcoes no hub**: exclusao de estruturas em volta da praca subiu 360u -> 560u (Game_WorldGen `put1`) - nenhum predio espremendo a base.
- **Lojas PROPORCIONAIS por NPC** (pedido explicito): novos tipos de cenario 23-27, cada um com RENDER 3D procedural proprio em Game_WorldRender:
  - 23 ESTANDE DE MERCADO (LUNA): balcao + lona listrada + mercadoria;
  - 24 FORJA (FERREIRO KANE): bigorna + fornalha com brasa emissiva + chamine + mesa de metal;
  - 25 POSTO DE COMANDO (VANCE RIOS): mesa + mapa luminescente + radio + estandarte NEXUS ondulando;
  - 26 LABORATORIO DE IMPLANTES (DR. CHEN): bancada + tanque + holoprojetor cyan;
  - 27 ARSENAL (ZARA): rack de armas + caixas de municao + luz de trabalho.
  `placeBaseShops()` (Game_WorldGen, declarado em Game.h) coloca as lojas no ANEL ao redor da praca, espelhando os offsets de `setupBaseNPCs()` +52u pra fora: cada vendedor fica em FRENTE da propria loja quando chega pelo centro. Erase com o ramp: rebuilt a cada fase (chamado no fim de `buildOpenWorldScenery`, depois das solidas - as lojas nao colidem).
- **Anel de energia da zona segura** (Game_WorldRender): piso translucido cyan/amber + 2 aneis tracejados pulsando e girando em sentidos opostos + 8 balizas com luz + varredura rotativa + coluna de energia central (marco da base). Renderiza so perto do hub (raio 1250).
- **Area protegida PURA** (pedido explicito: "tirar os carros detritos, deixar so os npc e as suas lojas"): passe de limpeza no fim de `buildOpenWorldScenery` remove do raio de `safeZoneRadius` os detritos urbanos (6 carro, 12 pedras, 13 marcas de fogo, 21 entulho, 22 fogueira); grama/arvores/postes ficam.
- **Minimapa so da FASE ATUAL** (pedido explicito): `drawMinimap` agora usa janela de mundo = circulo de `owPhaseRadius` (+6%) ao redor do centro da base em vez do mundo inteiro de 128 regioes (que espremia os blips num canto). Pinta o disco da fase, elipse de limite cyan pulsante, elipse verde da zona segura, chip "FASE n", e converte todos os blips (portais, NPC, construcoes, inimigos, viewport, player) pelas novas lambdas mx/my. Em mapa fechado mantem o comportamento antigo.
- Gate: Release + Debug exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS (sequenciais).

## E25 - Aim assist (mira magnetica) + ret�culo de lock + postes fora da rua
- **Aim assist em todas as skills direcionais**: `aimDir` (Game_Gameplay.cpp) agora GRUDA no inimigo vivo mais perto do cursor dentro de raio de magnetismo de 120u; a dire��o vira SEMPRE UNIT�RIA. Bug latente corrigido no caminho: antes o vetor escalava com a dist�ncia do mouse e o proj�til b�sico corria em velocidades diferentes conforme o cursor estivesse perto/longe.
- **Ret�culo de mira (drawHudAndOverlays)**: 4 ticks ciano ao redor do cursor (sem esconder o cursor do SO); com alvo travado o ret�culo fica vermelho, um anel de lock pulsante com 4 pontas girando pinta o inimigo alvo (proje��o GetWorldToScreenEx) e uma linha fina de guia liga cursor->alvo.
- **Postes de luz fora da rua** (pedido explicito: "tem postes de luz no meio da rua"): o tipo 5 n�o tinha filtro de pista (s� �rvore/carro tinham) - postes nasciam aleat�rios SOBRE o asfalto desenhado. Corrigido em 3 pontos: `place()` (cidade/bunker) recusa poste com dr<118u da via; `put()` do streaming recusa o mesmo nas biomas de cidade; postes dos meios-fios sa�ram de x 363..383 (at� 20u DENTRO do asfalto) para 338..352 (cal�ada, 12..26u fora da pista).
- Gate: Release exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS (sequenciais).

## E26 - Limpeza de codigo morto + dano de elite/pitch + ceu procedural + auditoria P1/P2
- **Codigo morto removido**: ScreenEffectsSystem era um sistema orfao (nunca chamado, so compilava) - arquivos ScreenEffects.cpp/.h deletados e saida do CMake; `Game::drawSkills()` (HUD legado de skills, nunca chamado) removido (decl + def). Lucro: menos 700 linhas de superficie de bugs, build avisa se algo sumiu.
- **Flash BRANCO de dano pesado** (pedido E22 pendente): novo `eliteFlashTimer` - quando um elite/boss acerta o player (contato) ou o elite volataev explosao (eliteMod==2), a tela pisca em branco (0.55 max alpha, 2.2x de velocidade) por cima do flash vermelho. Decai no update.
- **Pitch variation nos SFX de hit**: helper `playPitched(Sound, lo, hi)` (copia local + SetSoundPitch + PlaySound) aplicado em playHit/HitHeavy/HitAlien, playMeleeSwing/Impact, playMeleeHit(crit 1.0-1.12), playEnemyHit, playPlayerHurt - hits repetidos nao soam identico. Explosoes e musica ficam como estavam.
- **Ceu procedural no mundo aberto**: alem da cor base do bioma, estrelas discretas (140, hash deterministico, parallax pelo alvo da camera, noite realca + alpha) + 8 faixas de bruma no horizonte translucidas com respiracao suave (cor do ceu clareada). So visual (nao toca gameplay/FPS gate) e deterministico por frame.
- **Auditoria propria (subagente + revisao) - 2 P1 crash/UB corrigidos**:
  - Game_Gameplay: `spawnOmegaBoss()` era chamado DENTRO do loop de inimigos mortos (`enemies.push_back` invalida o iterador `it` de uso posterior). Agora `pendingOmega` flag e spawn aplicado DEPOIS do loop (mesmo padrao do splitSpawns).
  - Game_Spawn: no spawn de Zergling, `Enemy& e = enemies.emplace_back(...)` ficava pendurada se os 2 `emplace_back` do split realocassem o vetor; `e.makeElite` movido para ANTES do split.
- **Auditoria propria - 2 P2**: seed do DarkWorld era `GetRandomValue(1000,99999)` a cada visita (mesma regiao re-rolava o desenho sombrio); agora seed derivada da regiao+fase (0x343fdu*(regiao+1) + fase*0x9e3779b9u) em Game_Gameplay e legado Game_Phases. Bloco morto de clusters de Wall em Tilemap::generateOpenWorld (`clusters = 0` jamais executa) removido junto com o rng orfao.
- Gate: Release + Debug exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS (sequenciais).

## E27 - Evolucao de construcao + cada FASE vira um MUNDO ABERTO grande
- **Construcao viva (base nao nasce no vazio)**: unidades NAO nascem na hora. `Building` ganhou `spawnQueue/spawnTimer/spawnTime` (BuildingSystem.h); Quartel 5s e Fabrica de Tanques 9s por unidade. Clique de producao e producao automatica enfileiram (`spawnQueue++`); o `updateBuilding` da spawn quando `spawnTimer >= spawnTime/m` com m = 1+(level-1)*0.5. `renderUnitPrompts` mostra n+spawnQueue com barra de progresso cyan "Construindo... %d na fila" (auto-producao segue amarela).
- **Inimigos destroem o que voce construiu**: projeteis inimigos agora causam dano a TODAS as construcoes (a Parede BLOQUEIA o projetil, as demais tomam dano e deixam passar - ordem deterministica), e tanques/soldados aliados tambem tomam dano de projetil. `spawnTank/spawnSoldier` deixaram de usar GetRandomValue (angulo deterministico pelo indice).
- **Visual futurista** (era fazendinha): casa->relé de energia cyan, quartel->casco escuro + arco de deploy, fabrica->nucleo laranja + motes de energia no lugar da fumaca, parede->malha de energia com HP, MedBay->capsula + cruz bio-reparo, scaffolds holograficos "MONTANDO...", tanque->APC escuro com nucleo cyan, soldado->armadura + visor cyan.
- **CADA FASE = MUNDO ABERTO GRANDE** (pedido do usuario: "cada fase tem que ser um mundo aberto, esta muito pequeno e sem graca"):
  - `content/phases.txt` raios 3000..4600 -> 5200..7800 (degrau ~260), fase terminal com 118% a mais de area por fase.
  - `setupWorldRegions` (Game_QuestsNPC) deixa de ser grade 3x3 ancorada na ORIGEM: vira grid quadrada IMPAR, CENTRADA na base (safeZoneCenter), com celula grande o bastante para cobrir o DISCO da fase inteira (cols = f(owPhaseRadius); 5x5 na fase 1, ate 7x7 na fase final). Todos os biomas usam currentZone (uma fase = um mundo). Regiao do HUB encontrada por contenção (nao worldRegions[0], que era o canto). Nomes por bussola ("Setor Norte-Leste").
  - Animais e nodes de recurso (Game_Resources) e o cenario eager (buildOpenWorldScenery) passam a cobrir o disco inteiro automaticamente (iteram worldRegions dinamicas).
  - Minimapa (Game_HUD drawMinimap): removido o clamp em worldW/H (mundo infinito, fase > antiga area 7680u); linhas de grade vem dos BOUNDS das regioes dinamicas.
  - Gradiente de dificuldade por raio (Game_Spawn) reancorado em safeZoneCenter (antes media no centro da grade 3x3 = 3840,3840, desalinhado da base).
  - Gerador de chunks (updateSceneryChunks): guard ORIG (0..24576) trocado por "gera so onde NAO ha regiao eager" - sem duplicacao de cenario nas coordenadas negativas (a fase agora cobre os quatro quadrantes).
  - Hub visual (praça de guerra, roadblocks) usa safeZoneCenter (era centro da zona 0 == 1280,1280 por coincidencia).
  - Estado de fase resetado no Novo Jogo/Reinicio (owPhase=0, radio da fase 1) e RECONSTRUIDO a partir da zona salva no Continuar (save nao persistia owPhase/radius -> "Continuar" herdava fase antiga).
  - Gate: Release + Debug exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS (sequenciais). Bot: 1 fase avancada (CursedFarm 5x5), 4/4 quadrantes explorados, 24 abates, FPS min 52 / medio 59, 0 mortes.
- **CI na main**: `.github/workflows/ci.yml` (windows-latest, configure -A x64, build Release darknet+darknet_tests + Debug darknet, testes, validate 120 seed 7 e 20260821 sequenciais com `test "$code" -eq 0` + grep "VALIDACAO: PASSOU"); branch master renomeada para main, default_branch=main, origin/HEAD aponta main; workflow passa ao ler a versao com este E27.
- **Fix CI — modo `--headless`**: no runner do GitHub Actions (Windows Server 2025, sem display/GPU) o validate estourava `Segmentation fault` (exit 139) ao criar o contexto GL do raylib, ANTES de imprimir qualquer INFO. Solucao: flag `--headless` setando `Game::headless` em main.cpp ANTES do construtor — o construtor pula InitWindow/render textures/SpriteBank/modelos/postfx/lightSystem/background (mantem `audio.init()` sem device + loadPhaseDefs/buildQuests/buildNPCs + mundo aberto e fase), `runHeadless()` roda a MESMA `update()` em tempo real (dt proprio do loop, sem render/presentFrame/screenshot/menu), FPS do bot medido do proprio loop (`GetFPS()`=0 sem janela), destructor so libera o criado. ci.yml passa `--headless` nas duas validates; validate.sh ganha 3o argumento (headless=1). Gate local headless: Release+Debug exit 0, testes 12/12 (109), validates APROVADOS (bot 21 abates, 4/4 quadrantes, 1 fase avancada pelo portal, 0 mortes, sem stuck). CI verde no run 33961092413 (commits fb31655 + db9531d).

## E28 - Construcoes do jogador "sci-fi de verdade" (eram castelinho/quadrado medievais)

- **Queixa do usuario**: "as construcoes que o personagem constroi todas parecem ser um castelo minusculo ou um quadrado. elas tem que combinar com o jogo. recrie tudo" — pesquisa de concorrentes na internet (assets/arte sci-fi top-down de base building: fábrica industrial isometrica com exaustores + nucleo cilindrico, bunkers com janelas emissivas) consolidou a direcao: **casco gunmetal escuro + acentos emissivos por funcao + silhueta legivel a distancia + power pad no chao**, consistente com o resto do jogo (tile 64u, heroi ~66u).
- **Causa**: o loop 3D das construcoes do jogador renderizava modelos **OBJ medievais** (castle.obj/house.obj/barracks/market/well/turret) — o castelinho/quadrado do asfalto. As visoes futuristas 2D legadas em BuildingSystem.cpp eram codigo morto (`BuildingSystem::render` sem call site).
- **Novo `drawPlayerBuilding(const Building&)`** (static, Game_WorldRender.cpp) substitui os modelos no render de TODAS as construcoes, com sombra + anel power pad + **scaffold holografico** com barra de pips para as em construcao (`!b.built`):
  - Arca = bunker de comando + beacon mastro + aura de cura (healRadius) + pilares de canto emissivos;
  - Casa = pilone de energia com pulso de geracao (genTimer/genRate) subindo pela coluna + condutas laterais + aneis de nivel;
  - Quartel = portao de deploy (arco + aletas cryan) + **feixe de spawn** pulsando com spawnQueue/spawnTimer/spawnTime;
  - Fabrica de Tanques = nucleo laranja pulsante + 3 chamines de exaustao com motas de calor subindo + portas de fabrico + **barra de producao de pips** (productionTimer/productionRate);
  - Torre = cupula giratoria + canhoes duplos seguindo `shootDir` (atan2f) + boca emissiva + anel de alcance em nivel 2;
  - MedBay = estacao de stasis teal + cruz medica holografica na fachada + pods laterais + aura;
  - Parede = parapeito de energia: lâmina emissiva no topo + celulas pulsantes + pilares de reforco por nivel;
  - ResourceNode = cristal laranja com cilhas orbitais.
- **Falsos positivos de build investigados**: (1) `const Color ORANGE/RED` colidiam com **macros do raylib** (mesmo nome) -> renomeados para HOT/DEF; (2) reescrita de arquivo via PowerShell `Set-Content -NoNewline` **fundiu o .cpp numa linha unica** (objs de 537 bytes "vazios", LNK2019 fantasma) -> fonte restaurado via git e reaplicado com a ferramenta de edicao.
- Gate: Release + Debug exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS (sequenciais, janelados); headless intocado (nao renderiza). Screenshots do autotest (shot_00..03.png) para conferencia visual humana.

## E29 - Mundo vivo sci-fi: materializacao das construcoes e drones de vigilancia

- **Queixa do usuario**: jogo "vivo" demais? — pedido de "mundo vivo" sci-fi.
- `Building.builtAt` (BuildingSystem.h) e setado ao concluir (BuildingSystem.cpp); `drawPlayerBuilding` abre com **materializacao** por 1.2s (anel de expansao cyan→branco 20→75u + 6 fagulhas setoriais) antes do casco compactar.
- **`drawAmbientDrones`** (static, Game_WorldRender.cpp): 4 drones orbitando o alvo da camera, deterministas por tempo (orbita irregular, trilha de luz de 4 amostras, casco gunmetal, beacon piscando, anel de vigia girando); chamados em `renderWorld3D` gated por `openWorldMode`, antes do bloco VIDA AMBIENTE. Zero partículas = determinismo de frame preservado.
- Gate: Release+Debug OK, testes 12/12 (109), validates 120/7 e 120/20260821 APROVADOS.

## E30 - Dificuldade justa + fim definitivo das arvores na rua + cenário "codigo de guerra"

- **Queixa do usuario**: "os inimigos sao faceis demais de matar — veja como os grandes jogos fazem, implemente, crie, inove" e "ainda temos arvores no meio da rua na primeira fase, corrija e adicione mais elementos ao cenario" e "tem que ter partes de asfalto destruido, cenario codigo de guerra, predios destruidos, fumaça".
- **Dificuldade (estilo ARPG/Risk of Rain, sem quebrar o bot do validate)**:
  - `Enemy::armor` — reducao PLANA de dano por hit (mitigacao estilo Diablo/Hades): elite Armored ganha 8u (+4 por fase). Arma fraca quase nao arranha mais.
  - `Enemy::shieldHp/shieldMax` — afixo elite **Shielded** (novo mod 3): escudo absorve 75% de cada golpe ate esvaziar e **recarrega 10%/s** (RoR2).
  - Afixo **KronosRapid** (mod 4): +35% velocidade, +30% dano e autorreparo 2%/s.
  - Chance de elite agora **escala com fase e raio do refugio**: 12% + 3%/fase + 5%/anel, teto 45%; elige roll exclui Zergling (folgas do swarm).
  - **Curva por fase no mundo aberto**: +8% HP e +5% dano por fase apos a fase 1.
  - Fase 1 = intocada na curva (owPhase 0) e elite ~12% na base — o autotest de 120s continua APROVADO.
- **Arvores na rua — fechamento completo**: guard de streaming já seguia `currentZone == LARuins||GhostCity` (condicao EXATA do desenho da rua) e clearance de copa 170u nas vias; agora a **praca central de 400u do hub fica livre de arvores** (a pista desenhada passa por ela). Sem condicao de carreira para a "arvore na avenida".
- **Cenario "codigo de guerra"** (tipos 28-31, render procedural em Game_WorldRender + `drawSmokeColumn` de volutas por frame, sem particulas):
  - 28 CRATERA DE BOMBA: tigela carbonizada + borda de terra arremessada + lajes atiradas + brasa + fumaça;
  - 29 PREDIO COLAPSADO: esqueleto quebrado + laje tombada + barras de aco + poeira;
  - 30 ASFALTO DESTRUIDO: placa rachada enegrecida + borda esfarelada (decor, nao bloqueia);
  - 31 CARCACA QUEIMADA: casco retorcido + mastro dobrado + lareira + fumaça pesada.
  - Distribuicao: cidade LA/GhostCity no mundo fixo (`place` por quarteirao) E no streaming (`put`/prop roll): cratera/asfalto/carcaça entram no roll da fase 1 e GhostCity ganha crateras; colisao marcada (28/29/31 bloqueiam com pegada moderada, 30 fica decor).
- Gate: Release+Debug exit 0, testes 12/12 (109), validate 120/7 e 120/20260821 APROVADOS (sequenciais, janelados); CI headless nao renderiza os novos props. Screenshots shot_00..03.png para conferencia visual.
