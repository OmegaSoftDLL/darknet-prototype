# Auditoria Completa — Darknet Prototype

**Data:** 2026-09-06  
**Auditor:** Revisão multi-domínio com especialistas sêniores (arquitetura, gameplay, render, netcode, save, mundo, áudio/input)  
**Commit base:** `58d4019` (`main`)  
**Escopo:** Código-fonte em `src/`, shaders em `resources/shaders/`, backend em `server/`, documentação de design (`GAME_DESIGN.md`, `ROADMAP.md`, `DARKNET_STORY.md`).  

---

## 1. Resumo Executivo

O `darknet-prototype` é um ARPG 2.5D/3D isométrico funcional, com ~75 arquivos-fonte e ~35.000 linhas de C++17 sobre raylib 5.5. O jogo **compila e passa nos portões de validação headless**, mas apresenta **dívida técnica massiva** concentrada na classe `Game`, **sistemas desconectados** (tutorial, conquistas), **save fragilíssimo**, **problemas de determinismo** e **riscos de segurança sérios** para multiplayer/produção.

### Balanço de severidade consolidado

| Domínio | P0 | P1 | P2 | P3 |
|---|---:|---:|---:|---:|
| Arquitetura & Código | 5 | 6 | 5 | 3 |
| Gameplay & Balanceamento | 2 | 6 | 8 | 6 |
| Renderização & Performance | 2 | 8 | 9 | 5 |
| Networking & Segurança | 2 | 5 | 5 | 4 |
| Save & Serialização | 5 | 9 | 9 | 6 |
| Mundo, Colisão & Física | 4 | 8 | 8 | 3 |
| Áudio, Input & Auxiliares | 9 | 19 | 24 | 5 |
| **Total** | **29** | **61** | **68** | **32** |

*(P0 = crítico, quebra mecânica/segurança; P1 = grave; P2 = moderado; P3 = leve/polimento)*

### Principais conclusões

1. **A arquitetura é o maior risco de longo prazo.** `Game.h` declara ~160 membros e inclui 30 headers. Qualquer mudança tem alto risco de regressão.
2. **Vários sistemas documentados no GDD não funcionam.** Tutorial, conquistas, árvore de habilidades tier 3, recompensas de level-up e quests de NPC estão quebrados ou desconectados.
3. **O save não é confiável.** Metadados zerados, equipamentos perdidos, estado residual entre partidas, sem checksum/backup.
4. **Multiplayer não está pronto para produção.** WebSocket sem TLS, servidor não autoritativo, data races no cliente, autenticação stub.
5. **Performance gráfica tem gargalos claros.** Geração de modelos voxel nunca usados, milhares de state changes, culling insuficiente.
6. **O jogo passa nos testes automatizados, mas a cobertura é baixa.** 26 testes cobrem Crafting, Enemy, SaveManager, Player, Projectile, Tilemap — nada da classe `Game`.

---

## 2. Metodologia

- **Análise estática manual** de `src/` com foco em arquitetura, gameplay, render, netcode, save, mundo e sistemas auxiliares.
- **Subagentes especializados** revisaram domínios específicos em paralelo.
- **Validação empírica:**
  - `validate.sh 60 20260905 1` — APROVADO
  - `validate.sh 120 7 1` — APROVADO
  - `validate.sh 120 20260821 1` — APROVADO
  - `validate.sh 300 7 1` — APROVADO
  - `darknet_tests.exe` — 26/26 casos, 148/148 asserções passando
- **Grep automatizado** por `TODO/FIXME/HACK/XXX`, `rand()`, `GetRandomValue`, divisões e padrões críticos.

---

## 3. Resultados da Validação Empírica

| Teste | Duração/Seed | Resultado |
|---|---|---|
| Build Debug | — | `darknet.exe` gerado |
| Build Release | — | `darknet.exe` gerado |
| Autotest headless | 60s, seed 20260905 | `VALIDACAO: PASSOU` |
| Autotest headless | 120s, seed 7 | `VALIDACAO: PASSOU` |
| Autotest headless | 120s, seed 20260821 | `VALIDACAO: PASSOU` |
| Autotest headless | 300s, seed 7 | `VALIDACAO: PASSOU` |
| Testes unitários | — | 26/26 casos, 148/148 asserções |

O jogo é estável para execução autônoma, mas os testes não cobrem regressões de gameplay, save, render ou netcode.

---

## 4. Achados por Domínio

### 4.1 Arquitetura e Qualidade de Código

#### P0 — `Game` é um God Object
- **Arquivo:** `src/Game.h:87-698`
- **Descrição:** A classe `Game` agrega ~160 membros de dados e dezenas de responsabilidades (física, spawn, input, render 2D/3D, HUD, menus, rede, loja premium, save, áudio, shaders, mundo aberto, fases, crafting, construção, animais, civis). Viola o Princípio da Responsabilidade Única de forma extrema.
- **Recomendação:** Refatorar para subsistemas independentes (`World`, `Render`, `UI`, `Audio`, `Input`, `Save`, `Network`, `Economy`, `SpawnDirector`, `PhaseDirector`). `Game` deve orquestrar, não conter todos os dados.

