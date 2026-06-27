# Roadmap e Pendências

## ✅ Concluído Nesta Sessão (2026-06-27)

- [x] F11 fullscreen sem bug (RenderTexture2D)
- [x] Speech bubble acima da cabeça do personagem
- [x] Animação de caminhada (sem flutuar no ar)
- [x] OmegaBoss (boss hard a cada 50 kills)
- [x] Zerglings em swarm de 3
- [x] Enemy.h: OrcCibernetico, PaladinCorrompido, UndeadEnforcer
- [x] Tabelas de spawn com mais aliens em KRONOSFactory/KronosNexus
- [x] Áudio completamente reescrito (FM synthesis real)
- [x] Música: loops de 30s (era 24s), mais variação
- [x] Game Design Document: `C:\Users\ricar\darknet-prototype\GAME_DESIGN.md`
- [x] Frustum culling em `Tilemap::render()` para otimização de render (14.400 tiles)
- [x] BFS Pathfinding no bot para evitar travamento em cantos côncavos (ex: CursedFarm)
- [x] Efeitos visuais avançados: pilares de luz, raios e partículas ambientais
- [x] Efeito hit-stop (micro-delay no impacto) e screenshake de combate
- [x] Decalques de chão persistentes (sangue/queimado) limitados a 120
- [x] Multiplayer via WebSockets (RFC 6455) nativo sobre Winsock (conexão em ws://127.0.0.1:9000/ws)
- [x] Cliente HTTP REST assíncrono (WinHTTP) integrado em C++
- [x] Loja Premium (Gems) com catálogo dinâmico e Stripe Checkout no navegador
- [x] Servidor Node.js integrado com Stripe Sessions e Webhooks assinados
- [x] Banco de dados Postgres configurado no backend para salvar Gems e inventário

## 🔄 Em Implementação (workflow rodando)

- [ ] Itens visuais reais (espada, armadura, chip no chão)
- [ ] Companion system (MARCO VEIL / STEEL / REX — F2/F3/F4)
- [ ] Vendor NPC Shop (loja via tecla E nos NPCs)
- [ ] Crafting System (tecla C — materiais + receitas)
- [ ] Integração de todos os sistemas + WarCraft enemies no spawn

## 📋 Backlog (Próximas Features)

### Alta Prioridade (Pendências da Nova Auditoria)
- [ ] Aplicar as cores cosméticas (Red Chrome, Gold Plating) do ShopSystem no render do Player em [Player.cpp](file:///c:/Users/ricar/darknet-prototype/src/Player.cpp)
- [ ] Implementar os efeitos visuais das skins de Gems (Skin Neon, Skin Dragão) e spawnar o Drone de Estimação ao lado do player
- [ ] Harmonizar o esquema de tabelas do [init.sql](file:///c:/Users/ricar/darknet-prototype/server/db/init.sql) com a tabela dinâmica players (JSONB) no `index.js`
- [ ] Modo pseudo-3D isométrico (usuário perguntou sobre 3D)

### Média Prioridade
- [ ] Sistema de mineração/coleta de recursos no mapa
- [ ] Missões secundárias com recompensas únicas
- [ ] Boss secreto crossover WarCraft+StarCraft (3 fases)
- [ ] Sistema de reputação com facções (KRONOS / Resistência / Aliens)
- [ ] Masmorras aleatórias com salas procedurais
- [ ] Weather system (chuva ácida, tempestade elétrica)
- [ ] Mapa-múndi com waypoints entre zonas

### Baixa Prioridade / Futuro
- [ ] Multiplayer cooperativo
- [ ] Editor de builds (importar/exportar)
- [ ] Lore codex (histórico do universo)
- [ ] Conquistas / Achievements

## 🐛 Bugs Conhecidos

| Bug | Status |
|-----|--------|
| F11 só expandia fundo | ✅ Corrigido (RenderTexture) |
| Player flutuava ao caminhar | ✅ Corrigido (bodyBob invertido) |
| Bot travava em paredes | ✅ Corrigido (Pathfinding BFS - 0 stucks na telemetria) |
| Bot não coletava itens | ✅ Corrigido (Coleta de itens na tecla E) |
| Texto sobreposto no menu | 🔄 Em verificação |
| Bot não avançava de fase | ✅ Corrigido (Portal de avanço) |
