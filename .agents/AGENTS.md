# Regras do Projeto e Memória de Auditoria

Este arquivo contém diretrizes e memórias importantes do projeto **Darknet Prototype** para orientar o comportamento dos agentes de IA neste workspace.

---

## 🧠 Memória de Auditoria Técnica (Junho 2026)

### 1. Gargalo de Renderização do Mapa (Crítico)
*   **Contexto**: O método `Tilemap::render` em [Tilemap.cpp](file:///c:/Users/ricar/darknet-prototype/src/Tilemap.cpp) desenha os 14.400 tiles (120x120) do mundo aberto a cada frame, causando sérias quedas de FPS relatadas nos testes de telemetria.
*   **Diretriz**: Qualquer futura alteração no sistema de renderização do mapa deve implementar **Frustum Culling** (limitar o loop de renderização apenas aos blocos contidos na área visível da câmera baseado em `camera.target` e `tileSize`).

### 2. Pathfinding e Travamentos do Bot
*   **Contexto**: O bot de testes fica preso por mais de 10s em cantos côncavos, especialmente em cercas e ruínas na região **CursedFarm** nas coordenadas aproximadas de tiles **tx=26, ty=49** (física: `(1664, 3166)`). O algoritmo reativo em 8 direções no [BotController.cpp](file:///c:/Users/ricar/darknet-prototype/src/BotController.cpp) oscila ciclicamente a cada 1.5s sem recuar.
*   **Diretriz**: Qualquer refatoração no movimento do bot de testes deve visar a substituição da lógica de sensores locais por um algoritmo de **Pathfinding global** (como A* ou BFS) na grade de tiles transitáveis.

### 3. Integridade do Código e Persistência
*   **Estrutura de Lojas e Diálogos**: Impedir processamento simultâneo de inputs de combate e interfaces UI.
*   **SaveManager**: O sistema possui compatibilidade com saves legados (`slot 0` com fallback para `darknet_save.txt`), a qual deve ser mantida para evitar quebra de compatibilidade com versões anteriores.

### 4. Multiplayer e Sincronização (WSL)
*   **Contexto**: O jogo deve rodar multiplayer sincronizando posições em tempo real. O backend reside em `/server` (Docker Compose).
*   **Diretriz**: A comunicação em tempo real deve ser feita integrando um WebSocket client em C++ (utilizando bibliotecas leves como `ixwebsocket`) no cliente do jogo, apontando localmente para `ws://127.0.0.1:8080/ws` no WSL durante o desenvolvimento antes da migração em nuvem.

### 5. Monetização e Transações Seguras (F2P)
*   **Contexto**: A sustentabilidade do jogo é baseada em Gems (premium) para itens estéticos/boosts.
*   **Diretriz**: O saldo de Gems e inventário devem ser mantidos e validados estritamente no backend. Compras de Gems devem ocorrer abrindo uma URL de pagamento seguro do Stripe e confirmando o saldo apenas via webhooks criptográficos assinados do Stripe no servidor, impedindo modificações unilaterais do cliente.

### 6. Polimento de Juice Visual (Diablo/Stardew/StarCraft)
*   **Contexto**: A apresentação gráfica e os feedbacks visuais devem remeter aos benchmarks visuais passados.
*   **Diretriz**: Drops raros exigem feixes de luz verticais coloridos na tela (`Item::render`). Os biomas exigem partículas suspensas (folhas na floresta, cinzas na lava) e os acertos de combate exigem efeitos de micro-delay (hit-stop), screenshake proporcional e sangue/faíscas no chão.

