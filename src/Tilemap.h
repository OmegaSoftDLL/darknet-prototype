#pragma once
#include <raylib.h>
#include <vector>
#include <string>
#include "Zone.h"

enum class TileType {
    Floor,
    Wall,
    BrokenFloor,
    Portal
};

struct Tile {
    TileType  type = TileType::Floor;
    Rectangle rect;
    int       portalZone = -1;  // if Portal, which zone to go to
    bool      solid = false;    // colisao de cenario (predio/casa/carro) sem mudar o visual
};

struct ZonePortal {
    Vector2 position;
    ZoneID  destination;
    Color   color;
};

class Tilemap {
public:
    static constexpr int tileSize   = 64;
    // Open world grid — 3×3 blocks of 40×40 tiles each
    static constexpr int OW_COLS    = 3;
    static constexpr int OW_ROWS    = 3;
    // 128 tiles = 8192 unidades por regiao. Com 40 (2560u) o jogador atravessava
    // uma REGIAO INTEIRA em ~10s e o mundo trocava de tema/nome o tempo todo -
    // parecia teleporte, nao viagem.
    static constexpr int OW_ZONE_W  = 128;
    static constexpr int OW_ZONE_H  = 128;

    int width  = 40;
    int height = 40;
    bool   openWorld = false;
    ZoneID owLayout[OW_ROWS][OW_COLS] = {};

    std::vector<std::vector<Tile>> tiles;
    std::vector<ZonePortal>        portals;

    ZoneID currentZone = ZoneID::LARuins;

    Tilemap();

    void   generate(ZoneID zone = ZoneID::LARuins);
    void   generateOpenWorld();
    ZoneID tileZone(int tx, int ty) const;
    ZoneID biomeAtWorld(float wx, float wy) const;  // bioma INFINITO (módulo 3x3) na posição do mundo — bate com o chão
    void   render(Vector2 camTarget = {0,0}, float zoom = 1.0f) const; // frustum culling
    void   render3D(Vector2 camTarget, const Camera3D& cam3D, float aspect) const;  // 2.5D: chão (batch único) + paredes (DrawCube), com frustum culling
    bool   isWall(int x, int y) const;
    bool   isWallAtPosition(Vector2 pos) const;
    bool   isPortalAtPosition(Vector2 pos, ZoneID& outDest) const;
    Rectangle getBounds(int x, int y) const;

    // Colisao de cenario: marca como solido o entorno de uma construcao/objeto.
    void   markSolidAt(Vector2 worldPos, float radius);
    void   clearSolidAt(Vector2 worldPos, float radius);  // desmarca (fallback do portal)
    void   clearSolidFlags();   // limpa a colisao de cenario (mantem paredes)

private:
    void setTile(int x, int y, TileType type, int portalZone = -1);
    void placePortals(ZoneID zone);
    void generateRooms();
    void generateCorridor(int x1, int y1, int x2, int y2);
};
