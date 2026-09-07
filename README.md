# Darknet Prototype

[![CI](https://github.com/OmegaSoftDLL/darknet-prototype/actions/workflows/ci.yml/badge.svg)](https://github.com/OmegaSoftDLL/darknet-prototype/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

A top-down action RPG prototype built in C++17 with raylib 5.5, featuring a 2.5D isometric renderer, procedural open-world zones, crafting, building, skill trees, NPCs, quests, and multiplayer groundwork.

> **Note:** Internal design documents are now available in English. The codebase, comments, and this README are the primary English-facing surfaces for contributors.

## Current Features

- **2.5D Isometric Rendering**: 3D world with voxelized entities captured from procedural 2D art (`ensureVoxel` / `drawVoxel` via `src/SpriteExtrude.cpp`).
- **Open World & Phases**: Multiple biomes (LA Ruins, Ghost City, Dark Forest, Cursed Farm, Cemetery, Bunker, Kronos Forge, Abandoned Manor, Kronos Nexus, Inferno Zone, Catacombs) with chunk streaming.
- **Combat**: WASD movement, mouse aiming, melee/ranged attacks, skills, dodge, hit-stop, screen shake, and damage numbers.
- **Progression**: Level-ups, evolution paths, skill tree with perks, equipment, crafting materials, and gem store.
- **NPCs & Quests**: Dialog system, merchants, service NPCs, and quest lines.
- **Building System**: Defensive structures (turrets, barracks, houses, walls) that produce units and generate passive income.
- **Audio**: Procedural music per zone, SFX, and ambient audio.
- **Validation**: Headless bot autotest runs in CI on every push and PR.

## Requirements

- CMake 3.14+
- C++17 compiler (GCC, Clang, or MSVC)
- Git (raylib 5.5 is fetched automatically via CMake FetchContent)

## Building

### Windows (Visual Studio / MSBuild)

```bash
cd darknet-prototype
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

### Windows / Linux / macOS (Make/Ninja)

```bash
cd darknet-prototype
mkdir build && cd build
cmake ..
cmake --build . --config Release -j
```

## Running

```bash
# Windows
./build/Release/darknet.exe

# Linux / macOS
./build/darknet
```

### Headless autotest (CI mode)

```bash
./build/Release/darknet.exe --headless --autotest --test-seconds=120 --seed=7
```

## Controls

| Key | Action |
|-----|--------|
| `WASD` / `Arrows` | Move |
| `Mouse` | Aim |
| `Left Click` | Attack / interact |
| `Right Click` | Move to cursor |
| `1-6` | Skills |
| `E` | Interact / talk / use portal |
| `I` | Inventory |
| `X` | Skill tree |
| `L` / `K` | Level-up / Evolution screen |
| `P` | Cycle zones (debug) |
| `F12` | Toggle bot |
| `ESC` | Pause / close UI |

## Validation

Every push and pull request to `main` runs the CI pipeline:

1. Build `darknet` in Release and Debug.
2. Build and run unit tests (`darknet_tests`, 26 cases / 159 assertions).
3. Run the headless bot autotest for 120 seconds with two fixed seeds.

Local validation script:

```bash
./validate.sh 120 7
./validate.sh 120 20260821
```

## Contributing

Contributions are welcome. The project is still a prototype with active technical debt, so the best way to help is:

1. **Open an issue** describing a bug, imbalance, or missing feature.
2. **Fork the repository**, create a feature branch, and keep changes focused.
3. **Run the validation gates** before opening a PR:
   - Build must pass in both Debug and Release.
   - `darknet_tests.exe` must pass.
   - `./validate.sh 120 7` and `./validate.sh 120 20260821` must print `APROVADO` / `VALIDACAO: PASSOU`.
4. **Open a pull request** against `main` with a clear description and test results.

See the audit report at `audit/2026-09-06-full-audit.md` for a detailed list of known issues and priorities.

## Getting the Word Out

To attract contributors and players:

- **Add topics** to the GitHub repository (e.g., `game`, `rpg`, `raylib`, `cpp`, `isometric`, `open-world`).
- **Enable Discussions and Issues** in the repository settings for feedback.
- **Post on relevant communities**:
  - [r/roguelikedev](https://www.reddit.com/r/roguelikedev/) / [r/gamedev](https://www.reddit.com/r/gamedev/)
  - [raylib Discord](https://discord.gg/raylib)
  - [itch.io](https://itch.io) (upload builds)
  - [Game Dev Stack Exchange](https://gamedev.stackexchange.com)
- **Create a short GIF/video** of gameplay and add it to the README.
- **Add a `CONTRIBUTING.md`** and issue templates once the project grows.

## License

This project is licensed under the MIT License — see [LICENSE](./LICENSE).

## Links

- Repository: https://github.com/OmegaSoftDLL/darknet-prototype
- CI Status: https://github.com/OmegaSoftDLL/darknet-prototype/actions
