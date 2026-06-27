# Infraestrutura e Distribuição — DARKNET

> [[DARKNET - Visão Geral]] · backend, multiplayer, monetização, lojas.

## Cyber Station (servidor) — `server/`
Containers Docker prontos:
- **gateway** (nginx) · **game-server** (Node/Express + WebSocket) · **db** (Postgres) · **cache** (Redis)
- REST: `/auth`, `/store`, `/store/buy-item`, `/me` · Realtime: `/ws` (sync de posição, chat, salas)
- Loja de **gems** (moeda premium, R$ real) — modelo **free-to-play**
- Rodar: `cd server && docker compose up -d --build`
- **Pagamento real**: só via webhook confirmado (Stripe) — precisa conta + chaves

## WSL (Windows Subsystem for Linux) — CHECKLIST PÓS-REBOOT
1. Recursos já habilitados via DISM (WSL + VirtualMachinePlatform) — **REINICIAR o PC**
2. Pós-reboot: `wsl --install -d Ubuntu` (cria usuário/senha)
3. Instalar **Docker Desktop** (com integração WSL2) OU docker dentro do Ubuntu
4. `cd /mnt/c/Users/ricar/darknet-prototype/server && docker compose up -d --build`
5. Testar: `curl http://localhost:8080/api/healthz`
6. (depois) Cliente C++ WebSocket → `ws://127.0.0.1:8080/ws`

## Steam + Epic — `DISTRIBUTION.md`
- **Steam**: Steamworks (US$100 Steam Direct), App ID, SDK, SteamPipe. Stub: `integration/steam/SteamIntegration.*`
- **Epic**: EOS SDK (grátis), BuildPatchTool
- Multiplayer recomendado: **lobbies Steam/EOS** (sem servidor próprio) OU Cyber Station

## Multiplayer LAN (cliente) — `src/NetClient.*`
- Winsock UDP broadcast (porta 45777) — jogadores na mesma rede se veem, sem conta/servidor
- API: init / sendState / poll / peers()

## Honesto: depende do usuário (não é só código)
1. Contas: Stripe (pagamento), Steam (US$100), Epic (grátis)
2. SDKs (Steamworks/EOS) — licença não permite incluir no repo
3. Hospedagem cloud + HTTPS (Fly.io/Railway/AWS) = "Cyber Station" no ar
4. Conformidade legal: LGPD, impostos, regras das lojas
5. Anti-fraude/anti-cheat server-side
