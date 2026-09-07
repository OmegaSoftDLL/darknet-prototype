# DARKNET — Distribution (Steam + Epic Games)

The game will be sold on **Steam** and **Epic Games Store**. Below what is ready
in the project and what depends on external accounts/registrations.

## Model
- C++ client (raylib) — the game itself.
- **Cyber Station** (folder `server/`) — multiplayer backend + gem store (free-to-play
  with microtransactions). On PC via Steam/Epic, gem purchases can use Stripe
  directly; on consoles/mobile they use platform billing.

## Steam (Steamworks)
1. **Steamworks** Account (Steam Direct fee **US$100** per app, recoverable).
2. Receive the **App ID**. Create `steam_appid.txt` (in the .exe dir in dev).
3. Integrate the **Steamworks SDK** (overlay, achievements, cloud saves, light DRM, friends/lobby
   for P2P multiplayer). See `integration/steam/` (wrapper stub).
4. Package build via **SteamPipe** (`steamcmd` + depot scripts).
5. Store page, languages, age rating, price, screenshots/trailer.

## Epic Games Store (EOS)
1. **Epic developer account** (free) + product/sandbox/deployment IDs.
2. Integrate **Epic Online Services (EOS) SDK** (auth, achievements, lobbies/matchmaking,
   P2P) — works cross-platform including with Steam.
3. Package via **Epic BuildPatchTool**.
4. Store Page, Epic Review.

## What ALREADY exists in the repo
- `server/` — containerized backend (realtime multiplayer + store). See `server/README.md`.
- `integration/steam/SteamIntegration.*` — STUB wrapper to link the Steamworks SDK when
  you have the App ID and download the SDK (you cannot redistribute the SDK in the repo).
- `integration/build/` — packaging notes (SteamPipe / Epic BPT).

## Honest: what depends on you
- Buy/create accounts (Steam US$100, Epic free) and get IDs.
- Download the SDKs (Steamworks / EOS) — license does not allow me to include them here.
- Submit builds and pages for store review.
- For multiplayer "see other players": use Steam/EOS lobbies (recommended, without
  own server) OR Cyber Station (dedicated server). I have already left a network client
  LAN/UDP in game to test locally without any account.