#### P0 — Separação física `Game_*.cpp` sem separação lógica
- **Arquivo:** todos os `src/Game_*.cpp`
- **Descrição:** Os arquivos `Game_Gameplay.cpp`, `Game_WorldRender.cpp`, etc., são partições físicas da mesma classe. Todos fazem `#include "Game.h"` e acessam membros privados livremente. Não há encapsulamento real.
- **Recomendação:** Converter em subsistemas autônomos que recebem dependências via construtor.

#### P0 — Métodos monolíticos
- **Arquivo:** `src/Game_WorldRender.cpp:632` (`renderWorld3D`, ~1.880 linhas), `src/Game_Gameplay.cpp:1267` (`handleInput`, ~580 linhas), `src/Game.cpp:552` (`run`, ~240 linhas)
- **Descrição:** Métodos com múltiplas responsabilidades, profundidade alta e estados locais dispersos.
- **Recomendação:** Decompor em funções pequenas: `updateMenu()`, `renderSky()`, `renderEntities()`, `processCombatInput()`, etc.

#### P0 — `Game.h` inclui 30 headers sem forward declarations
- **Arquivo:** `src/Game.h:3-38`
- **Descrição:** Inclui diretamente `Player.h`, `Enemy.h`, `Item.h`, `Tilemap.h`, `AudioManager.h`, `NetClient.h`, `StoreClient.h`, etc. Força recompilação em cascata e cria header inchado.
- **Recomendação:** Usar ponteiros/opacos (`std::unique_ptr<class AudioManager>`) e forward declarations.

#### P0 — Código duplicado massivo
- **Arquivo:** `src/Game.cpp:19-43`, `src/Game_WorldRender.cpp:19-43` (`structureTintFor`, `isMedievalZone` idênticos); `src/Game.cpp:49-55`, `src/Game_WorldRender.cpp:49-55`, `src/Game_WorldGen.cpp:13-16` (`FIT_HOUSE`, `FIT_BARRACKS`, etc.)
- **Descrição:** Helpers e constantes copiados literalmente entre arquivos. Risco de divergência.
- **Recomendação:** Centralizar em `World/BiomeUtils.h` e `World/BuildingMetrics.h` como `inline constexpr`.

#### P1 — Estado global disfarçado
- **Arquivo:** `src/Game.h:98-99` (`static bool headless`, `static int startPhaseOverride`); `src/Game.cpp:8` / `src/Game_WorldRender.cpp:17-182` (`g_voxelCapture`); `src/SpriteGen.h:43` (`SpriteBank` singleton); `src/SaveManager.h:22-44` (`SaveManager` totalmente estático)
- **Descrição:** Estado global dificulta testes unitários e cria acoplamento oculto.
- **Recomendação:** Passar configurações por construtor/struct; injetar `SpriteBank`; converter `SaveManager` em instância com interface.

#### P1 — `std::function` alocado por entidade a cada frame
- **Arquivo:** `src/Game.h:192`, `src/Game_WorldRender.cpp:172`
- **Descrição:** `ensureVoxel` recebe `std::function<void()>`. O próprio comentário admite que montar o `std::function` custa uma alocação por entidade por frame.
- **Recomendação:** Trocar por template `typename DrawFn` ou ponteiro de função/estado explícito.

#### P2 — Inconsistência de nomenclatura e magic numbers
- **Arquivo:** `src/Game.h` e todos os `Game_*.cpp`
- **Descrição:** Mistura de prefixo `m_` com nomes sem prefixo; centenas de literais (`52.0f`, `230.0f`, `560.0f`, `3000.0f`, etc.); 450+ ocorrências de cores literais.
- **Recomendação:** Criar `namespace Constants`, `namespace Palette`, `GameplayConstants` com `inline constexpr`.

#### P1 — Ausência de warnings rigorosos
- **Arquivo:** `CMakeLists.txt:13-15`
- **Descrição:** Não há `-Wall -Wextra -Wconversion -Wshadow` nem `/W4 /permissive-`.
- **Recomendação:** Adicionar flags por compilador e tratar warnings como erros no CI.

---

### 4.2 Gameplay e Balanceamento

#### P0 — Bônus de level-up são perdidos no próximo equip/level-up
- **Arquivo:** `src/Game_Menus.cpp:312`
- **Descrição:** `applyLevelUpChoice` modifica stats efetivos (`maxHealth`, `attackDamage`, etc.), mas `Player::applyEquipmentStats()` recalcula tudo a partir das bases. Os bônus da tela de level-up desaparecem.
- **Recomendação:** Aplicar bônus sobre `baseMaxHealth`, `baseAttackDamage`, `baseMoveSpeed`, etc.

#### P0 — Árvore de habilidades tier 3 é inatingível
- **Arquivo:** `src/SkillTree.cpp:53`
- **Descrição:** `SkillTree::canBuy` exige `spentInBranch >= branchSpentReq(tier)`, onde `branchSpentReq(2) = 4`. Cada branch tem apenas 4 perks; para tier 3 é impossível ter 4 pontos gastos sem já ter comprado o próprio tier 3.
- **Recomendação:** Exigir 3 pontos para T3, não 4.

