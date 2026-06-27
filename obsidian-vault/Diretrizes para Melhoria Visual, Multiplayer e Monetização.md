# Diretrizes de Evolução: Visuais (Estilo Diablo/Stardew/StarCraft), Multiplayer (WSL) e Monetização

Este guia contém as especificações técnicas, designs de sistemas e passos detalhados para orientar o **Claude Code** (ou outro agente de desenvolvimento) na transformação do protótipo atual em um jogo completo, polido, multiplayer e monetizado.

---

## 🚀 PARTE 1: POLIMENTO VISUAL E GAMEPLAY (JUICE)

Para elevar o jogo do estado cru ("cru") para o nível de jogos consagrados como **Diablo 3**, **Path of Exile**, **Stardew Valley** e **StarCraft**, é preciso adicionar "juice" (feedback audiovisual intensivo):

### 1.1 Efeito de Feixe de Luz para Drops Raros (Inspiração: Diablo 3)
*   **O que fazer**: Quando um item de raridade **Rara**, **Épica**, **Lendária** ou **Omega** cair no chão, desenhe um feixe de luz vertical brilhante subindo do item em direção ao topo da tela, acompanhado de partículas flutuantes.
*   **Implementação em `Item::render()`**:
    *   Use `DrawRectangleGradientV` ou `DrawTriangle` com cores semi-transparentes correspondentes à raridade do item (Dourado/Laranja para Lendário, Roxo para Épico, Vermelho para Omega).
    *   Adicione pequenas partículas de luz ascendentes (partículas circulares suaves com fade out vertical) que flutuam ao redor do item.
    *   Adicione um anel pulsante de luz no solo (usando `DrawCircleLines` com alpha variável).

### 1.2 Clima e Partículas Ambientais (Inspiração: Stardew Valley & Diablo)
*   **Efeitos de Bioma**:
    *   **Floresta Negra (DarkForest)**: Adicione folhas caindo lentamente com vento lateral, usando uma oscilação senoidal simples na rotação e no deslocamento horizontal.
    *   **Zona Infernal (InfernoZone)**: Insira cinzas e faíscas incandescentes (partículas laranjas que sobem rapidamente com pequenos desvios horizontais em zigue-zague).
    *   **Ruínas / Cidade Fantasma**: Adicione névoa rasteira ou poeira flutuante e vento.
*   **Feedback de Impacto (Juice de Combate)**:
    *   **Hit-Stop (Freeze-Frame)**: Pause temporariamente a atualização de posições do jogo por 0.05 a 0.1 segundos (cerca de 3 a 6 frames) quando o jogador acertar um golpe crítico ou sofrer um dano massivo.
    *   **Stains de Sangue / Faíscas**: Inimigos orgânicos devem deixar manchas de sangue permanentes (ou que duram 10 segundos) no chão ao morrer. Inimigos mecânicos devem disparar faíscas elétricas metálicas.

### 1.3 Iluminação Dinâmica Suave e Sombras
*   **Melhoria em `LightSystem.cpp`**:
    *   Atualmente, as luzes são cortadas como círculos puros. Implemente uma atenuação suave (radial decay) usando shaders de fragmento simples no RenderTexture de luz ou desenhando círculos concêntricos com opacidade decrescente (Gradiente Radial).
    *   Adicione sombras projetadas (estruturas e paredes bloqueiam a luz, gerando sombras pretas projetadas na direção oposta ao jogador ou tochas).

### 1.4 Tooltips de Inventário Avançados (Inspiração: Path of Exile)
*   **Descrição**: Ao passar o mouse sobre um item no inventário ou equipamento, exiba uma janela flutuante detalhada descrevendo:
    *   Nome colorido de acordo com a raridade.
    *   Atributos detalhados (ex.: `+12% Dano Crítico`, `+5 Regeneração de Vida por Segundo`).
    *   Lore/História de sabor em itálico na cor cinza escuro.

---

## 🌐 PARTE 2: MULTIPLAYER ONLINE (WSL LOCAL -> CLOUD)

O jogo usará o servidor Node.js local (Cyber Station) rodando no Docker via WSL e depois migrará para servidores em nuvem.

### 2.1 Conectando o Cliente C++ (raylib) ao Servidor
Para que o jogo offline se torne multiplayer, precisamos adicionar uma biblioteca de rede em C++ no projeto:
1.  **Escolha de Biblioteca**: Recomenda-se integrar a **`ixwebsocket`** (C++ WebSocket client leve e sem dependências pesadas) ou a **`easywsclient`** no `CMakeLists.txt`.
2.  **Módulo de Rede (`src/Network.h` / `src/Network.cpp`)**:
    *   Crie uma classe `NetworkManager` que gerencia a conexão em background (usando uma thread separada para não bloquear a renderização da raylib).
    *   Métodos essenciais: `connect(string url)`, `sendState(float x, float y, int animFrame, int action)`, `sendChat(string text)`.

### 2.2 Sincronização em Tempo Real via WebSocket (`/ws`)
*   **Protocolo de Mensagens (JSON)**:
    *   **Envio de Estado (Client -> Server)**: A cada frame (ou com rate-limiting de 30 updates por segundo), envie a posição do jogador:
        ```json
        { "t": "state", "id": "u_player123", "x": 1280.0, "y": 720.0, "a": "idle_0" }
        ```
    *   **Recebimento de Peers (Server -> Client)**: O servidor retransmite os dados para a sala. O cliente lê a mensagem e atualiza as posições dos "outros jogadores" (Peers):
        ```json
        { "t": "peer", "id": "u_player456", "x": 1320.0, "y": 710.0, "a": "run_2" }
        ```
