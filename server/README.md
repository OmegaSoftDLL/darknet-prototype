# DARKNET — Cyber Station (game server)

DARKNET backend in **Docker containers**: real-time multiplayer, accounts, store with
premium currency (gems purchased with real money), inventory and room matchmaking.

## Start the containers
```bash
cd server
docker compose up -d --build
# API:  http://localhost:8080/api/healthz
# WS :  ws://localhost:8080/ws
```
Containers: **gateway** (nginx, public port 8080) · **game-server** (Node, internal port 9000) · **db** (Postgres).

## Endpoints (via gateway: `http://localhost:8080/api/...`; directly on the Node: `http://localhost:9000/...`)

| Method | Route | Description |
|--------|------|-----------|
| POST | `/auth/login` | Login stub (name) → returns `{ token, id }` (JWT) |
| GET | `/store` | Store catalog (gem + item packs) |
| POST | `/store/buy-gems` | Create Stripe Checkout Session → `{ url }` (auth) |
| POST | `/store/buy-item` | Buy item with gems, validated on the server (auth) |
| POST | `/store/webhook` | Stripe signed webhook — the ONLY source that credits gems |
| POST | `/store/dev-grant-gems` | DEV ONLY: credits gems without payment (requires `ALLOW_DEV_GRANT=1`) |
| GET | `/me` | Profile: gems + inventory (auth) |
| GET/POST | `/progress` | Read/write account progress (level, credits, save_json) (auth) |
| GET | `/healthz` | Health check (Stripe and database status) |
| GET | `/store/success`, `/store/cancel` | Stripe Checkout Return Pages |
| WS | `/ws` | Realtime: player synchronization, chat, rooms (memory matchmaking) |

## What is ALREADY ready (functional skeleton)
- `game-server`: REST (table above) + **WebSocket** (`/ws`) with position
  synchronization between players, chat and rooms (simple matchmaking in memory).
- Bank with tables of accounts, inventory (with `qty` aggregated per item), transactions
  and progress. **Canonical DDL in `game-server/src/index.js`** (`CREATE TABLE IF NOT
  EXISTS` on every boot); `db/init.sql` is just a pointer.
- Store catalog: packages of **gems** (R$) and items (cosmetics/boosts) purchased with gems.
- **free-to-play** model: free game; revenue from gems (cosmetics + convenience).

## What is MISSING to become production (needs you / external accounts)
This **isn't** something I can "implement" on my own — it requires registration, money, and legal work:
1. **Actual payment**: create a **Stripe** account (or Mercado Pago/Google Play/Apple) and fill out
   `STRIPE_SECRET_KEY` / `STRIPE_WEBHOOK_SECRET`. Gems are only credited via **webhook
   confirmed** server-side (never trust the client). Without this, `/store/buy-gems` is stub.
2. **Hosting**: deploy containers to a cloud provider (Fly.io, Railway, AWS, GCP) with
   **TLS/HTTPS** and a domain. "Cyber Station" becomes this deployment.
3. **Secure accounts**: email+hashed password (bcrypt/argon2) or OAuth, verification, reset.
4. **Anti-cheat / anti-fraud**: server-side validation of EVERY action that results in a paid item,
   rate limiting, abuse detection, refunds.
5. **Legal compliance**: terms of use, privacy (LGPD), taxes, store rules
   (Apple App Store / Google Play charge commission and require billing on mobile).
6. **C++ client integration**: the game (raylib) needs a network module to talk
   with API/WS (login, download store, synchronize players). Today the client is offline.

## Suggested next steps (order)
1. Run the local containers and test `/api/healthz` + connect 2 clients to `/ws`.
2. Integrate a simple network module into the client (login + see other players in the lobby).
3. Create a Stripe account in test mode and connect `buy-gems` + webhook (sandbox environment).
4. Deploy on a cloud with HTTPS = "Cyber Station" online.

> Honest summary: **game client** is advanced and playable offline. The **backend
> multiplayer + monetization** has the **architecture and containers ready** here,
> but becoming an online game with real money depends on payment accounts,
> hosting and legal compliance — steps that involve you and external services.