#### P1 — Zerglings extras nascem sem scaling
- **Arquivo:** `src/Game_Spawn.cpp:276-287`
- **Descrição:** Zerglings extras são spawnados depois de todo o scaling aplicado ao primeiro. Nascem com stats base, tornando-se irrelevantes no late-game.
- **Recomendação:** Aplicar o mesmo scaling aos Zerglings extras.

#### P1 — Knockback aplicado 2x em tipos específicos
- **Arquivo:** `src/Enemy.cpp`
- **Descrição:** `Enemy::update()` processa knockback no início, mas `updateZergling`, `updateHydra`, `updateBroodmother`, `updateAlienBoss`, etc. processam knockback novamente.
- **Recomendação:** Remover o knockback manual dos updates específicos.

#### P1 — `EnemyDirector::observe` não usa média móvel real
- **Arquivo:** `src/EnemyDirector.cpp:30`
- **Descrição:** `damageTaken = damageTaken * 0.0f + dpm * 1.0f` zera o valor anterior. Reage apenas ao dano da última janela de 6s.
- **Recomendação:** Usar média móvel, e.g. `* 0.6f + dpm * 0.4f`.

#### P1 — Quests de NPC concluem por critérios errados
- **Arquivo:** `src/Game_QuestsNPC.cpp:79` (`q_portais` conta qualquer kill), `src/Game_QuestsNPC.cpp:144` (`q_coleta_raro` não verifica raridade)
- **Descrição:** Recompensas gratuitas por condições incorretas.
- **Recomendação:** Criar tipos de quest específicos (`ClosePortal`, `CollectRare`) e validar critérios.

#### P2 — Scaling multiplicativo pode explodir
- **Arquivo:** `src/Game_Spawn.cpp:191-251`, `src/Game_Evolution.cpp`
- **Descrição:** Player level + global scaling (cap 4x) + dificuldade + threat level (sem teto) + mutator + ring + phase. Inimigos podem chegar a 50x–100x HP/dano.
- **Recomendação:** Aplicar teto global de scaling combinado ou tornar parte aditivo.

#### P2 — Renda passiva de construções explode
- **Arquivo:** `src/BuildingSystem.cpp`
- **Descrição:** House gera 10 créditos/8s; com várias casas e upgrades, renda passiva explode. Barracks/TankFactory geram unidades gratuitas além da fila paga.
- **Recomendação:** Reduzir renda passiva, torná-la por jogador, cobrar custo para unidades automáticas.

#### P3 — Textos de evolução não correspondem aos efeitos
- **Arquivo:** `src/Game_Menus.cpp:404-407` vs `src/Game_Menus.cpp:325-348`
- **Descrição:** "SOLDADO CYBORG — +HP +Defesa +Armadura" aplica apenas `attackDamage *= 1.30`; "HACKER FANTASMA — +Vel +Dano +Alcance" aplica apenas `speed *= 1.40`.
- **Recomendação:** Alinhar textos aos efeitos ou ajustar os efeitos.

---

### 4.3 Renderização e Performance

#### P0 — Modelos voxel 3D são gerados mas nunca renderizados
- **Arquivo:** `src/Game_WorldRender.cpp:172-210`, `src/Game_WorldRender.cpp:632-699`
- **Descrição:** A cada novo tipo/pose de entidade o jogo captura sprite 2D, cria contorno, cropa **e gera um `Model` 3D real via `SpriteExtrude::BuildVoxelModel`**. O comentário confirma: *"Voxel 3D real: cacheado mas NAO renderizado"*. Cada pose gera malha na GPU, ocupando VRAM e stalls, sem nunca ser usada.
- **Recomendação:** Remover `BuildVoxelModel` do fluxo de `ensureVoxel` se não é usado.

#### P1 — Sprites desabilitam depth-write e backface culling por entidade
- **Arquivo:** `src/Game_WorldRender.cpp:563-588`
- **Descrição:** Cada entidade chama `rlDisableBackfaceCulling()`/`rlDisableDepthMask()`. State change por entidade + overdraw massivo.
- **Recomendação:** Ordenar entidades por profundidade e usar um único par Begin/End de state change.

#### P1 — Inimigos/bosses emitem 50–100 draw calls cada
- **Arquivo:** `src/Enemy_Render.cpp` (ex.: `renderBossAura`, `renderOrcCibernetico`, `renderOmegaBoss`)
- **Descrição:** Cada inimigo desenhado com dezenas de primitivas individuais. Sem batching/instancing.
- **Recomendação:** Renderizar em atlas/texture atlas único ou acumular geometria em CPU-side vertex buffer.

#### P1 — Partículas alternam blend mode individualmente
- **Arquivo:** `src/Particle.cpp:42-73`
- **Descrição:** Partículas `glow=true` chamam `BeginBlendMode(BLEND_ADDITIVE)`/`EndBlendMode()` por partícula. Com até 1.400 partículas, gera milhares de state changes.
- **Recomendação:** Separar em duas fases: normais primeiro, depois aditivas em um único Begin/End.