*   **Renderização de Outros Jogadores**:
    *   A classe `Game` deve manter um mapa de outros jogadores ativos: `std::map<std::string, PlayerPeer> otherPlayers;`.
    *   Em `Game::update`, atualize a posição desses peers (aplique uma interpolação linear suave (LERP) para evitar engasgos de conexão).
    *   Em `Game::render`, desenhe a sprite dos peers com os dados de suas respectivas classes e animações.

### 2.3 Rodando Localmente no WSL
1.  **WSL setup**: Instale o Docker e o Docker Compose no WSL2.
2.  **Subir o Servidor**:
    *   Abra o WSL, vá até a pasta `/server` do projeto e rode: `docker compose up -d --build`
3.  **Conexão local**: Aponte a URL do WebSocket client do jogo C++ para `ws://127.0.0.1:8080/ws` (através do Nginx gateway) ou `ws://127.0.0.1:9000/ws` (direto no Node).
4.  **Matchmaking e Salas**: Use o parâmetro `ws.roomId` no servidor para separar os jogadores em instâncias (ex.: cada zona do mundo aberto pode ser uma "sala" no WebSocket).

---

## 💰 PARTE 3: MONETIZAÇÃO F2P (FREE-TO-PLAY) E SEGURANÇA

Para o jogo se manter financeiramente, adotaremos o modelo F2P com **Gems (Moeda Premium)** para cosméticos e conveniência, garantindo conformidade de transações.

### 3.1 Loja Premium e Gems (Interface e Fluxo)
*   **Loja no Cliente**: Crie uma aba "Loja Premium" na interface de diálogo com o NPC Merchant. Ela exibirá o catálogo obtido dinamicamente da API REST `/store` do servidor Node.js.
*   **Gems**: Exiba o saldo de Gems do jogador na interface do HUD.

### 3.2 Fluxo de Pagamento Seguro com Stripe (Webhooks)
Gems representam dinheiro real; portanto, o cliente nunca pode ditar o saldo de Gems. Todo o processo deve ser validado pelo servidor:

```
[Cliente C++]                    [Gateway Node.js]                  [Stripe API]
      │                                  │                                │
      ├─────── 1. Comprar Gems ─────────>│                                │
      │        (POST /store/buy-gems)    │                                │
      │                                  ├─────── 2. Criar Intent ───────>│
      │                                  │        (retorna client_secret) │
      │<────── 3. Retorna URL stripe ────┤                                │
      │        (abre no navegador)       │                                │
      │                                  │                                │
     [Jogador paga no navegador]         │                                │
      │                                  │<────── 4. Webhook pago ────────┤
      │                                  │        (signature check ok)    │
      │                                  ├─ 5. Credita Gems no banco ────┐│
      │                                  │<──────────────────────────────┘│
      ├─────── 6. Atualiza saldo ───────>│                                │
      │        (GET /me)                 │                                │
```

1.  **Compra**: O jogador clica em "Comprar Gems" no cliente. O jogo abre o navegador do usuário apontando para a rota de checkout/checkout do Stripe gerada no servidor.
2.  **Confirmação**: O jogador realiza o pagamento em segurança no site do Stripe.
3.  **Webhook (Crítico)**: O Stripe envia um webhook assinado para `/store/webhook` na Cyber Station. O servidor valida a assinatura, confere se o ID da transação é legítimo e adiciona os Gems correspondentes na conta do jogador no Postgres.
4.  **Atualização**: O cliente C++ faz um pull do endpoint `/me` e atualiza o saldo de Gems do jogador em tela.

### 3.3 Itens e Cosméticos
*   Itens cosméticos (ex.: *Skin Neon*, *Skin Dragão*, *Pet Drone*) devem ser comprados gastando Gems através da chamada segura `POST /store/buy-item`.
*   O servidor verifica se o jogador possui Gems suficientes, debita a quantidade, adiciona o item ao inventário da conta e retorna o sucesso.
*   Ao equipar um cosmético comprado, o cliente renderiza a sprite do jogador utilizando a paleta de cor customizada fornecida pelo banco de dados do peer.

---

## 📋 PLANO DE AÇÃO PARA O CLAUDE CODE (ORDEM DE EXECUÇÃO)

1.  **Etapa 1: Correção de Bugs de Base**
    *   Implementar *Frustum Culling* em `Tilemap::render()`.
    *   Implementar *A* Pathfinding* para desvio de paredes do bot em `BotController.cpp`.
2.  **Etapa 2: Instalação e Testes do Servidor no WSL**
    *   Rodar `docker compose up` na pasta `/server` e confirmar o funcionamento de `/healthz`.
    *   Implementar banco de dados Postgres no servidor para persistir contas, Gems e inventários.
3.  **Etapa 3: Módulo de Rede em C++**
    *   Adicionar biblioteca WebSocket (ex.: `ixwebsocket`) no CMake.
    *   Criar o sistema de conexão, envio e recebimento de posições de peers.
4.  **Etapa 4: Juicing Visual e Feedbacks (Estilo Diablo/Stardew)**
    *   Adicionar pilares de luz dourada nos drops lendários.
    *   Adicionar efeitos de poeira, vento, faíscas e cinzas de bioma.
    *   Implementar animações adicionais de sprites e tooltips de inventário.
5.  **Etapa 5: Loja Premium e Fluxo de Gems**
    *   Integração do painel da loja com a API Node.js.
    *   Integração com webhook Stripe para compras em ambiente de testes.
