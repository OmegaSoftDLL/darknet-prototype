# DARKNET — Distribuição (Steam + Epic Games)

O jogo será vendido na **Steam** e na **Epic Games Store**. Abaixo o que está pronto
no projeto e o que depende de contas/cadastros externos.

## Modelo
- Cliente C++ (raylib) — o jogo em si.
- **Cyber Station** (pasta `server/`) — backend multiplayer + loja de gems (free-to-play
  com microtransações). No PC via Steam/Epic, as compras de gems podem usar Stripe
  diretamente; em consoles/mobile usam o billing da plataforma.

## Steam (Steamworks)
1. Conta **Steamworks** (taxa Steam Direct **US$100** por app, recuperável).
2. Receber o **App ID**. Criar `steam_appid.txt` (no dir do .exe em dev).
3. Integrar o **Steamworks SDK** (overlay, conquistas, cloud saves, DRM leve, friends/lobby
   para multiplayer P2P). Ver `integration/steam/` (stub de wrapper).
4. Empacotar build via **SteamPipe** (`steamcmd` + scripts de depot).
5. Página da loja, idiomas, classificação etária, preço, screenshots/trailer.

## Epic Games Store (EOS)
1. Conta de **desenvolvedor Epic** (gratuita) + product/sandbox/deployment IDs.
2. Integrar **Epic Online Services (EOS) SDK** (auth, achievements, lobbies/matchmaking,
   P2P) — funciona cross-platform inclusive com Steam.
3. Empacotar via **Epic BuildPatchTool**.
4. Página da loja, revisão da Epic.

## O que JÁ existe no repo
- `server/` — backend em containers (multiplayer realtime + loja). Ver `server/README.md`.
- `integration/steam/SteamIntegration.*` — wrapper STUB para ligar o Steamworks SDK quando
  você tiver o App ID e baixar o SDK (não dá para redistribuir o SDK no repo).
- `integration/build/` — notas de empacotamento (SteamPipe / Epic BPT).

## Honesto: o que depende de você
- Comprar/criar as contas (Steam US$100, Epic grátis) e obter os IDs.
- Baixar os SDKs (Steamworks / EOS) — licença não permite eu incluí-los aqui.
- Submeter builds e páginas para revisão das lojas.
- Para multiplayer "ver outros jogadores": usar lobbies do Steam/EOS (recomendado, sem
  servidor próprio) OU o Cyber Station (servidor dedicado). Já deixei um cliente de rede
  LAN/UDP no jogo para testar localmente sem nenhuma conta.
