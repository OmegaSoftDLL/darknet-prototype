# Backlog de Implementação para o Claude Code

## Objetivo
Organizar a evolução do Darknet Prototype em tarefas pequenas, priorizadas e executáveis, com foco em estabilidade, gameplay, UX, performance e preparação para multiplayer.

---

## 1. Fase 1 — Estabilidade e base técnica

### Tarefa 1.1 — Validar ambiente de build
- Prioridade: Alta
- Estimativa: 1 a 2 dias
- Arquivos/áreas: [CMakeLists.txt](CMakeLists.txt), ambiente local, toolchain
- Ação:
  - confirmar CMake e compilador disponíveis
  - documentar comando de build para Debug/Release
  - criar instrução de rebuild simples

### Tarefa 1.2 — Criar smoke test básico
- Prioridade: Alta
- Estimativa: 1 a 2 dias
- Arquivos/áreas: [test_bot_report.ps1](test_bot_report.ps1), [bot_report.txt](bot_report.txt)
- Ação:
  - transformar o script atual em validação com PASS/FAIL
  - adicionar checagem de FPS mínimo, crash, progresso e stuck

### Tarefa 1.3 — Melhorar save/load
- Prioridade: Alta
- Estimativa: 2 dias
- Arquivos: [src/SaveManager.cpp](src/SaveManager.cpp), [src/SaveManager.h](src/SaveManager.h)
- Ação:
  - validar integridade do save
  - adicionar versionamento simples
  - tratar casos de save corrompido

---

## 2. Fase 2 — Arquitetura e modularização

### Tarefa 2.1 — Extrair contexto de jogo
- Prioridade: Alta
- Estimativa: 2 a 3 dias
- Arquivos: [src/Game.h](src/Game.h), [src/Game.cpp](src/Game.cpp)
- Ação:
  - criar um contexto de jogo com estado global
  - separar estado de runtime e estado de UI

### Tarefa 2.2 — Isolar sistemas principais
- Prioridade: Alta
- Estimativa: 3 a 4 dias
- Arquivos: [src/Game.cpp](src/Game.cpp), [src/Player.cpp](src/Player.cpp), [src/Enemy.cpp](src/Enemy.cpp), [src/Companion.cpp](src/Companion.cpp)
- Ação:
  - mover controle de combate, spawn, eventos e transições para módulos menores
  - reduzir a responsabilidade direta de Game

### Tarefa 2.3 — Criar pipeline de eventos simples
- Prioridade: Média
- Estimativa: 2 dias
- Arquivos: [src/Game.cpp](src/Game.cpp), novos arquivos auxiliares
- Ação:
  - implementar eventos como questCompleted, zoneEntered, itemPickedUp, enemyKilled
  - permitir que diferentes sistemas respondam sem acoplamento forte

---

## 3. Fase 3 — Gameplay e progression

### Tarefa 3.1 — Reorganizar inimigos por papéis
- Prioridade: Alta
- Estimativa: 2 a 3 dias
- Arquivos: [src/Enemy.cpp](src/Enemy.cpp), [src/Enemy.h](src/Enemy.h)
- Ação:
  - criar papéis claros: tank, ranged, flanker, assassin
  - ajustar comportamento por papel

### Tarefa 3.2 — Melhorar diferença entre classes do player
- Prioridade: Alta
- Estimativa: 2 dias
- Arquivos: [src/Player.cpp](src/Player.cpp), [src/Player.h](src/Player.h)
- Ação:
  - reforçar diferenças de estilo de jogo
  - dar mais identidade para cada classe

### Tarefa 3.3 — Tornar progression de loot mais clara
- Prioridade: Alta
- Estimativa: 2 a 3 dias
- Arquivos: [src/Item.cpp](src/Item.cpp), [src/Item.h](src/Item.h)
- Ação:
  - categorizar drops por tier e zona
  - aumentar legibilidade do loot encontrado

### Tarefa 3.4 — Melhorar crafting e shop como parte do loop
- Prioridade: Alta
- Estimativa: 3 dias
- Arquivos: [src/CraftingSystem.cpp](src/CraftingSystem.cpp), [src/ShopSystem.cpp](src/ShopSystem.cpp)
- Ação:
  - criar receitas com impacto real no poder
  - ajustar catálogo para refletir progressão do jogador

### Tarefa 3.5 — Tornar companions estratégicos
- Prioridade: Alta
- Estimativa: 3 a 4 dias
- Arquivos: [src/Companion.cpp](src/Companion.cpp), [src/Companion.h](src/Companion.h)
- Ação:
  - definir papéis formais: tank, support, sniper, scout
  - melhorar sinergia com o player

### Tarefa 3.6 — Melhorar quests com maior contexto
- Prioridade: Média
- Estimativa: 2 a 3 dias
- Arquivos: [src/Quest.cpp](src/Quest.cpp), [src/Quest.h](src/Quest.h), [src/Game.cpp](src/Game.cpp)
- Ação:
  - reduzir objetivos repetitivos
  - envolver contexto narrativo e recompensas mais relevantes

---

## 4. Fase 4 — UX e onboarding

### Tarefa 4.1 — Criar onboarding inicial
- Prioridade: Alta
- Estimativa: 2 dias
- Arquivos: [src/Game.cpp](src/Game.cpp), [src/Game.h](src/Game.h)
- Ação:
  - introduzir 3 a 5 passos guiados no início do jogo
  - mostrar o loop principal

### Tarefa 4.2 — Reduzir ruído do HUD
- Prioridade: Alta
- Estimativa: 2 dias
- Arquivos: [src/Game.cpp](src/Game.cpp)
- Ação:
  - organizar painel de status e informações prioritárias
  - reduzir sobrecarga visual

