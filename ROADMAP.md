# DARKNET — Roadmap Consolidado

Documento único de estado e direção do projeto. Substitui a leitura cruzada de
`ROADMAP_EXECUTIVO_CLAUDE.md`, `BACKLOG_IMPLEMENTACAO_CLAUDE.md` e
`obsidian-vault/Roadmap e Pendências.md` (mantidos como histórico/detalhe).

**Estado atual:** ARPG 3D isométrico (voxel) em C++/raylib, jogável offline, com
backend Node.js (auth, loja premium, realtime WS) em containers Docker.

---

## Concluído (auditoria e correção técnica 2026-08)

### Bot de teste (autotest)
- Correção da exploração fora da zona segura, melee que não conectava e coleta de
  itens não contabilizada.
- Thresholds de skills ajustados; timers `static` convertidos em membros com reset.
- Pathfinding BFS unificado; remoção de código morto.

### Pipeline 3D consolidado
- Removido o pipeline 2D paralelo e o toggle F10; o sistema voxel é o pipeline único.
- Removidos `render3D()` órfãos (Player/Enemy/NPC/Companion).
- Flag renomeada para `g_voxelCapture`.

### Performance
- Máscara de luz otimizada: 31 elipses → 1 quad texturizado por luz, em meia resolução.
- Batch único do piso do tilemap; frustum culling no cenário e no piso.
- LOD de sombras voxel; screenshot do autotest assíncrono.
- **Resultado:** pior frame de render 95.9ms → ~19ms; FPS mínimo 6 → 44–52; FPS médio 58–59.

### Refatoração
- `Game.cpp` de ~8.700 → ~4.700 linhas, com 11 módulos extraídos: `Game_Shaders`,
  `Game_Network`, `Game_PremiumStore`, `Game_Resources`, `Game_Evolution`,
  `Game_QuestsNPC`, `Game_Phases`, `Game_Spawn`, `Game_WorldGen`, `Game_HUD`, `Game_Bot`.

### Documentação
- Protagonista padronizado (Vance Rios), ano 2047; README atualizado para 3D isométrico.

### Backend (server/)
- Inventário passou a usar a coluna `qty` de verdade: `savePlayer` agrega a lista de
  itens e faz UPSERT (`ON CONFLICT (account,item_id) DO UPDATE SET qty`), em vez de
  DELETE + re-INSERT com qty=1; `getPlayer` expande `qty` para a lista plana.
- Migração idempotente no boot: consolida duplicatas legadas e cria índice único
  `(account,item_id)`.
- DDL consolidado: `index.js` é a fonte única de verdade (`CREATE TABLE IF NOT EXISTS`
  em todo boot); `db/init.sql` virou apenas um apontador — fim da divergência.
- Redis removido do `docker-compose.yml` (era órfão: matchmaking é `Map` em memória);
  comentário registra quando reintroduzir (sessões distribuídas / múltiplas instâncias).
- `PUBLIC_URL` default corrigido para `http://localhost:8080` (porta do gateway) —
  `success_url`/`cancel_url` do Stripe Checkout saíam com a porta interna errada.
- `server/README.md`: tabela de endpoints completa e portas corrigidas
  (gateway 8080 público, app 9000 interno).

---

## Próximos passos (curto prazo)

- **Testes unitários (doctest)** para regras puras (economia, loot, progressão, combat math).
- **Substituir parsing JSON manual por nlohmann/json** em `NetClient`/`StoreClient`.
- **IDs estáveis em SaveManager**: `resolveEquipByName` usa `strcmp` de nome de exibição —
  migrar para IDs internos imutáveis.
- **Padronização de nomenclatura**: prefixo `m_` em membros, sem abreviações obscuras.
- **Aplicar visualmente os cosméticos premium comprados**: skins/tintas não renderizam
  em `Player.cpp` — verificar e ligar o inventário premium ao render do player
  (inclui spawn do Drone de Estimação).

---

## Médio/longo prazo

- **Multiplayer cooperativo real**: protocolo de mensagens versionado, validação
  server-side de payloads, lobby/matchmaking com ready/start, sincronização
  autoritativa de estado (hoje é retransmissão simples por sala).
- **Conteúdo dos atos 2–5**: zonas, quests e bosses além do ato 1 (ver `DARKNET_STORY.md`).
- **Achievements completos** e **meta-progressão** entre runs.
- **UX/onboarding**: tutorial guiado de 3–5 passos, HUD com hierarquia de informação,
  feedback visual de eventos (missão, loot, nível).
- **Mundo aberto**: hubs e rotas entre zonas, POIs com propósito, eventos dinâmicos
  que alteram o mapa.
- **IA**: papéis táticos de inimigos (tank/ranged/flanker), companions estratégicos,
  bosses com fases e telegraph.
- **Porte Linux/macOS** (toolchain e dependências raylib).
- **Steam/EOS reais**: integração de verdade (hoje há apenas o diretório
  `integration/steam`), contas seguras (e-mail+senha com hash ou OAuth).
- **MVP de lançamento**: deploy do backend com HTTPS + Stripe em produção,
  conformidade legal (termos, LGPD), anti-fraude básico.

---

## Critérios de validação

Toda mudança relevante deve passar por:

```bash
# Build
cmake --build build --config Release

# Autotest (bot joga sozinho por 120s)
./build/Release/darknet.exe --autotest --test-seconds=120
```

- O autotest deve sair com **exit 0** e imprimir `VALIDACAO: PASSOU`.
- Sem regressão de FPS (referência pós-otimização: médio ~58–59, mínimo 44–52).
- Backend: `node --check server/game-server/src/index.js` deve passar.