#### P1 — Scanlines como centenas de retângulos
- **Arquivo:** `src/Effects.h:33-40`
- **Descrição:** `DrawScanlines` desenha um retângulo por ~6 pixels de altura (≈120 draw calls para 720p).
- **Recomendação:** Usar textura alpha mask ou shader pós-processo.

#### P1 — Entidades não usam frustum culling
- **Arquivo:** `src/Game_WorldRender.cpp:1989-2027`
- **Descrição:** Usam apenas teste de caixa com limites fixos. Objetos atrás da câmera ou fora do cone de visão ainda são desenhados.
- **Recomendação:** Aplicar `sphereInCameraFrustum` (já implementado para `owDecor`) a todas as entidades.

#### P1 — Vertex shader usa `w = 1.0` para normal
- **Arquivo:** `resources/shaders/world.vs:21`
- **Descrição:** `fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));` adiciona translação da matriz. Normais devem usar `vec4(vertexNormal, 0.0)`.
- **Recomendação:** Corrigir para `vec4(vertexNormal, 0.0)`.

#### P2 — Vignette duplicada
- **Arquivo:** `src/Game_WorldRender.cpp:2266`, `resources/shaders/grade.fs:32-35`
- **Descrição:** Cena já aplica `DrawVignette`; shader `grade.fs` aplica outra vignette matemática.
- **Recomendação:** Escolher uma das duas.

#### P2 — Thread de screenshot sem sincronização/join
- **Arquivo:** `src/Game_Shaders.cpp:216-221`
- **Descrição:** `std::thread(...).detach()` exporta imagem em background. Se `Game` for destruído durante exportação, há risco de use-after-free/corruptação.
- **Recomendação:** Usar `std::future`/fila de tarefas com join explícito no destrutor.

#### P2 — Resolução interna fixa em 1280×720
- **Arquivo:** `src/Game.h:153-154`, `src/Game_Shaders.cpp:117-118`
- **Descrição:** Janela é redimensionável, mas conteúdo sempre renderizado em 720p e escalado com letterbox.
- **Recomendação:** Recriar `gameTarget`, `m_bloomA/B` e `lightMask` no resize.

#### P1 — Flash branco de hit e luzes piscantes
- **Arquivo:** `src/Enemy_Render.cpp` (múltiplas linhas), `src/Game_WorldRender.cpp:1008-1042`
- **Descrição:** Múltiplos inimigos piscando branco simultaneamente e pulsações rápidas podem ser problemáticos para fotossensibilidade.
- **Recomendação:** Limitar frequência/amplitude e adicionar opções de acessibilidade.

---

### 4.4 Networking e Segurança

#### P0 — Cliente recusa `wss://` — WebSocket sem TLS
- **Arquivo:** `src/NetClient.cpp:132-137`, `src/Game_Network.cpp:14-18`
- **Descrição:** `NetClient::init` retorna `false` se a URL começar com `wss://`. Só fala WebSocket puro sobre TCP. Trafegar JWT, posições e chat em `ws://` é inaceitável em produção.
- **Recomendação:** Implementar TLS (OpenSSL/Schannel) ou migrar para `websocketpp`/`libwebsockets`.

#### P0 — Progresso do jogador é escrito sob autoridade do cliente
- **Arquivo:** `server/game-server/src/index.js:438-447`
- **Descrição:** `POST /progress` aceita `level`, `credits`, `char_class`, `save_json` do cliente. Servidor não valida coerência com histórico da conta.
- **Recomendação:** Não permitir cliente escrever progresso diretamente; validar delta ou usar checkpoints assinados.

#### P1 — `NetClient::enabled` é `bool` comum — data race
- **Arquivo:** `src/NetClient.h:37`, `src/NetClient.cpp:135,149,153,161,170,234`
- **Descrição:** `enabled` é lido pela thread principal e escrito pela thread de rede. `bool` comum = undefined behavior em C++17.
- **Recomendação:** Tornar `std::atomic<bool>`.

