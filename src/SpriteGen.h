#pragma once
#include <raylib.h>

// ─────────────────────────────────────────────────────────────────────────────
// SpriteBank — generates arte pixel-art proceduralmente as Texture2D in the inicializacao.
// Tudo drawn via Image API of the raylib (without arquivos externos), after cacheado
// as texture. Substitui the formas geometricas by sprites of verdade.
//
// Uso:
//   SpriteBank::get().init();      // UMA vez, apos InitWindow()
//   SpriteBank::get().shutdown();  // to the close
//   DrawTexture... usando the handles publicos
// ─────────────────────────────────────────────────────────────────────────────

struct SpriteBank {
    // Tiles of floor by biome (64x64). Index = ZoneID.
    static const int NUM_ZONE_TILES = 11;
    Texture2D tileFloor[NUM_ZONE_TILES];   // floor
    Texture2D tileWall [NUM_ZONE_TILES];   // wall/obstaculo

    // Player — 4 directions x frames of caminhada. Layout: [dir][frame]
    // dir: 0=down 1=up 2=left 3=right ; frames: 0=idle,1-2 walk
    static const int PLAYER_DIRS   = 4;
    static const int PLAYER_FRAMES = 4;
    Texture2D player[PLAYER_DIRS][PLAYER_FRAMES];

    // Enemies — 1 texture by EnemyType (with frames of animation idle)
    static const int NUM_ENEMY_TYPES = 64;   // cobre all the EnemyType with folga
    static const int ENEMY_FRAMES    = 2;    // 2 frames of "respiracao"/anim
    Texture2D enemy[NUM_ENEMY_TYPES][ENEMY_FRAMES];

    // Scenario — 11 types (same indice of the SceneryObject.type), 3 variantes cada
    static const int NUM_SCENERY      = 11;
    static const int SCENERY_VARIANTS = 3;
    Texture2D scenery[NUM_SCENERY][SCENERY_VARIANTS];

    // Avatares (retratos) of the classes of character — index = CharacterClass
    static const int NUM_CHAR_AVATARS = 6;
    Texture2D charAvatar[NUM_CHAR_AVATARS];

    bool ready = false;

    static SpriteBank& get();
    void init();
    void shutdown();

    // Helpers of geracao (Image -> Texture)
    void buildTiles();
    void buildPlayer();
    void buildEnemies();
    void buildScenery();
    void buildCharAvatars();
};
