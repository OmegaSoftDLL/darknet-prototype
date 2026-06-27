# Roadmap Executivo para o Claude Code

## Objetivo
Transformar o Darknet Prototype de um protótipo avançado com muitos sistemas em uma base mais madura, estável e preparada para evolução de gameplay, performance, UX e multiplayer.

## Estratégia geral
Priorizar evolução em 4 eixos:
1. Estabilidade técnica e build
2. Arquitetura e modularização
3. Loop principal e experiência do jogador
4. Escalabilidade para mundo aberto, IA e multiplayer

---

## Fase 1 — Estabilizar a base (Semana 1 a 2)

### Objetivo
Garantir que o projeto seja compilável, testável e com um fluxo de desenvolvimento previsível.

### Tarefas
- Corrigir ou documentar o ambiente de build local
- Garantir que o projeto rode corretamente com CMake e toolchain adequados
- Criar um script simples de build e smoke test
- Formalizar um fluxo mínimo de validação antes de cada mudança

### Entregáveis
- Build reproduzível localmente
- Script de validação básica
- Checklist de QA para mudanças críticas

### Responsável sugerido
- game-tools-qa-specialist
- game-engine-architect

---

## Fase 2 — Reduzir o acoplamento do fluxo principal (Semana 2 a 3)

### Objetivo
Tirar parte da lógica do núcleo do jogo da classe principal e organizar os sistemas em módulos.

### Tarefas
- Extrair contexto de jogo e estado global
- Separar sistemas de gameplay em módulos claros
- Mover spawn, transições de zona e eventos globais para módulos específicos
- Reduzir a dependência direta de [src/Game.cpp](src/Game.cpp) para lógica secundária

### Entregáveis
- Estrutura de sistemas mais clara
- Menor acoplamento entre gameplay, UI e estado do jogo
- Código mais fácil de evoluir

### Responsável sugerido
- game-engine-architect

---

## Fase 3 — Melhorar o loop principal e progressão (Semana 3 a 5)

### Objetivo
Tornar o jogo mais convincente para o jogador, com combate mais estratégico e progressão mais clara.

### Tarefas
- Reorganizar inimigos por papéis táticos
- Fazer as classes do player terem diferenças mais fortes de estilo de jogo
- Tornar a progression de loot, crafting e loja mais coerente
- Melhorar o valor das quests e recompensas
- Integrar companions como parte do planejamento do combate

### Entregáveis
- Combate mais legível e satisfatório
- Progressão econômica e de poder mais clara
- Companions com papel estratégico real

### Responsável sugerido
- game-systems-specialist
- game-ai-specialist
- game-ui-ux-specialist

---

## Fase 4 — Melhorar UX e onboarding (Semana 4 a 5)

### Objetivo
Tornar a experiência do jogador menos confusa e mais guiada.

### Tarefas
- Criar onboarding inicial de 3 a 5 passos
- Melhorar HUD com hierarquia de informação
- Tornar missão, save, loot e loja mais claros
- Adicionar feedback visual para eventos importantes
- Melhorar a leitura de inventário e equipamento

### Entregáveis
- Tutorial guiado simples e eficiente
- HUD menos carregado e mais intuitivo
- Melhor compreensão do loop principal pelo jogador

### Responsável sugerido
- game-ui-ux-specialist

---

## Fase 5 — Evoluir renderização e performance (Semana 5 a 6)

### Objetivo
Manter o visual rico sem perder desempenho em mapas grandes e combates intensos.

### Tarefas
- Otimizar o tilemap com chunks ou batching
- Reduzir custo de iluminação e partículas
- Introduzir LOD para itens, decorativos e eventos visuais
- Controlar o número de efeitos ativos por frame

### Entregáveis
- Melhor FPS e estabilidade em mapas abertos
- Menor custo de renderização em situações intensas
- Visual mais polido sem regressão de performance

### Responsável sugerido
- game-render-specialist

---

## Fase 6 — Expandir mundo aberto e exploração (Semana 6 a 8)

### Objetivo
Construir uma sensação mais forte de exploração e mundo vivo.

### Tarefas
- Criar hubs e rotas entre zonas
- Adicionar POIs com propósito claro
- Tornar eventos dinâmicos mais relevantes espacialmente
- Melhorar landmarks visuais e legibilidade do mapa
- Introduzir um fluxo de descoberta mais natural

### Entregáveis
- Mapa mais legível e navegável
- Exploração com mais sentido e recompensa
- Mundo mais vivo e memorável

### Responsável sugerido
- game-open-world-specialist

---

## Fase 7 — Evoluir IA e comportamento dos agentes (Semana 7 a 9)

### Objetivo
Tornar inimigos, companions e bosses mais inteligentes e táticos.

### Tarefas
- Unificar a tomada de decisão em um modelo simples de estados
- Integrar pathfinding ao comportamento de inimigos e companions
- Dar papéis claros aos companions
- Melhorar bosses com fases, telegraph e reação ao player

### Entregáveis
- Combate mais vivo e estratégico
- IA mais consistente e escalável
- Bosses mais memoráveis

### Responsável sugerido
- game-ai-specialist

---

## Fase 8 — Preparar multiplayer e rede (Semana 8 a 10)

### Objetivo
Levar o projeto de um protótipo local para uma base mais realista de rede.

### Tarefas
- Melhorar protocolo de mensagens
- Implementar autenticação e validação do servidor
- Criar fluxo de lobbies e matchmaking simples
- Definir sincronização autoritativa de estado
- Preparar integração com identidade externa

### Entregáveis
- Base mais sólida de multiplayer
- Menor risco de desync e abuso
- Preparação para online real

### Responsável sugerido
- game-network-specialist

---

## Critérios de sucesso do roadmap

O projeto pode ser considerado em evolução saudável quando:
- o build é reproduzível
- o gameplay principal é claro e divertido
- a experiência do jogador é guiada e intuitiva
- há melhoria de performance em mapas e combate
- a arquitetura ficou mais modular
- há uma base de QA e regressão mínima

---

## Ordem recomendada de execução

1. Estabilizar build e QA
2. Reduzir acoplamento da arquitetura
3. Melhorar gameplay e progressão
4. Melhorar UX e onboarding
5. Otimizar renderização
6. Evoluir mundo aberto
7. Evoluir IA
8. Preparar multiplayer

---

## Prompt pronto para o Claude Code

Use este texto como handoff direto:

“Continue o desenvolvimento do Darknet Prototype como um protótipo avançado de ARPG 2D em C++ com raylib. Priorize estabilidade técnica, modularização, melhoria do loop principal, UX, performance e preparação para expansão de mundo aberto e multiplayer. Preserve o gameplay atual, trabalhe por módulos, reduza o acoplamento do fluxo principal, melhore combate/progression/companions, otimize renderização e introduza validação básica de build e regressão. Use a arquitetura já existente, mantenha compatibilidade com o projeto atual e entregue mudanças incrementais com validação objetiva.”