#### P1 — `NetClient::sendChat` ainda monta JSON manualmente
- **Arquivo:** `src/NetClient.cpp:207-224`
- **Descrição:** `jsonEscape()` trata `"`, `\`, `\n`, `\r`, `\t`, mas não controles restantes, UTF-8 inválido, etc.
- **Recomendação:** Usar `nlohmann::json` (já usado em `sendState`/`joinParty`).

#### P1 — Handshake não valida `Sec-WebSocket-Accept`
- **Arquivo:** `src/NetClient.cpp:317`
- **Descrição:** Cliente só verifica `" 101"`. Não computa accept esperado a partir do `Sec-WebSocket-Key`.
- **Recomendação:** Calcular `BASE64(SHA1(key + GUID))` e comparar.

#### P1 — Parser de frames WebSocket vulnerável a overflow aritmético
- **Arquivo:** `src/NetClient.cpp:376-392`
- **Descrição:** `rx.size() < pos + len` pode dar wrap-around se `len` próximo de `UINT64_MAX`.
- **Recomendação:** Verificar `len <= rx.size() - pos` com proteção contra underflow.

#### P1 — `/auth/login` é stub — qualquer nome gera token válido
- **Arquivo:** `server/game-server/src/index.js:345-354`
- **Descrição:** Não há senha, OAuth ou vinculação de conta. Qualquer pessoa cria infinitas contas.
- **Recomendação:** Substituir por OAuth2 (Steam/Google) ou e-mail+senha com bcrypt/argon2.

#### P2 — `StoreClient` acumula threads sem limite
- **Arquivo:** `src/StoreClient.cpp:51-63`, `src/StoreClient.h:56`
- **Descrição:** `startThread()` empilha `std::thread` em `threads_` sem limite de concorrência.
- **Recomendação:** Usar `std::async` com política explícita ou thread-pool com fila limitada.

#### P2 — Gateway nginx tem bloco HTTPS comentado
- **Arquivo:** `server/gateway/nginx.conf:54-78`
- **Descrição:** Configuração de TLS está comentada.
- **Recomendação:** Descomentar bloco 443, redirecionar 80→443, usar certificados reais.

#### P2 — Cliente abre URL do Stripe sem validação
- **Arquivo:** `src/StoreClient.cpp:216-224`, `src/HttpClient.cpp:91-93`
- **Descrição:** `buyGemsAsync` abre no navegador qualquer URL retornada pelo servidor via `ShellExecuteA`.
- **Recomendação:** Validar que a URL pertence a `https://checkout.stripe.com/` antes de abrir.

---

### 4.5 Save e Serialização

#### P0 — `Game::autoSave()` persiste metadados zerados
- **Arquivo:** `src/Game.cpp:1413-1415`
- **Descrição:** `autoSave()` chama `SaveManager::save()` sem passar `playMinutes`, `totalDeaths`, `bossesKilled`, `portalsSealed`, `difficultyLevel`. Todos ficam com default 0.
- **Recomendação:** Passar metadados reais do `Game` para `SaveManager::save`.

#### P0 — `Game::totalKills` não é salvo nem restaurado
- **Arquivo:** `src/Game.h:266`, `src/Game_Gameplay.cpp:1135`, `src/SaveManager.cpp:146`
- **Descrição:** Save salva apenas `Player::totalKills`. Após load, `Game::totalKills` fica com lixo da sessão anterior, quebrando curva de dificuldade e trigger do Omega Boss.
- **Recomendação:** Persistir `Game::totalKills` explicitamente.

#### P0 — `equipBag` (mochila de equipamentos) não é salva
- **Arquivo:** `src/Player.h:83`, `src/SaveManager.cpp:95-168`
- **Descrição:** Todo equipamento não equipado é perdido ao recarregar.
- **Recomendação:** Serializar `Player::equipBag`.

#### P0 — Upgrades e atributos de equipamentos são perdidos
- **Arquivo:** `src/Equipment.h:17`, `src/SaveManager.cpp:88-91`, `154-156`
- **Descrição:** `equipSaveToken` salva apenas `id` ou `name`. `upgradeLevel`, `primary`, `secondary`, `tier`, `color` e bônus customizados não são gravados. Arma +3 volta a +0.
- **Recomendação:** Expandir formato para campos explícitos ou migrar para JSON schema-versionado.

#### P0 — Inventário perde raridade, afixos e atributos individuais
- **Arquivo:** `src/SaveManager.cpp:164-167`, `src/Item.cpp:9-54`, `232-289`
- **Descrição:** Save grava apenas `ItemType`. No load, chama `Item::createRandom()`, recriando raridade/afixos aleatoriamente. Item lendário pode voltar comum.
- **Recomendação:** Serializar todos os campos significativos do `Item`.

#### P1 — `BuildingSystem` não é persistido
- **Arquivo:** `src/BuildingSystem.h:175-190`, `src/SaveManager.cpp:95-168`
- **Descrição:** `buildings`, `tanks`, `soldiers`, níveis, filas, timers, créditos pendentes e materiais são perdidos.
- **Recomendação:** Adicionar serialização completa do `BuildingSystem`.

#### P1 — Estado residual não é limpo no load
- **Arquivo:** `src/Game.cpp:968-999`, `src/SaveManager.cpp:173-315`
- **Descrição:** `startLoadedGame` não limpa `enemies`, `items`, `projectiles`, `groundEquips`, `buildingSystem`. `Player` não é resetado antes do load; campos não salvos mantêm valores da sessão anterior.
- **Recomendação:** Reconstruir `Player` e limpar entidades voláteis do `Game` antes de aplicar o save.

#### P1 — Escrita não atômica
- **Arquivo:** `src/SaveManager.cpp:101`
- **Descrição:** `fopen(path, "w")` trunca imediatamente. Se o processo morrer durante `fprintf`, o save anterior é destruído.
- **Recomendação:** Escrever em `.tmp` e usar `rename()` atômico.

#### P1 — Save sem checksum/integridade
- **Arquivo:** `src/SaveManager.cpp:95-315`
- **Descrição:** Texto plano sem hash, assinatura ou criptografia. Edição trivial.
- **Recomendação:** Adicionar checksum do payload após o header.

