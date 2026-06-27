# DARKNET — Cyber Station (servidor do jogo)

Backend do DARKNET em **containers Docker**: multiplayer em tempo real, contas, loja com
moeda premium (gems compradas com dinheiro real), inventário e matchmaking por salas.

## Subir os containers
```bash
cd server
docker compose up -d --build
# API:  http://localhost:8080/api/healthz
# WS :  ws://localhost:8080/ws
```
Containers: **gateway** (nginx) · **game-server** (Node) · **db** (Postgres) · **cache** (Redis).

## O que JÁ está pronto (esqueleto funcional)
- `game-server`: REST (`/auth/login`, `/store`, `/store/buy-item`, `/me`) + **WebSocket** (`/ws`)
  com sincronização de posição entre jogadores, chat e salas (matchmaking simples).
- Banco com tabelas de contas, inventário, transações e progresso.
- Catálogo da loja: pacotes de **gems** (R$) e itens (cosméticos/boosts) comprados com gems.
- Modelo **free-to-play**: jogo grátis; receita por gems (cosméticos + conveniência).

## O que FALTA para virar produção (precisa de você / contas externas)
Isto **não** dá para eu "implementar" sozinho — exige cadastros, dinheiro e questões legais:
1. **Pagamento real**: criar conta **Stripe** (ou Mercado Pago/Google Play/Apple) e preencher
   `STRIPE_SECRET_KEY` / `STRIPE_WEBHOOK_SECRET`. Gems só são creditados via **webhook
   confirmado** server-side (nunca confiar no cliente). Sem isso, `/store/buy-gems` é stub.
2. **Hospedagem**: subir os containers num provedor cloud (Fly.io, Railway, AWS, GCP) com
   **TLS/HTTPS** e domínio. "Cyber Station" passa a ser esse deploy.
3. **Contas seguras**: e-mail+senha com hash (bcrypt/argon2) ou OAuth, verificação, reset.
4. **Anti-cheat / anti-fraude**: validação server-side de TODA ação que dá item pago,
   rate limiting, detecção de abuso, reembolsos.
5. **Conformidade legal**: termos de uso, privacidade (LGPD), impostos, regras das lojas
   (Apple App Store / Google Play cobram comissão e exigem o billing delas no mobile).
6. **Integração no cliente C++**: o jogo (raylib) precisa de um módulo de rede para falar
   com a API/WS (login, baixar loja, sincronizar jogadores). Hoje o cliente é offline.

## Próximos passos sugeridos (ordem)
1. Rodar os containers local e testar `/api/healthz` + conectar 2 clientes no `/ws`.
2. Integrar um módulo de rede simples no cliente (login + ver outros jogadores no lobby).
3. Criar conta Stripe em modo teste e ligar `buy-gems` + webhook (ambiente sandbox).
4. Deploy num cloud com HTTPS = "Cyber Station" no ar.

> Resumo honesto: o **cliente do jogo** está avançado e jogável offline. O **backend
> multiplayer + monetização** está com a **arquitetura e os containers prontos** aqui,
> mas virar um jogo online com dinheiro real de verdade depende de contas de pagamento,
> hospedagem e conformidade legal — passos que envolvem você e serviços externos.
