# Darknet Prototype

ARPG em C++ com renderização 3D isométrica (2.5D), inspirado em Diablo e Path of Exile.

## Funcionalidades atuais

- Janela com renderização 3D isométrica (2.5D) via raylib — ativada por padrão (`render3D = true` em `src/Game.h`); as entidades são voxelizadas a partir da arte 2D procedural (`ensureVoxel`/`drawVoxel` via `src/SpriteExtrude.cpp`)
- Player se movimenta com WASD
- Câmera segue o player
- Inimigos nascem periodicamente e perseguem o player
- Ataque básico com clique esquerdo
- Inimigos causam dano ao encostar
- Respawn ao morrer
- Sistema de vida simples

## Requisitos

- CMake 3.14+
- Compilador C++17 (GCC, Clang, MSVC)
- Git (para baixar o raylib automaticamente)

## Como compilar no Windows (MinGW/Git Bash)

```bash
cd darknet-prototype
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Como compilar no Windows (Visual Studio)

```bash
cd darknet-prototype
mkdir build
cd build
cmake .. -A x64
cmake --build . --config Release
```

## Como compilar no Linux/macOS

```bash
cd darknet-prototype
mkdir build
cd build
cmake ..
make -j
```

## Executar

```bash
./darknet
```

No Windows:

```bash
Release\darknet.exe
```

## Controles

- **WASD**: mover o player
- **Clique esquerdo**: atacar inimigos dentro do alcance

## Próximos passos sugeridos

1. Sistema de animação por sprite
2. Inventário e loot
3. Skills e cooldowns
4. Sistema de quests e diálogo com NPCs
5. Mapa com tilemap e streaming de chunks
6. Áudio e efeitos visuais