#### P1 — Header de versão é aceito cegamente
- **Arquivo:** `src/SaveManager.cpp:184-186`
- **Descrição:** Código comenta "Accept any DARKNET_SAVE_V* version", mas não há migração por versão.
- **Recomendação:** Extrair número da versão e rejeitar/migrar saves antigos.

#### P1 — Auto-save em momentos inseguros
- **Arquivo:** `src/Game_Gameplay.cpp:602-604`, `src/Game_Phases.cpp:381`
- **Descrição:** Save automático a cada 30s e em toda transição de zona, sem guarda contra combate ativo, transição, jogador morto, diálogo ou pause.
- **Recomendação:** Adicionar `canAutoSave()` que bloqueia nesses estados.

#### P2 — API multi-slot é código morto
- **Arquivo:** `src/SaveManager.h:22-44`, `src/SaveManager.cpp:319-425`, `src/Game.cpp:563`, `971`
- **Descrição:** `SaveManager` implementa 3 slots, mas `Game` só usa arquivo legado `darknet_save.txt` (slot 0).
- **Recomendação:** Remover API multi-slot ou integrá-la no menu principal.

---

### 4.6 Mundo, Colisão e Física

#### P0 — Portais de anomalia nascem fora da área jogável
- **Arquivo:** `AnomalyPortal.cpp:371`, `src/Game_Gameplay.cpp:821-823`
- **Descrição:** `spawnWave` recebe `zoneW/zoneH` do tilemap (24.576×24.576 unidades) e coloca candidatos fixos nesse retângulo. O mundo aberto real é um disco de `owPhaseRadius` (3.000–5.000). A maioria dos candidatos fica além da barreira de fase.
- **Recomendação:** Gerar candidatos dentro do círculo `safeZoneCenter + owPhaseRadius - margem`.

#### P0 — Jogador não faz sliding X/Y separado
- **Arquivo:** `src/Game_Gameplay.cpp:1506-1524`
- **Descrição:** `Player::move` aplica velocidade simultaneamente em X e Y; só depois testa `isBlocked` e reverte para `old`. Permite atravessar cantos de parede e prende em degraus.
- **Recomendação:** Implementar sliding separado como os inimigos (`Game_Gameplay.cpp:637-642`).

#### P0 — `InfernoZone` usa `rand()` global em vez da seed
- **Arquivo:** `src/InfernoZone.cpp:83`, `119`, `126-132`
- **Descrição:** Apesar de receber `seed`, timers de gêiser, escolha de poça e spawn de cinzas usam `rand()`. `rand()` não é re-inicializado pelo `--seed` do jogo.
- **Recomendação:** Substituir todos os `rand()` por `frand(seed)` mantendo estado local.

#### P0 — Inimigos de anomalia nascem dentro de paredes/estruturas
- **Arquivo:** `src/AnomalyPortal.cpp:76-82`
- **Descrição:** `getSpawnPosition` retorna ponto aleatório em elipse ao redor do portal, sem testar colisão.
- **Recomendação:** Testar `isBlocked(portalSpawnPos)` e deslocar radialmente até encontrar ponto livre.

#### P1 — `isWallAtPosition` usa ponto único, ignorando raio da entidade
- **Arquivo:** `src/Tilemap.cpp:1533-1537`
- **Descrição:** Só consulta o tile do centro. Entidades com raio > metade do tile atravessam paredes.
- **Recomendação:** Adicionar overload `isWallAtPosition(Vector2 pos, float radius)`.

#### P1 — Projeteis não colidem com limite do mundo aberto
- **Arquivo:** `src/Game.cpp:1353`, `1385`
- **Descrição:** `tilemap.isWallAtPosition` retorna `false` fora dos limites quando `openWorld=true`. Granadas não explodem no limite.
- **Recomendação:** Adicionar teste explícito contra `owPhaseRadius`.

#### P1 — Spawn de inimigos pode empurrar além da barreira
- **Arquivo:** `src/Game_Spawn.cpp:27-31`, `37-44`
- **Descrição:** `slideToFree` pode continuar empurrando para fora sem checar `owPhaseRadius`.
- **Recomendação:** Clamp final dentro de `owPhaseRadius - margem`.

#### P1 — `buildOpenWorldScenery` gera mundo fixo inteiro de uma vez
- **Arquivo:** `src/Game_WorldGen.cpp:86-480`
- **Descrição:** Todas as regiões são populadas na transição. Com `owPhaseRadius` crescendo, número de regiões cresce quadraticamente.
- **Recomendação:** Adotar geração lazy por região ou amortizar em múltiplos frames.

#### P2 — `Tilemap::tileZone` e `biomeAtWorld` divergem
- **Arquivo:** `src/Tilemap.cpp:461-476`
- **Descrição:** `tileZone` computa layout 3×3; `biomeAtWorld` sempre retorna `currentZone`. Render 2D pode mostrar biomas diferentes do 3D.
- **Recomendação:** Unificar em uma única função de consulta.

#### P2 — Não há pathfinding para inimigos
- **Arquivo:** `Enemy.cpp`, `Game_Gameplay.cpp:620-660`
- **Descrição:** Inimigos usam steering direto. Podem ficar presos em recuos ou C-shapes.
- **Recomendação:** Implementar A* no grid de tiles para inimigos terrestres.