### Tarefa 4.3 — Melhorar feedback de eventos
- Prioridade: Média
- Estimativa: 2 dias
- Arquivos: [src/Game.cpp](src/Game.cpp), [src/Companion.cpp](src/Companion.cpp)
- Ação:
  - adicionar notificações para missão nova, save, loot, nível e companheiro

### Tarefa 4.4 — Melhorar inventário e equipamento
- Prioridade: Média
- Estimativa: 2 a 3 dias
- Arquivos: [src/Game.cpp](src/Game.cpp), [src/Item.cpp](src/Item.cpp)
- Ação:
  - melhorar comparação visual e clareza de benefícios

---

## 5. Fase 5 — Renderização e performance

### Tarefa 5.1 — Otimizar tilemap
- Prioridade: Alta
- Estimativa: 3 a 4 dias
- Arquivos: [src/Tilemap.cpp](src/Tilemap.cpp), [src/Tilemap.h](src/Tilemap.h)
- Ação:
  - introduzir chunking ou batching de tiles
  - reduzir custo de desenho por frame

### Tarefa 5.2 — Reduzir custo de iluminação
- Prioridade: Alta
- Estimativa: 2 dias
- Arquivos: [src/LightSystem.cpp](src/LightSystem.cpp)
- Ação:
  - limitar número de luzes ativas por câmera
  - reduzir resolução ou passos da máscara

### Tarefa 5.3 — Controlar partículas
- Prioridade: Alta
- Estimativa: 2 dias
- Arquivos: [src/Particle.cpp](src/Particle.cpp), [src/Game.cpp](src/Game.cpp)
- Ação:
  - implementar limite global e culling por câmera
  - reduzir efeitos excessivos em combates pequenos

### Tarefa 5.4 — Melhorar LOD visual
- Prioridade: Média
- Estimativa: 2 dias
- Arquivos: [src/Background.cpp](src/Background.cpp), [src/Item.cpp](src/Item.cpp)
- Ação:
  - simplificar efeitos de objetos distantes
  - reduzir custo visual para decorativos e drops

---

## 6. Fase 6 — Mundo aberto e exploração

### Tarefa 6.1 — Criar hubs e rotas entre zonas
- Prioridade: Média/Alta
- Estimativa: 3 dias
- Arquivos: [src/Tilemap.cpp](src/Tilemap.cpp), [src/Game.cpp](src/Game.cpp), [src/Zone.h](src/Zone.h)
- Ação:
  - estruturar o mapa como rede com hubs e trilhas

### Tarefa 6.2 — Adicionar POIs com propósito
- Prioridade: Média
- Estimativa: 3 dias
- Arquivos: [src/Game.cpp](src/Game.cpp), [src/Background.cpp](src/Background.cpp)
- Ação:
  - criar pontos de interesse relevantes para exploração

### Tarefa 6.3 — Tornar eventos dinâmicos mais relevantes
- Prioridade: Média
- Estimativa: 2 a 3 dias
- Arquivos: [src/AnomalyPortal.cpp](src/AnomalyPortal.cpp), [src/InfernoZone.cpp](src/InfernoZone.cpp)
- Ação:
  - fazer portais e eventos alterarem o mapa e a navegação

---

## 7. Fase 7 — IA e comportamento

### Tarefa 7.1 — Criar camada simples de tomada de decisão
- Prioridade: Alta
- Estimativa: 3 dias
- Arquivos: [src/Enemy.cpp](src/Enemy.cpp), [src/Companion.cpp](src/Companion.cpp)
- Ação:
  - implementar estados simples de combate e suporte

### Tarefa 7.2 — Integrar pathfinding aos agentes
- Prioridade: Alta
- Estimativa: 2 a 3 dias
- Arquivos: [src/BotController.cpp](src/BotController.cpp), [src/Enemy.cpp](src/Enemy.cpp), [src/Companion.cpp](src/Companion.cpp)
- Ação:
  - usar o pathfinding já existente em movimentos de inimigos e companions

### Tarefa 7.3 — Melhorar bosses com fases e telegraph
- Prioridade: Média
- Estimativa: 3 dias
- Arquivos: [src/Enemy.cpp](src/Enemy.cpp)
- Ação:
  - adicionar padrões mais ricos e reação ao player

---

## 8. Fase 8 — Multiplayer e networking

### Tarefa 8.1 — Definir protocolo de mensagens
- Prioridade: Alta
- Estimativa: 2 dias
- Arquivos: [src/NetClient.cpp](src/NetClient.cpp), [src/NetClient.h](src/NetClient.h)
- Ação:
  - criar tipos explícitos de mensagem e versão de protocolo

### Tarefa 8.2 — Implementar autenticação e validação no servidor
- Prioridade: Alta
- Estimativa: 3 dias
- Arquivos: [server/game-server/src/index.js](server/game-server/src/index.js), [server/README.md](server/README.md)
- Ação:
  - validar sessão e payload de mensagens
  - proteger o servidor contra abuso

### Tarefa 8.3 — Criar lobbies e matchmaking simples
- Prioridade: Média
- Estimativa: 3 dias
- Arquivos: [server/game-server/src/index.js](server/game-server/src/index.js)
- Ação:
  - criar salas, join/leave, ready e início de partida

---

## 9. Ordem sugerida de execução

1. Estabilidade e build
2. Save/load
3. Arquitetura e modularização
4. Gameplay e progression
5. UX e onboarding
6. Renderização e performance
7. Mundo aberto e IA
8. Multiplayer

---

## 10. Critério de conclusão de cada tarefa

Uma tarefa pode ser considerada concluída quando:
- foi implementada de forma objetiva
- não causou regressão clara no fluxo principal
- foi validada de forma simples
- ficou documentada se houve mudança relevante de comportamento
