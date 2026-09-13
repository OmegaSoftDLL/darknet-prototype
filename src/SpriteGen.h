#pragma once
#include <raylib.h>

// ─────────────────────────────────────────────────────────────────────────────
// SpriteBank — gera arte pixel-art proceduralmente como Texture2D na inicializacao.
// Tudo desenhado via Image API do raylib (sem arquivos externos), depois cacheado
// como textura. Substitui as formas geometricas por sprites de verdade.
//
// Uso:
//   SpriteBank::get().init();      // UMA vez, apos InitWindow()
//   SpriteBank::get().shutdown();  // ao fechar
//   DrawTexture... usando os handles publicos
// ─────────────────────────────────────────────────────────────────────────────

struct SpriteBank {
    // Tiles de chao por bioma (64x64). Index = ZoneID.
    static const int NUM_ZONE_TILES = 11;
    Texture2D tileFloor[NUM_ZONE_TILES];   // chao
    Texture2D tileWall [NUM_ZONE_TILES];   // parede/obstaculo

    // Player — 4 direcoes x frames de caminhada. Layout: [dir][frame]
    // dir: 0=down 1=up 2=left 3=right ; frames: 0=idle,1-2 walk
    static const int PLAYER_DIRS   = 4;
    static const int PLAYER_FRAMES = 4;
    Texture2D player[PLAYER_DIRS][PLAYER_FRAMES];

    // Inimigos — 1 textura por EnemyType (com frames de animacao idle)
    static const int NUM_ENEMY_TYPES = 64;   // cobre todos os EnemyType com folga
    static const int ENEMY_FRAMES    = 2;    // 2 frames de "respiracao"/anim
    Texture2D enemy[NUM_ENEMY_TYPES][ENEMY_FRAMES];

    // Cenario — 11 tipos (mesmo indice do SceneryObject.type), 3 variantes cada
    static const int NUM_SCENERY      = 11;
    static const int SCENERY_VARIANTS = 3;
    Texture2D scenery[NUM_SCENERY][SCENERY_VARIANTS];

    // Avatares (retratos) das classes de personagem — index = CharacterClass
    static const int NUM_CHAR_AVATARS = 6;
    Texture2D charAvatar[NUM_CHAR_AVATARS];

    bool ready = false;

    static SpriteBank& get();
    void init();
    void shutdown();

    // Helpers de geracao (Image -> Texture)
    void buildTiles();
    void buildPlayer();
    void buildEnemies();
    void buildScenery();
    void buildCharAvatars();
};