---

### 4.7 Áudio, Input e Sistemas Auxiliares

#### P0 — `rand()` do CRT nunca é semeado
- **Arquivo:** `src/main.cpp:37`, `src/AudioManager.cpp:26`, `1426`, `2057`, `2097`
- **Descrição:** `main.cpp` só chama `SetRandomSeed(seed)` quando `--seed` é passado. Nunca chama `srand()`. `AudioManager` usa `rand()` para footsteps, ambientes e ruído. Sem `srand()`, o CRT inicia com seed 1, tornando áudio idêntico em todas as execuções.
- **Recomendação:** Chamar `srand((unsigned)time(nullptr))` no início de `main()` quando não houver `--seed`; migrar `rand()` para `GetRandomValue`.

#### P0 — `TutorialSystem` não é instanciado nem usado
- **Arquivo:** `src/Game.h` (ausência), `src/TutorialSystem.cpp`
- **Descrição:** A classe existe, mas **não há instância de `TutorialSystem` em `Game`**. Sistema inteiro é código morto.
- **Recomendação:** Adicionar instância, chamar init/update/render e conectar callbacks.

#### P0 — Recompensa do tutorial nunca é aplicada
- **Arquivo:** `src/TutorialSystem.cpp:84-88`, `103-123`
- **Descrição:** Badge promete 500 XP / 100 créditos, mas `completeStep` apenas avança o passo.
- **Recomendação:** Aplicar recompensa em `completeStep(TutorialStep::Completed)`.

#### P0 — `AchievementSystem` é inicializado, mas nenhum evento é reportado
- **Arquivo:** `src/Game.cpp:245`, `src/Achievement.cpp:131-206`
- **Descrição:** `achievements.init()` é chamado, mas nenhum `achievements.onKill`, `onLevelUp`, `onPortalClosed`, etc. é invocado.
- **Recomendação:** Conectar reporters aos eventos do jogo.

#### P0 — Recompensas de conquistas nunca são concedidas
- **Arquivo:** `src/Achievement.cpp:83-91`
- **Descrição:** `unlock()` apenas seta flag e texto do popup. `rewardXP`/`rewardCredits` não são aplicados ao `Player`.
- **Recomendação:** Em `unlock()`, aplicar recompensas ao `Player`.

#### P0 — Teclas duramente codificadas; sem sistema de rebind
- **Arquivo:** `src/Game_Gameplay.cpp` (dezenas de linhas), `src/Game_Menus.cpp`, `src/Game_HUD.cpp`
- **Descrição:** WASD, setas, 1-6, Q, E, I, G, J, B, C, TAB, F1-F12, SHIFT, SPACE estão hardcoded.
- **Recomendação:** Implementar `InputMap` carregável de JSON/INI.

#### P0 — `BotController::zonesVisited` incrementa a cada frame perto do portal
- **Arquivo:** `src/BotController.cpp:786-788`
- **Descrição:** Em `AdvancePhase`, quando `nearestPortalDist < 40.0f`, `zonesVisited++` acontece todo frame.
- **Recomendação:** Incrementar apenas na transição (uma vez por portal).

#### P1 — `SHIFT` sobrecarregado: sprint e seleção RTS
- **Arquivo:** `src/Game_Gameplay.cpp:1457`, `1492`
- **Descrição:** Mesma tecla para sprint e modo de seleção RTS.
- **Recomendação:** Separar sprint de seleção RTS (ex.: ALT/CTRL).

#### P1 — `KEY_E` sobrecarregada
- **Arquivo:** `src/Game_Gameplay.cpp:479`, `1725`, `1753`
- **Descrição:** `E` abre diálogo com NPC, avança diálogo, interage com equipamento no chão e aciona portal.
- **Recomendação:** Definir prioridades explícitas e feedback visual.

#### P1 — `sfxPlayerDeath` gerado mas nunca tocado
- **Arquivo:** `src/AudioManager.cpp:1601-1612`, `1871`
- **Descrição:** Som de morte sintetizada é carregado, ocupa memória e tempo de init, mas não tem chamador.
- **Recomendação:** Remover ou substituir `playDeathCry()` por `playPlayerDeath()`.

#### P1 — Múltiplos métodos de áudio nunca invocados
- **Arquivo:** `src/AudioManager.cpp:1967-1984`, `src/AudioManager.h:124-137`
- **Descrição:** `playPlayerHurt`, `playEvolve`, `playHeal`, `playItemPickup`, `playSkillUnlock`, `playAchievement`, etc. não são chamados fora das próprias definições.
- **Recomendação:** Remover assets/métodos não usados ou conectar aos eventos.

#### P1 — Código morto após `return composeTrack(...)` em synths de zona
- **Arquivo:** `src/AudioManager.cpp:464-567`, `570-662`, `665-770`, etc.
- **Descrição:** Corpos enormes de geração manual nunca executam.
- **Recomendação:** Remover código morto.

