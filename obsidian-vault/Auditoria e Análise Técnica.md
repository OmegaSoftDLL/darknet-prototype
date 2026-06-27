# Relatório de Auditoria e Análise Técnica (Atualizado Junho 2026): Darknet Prototype

Este documento apresenta uma análise detalhada e auditoria completa do código-fonte, arquitetura de sistemas e desempenho técnico do projeto **Darknet Prototype** após a rodada de modificações de rede, persistência e cobrança efetuadas pelo parceiro de desenvolvimento.

---

## 1. Visão Geral da Arquitetura do Jogo

O projeto é estruturado como um ARPG Top-Down em C++ utilizando a biblioteca **raylib 5.0** para renderização 2D e áudio, compilado via **CMake 3.14+**. Ele possui três componentes principais:

1. **Cliente do Jogo (Pasta `src/` e `integration/`)**: Contém o loop de gameplay, sistemas de RPG, construção de base RTS, geração procedural, rendering, áudio e os novos clientes de rede e loja.
2. **Cyber Station (Pasta `server/`)**: Backend conteinerizado rodando Express + WebSocket Server na porta `9000` (sincronização de salas e auth) e integração real com API Stripe para compra de Gems e persistência com Postgres.
3. **Mecanismo de Testes (PowerShell e BotController)**: Permite a execução autônoma do jogo monitorando CPU, memória, travamentos, FPS e emitindo relatórios de qualidade (`bot_report.txt`).

---

## 2. Desempenho e Otimizações de IA (Auditado e Aprovado)

*   **Frustum Culling**: Mantém as chamadas de desenho limitadas ao retângulo visível da câmera em [Tilemap.cpp](file:///c:/Users/ricar/darknet-prototype/src/Tilemap.cpp), mantendo a média de FPS em **59** sob estresse.
*   **Pathfinding BFS**: O bot IA agora faz busca em largura (BFS) em uma grade de $57 \times 57$ tiles em [BotController.cpp](file:///c:/Users/ricar/darknet-prototype/src/BotController.cpp), reduzindo os eventos de travamento (stuck events) a **zero** nos relatórios finais.

---

## 3. Implementação de Multiplayer via WebSockets Nativo

Para sincronização em tempo real sem dependências externas complexas, o parceiro implementou uma solução customizada de WebSocket (RFC 6455) diretamente sobre sockets TCP Winsock em [NetClient.cpp](file:///c:/Users/ricar/darknet-prototype/src/NetClient.cpp):
*   **Worker Thread**: Roda a I/O de rede de forma paralela para não congelar o loop gráfico do jogo.
*   **Upgrade Handshake**: Conecta e envia cabeçalhos HTTP com chave Base64 para trocar protocolos com o servidor na porta `9000/ws`.
*   **Masking & Framing**: Codifica mensagens de texto no padrão RFC 6455 com chaves de máscara randômicas de 4 bytes e decodifica quadros de dados do servidor.
*   **JSON Sync**: Transmite a posição e estado do player local e reconstrói o estado dos peers para renderização em [Game.cpp:L1173](file:///c:/Users/ricar/darknet-prototype/src/Game.cpp#L1173).

---

## 4. Integração de Loja Premium (Gems) e Faturamento Stripe

*   **REST Cliente (WinHTTP)**: Criado invólucro nativo em [HttpClient.cpp](file:///c:/Users/ricar/darknet-prototype/src/HttpClient.cpp) usando a API WinHTTP do Windows para requisições GET/POST síncronas em threads de fundo via [StoreClient.cpp](file:///c:/Users/ricar/darknet-prototype/src/StoreClient.cpp).
*   **Aba Premium no Jogo**: Puxa dinamicamente pacotes e itens premium do backend Node.js. Exibe o saldo de Gems no HUD.
*   **Stripe Webhooks**: No servidor (`server/game-server/src/index.js`), o Stripe cria Checkout Sessions para compras de Gems e as confirma através de webhooks assinados com `stripe.webhooks.constructEvent` para creditar a moeda de forma segura no banco de dados.

---

## 5. Persistência de Banco de Dados Postgres

O backend orquestra conexões persistentes via pool do `pg` (node-postgres) em banco de dados contêiner PostgreSQL:
*   Inicializa a tabela `players` dinamicamente no startup.
*   Persiste nome, Gems e inventários de contas ativas.
*   Mantém fallback automático em memória caso a conexão com o banco caia.

---

## ⚠️ Pendências Críticas Identificadas (Brechas Visuais)

Embora as APIs de faturamento e sincronização estejam 100% corretas no backend, o cliente C++ apresenta falhas na aplicação visual dos cosméticos:

1.  **Skins Premium Sem Efeito**: O jogador pode comprar "Skin Neon", "Skin Dragão" e "Drone de Estimação" com Gems, porém estas skins não são de fato renderizadas ou aplicadas na sprite do player em [Player.cpp](file:///c:/Users/ricar/darknet-prototype/src/Player.cpp).
2.  **Cosmético de Loja Comum Não Aplicado**: As tintas "Red Chrome" e "Gold Plating" compradas na loja de créditos salvam `playerColor` no [ShopSystem.cpp](file:///c:/Users/ricar/darknet-prototype/src/ShopSystem.cpp) mas não alteram a coloração na renderização física do modelo.
3.  **Divergência de Esquemas**: O banco de dados no contêiner inicializa tabelas separadas (`accounts`, `inventory`) via [init.sql](file:///c:/Users/ricar/darknet-prototype/server/db/init.sql), mas o servidor Node.js manipula uma única tabela simplificada (`players`) com dados JSONB.