#### P2 — `CraftingSystem` tem índice ambíguo
- **Arquivo:** `src/CraftingSystem.cpp:209-217`, `246-257`, `363`, `493-497`
- **Descrição:** `selected` mistura índice filtrado e global. Pode craftar receita errada ou crashar.
- **Recomendação:** Usar sempre índice dentro do vetor `filtered`.

#### P2 — `ShopSystem` permite comprar cosméticos idênticos repetidamente
- **Arquivo:** `src/ShopSystem.cpp:473-501`
- **Descrição:** `tryBuy` não verifica se cosmético já foi adquirido.
- **Recomendação:** Marcar cosméticos comprados e desabilitar itens já possuídos.

---

## 5. Problemas Transversais

### Determinismo e aleatoriedade
- `rand()` é usado em `AudioManager`, `InfernoZone`, `Player` (evasão), `Particle`, `Tilemap`, `Enemy`, `Game_WorldGen` — sem `srand()`.
- `GetRandomValue` é usado 278 vezes em `src/`.
- A mistura de PRNGs quebra a promessa de mundo reprodutível por `--seed`.
- **Recomendação:** Padronizar para um único PRNG seedeado por `--seed` em todo o jogo.

### Cobertura de testes
- 26 testes cobrem `CraftingSystem`, `Enemy`, `SaveManager`, `Player`, `Projectile`, `Tilemap`.
- A classe `Game` não tem testes unitários por causa do acoplamento.
- **Recomendação:** Refatorar `Game` para depender de interfaces; criar testes para `SpawnSystem`, `PhaseSystem`, `Economy`, `Collision`.

### Documentação vs implementação
- `GAME_DESIGN.md` prevê tutorial, conquistas, estoque por NPC, meta-progressão na árvore de habilidades — nenhum funciona corretamente.
- `ROADMAP.md` cita migração para IDs estáveis e `nlohmann/json` — parcialmente feita.
- **Recomendação:** Atualizar GDD ou implementar funcionalidades faltantes.

---

## 6. Recomendações Prioritárias

### Imediatas (P0) — corrigir antes de qualquer release
1. **Refatorar `Game` em subsistemas** ou, no mínimo, quebrar `renderWorld3D()` e `handleInput()`.
2. **Conectar TutorialSystem e AchievementSystem** ou removê-los do jogo.
3. **Corrigir save:** passar metadados reais, salvar `Game::totalKills`, `equipBag`, upgrades de equipamentos, raridade/afixos de itens.
4. **Corrigir gameplay crítico:** bônus de level-up sobre bases, desbloqueio de tier 3 na árvore, critérios de quests.
5. **Corrigir mundo:** spawn de anomalias dentro do disco da fase, sliding X/Y do jogador, determinismo do `InfernoZone`.
6. **Corrigir segurança:** implementar TLS no WebSocket (`wss://`) e tornar servidor autoritativo para progresso/ações.
7. **Padronizar aleatoriedade:** seedear `srand()` ou migrar tudo para um PRNG único.

### Curtíssimo prazo (P1)
1. Tornar `NetClient::enabled` atômico; validar `Sec-WebSocket-Accept`; usar `nlohmann::json` em `sendChat`.
2. Implementar frustum culling para entidades e luzes.
3. Remover geração de modelos voxel não usados.
4. Validar spawn de inimigos/bosses/companions contra `m_chunkSolids`.
5. Adicionar `canAutoSave()` e escrita atômica de save.
6. Implementar `InputMap` configurável.
7. Corrigir `BotController::zonesVisited`.

### Médio prazo (P2/P3)
1. Consolidar constantes, cores e helpers duplicados.
2. Reduzir state changes no render (partículas, billboards, scanlines).
3. Corrigir vertex shader `world.vs`.
4. Adicionar opções de acessibilidade (flashes, saturação, UI scale).
5. Migrar save para JSON schema-versionado com checksum.
6. Implementar pathfinding A* para inimigos terrestres.
7. Adicionar warnings rigorosos no CMake.

---

## 7. Conclusão

O `darknet-prototype` é um protótipo impressionante em volume de funcionalidades, mas a **dívida técnica supera a maturidade do código**. A classe `Game` monolítica é o epicentro do risco: ela concentra acoplamento, dificulta testes e torna cada nova feature propensa a regressões. Paralelamente, **sistemas inteiros documentados no GDD (tutorial, conquistas, árvore tier 3, save confiável) não funcionam**, e **a arquitetura de multiplayer não é segura para produção**.

Do ponto de vista de estabilidade, o jogo passa nos portões de validação headless, o que é positivo. No entanto, **passar no autotest não equivale a estar pronto para jogar**: o save corrompe metadados, o balanceamento pode explodir por scaling acumulado, e o mundo procedural tem bugs de spawn e colisão.

A recomendação estratégica é **parar de adicionar features até que os P0 sejam resolvidos**. Em ordem: (1) arquitetura/subsistemas, (2) save e persistência, (3) gameplay crítico, (4) segurança de rede, (5) render/performance. Sem isso, cada novo ciclo aumentará exponencialmente o custo de correção.

---

*Relatório gerado automaticamente a partir de análise multi-agente e validação empírica.*
