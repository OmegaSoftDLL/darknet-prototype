#include "Tilemap.h"
#include "SpriteGen.h"
#include "rlgl.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

// â”€â”€ helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
static void fillRect(std::vector<std::vector<Tile>>& tiles,
                     int x, int y, int w, int h,
                     TileType type, int mapW, int mapH, int pz = -1)
{
    for (int dy = 0; dy < h; ++dy)
        for (int dx = 0; dx < w; ++dx) {
            int tx = x + dx, ty = y + dy;
            if (tx < 0 || tx >= mapW || ty < 0 || ty >= mapH) continue;
            tiles[ty][tx].type       = type;
            tiles[ty][tx].portalZone = pz;
        }
}

Tilemap::Tilemap() {
    tiles.resize(height, std::vector<Tile>(width));
    generate(ZoneID::LARuins);
}

// â”€â”€ setTile / getBounds â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void Tilemap::setTile(int x, int y, TileType type, int portalZone) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    tiles[y][x].type       = type;
    tiles[y][x].rect       = getBounds(x, y);
    tiles[y][x].portalZone = portalZone;
}

Rectangle Tilemap::getBounds(int x, int y) const {
    return {(float)(x * tileSize), (float)(y * tileSize),
            (float)tileSize,       (float)tileSize};
}

// â”€â”€ portal placement (shared) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void Tilemap::placePortals(ZoneID zone) {
    portals.clear();
    // Next zone â€” lower-right
    int nextIdx = ((int)zone + 1) % 4;
    ZoneID nextZone = (ZoneID)nextIdx;
    int nx = width - 6, ny = height - 6;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            setTile(nx + dx, ny + dy, TileType::Portal, nextIdx);
    portals.push_back({{(float)(nx * tileSize + tileSize / 2),
                        (float)(ny * tileSize + tileSize / 2)},
                       nextZone, getZoneInfo(nextZone).portalColor});

    // Prev zone â€” upper-left
    int prevIdx = ((int)zone - 1 + 4) % 4;
    ZoneID prevZone = (ZoneID)prevIdx;
    int px = 5, py = 5;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            setTile(px + dx, py + dy, TileType::Portal, prevIdx);
    portals.push_back({{(float)(px * tileSize + tileSize / 2),
                        (float)(py * tileSize + tileSize / 2)},
                       prevZone, getZoneInfo(prevZone).portalColor});
}

// â”€â”€ corridor helper (used by Bunker) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void Tilemap::generateCorridor(int x1, int y1, int x2, int y2) {
    int mx = x1 < x2 ? x1 : x2, px = x1 < x2 ? x2 : x1;
    for (int x = mx; x <= px; ++x)
        setTile(x, y1, TileType::Floor);
    int my = y1 < y2 ? y1 : y2, py2 = y1 < y2 ? y2 : y1;
    for (int y = my; y <= py2; ++y)
        setTile(x2, y, TileType::Floor);
}

void Tilemap::generateRooms() {
    int numRooms = GetRandomValue(4, 7);
    for (int r = 0; r < numRooms; ++r) {
        int rx = GetRandomValue(3, width - 12);
        int ry = GetRandomValue(3, height - 12);
        int rw = GetRandomValue(4, 9), rh = GetRandomValue(4, 9);
        for (int dy = 0; dy < rh; ++dy)
            for (int dx = 0; dx < rw; ++dx)
                setTile(rx + dx, ry + dy, TileType::Floor);
    }
    int cx = width / 2, cy = height / 2;
    generateCorridor(2, cy, width - 2, cy);
    generateCorridor(cx, 2, cx, height - 2);
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// ZONE GENERATION
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

static void generateLARuins(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Fill with walls (buildings)
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Wall;

    // Carve horizontal streets every 6-8 tiles
    std::vector<int> hStreets, vStreets;
    for (int y = 4; y < H - 4; y += GetRandomValue(6, 9)) {
        int sw = GetRandomValue(3, 4); // street width
        for (int dy = 0; dy < sw; ++dy)
            for (int x = 1; x < W - 1; ++x)
                tiles[y + dy][x].type = TileType::Floor;
        hStreets.push_back(y + sw / 2);
    }
    // Carve vertical streets
    for (int x = 4; x < W - 4; x += GetRandomValue(6, 9)) {
        int sw = GetRandomValue(3, 4);
        for (int dx = 0; dx < sw; ++dx)
            for (int y = 1; y < H - 1; ++y)
                tiles[y][x + dx].type = TileType::Floor;
        vStreets.push_back(x + sw / 2);
    }

    // Damage some building blocks (BrokenFloor at edges near streets)
    for (int y = 1; y < H - 1; ++y) {
        for (int x = 1; x < W - 1; ++x) {
            if (tiles[y][x].type == TileType::Wall) {
                // Check adjacency to floor
                bool adjFloor = (tiles[y-1][x].type==TileType::Floor ||
                                 tiles[y+1][x].type==TileType::Floor ||
                                 tiles[y][x-1].type==TileType::Floor ||
                                 tiles[y][x+1].type==TileType::Floor);
                if (adjFloor && GetRandomValue(0,100) < 20)
                    tiles[y][x].type = TileType::BrokenFloor;
            }
        }
    }

    // Ensure center spawn is clear
    int cx = W / 2, cy = H / 2;
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
            if (abs(dx) <= 3 && abs(dy) <= 3)
                tiles[cy+dy][cx+dx].type = TileType::Floor;
}

static void generateBunker(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Fill solid
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Wall;

    // Place 8-12 rectangular rooms
    struct Room { int x, y, w, h; };
    std::vector<Room> rooms;
    for (int attempt = 0; attempt < 40 && (int)rooms.size() < 12; ++attempt) {
        Room r;
        r.w = GetRandomValue(4, 8); r.h = GetRandomValue(3, 7);
        r.x = GetRandomValue(2, W - r.w - 2);
        r.y = GetRandomValue(2, H - r.h - 2);
        rooms.push_back(r);
        for (int dy = 0; dy < r.h; ++dy)
            for (int dx = 0; dx < r.w; ++dx)
                tiles[r.y+dy][r.x+dx].type = TileType::Floor;
    }

    // Connect consecutive rooms with 2-wide L-shaped corridors
    for (int i = 0; i + 1 < (int)rooms.size(); ++i) {
        int ax = rooms[i].x + rooms[i].w/2,   ay = rooms[i].y + rooms[i].h/2;
        int bx = rooms[i+1].x + rooms[i+1].w/2, by = rooms[i+1].y + rooms[i+1].h/2;
        // horizontal leg
        int mx = ax < bx ? ax : bx, px = ax < bx ? bx : ax;
        for (int x = mx; x <= px; ++x) {
            tiles[ay][x].type = TileType::Floor;
            if (ay+1 < H-1) tiles[ay+1][x].type = TileType::Floor;
        }
        // vertical leg
        int my = ay < by ? ay : by, py2 = ay < by ? by : ay;
        for (int y = my; y <= py2; ++y) {
            tiles[y][bx].type = TileType::Floor;
            if (bx+1 < W-1) tiles[y][bx+1].type = TileType::Floor;
        }
    }

    // Spawn center
    int cx = W/2, cy = H/2;
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
            tiles[cy+dy][cx+dx].type = TileType::Floor;
}

static void generateFactory(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Wall;

    // 4-6 large factory rooms
    struct Room { int x, y, w, h; };
    std::vector<Room> rooms;
    for (int attempt = 0; attempt < 20 && (int)rooms.size() < 6; ++attempt) {
        Room r;
        r.w = GetRandomValue(8, 13); r.h = GetRandomValue(7, 12);
        r.x = GetRandomValue(2, W - r.w - 2);
        r.y = GetRandomValue(2, H - r.h - 2);
        rooms.push_back(r);
        for (int dy = 0; dy < r.h; ++dy)
            for (int dx = 0; dx < r.w; ++dx)
                tiles[r.y+dy][r.x+dx].type = TileType::Floor;
    }

    // 1-tile maintenance corridors between rooms
    for (int i = 0; i + 1 < (int)rooms.size(); ++i) {
        int ax = rooms[i].x + rooms[i].w/2,    ay = rooms[i].y + rooms[i].h/2;
        int bx = rooms[i+1].x + rooms[i+1].w/2, by = rooms[i+1].y + rooms[i+1].h/2;
        int mx = ax < bx ? ax : bx, px = ax < bx ? bx : ax;
        for (int x = mx; x <= px; ++x) tiles[ay][x].type = TileType::Floor;
        int my = ay < by ? ay : by, py2 = ay < by ? by : ay;
        for (int y = my; y <= py2; ++y) tiles[y][bx].type = TileType::Floor;
    }

    // Spawn center
    int cx = W/2, cy = H/2;
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
            tiles[cy+dy][cx+dx].type = TileType::Floor;
}

// ── Dark Zone Generators ────────────────────────────────────────────────────

static void generateCemetery(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Winding gravel paths between grave clusters
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Wall;

    // Cross-shaped main path
    int cy = H/2, cx = W/2;
    for (int x = 1; x < W-1; ++x) { tiles[cy][x].type = TileType::Floor; tiles[cy+1][x].type = TileType::Floor; }
    for (int y = 1; y < H-1; ++y) { tiles[y][cx].type = TileType::Floor; tiles[y][cx+1].type = TileType::Floor; }

    // Grave plots (open areas with wall borders)
    int plotW = 7, plotH = 6;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (row==1 && col==1) continue;
            int px = 3 + col*(plotW+5), py = 3 + row*(plotH+5);
            for (int dy = 1; dy < plotH-1; ++dy)
                for (int dx = 1; dx < plotW-1; ++dx)
                    if (px+dx < W && py+dy < H) tiles[py+dy][px+dx].type = TileType::Floor;
        }
    }
    // Center spawn
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
            tiles[cy+dy][cx+dx].type = TileType::Floor;
}

static void generateCursedFarm(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Open fields with farmhouse clusters
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Floor;

    // Farmhouse ruins (rectangular wall clusters)
    struct House { int x, y, w, h; };
    House houses[] = {
        {3,3,6,5}, {3,H-9,6,5}, {W-10,3,7,6}, {W-10,H-9,7,5},
        {W/2-4,4,8,5}, {W/2-4,H-9,8,5}
    };
    for (auto& h : houses) {
        for (int dy = 0; dy < h.h; ++dy)
            for (int dx = 0; dx < h.w; ++dx) {
                if (dy==0||dy==h.h-1||dx==0||dx==h.w-1)
                    if (h.x+dx<W && h.y+dy<H)
                        tiles[h.y+dy][h.x+dx].type = TileType::Wall;
            }
    }
    // Fence rows
    for (int x = 0; x < W; x+=3)
        if ((x/3)%4 != 2 && x < W) tiles[H/2-5][x].type = TileType::Wall;
    // Spawn clear
    int cx = W/2, cy = H/2;
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
            tiles[cy+dy][cx+dx].type = TileType::Floor;
}

static void generateGhostCity(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Dense city grid — narrower streets than LARuins
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Wall;

    // Streets every 5 tiles, 2 wide
    for (int y = 3; y < H-3; y += 5)
        for (int x = 1; x < W-1; ++x)
            for (int dy = 0; dy < 2; ++dy) tiles[y+dy][x].type = TileType::Floor;
    for (int x = 3; x < W-3; x += 5)
        for (int y = 1; y < H-1; ++y)
            for (int dx = 0; dx < 2; ++dx) tiles[y][x+dx].type = TileType::Floor;

    // Some broken walls
    for (int y = 1; y < H-1; ++y)
        for (int x = 1; x < W-1; ++x)
            if (tiles[y][x].type == TileType::Wall && (x*13+y*7)%11==0)
                tiles[y][x].type = TileType::BrokenFloor;

    int cx = W/2, cy = H/2;
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
            tiles[cy+dy][cx+dx].type = TileType::Floor;
}

static void generateDarkForest(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Open with random "tree" walls scattered
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Floor;

    // Tree clusters (3x3 wall blobs)
    for (int i = 0; i < 30; ++i) {
        int tx = GetRandomValue(2, W-4), ty = GetRandomValue(2, H-4);
        for (int dy = 0; dy < 2; ++dy)
            for (int dx = 0; dx < 2; ++dx)
                tiles[ty+dy][tx+dx].type = TileType::Wall;
    }

    // Winding path: carve clear strip across map
    int cy = H/2;
    for (int x = 1; x < W-1; ++x) {
        int yo = (int)(std::sin(x * 0.4f) * 3);
        tiles[cy+yo][x].type = TileType::Floor;
        tiles[cy+yo+1][x].type = TileType::Floor;
    }
    // Spawn
    int cx = W/2;
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
            tiles[cy+dy][cx+dx].type = TileType::Floor;
}

static void generateCatacombs(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Very narrow passages
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Wall;

    // Maze-like corridors
    int cx = W/2, cy = H/2;
    // Horizontal and vertical passages every 4 tiles, width 2
    for (int y = 2; y < H-2; y += 4)
        for (int x = 1; x < W-1; ++x) {
            tiles[y][x].type = TileType::Floor;
            if (y+1 < H-1) tiles[y+1][x].type = TileType::Floor;
        }
    for (int x = 2; x < W-2; x += 4)
        for (int y = 1; y < H-1; ++y) {
            tiles[y][x].type = TileType::Floor;
            if (x+1 < W-1) tiles[y][x+1].type = TileType::Floor;
        }
    // Small burial chambers
    for (int i = 0; i < 8; ++i) {
        int rx = GetRandomValue(4, W-8), ry = GetRandomValue(4, H-8);
        for (int dy = 0; dy < 4; ++dy)
            for (int dx = 0; dx < 4; ++dx)
                tiles[ry+dy][rx+dx].type = TileType::Floor;
    }
    for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
            tiles[cy+dy][cx+dx].type = TileType::Floor;
}

static void generateAbandonedManor(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Large manor rooms connected by doorways
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Wall;

    // Outer hall
    for (int y = 3; y < H-3; ++y)
        for (int x = 3; x < W-3; ++x)
            tiles[y][x].type = TileType::Floor;
    // Inner walls partitioning rooms
    for (int y = 3; y < H-3; ++y) tiles[y][W/3].type = TileType::Wall;
    for (int y = 3; y < H-3; ++y) tiles[y][W*2/3].type = TileType::Wall;
    for (int x = 3; x < W-3; ++x) tiles[H/2][x].type = TileType::Wall;
    // Doorways
    tiles[H/4][W/3].type   = TileType::Floor; tiles[H/4+1][W/3].type   = TileType::Floor;
    tiles[H*3/4][W/3].type = TileType::Floor; tiles[H*3/4+1][W/3].type = TileType::Floor;
    tiles[H/4][W*2/3].type = TileType::Floor; tiles[H/4+1][W*2/3].type = TileType::Floor;
    tiles[H*3/4][W*2/3].type=TileType::Floor; tiles[H*3/4+1][W*2/3].type=TileType::Floor;
    for (int x = 3; x < W-3; ++x) { tiles[H/2][x].type = TileType::Wall; }
    tiles[H/2][W/2].type=TileType::Floor; tiles[H/2][W/2+1].type=TileType::Floor;
    // Clear spawn
    int cx=W/2, cy=H/2;
    for (int dy=-3;dy<=3;++dy) for(int dx=-3;dx<=3;++dx) tiles[cy+dy][cx+dx].type=TileType::Floor;
}

static void generateInferno(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    // Mostly open with lava rivers (wall obstacles)
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Floor;

    // Lava rock islands (wall clusters)
    for (int i = 0; i < 20; ++i) {
        int rx = GetRandomValue(2, W-6), ry = GetRandomValue(2, H-6);
        int rw = GetRandomValue(2,5), rh = GetRandomValue(2,4);
        for (int dy=0;dy<rh;++dy)
            for (int dx=0;dx<rw;++dx)
                if (rx+dx<W && ry+dy<H) tiles[ry+dy][rx+dx].type = TileType::Wall;
    }
    // Lava channel across middle
    for (int x = 1; x < W-1; ++x)
        tiles[H/2][x].type = TileType::Wall;
    // Gap in channel
    for (int x = W/2-3; x < W/2+4; ++x) tiles[H/2][x].type = TileType::Floor;

    int cx=W/2, cy=H/2;
    for (int dy=-4;dy<=4;++dy) for(int dx=-4;dx<=4;++dx) tiles[cy+dy][cx+dx].type=TileType::Floor;
}

// ── end Dark Zone Generators ────────────────────────────────────────────────

static void generateCore(std::vector<std::vector<Tile>>& tiles, int W, int H) {
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            tiles[y][x].type = TileType::Wall;

    int cx = W/2, cy = H/2;
    int coreR = 8;

    // Central circular chamber
    for (int dy = -coreR; dy <= coreR; ++dy)
        for (int dx = -coreR; dx <= coreR; ++dx)
            if (dx*dx + dy*dy <= coreR*coreR)
                tiles[cy+dy][cx+dx].type = TileType::Floor;

    // 6 radial spokes
    int spokes = 6;
    for (int s = 0; s < spokes; ++s) {
        float angle = s * 6.28318f / spokes;
        int spokeLen = GetRandomValue(7, 12);
        // carve 2-wide spoke
        for (int r = coreR; r <= coreR + spokeLen; ++r) {
            int sx = cx + (int)(std::cos(angle) * r);
            int sy = cy + (int)(std::sin(angle) * r);
            for (int off = -1; off <= 1; ++off) {
                int ox = sx + (int)(std::sin(angle) * off);
                int oy = sy - (int)(std::cos(angle) * off);
                if (ox > 0 && ox < W-1 && oy > 0 && oy < H-1)
                    tiles[oy][ox].type = TileType::Floor;
            }
        }
        // terminal room at end of spoke
        int endX = cx + (int)(std::cos(angle) * (coreR + spokeLen));
        int endY = cy + (int)(std::sin(angle) * (coreR + spokeLen));
        for (int dy = -2; dy <= 2; ++dy)
            for (int dx = -2; dx <= 2; ++dx)
                if (endX+dx > 0 && endX+dx < W-1 && endY+dy > 0 && endY+dy < H-1)
                    tiles[endY+dy][endX+dx].type = TileType::Floor;
    }
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
ZoneID Tilemap::tileZone(int tx, int ty) const {
    if (!openWorld) return currentZone;
    int col = tx / OW_ZONE_W;
    int row = ty / OW_ZONE_H;
    if (col < 0 || col >= OW_COLS || row < 0 || row >= OW_ROWS) return currentZone;
    return owLayout[row][col];
}

// Bioma na POSIÇÃO do mundo, repetindo o layout 3x3 ao infinito (módulo) — usa
// EXATAMENTE a mesma fórmula do piso em render3D, então o cenário gerado por aqui
// sempre combina com o chão embaixo dele.
ZoneID Tilemap::biomeAtWorld(float, float) const {
    // O mundo inteiro da FASE tem um bioma so: o cenario gerado sempre combina com
    // o chao e o jogador nunca "atravessa" para outro mundo andando.
    return currentZone;
}

void Tilemap::generateOpenWorld() {
    openWorld = true;

    owLayout[0][0] = ZoneID::LARuins;
    owLayout[0][1] = ZoneID::Bunker;
    owLayout[0][2] = ZoneID::DarkForest;
    owLayout[1][0] = ZoneID::CursedFarm;
    owLayout[1][1] = ZoneID::Cemetery;
    owLayout[1][2] = ZoneID::GhostCity;
    owLayout[2][0] = ZoneID::KronosForge;
    owLayout[2][1] = ZoneID::AbandonedManor;
    owLayout[2][2] = ZoneID::KronosNexus;

    currentZone = ZoneID::LARuins;
    width  = OW_COLS * OW_ZONE_W;
    height = OW_ROWS * OW_ZONE_H;

    tiles.assign(height, std::vector<Tile>(width));
    portals.clear();

    // MUNDO ABERTO DE VERDADE: tudo chao livre. Sem labirinto, sem corredores.
    // O visual de cada bioma vem de tileZone()/getZoneInfo no render; aqui so
    // definimos a transitabilidade — chao aberto com obstaculos esparsos.
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            tiles[y][x].type       = TileType::Floor;
            tiles[y][x].portalZone = -1;
            tiles[y][x].rect       = getBounds(x, y);
        }

    // SEM borda: mundo aberto é INFINITO (isWall libera fora dos limites; o chão
    // e o cenário se auto-geram por posição conforme o jogador explora).

    for (int row = 0; row < OW_ROWS; ++row) {
        for (int col = 0; col < OW_COLS; ++col) {
            int offX = col * OW_ZONE_W;
            int offY = row * OW_ZONE_H;
        }
    }
}

void Tilemap::generate(ZoneID zone) {
    currentZone = zone;
    portals.clear();

    // Reset rects
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            tiles[y][x].type       = TileType::Wall;
            tiles[y][x].portalZone = -1;
            tiles[y][x].rect       = getBounds(x, y);
        }

    switch (zone) {
        case ZoneID::LARuins:       generateLARuins      (tiles, width, height); break;
        case ZoneID::Bunker:        generateBunker       (tiles, width, height); break;
        case ZoneID::KronosForge:   generateFactory      (tiles, width, height); break;
        case ZoneID::KronosNexus:   generateCore         (tiles, width, height); break;
        case ZoneID::Cemetery:      generateCemetery     (tiles, width, height); break;
        case ZoneID::CursedFarm:    generateCursedFarm   (tiles, width, height); break;
        case ZoneID::GhostCity:     generateGhostCity    (tiles, width, height); break;
        case ZoneID::DarkForest:    generateDarkForest   (tiles, width, height); break;
        case ZoneID::Catacombs:     generateCatacombs    (tiles, width, height); break;
        case ZoneID::AbandonedManor:generateAbandonedManor(tiles, width, height); break;
        case ZoneID::InfernoZone:   generateInferno      (tiles, width, height); break;
        default:                    generateLARuins      (tiles, width, height); break;
    }

    // Apply rects and portal zones (-1 preserved)
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            tiles[y][x].rect = getBounds(x, y);

    placePortals(zone);
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// RENDERING
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

static float mapTime = 0.0f; // global anim timer, advanced in render

void Tilemap::render(Vector2 camTarget, float zoom) const {
    mapTime += GetFrameTime() * 1.5f;
    float t = mapTime;

    // FRUSTUM CULLING — desenha SO os tiles visiveis (mundo 120x120 = 14400 tiles).
    // Evita processar o mapa inteiro por frame (queda severa de FPS).
    if (zoom <= 0.01f) zoom = 1.0f;
    float halfW = (1280.0f * 0.5f) / zoom;
    float halfH = (720.0f  * 0.5f) / zoom;
    int startX = std::max(0,      (int)((camTarget.x - halfW) / tileSize) - 1);
    int endX   = std::min(width,  (int)((camTarget.x + halfW) / tileSize) + 2);
    int startY = std::max(0,      (int)((camTarget.y - halfH) / tileSize) - 1);
    int endY   = std::min(height, (int)((camTarget.y + halfH) / tileSize) + 2);
    // Sem câmera (default {0,0}) cai para o mapa todo (compatibilidade)
    if (camTarget.x == 0 && camTarget.y == 0) { startX=0; startY=0; endX=width; endY=height; }

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            const Tile& tile = tiles[y][x];
            float rx = tile.rect.x, ry = tile.rect.y;
            float rw = tile.rect.width, rh = tile.rect.height;

            ZoneID tzone = tileZone(x, y);
            ZoneInfo info = getZoneInfo(tzone);

            switch (tile.type) {
                case TileType::Floor: {
                    SpriteBank& sb = SpriteBank::get();
                    int zi = (int)tzone;
                    if (sb.ready && zi >= 0 && zi < SpriteBank::NUM_ZONE_TILES) {
                        DrawTexturePro(sb.tileFloor[zi],
                                       {0,0,64,64}, tile.rect, {0,0}, 0.0f, WHITE);
                    } else {
                        Color base = (x + y) % 2 == 0 ? info.floorColorA : info.floorColorB;
                        DrawRectangleRec(tile.rect, base);
                    }

                    switch (tzone) {
                        case ZoneID::LARuins: {
                            // Road cracks
                            if ((x * 7 + y * 13) % 11 == 0) {
                                Color crack = {15,15,15,200};
                                DrawLine((int)rx+8,  (int)ry+8,  (int)rx+30, (int)ry+40, crack);
                                DrawLine((int)rx+30, (int)ry+40, (int)rx+50, (int)ry+22, crack);
                            }
                            // Road stripes (yellow center of certain rows)
                            // Identify horizontal streets: tiles surrounded by floor in a band
                            if (tiles[y][x].type == TileType::Floor &&
                                y > 0 && y < height-1 &&
                                tiles[y-1][x].type == TileType::Wall &&
                                tiles[y+1][x].type == TileType::Wall &&
                                (x % 8 < 4)) {
                                DrawRectangle((int)rx+26, (int)ry+28, 12, 8, {200,180,0,100});
                            }
                            // Scorch marks
                            if ((x * 11 + y * 5) % 23 == 0) {
                                DrawCircleV({rx+32, ry+32}, 14, ColorAlpha({10,10,10,255}, 0.55f));
                                DrawCircleV({rx+32, ry+32},  6, ColorAlpha({20,8,8,255},   0.40f));
                            }
                            break;
                        }
                        case ZoneID::Bunker: {
                            // Metal grating hash pattern
                            if ((x + y) % 3 == 0) {
                                Color hash = ColorAlpha({60,80,60,255}, 0.5f);
                                DrawLine((int)rx+4,  (int)ry+4,  (int)rx+18, (int)ry+18, hash);
                                DrawLine((int)rx+18, (int)ry+4,  (int)rx+4,  (int)ry+18, hash);
                                DrawLine((int)rx+36, (int)ry+36, (int)rx+58, (int)ry+58, hash);
                                DrawLine((int)rx+58, (int)ry+36, (int)rx+36, (int)ry+58, hash);
                            }
                            // Drain grate every ~10 tiles
                            if ((x * 3 + y * 7) % 31 == 0) {
                                DrawCircleLines((int)rx+32, (int)ry+32, 12, ColorAlpha({40,70,40,255}, 0.7f));
                                DrawLine((int)rx+20,(int)ry+32,(int)rx+44,(int)ry+32, ColorAlpha({40,70,40,255},0.5f));
                                DrawLine((int)rx+32,(int)ry+20,(int)rx+32,(int)ry+44, ColorAlpha({40,70,40,255},0.5f));
                            }
                            // Danger stripes near portals (x<8 or x>W-8)
                            if (x < 8 || x > width-8) {
                                if ((x+y)%2==0) {
                                    DrawRectangle((int)rx, (int)ry+58, 64, 6, ColorAlpha({180,140,0,255},0.35f));
                                }
                            }
                            break;
                        }
                        case ZoneID::KronosForge: {
                            // Conveyor belt dashed lines in larger open areas
                            if ((x % 12 < 6) && y % 3 == 0) {
                                Color conv = ColorAlpha({120,90,20,255}, 0.5f);
                                DrawRectangle((int)rx+28, (int)ry+2, 8, 14, conv);
                            }
                            // Hazard diamond
                            if ((x*5+y*9)%29==0) {
                                Color haz = ColorAlpha({180,120,0,255}, 0.35f);
                                DrawLineEx({rx+32,ry+4},{rx+58,ry+32},2.0f,haz);
                                DrawLineEx({rx+58,ry+32},{rx+32,ry+58},2.0f,haz);
                                DrawLineEx({rx+32,ry+58},{rx+6,ry+32},2.0f,haz);
                                DrawLineEx({rx+6,ry+32},{rx+32,ry+4},2.0f,haz);
                            }
                            // Machine footprint outline
                            if ((x*7+y*3)%41==0) {
                                DrawRectangleLinesEx({rx+6,ry+6,52,52}, 2.0f,
                                                     ColorAlpha({80,40,15,255},0.55f));
                                DrawRectangleLinesEx({rx+14,ry+14,36,36}, 1.5f,
                                                     ColorAlpha({60,30,10,255},0.4f));
                            }
                            break;
                        }
                        case ZoneID::KronosNexus: {
                            // Hexagonal grid approximation: draw 6 lines around center
                            if ((x+y)%4==0) {
                                Color hex = ColorAlpha({100,50,200,255}, 0.22f);
                                float hx = rx+32, hy = ry+32, hr = 26.0f;
                                for (int s = 0; s < 6; ++s) {
                                    float a0 = s * 1.0472f, a1 = (s+1) * 1.0472f;
                                    DrawLineEx({hx + std::cos(a0)*hr, hy + std::sin(a0)*hr},
                                               {hx + std::cos(a1)*hr, hy + std::sin(a1)*hr},
                                               1.5f, hex);
                                }
                            }
                            // Pulsing energy conduit lines along corridors
                            if ((x * 3 + y * 7) % 13 == 0) {
                                float pulse2 = 0.4f + 0.4f * std::sin(t * 3.0f + x * 0.5f + y * 0.5f);
                                DrawLineEx({rx+2, ry+32},{rx+62,ry+32},
                                           2.5f, ColorAlpha({180,0,255,255}, pulse2 * 0.5f));
                                DrawLineEx({rx+32,ry+2},{rx+32,ry+62},
                                           2.5f, ColorAlpha({180,0,255,255}, pulse2 * 0.3f));
                            }
                            // Energy node pads
                            if ((x*11+y*13)%37==0) {
                                float np = 0.5f + 0.5f*std::sin(t*4.0f+x+y);
                                DrawCircleV({rx+32,ry+32}, 10,
                                            ColorAlpha({120,0,200,255}, 0.3f + np*0.3f));
                                DrawCircleLines((int)rx+32,(int)ry+32, 14,
                                                ColorAlpha({180,0,255,255}, np*0.6f));
                            }
                            break;
                        }
                        case ZoneID::Cemetery: {
                            // Dark earth with green mist wisps
                            if ((x*7+y*13)%9==0) {
                                float mp = 0.4f+0.4f*std::sin(t*1.2f+x*0.7f+y*0.5f);
                                DrawCircleV({rx+32,ry+32}, 14.0f,
                                            ColorAlpha({0,60,10,255}, mp*0.25f));
                            }
                            // Pebbles
                            if ((x*11+y*5)%7==0) {
                                DrawCircleV({rx+14,ry+20}, 2.0f, {30,25,20,200});
                                DrawCircleV({rx+42,ry+44}, 2.0f, {28,23,18,180});
                            }
                            break;
                        }
                        case ZoneID::CursedFarm: {
                            // Dead grass patches
                            if ((x+y*3)%4 != 0) {
                                int gx2 = (x*17+y*5)%48+8;
                                Color grassCol = {(unsigned char)(25+((x*3+y)%15)), 35, 8, 200};
                                DrawLine((int)rx+gx2, (int)(ry+rh)-4, (int)rx+gx2-3, (int)(ry+rh)-12, grassCol);
                                DrawLine((int)rx+gx2, (int)(ry+rh)-4, (int)rx+gx2+2, (int)(ry+rh)-11, grassCol);
                            }
                            // Mud puddle
                            if ((x*9+y*7)%11==0) {
                                DrawEllipse((int)rx+28,(int)ry+36, 16.0f, 8.0f,
                                            ColorAlpha({40,25,12,255}, 0.5f));
                            }
                            break;
                        }
                        case ZoneID::GhostCity: {
                            // Cracked asphalt, darker
                            if ((x*13+y*7)%8==0) {
                                DrawLine((int)rx+6,(int)ry+12,(int)rx+22,(int)ry+38,{12,12,12,255});
                                DrawLine((int)rx+22,(int)ry+38,(int)rx+46,(int)ry+20,{12,12,12,255});
                            }
                            // Pale glow from ghost energy
                            if ((x*3+y*11)%19==0) {
                                float gp = 0.3f+0.3f*std::sin(t*0.8f+x*0.4f+y*0.6f);
                                DrawCircleV({rx+32,ry+32}, 20.0f,
                                            ColorAlpha({180,200,255,255}, gp*0.12f));
                            }
                            break;
                        }
                        case ZoneID::DarkForest: {
                            // Root textures
                            if ((x*3+y)%7==0) {
                                DrawLine((int)rx+4,(int)ry+32,(int)(rx+rw)-4,(int)ry+36,
                                         ColorAlpha({50,30,10,255}, 0.6f));
                            }
                            // Bioluminescent mushrooms
                            if ((x*17+y*11)%23==0) {
                                float mp2 = 0.5f+0.5f*std::sin(t*1.5f+x+y);
                                DrawCircleV({rx+18,ry+50}, 5.0f,
                                            ColorAlpha({0,180,255,255}, 0.3f+mp2*0.25f));
                                DrawRectangle((int)rx+16,(int)ry+50,4,10,
                                              ColorAlpha({0,140,200,255}, 0.4f));
                            }
                            break;
                        }
                        case ZoneID::Catacombs: {
                            // Wet stone sheen
                            DrawRectangle((int)rx,(int)ry,2,(int)rh,
                                          ColorAlpha({60,60,80,255}, 0.25f));
                            // Small bone fragments
                            if ((x*7+y*13)%13==0) {
                                DrawLine((int)rx+20,(int)ry+30,(int)rx+38,(int)ry+34,
                                         ColorAlpha({200,190,170,255}, 0.5f));
                            }
                            break;
                        }
                        case ZoneID::AbandonedManor: {
                            // Dusty floorboard lines
                            int board = y % 3;
                            DrawLine((int)rx,(int)ry+board*21,(int)rx+(int)rw,(int)ry+board*21,
                                     ColorAlpha({25,18,15,255}, 0.5f));
                            // Spider web corner
                            if ((x*5+y*7)%17==0) {
                                for (int ws=0;ws<4;++ws)
                                    DrawLine((int)rx+2,(int)ry+2,
                                             (int)rx+ws*14,(int)ry+ws*8,
                                             ColorAlpha({180,180,180,255},0.2f));
                            }
                            break;
                        }
                        case ZoneID::InfernoZone: {
                            // Volcanic rock with heat shimmer
                            if ((x*23+y*41)%12==0) {
                                float glow2 = 0.4f+0.4f*std::sin(t*2.5f+x*0.5f+y*0.3f);
                                Color lc = {(unsigned char)(180+50*glow2),(unsigned char)(40+20*glow2),0,255};
                                DrawLine((int)rx+(x*17)%(int)(rw-8)+4,(int)ry+4,
                                         (int)rx+(x*23+y*7)%(int)(rw-8)+4,(int)ry+(int)rh-4, lc);
                            }
                            // Cinder glow
                            if ((x*11+y*7)%19==0) {
                                float cp = 0.5f+0.5f*std::sin(t*3.0f+x*0.8f);
                                DrawCircleV({rx+rw/2,ry+rh/2}, 6.0f,
                                            ColorAlpha({255,80,0,255}, cp*0.3f));
                            }
                            break;
                        }
                        default: break;
                    }
                    break;
                }

                // â”€â”€ WALL â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
                case TileType::Wall: {
                    SpriteBank& sb = SpriteBank::get();
                    int zi = (int)tzone;
                    if (sb.ready && zi >= 0 && zi < SpriteBank::NUM_ZONE_TILES) {
                        DrawTexturePro(sb.tileWall[zi],
                                       {0,0,64,64}, tile.rect, {0,0}, 0.0f, WHITE);
                    } else {
                        DrawRectangleRec(tile.rect, info.wallColor);
                    }

                    switch (tzone) {
                        case ZoneID::LARuins: {
                            // Concrete panel with darker interior
                            DrawRectangle((int)rx+4,(int)ry+4,(int)rw-8,(int)rh-8,
                                          ColorAlpha({60,60,65,255},0.6f));
                            // Window (random)
                            if ((x*9+y*3)%19==0) {
                                DrawRectangle((int)rx+14,(int)ry+10,(int)rw-28,(int)rh-28,
                                              {15,15,25,255});
                                DrawLine((int)rx+14,(int)ry+22,(int)rx+(int)rw-14,(int)ry+22,
                                         ColorAlpha(WHITE,0.15f));
                                DrawLine((int)rx+32,(int)ry+10,(int)rx+32,(int)ry+(int)rh-18,
                                         ColorAlpha(WHITE,0.10f));
                            }
                            // Top highlight + neon edge
                            DrawRectangle((int)rx,(int)ry,(int)rw,6,ColorAlpha(WHITE,0.07f));
                            DrawRectangleLinesEx(tile.rect,1.5f,ColorAlpha(info.wallOutline,0.6f));
                            DrawLine((int)rx,(int)ry,(int)rx+(int)rw,(int)ry,info.wallOutline);
                            break;
                        }
                        case ZoneID::Bunker: {
                            // Heavy reinforced metal
                            DrawRectangle((int)rx+3,(int)ry+3,(int)rw-6,(int)rh-6,
                                          ColorAlpha({20,40,20,255},0.7f));
                            // Rivets at corners
                            Color rivet = {45,75,45,255};
                            DrawCircleV({rx+8,  ry+8  }, 3.5f, rivet);
                            DrawCircleV({rx+rw-8,ry+8  }, 3.5f, rivet);
                            DrawCircleV({rx+8,  ry+rh-8}, 3.5f, rivet);
                            DrawCircleV({rx+rw-8,ry+rh-8}, 3.5f, rivet);
                            DrawRectangleLinesEx(tile.rect,2.0f,ColorAlpha(info.wallOutline,0.8f));
                            DrawLine((int)rx,(int)ry,(int)rx+(int)rw,(int)ry,info.wallOutline);
                            break;
                        }
                        case ZoneID::KronosForge: {
                            // Industrial steel with orange warning stripe at base
                            DrawRectangle((int)rx+2,(int)ry+2,(int)rw-4,(int)rh-4,
                                          ColorAlpha({70,35,15,255},0.6f));
                            // Orange warning stripe at bottom
                            DrawRectangle((int)rx,(int)ry+(int)rh-10,(int)rw,10,
                                          ColorAlpha({180,80,0,255},0.45f));
                            // Hazard diagonal stripes in the bottom band
                            for (int s = 0; s < 5; ++s)
                                DrawLine((int)rx+s*14,(int)ry+(int)rh-10,
                                         (int)rx+s*14+8,(int)ry+(int)rh,
                                         ColorAlpha({20,10,0,255},0.4f));
                            // Large panel outline
                            DrawRectangleLinesEx(tile.rect,1.5f,ColorAlpha(info.wallOutline,0.7f));
                            DrawRectangleLinesEx({rx+6,ry+6,rw-12,rh-16},1.0f,
                                                 ColorAlpha({120,60,20,255},0.35f));
                            DrawLine((int)rx,(int)ry,(int)rx+(int)rw,(int)ry,info.wallOutline);
                            break;
                        }
                        case ZoneID::KronosNexus: {
                            // Black with red conduit
                            DrawRectangle((int)rx+2,(int)ry+2,(int)rw-4,(int)rh-4,
                                          {5,2,15,255});
                            // Pulsing red edge conduit
                            float pulse3 = 0.3f + 0.3f*std::sin(t*2.5f+x*0.8f+y*0.6f);
                            Color ec = ColorAlpha({220,0,80,255}, pulse3);
                            DrawLine((int)rx,(int)ry,(int)rx+(int)rw,(int)ry, ec);
                            DrawLine((int)rx,(int)ry,(int)rx,(int)ry+(int)rh, ec);
                            DrawRectangleLinesEx(tile.rect,1.5f,ColorAlpha(info.wallOutline,0.5f));
                            // Vertical conduit tube on some walls
                            if ((x*5+y*11)%17==0) {
                                float cp = 0.5f+0.5f*std::sin(t*5.0f+x+y);
                                DrawRectangle((int)rx+28,(int)ry,8,(int)rh,
                                              ColorAlpha({180,0,60,255},0.25f+cp*0.2f));
                            }
                            break;
                        }
                        case ZoneID::Cemetery: {
                            // Old stone with moss
                            DrawRectangle((int)rx+3,(int)ry+3,(int)rw-6,(int)rh-6,
                                          ColorAlpha({30,30,26,255},0.7f));
                            // Brick lines
                            DrawLine((int)rx+2,(int)ry+(int)rh/3,(int)rx+(int)rw-2,(int)ry+(int)rh/3,
                                     ColorAlpha({20,20,16,255},0.7f));
                            DrawLine((int)rx+2,(int)ry+(int)rh*2/3,(int)rx+(int)rw-2,(int)ry+(int)rh*2/3,
                                     ColorAlpha({20,20,16,255},0.7f));
                            // Moss
                            if ((x*5+y*3)%7==0)
                                DrawRectangle((int)rx+2,(int)ry+(int)rh-8,(int)rw-4,6,
                                              ColorAlpha({15,40,10,255},0.6f));
                            DrawRectangleLinesEx(tile.rect,1.0f,ColorAlpha(info.wallOutline,0.5f));
                            break;
                        }
                        case ZoneID::CursedFarm: {
                            // Rotted wood planks
                            DrawRectangle((int)rx+2,(int)ry+2,(int)rw-4,(int)rh-4,
                                          ColorAlpha({45,30,15,255},0.8f));
                            // Wood plank lines
                            for (int pl=0;pl<3;++pl)
                                DrawLine((int)rx+4,(int)ry+4+pl*18,(int)rx+(int)rw-4,(int)ry+4+pl*18,
                                         ColorAlpha({30,18,8,255},0.7f));
                            // Knot holes
                            if ((x*7+y*11)%9==0)
                                DrawCircleV({rx+rw/2,ry+rh/2}, 4.0f,
                                            ColorAlpha({25,12,4,255},0.8f));
                            DrawRectangleLinesEx(tile.rect,1.0f,ColorAlpha(info.wallOutline,0.5f));
                            break;
                        }
                        case ZoneID::GhostCity: {
                            // Crumbling concrete with ghost graffiti
                            DrawRectangle((int)rx+3,(int)ry+3,(int)rw-6,(int)rh-6,
                                          ColorAlpha({32,32,38,255},0.8f));
                            // Ghost graffiti shimmer
                            if ((x*13+y*7)%11==0) {
                                float gs = 0.2f+0.2f*std::sin(t*0.9f+x+y);
                                DrawLine((int)rx+10,(int)ry+20,(int)rx+(int)rw-10,(int)ry+30,
                                         ColorAlpha({180,200,255,255},gs));
                            }
                            DrawRectangleLinesEx(tile.rect,1.0f,ColorAlpha(info.wallOutline,0.5f));
                            break;
                        }
                        case ZoneID::DarkForest: {
                            // Tree trunk texture
                            DrawRectangle((int)rx+3,(int)ry+3,(int)rw-6,(int)rh-6,
                                          ColorAlpha({22,16,8,255},0.9f));
                            // Bark rings
                            DrawLine((int)rx+4,(int)ry+(int)rh/3,(int)rx+(int)rw-4,(int)ry+(int)rh/3,
                                     ColorAlpha({16,10,4,255},0.6f));
                            DrawLine((int)rx+4,(int)ry+(int)rh*2/3,(int)rx+(int)rw-4,(int)ry+(int)rh*2/3,
                                     ColorAlpha({16,10,4,255},0.4f));
                            // Glow from mushroom on base
                            if ((x*7+y*17)%13==0) {
                                float mg = 0.3f+0.3f*std::sin(t*1.8f+x+y);
                                DrawRectangle((int)rx+20,(int)ry+(int)rh-10,24,8,
                                              ColorAlpha({0,180,255,255},mg*0.25f));
                            }
                            DrawRectangleLinesEx(tile.rect,1.0f,ColorAlpha(info.wallOutline,0.4f));
                            break;
                        }
                        case ZoneID::Catacombs: {
                            // Ancient cut stone blocks
                            DrawRectangle((int)rx+2,(int)ry+2,(int)rw-4,(int)rh-4,
                                          ColorAlpha({25,20,28,255},0.9f));
                            bool cev = (y%2==0);
                            int cbW = (int)rw/2, cbH = (int)rh/2;
                            // 2x2 block pattern
                            DrawLine((int)rx+cbW,(int)ry+2,(int)rx+cbW,(int)ry+(int)rh-2,
                                     ColorAlpha({15,12,18,255},0.8f));
                            DrawLine((int)rx+2,(cev?(int)ry+cbH:(int)ry+cbH/2)+2,
                                     (int)rx+(int)rw-2,(cev?(int)ry+cbH:(int)ry+cbH/2)+2,
                                     ColorAlpha({15,12,18,255},0.6f));
                            // Dripping moisture
                            if ((x*5+y*9)%11==0) {
                                float dt2 = fmodf(t*0.4f+(x+y)*0.2f,1.0f);
                                int dropY = (int)(ry+dt2*rh);
                                DrawCircleV({rx+rw/2,(float)dropY}, 1.5f,
                                            ColorAlpha({40,50,70,255},0.7f));
                            }
                            DrawRectangleLinesEx(tile.rect,1.0f,ColorAlpha(info.wallOutline,0.5f));
                            break;
                        }
                        case ZoneID::AbandonedManor: {
                            // Dark stone with iron trim
                            DrawRectangle((int)rx+3,(int)ry+3,(int)rw-6,(int)rh-6,
                                          ColorAlpha({22,15,22,255},0.9f));
                            // Iron corner brackets
                            DrawLine((int)rx+2,(int)ry+2,(int)rx+12,(int)ry+2, {50,40,50,200});
                            DrawLine((int)rx+2,(int)ry+2,(int)rx+2,(int)ry+12, {50,40,50,200});
                            DrawLine((int)rx+(int)rw-12,(int)ry+2,(int)rx+(int)rw-2,(int)ry+2,{50,40,50,200});
                            DrawLine((int)rx+(int)rw-2,(int)ry+2,(int)rx+(int)rw-2,(int)ry+12,{50,40,50,200});
                            // Purple energy seep
                            if ((x*3+y*7)%13==0) {
                                float ap = 0.2f+0.2f*std::sin(t*1.2f+x*0.5f+y*0.7f);
                                DrawLine((int)rx,(int)ry,(int)rx,(int)ry+(int)rh,
                                         ColorAlpha({150,0,200,255},ap));
                            }
                            DrawRectangleLinesEx(tile.rect,1.0f,ColorAlpha(info.wallOutline,0.5f));
                            break;
                        }
                        case ZoneID::InfernoZone: {
                            // Black volcanic rock with lava seams
                            DrawRectangle((int)rx+2,(int)ry+2,(int)rw-4,(int)rh-4,
                                          ColorAlpha({15,5,2,255},0.95f));
                            // Lava seam
                            float ls = 0.4f+0.4f*std::sin(t*2.0f+x*0.6f+y*0.4f);
                            DrawLine((int)rx,(int)ry+(int)rh/2,(int)rx+(int)rw,(int)ry+(int)rh/2,
                                     ColorAlpha({255,60,0,255},ls*0.5f));
                            DrawLine((int)rx+(int)rw/2,(int)ry,(int)rx+(int)rw/2,(int)ry+(int)rh,
                                     ColorAlpha({200,40,0,255},ls*0.3f));
                            DrawRectangleLinesEx(tile.rect,1.0f,ColorAlpha(info.wallOutline,0.6f));
                            break;
                        }
                        default:
                            DrawRectangleLinesEx(tile.rect,1.0f,ColorAlpha(info.wallOutline,0.5f));
                            break;
                    }
                    break;
                }

                // â”€â”€ BROKEN FLOOR â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
                case TileType::BrokenFloor: {
                    Color bf = {(unsigned char)(info.floorColorA.r*5/10),
                                (unsigned char)(info.floorColorA.g*5/10),
                                (unsigned char)(info.floorColorA.b*5/10), 255};
                    DrawRectangleRec(tile.rect, bf);
                    Color crack = {20,20,20,255};
                    DrawLine((int)rx+6, (int)ry+6, (int)rx+32,(int)ry+44, crack);
                    DrawLine((int)rx+32,(int)ry+44,(int)rx+52,(int)ry+18, crack);
                    DrawLine((int)rx+22,(int)ry+10,(int)rx+14,(int)ry+30, crack);
                    break;
                }

                // â”€â”€ PORTAL â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
                case TileType::Portal: {
                    float pulse = (float)(0.5 + 0.5 * std::sin(t * 2.0 + x * 0.6 + y * 0.6));
                    ZoneID  dest = (ZoneID)tile.portalZone;
                    ZoneInfo di  = getZoneInfo(dest);
                    Color   pc   = di.portalColor;
                    Color   bg   = {(unsigned char)(pc.r/5),(unsigned char)(pc.g/5),
                                    (unsigned char)(pc.b/5),255};
                    DrawRectangleRec(tile.rect, bg);
                    Vector2 center = {rx + rw/2, ry + rh/2};
                    DrawCircleV(center, 28+pulse*8, ColorAlpha(pc, 0.08f+pulse*0.06f));
                    DrawCircleV(center, 20+pulse*5, ColorAlpha(pc, 0.15f+pulse*0.08f));
                    DrawCircleV(center, 12+pulse*3, ColorAlpha(pc, 0.30f));
                    Color glowC = {pc.r,pc.g,pc.b,(unsigned char)(100+(int)(155*pulse))};
                    DrawRectangleLinesEx(tile.rect,2.5f,glowC);
                    DrawText(">>", (int)center.x-12, (int)center.y-9, 18, glowC);
                    break;
                }
            }
        }
    }

    // Grid overlay
    {
        Color gridCol = ColorAlpha(getZoneInfo(currentZone).portalColor, 0.045f);
        for (int y = 0; y <= height; ++y)
            DrawLine(0, y*tileSize, width*tileSize, y*tileSize, gridCol);
        for (int x = 0; x <= width; ++x)
            DrawLine(x*tileSize, 0, x*tileSize, height*tileSize, gridCol);
    }

    // Region border lines in open world mode
    if (openWorld) {
        Color borderCol = ColorAlpha({0, 200, 255, 255}, 0.12f);
        for (int c = 1; c < OW_COLS; ++c)
            DrawLineEx({(float)(c * OW_ZONE_W * tileSize), 0},
                       {(float)(c * OW_ZONE_W * tileSize), (float)(height * tileSize)},
                       2.0f, borderCol);
        for (int r = 1; r < OW_ROWS; ++r)
            DrawLineEx({0, (float)(r * OW_ZONE_H * tileSize)},
                       {(float)(width * tileSize), (float)(r * OW_ZONE_H * tileSize)},
                       2.0f, borderCol);
        return; // skip per-zone large decorations in open world
    }

    // Zone-specific large decorations (single-zone mode only)
    if (currentZone == ZoneID::KronosNexus) {
        float cc = (float)(width/2 * tileSize + tileSize/2);
        float cy2 = (float)(height/2 * tileSize + tileSize/2);
        for (int r = 1; r <= 4; ++r) {
            float rp = 0.4f + 0.4f*std::sin(t*1.5f + r*0.8f);
            DrawCircleLines((int)cc,(int)cy2, (float)(r*30),
                            ColorAlpha({200,0,255,255}, rp * 0.45f));
        }
        DrawCircleV({cc,cy2}, 20,
                    ColorAlpha({255,0,200,255}, 0.3f+0.3f*std::sin(t*3.0f)));
    }

    if (currentZone == ZoneID::KronosForge) {
        for (int i = 0; i < 5; ++i) {
            float mpx = (float)((i * 137 + 80) % (width-10)  * tileSize + tileSize);
            float mpy = (float)((i * 97  + 60) % (height-10) * tileSize + tileSize);
            if (tiles[(int)(mpy/tileSize)][(int)(mpx/tileSize)].type == TileType::Floor) {
                DrawCircleV({mpx,mpy}, 48, ColorAlpha({40,20,5,255}, 0.65f));
                DrawCircleLines((int)mpx,(int)mpy, 48, ColorAlpha({150,75,0,255},0.6f));
                DrawCircleLines((int)mpx,(int)mpy, 32, ColorAlpha({100,50,0,255},0.4f));
                for (int v = 0; v < 3; ++v)
                    DrawLine((int)mpx-20+v*18,(int)mpy+55,
                             (int)mpx-20+v*18,(int)mpy+70,
                             ColorAlpha({80,40,0,255},0.5f));
            }
        }
    }

    // ── Dark zone large decorations ──────────────────────────────────────────
    if (currentZone == ZoneID::Cemetery) {
        for (int y = 1; y < height-1; ++y) {
            for (int x = 1; x < width-1; ++x) {
                if (tiles[y][x].type == TileType::Floor && (x*7+y*11)%31==0) {
                    float gpx = (float)(x*tileSize+tileSize/2);
                    float gpy = (float)(y*tileSize+tileSize/2);
                    // Cross gravestone
                    DrawRectangle((int)gpx-3,(int)gpy-20,6,16,{80,70,65,220});
                    DrawRectangle((int)gpx-9,(int)gpy-17,18,5,{80,70,65,220});
                    // Glow at base
                    DrawCircleV({gpx,gpy+2}, 8.0f, ColorAlpha({0,80,20,255},0.2f));
                }
            }
        }
    }
    if (currentZone == ZoneID::GhostCity) {
        for (int y = 1; y < height-1; ++y) {
            for (int x = 1; x < width-1; ++x) {
                if (tiles[y][x].type==TileType::Wall && tiles[y][x+1].type==TileType::Floor &&
                    (x*5+y*9)%19==0) {
                    float lpx = (float)((x+1)*tileSize+4);
                    float lpy = (float)(y*tileSize+tileSize/2);
                    // Broken street lamp
                    DrawRectangle((int)lpx,(int)lpy-30,4,35,{50,50,60,220});
                    DrawRectangle((int)lpx-4,(int)lpy-34,12,6,{50,50,60,220});
                    // Flickering light
                    float flk = (int)(t*7.0f+x)%5==0 ? 0.0f : 0.5f+0.4f*std::sin(t*11.0f+x);
                    DrawCircleV({lpx+2,lpy-34}, 12.0f, ColorAlpha({180,200,255,255},flk*0.25f));
                }
            }
        }
    }
    if (currentZone == ZoneID::DarkForest) {
        for (int y = 1; y < height-1; ++y) {
            for (int x = 1; x < width-1; ++x) {
                if (tiles[y][x].type==TileType::Floor && (x*13+y*7)%29==0) {
                    float mx = (float)(x*tileSize+tileSize/2);
                    float my = (float)(y*tileSize+tileSize/2);
                    float mp3 = 0.5f+0.5f*std::sin(t*1.3f+x*0.5f+y*0.7f);
                    // Glowing mushroom cluster
                    DrawCircleV({mx,my}, 8.0f, ColorAlpha({0,160,255,255},0.3f+mp3*0.2f));
                    DrawRectangle((int)mx-2,(int)my,(int)4,(int)10,
                                  ColorAlpha({0,120,200,255},0.5f));
                    DrawCircleV({mx+12,my+4}, 5.0f, ColorAlpha({0,180,255,255},0.25f+mp3*0.15f));
                }
            }
        }
    }
    if (currentZone == ZoneID::Catacombs) {
        for (int y = 1; y < height-1; ++y) {
            for (int x = 1; x < width-1; ++x) {
                if (tiles[y][x].type==TileType::Wall && tiles[y][x+1].type==TileType::Floor &&
                    (x*3+y*11)%17==0) {
                    float tpx = (float)((x+1)*tileSize+6);
                    float tpy = (float)(y*tileSize+tileSize/2);
                    // Wall torch
                    DrawRectangle((int)tpx-2,(int)tpy-5,4,12,{100,70,30,220});
                    DrawCircleV({tpx,tpy-8}, 6.0f, ColorAlpha({255,140,0,255},0.5f+0.3f*std::sin(t*8.0f+x)));
                    DrawCircleV({tpx,tpy-8}, 3.0f, ColorAlpha({255,220,100,255},0.8f));
                }
            }
        }
    }
    // ── End dark zone decorations ─────────────────────────────────────────────

    if (currentZone == ZoneID::Bunker) {
        for (int y = 1; y < height-1; ++y) {
            for (int x = 1; x < width-1; ++x) {
                if (tiles[y][x].type==TileType::Wall &&
                    tiles[y][x+1].type==TileType::Floor &&
                    (x*3+y*7)%23==0) {
                    float px2 = (float)(x*tileSize);
                    float py2 = (float)(y*tileSize);
                    DrawRectangle((int)px2+52,(int)py2+16,8,32, {10,30,10,255});
                    DrawRectangle((int)px2+54,(int)py2+20,4,6,
                                  ColorAlpha({0,220,0,255}, 0.8f+0.2f*std::sin(t*5.0f+x)));
                    DrawRectangle((int)px2+54,(int)py2+30,4,4,  ColorAlpha({0,100,0,255},0.7f));
                    DrawRectangle((int)px2+54,(int)py2+38,4,6,  ColorAlpha({0,180,0,255},0.6f));
                }
            }
        }
    }
}

// â”€â”€ isWall / isWallAtPosition / isPortalAtPosition â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool Tilemap::isWall(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return openWorld ? false : true; // mundo aberto = infinito
    return tiles[y][x].type == TileType::Wall || tiles[y][x].solid;
}

void Tilemap::markSolidAt(Vector2 worldPos, float radius) {
    // ANTES: r = (int)(radius/64) TRUNCAVA. Uma casa com raio 62 dava r=0, ou seja
    // UM tile (64u) de colisao para um predio de ~180u - por isso dava pra andar
    // atravessado nas construcoes. Agora arredonda pra cima e testa a distancia
    // real ao CENTRO do tile (pegada redonda, nao quadrada).
    int tx = (int)(worldPos.x / tileSize);
    int ty = (int)(worldPos.y / tileSize);
    int r  = std::max(1, (int)std::ceil(radius / tileSize));
    float r2 = radius * radius;
    for (int y = ty - r; y <= ty + r; ++y)
        for (int x = tx - r; x <= tx + r; ++x) {
            if (x < 0 || x >= width || y < 0 || y >= height) continue;
            float cx = (x + 0.5f) * tileSize - worldPos.x;
            float cy = (y + 0.5f) * tileSize - worldPos.y;
            if (cx * cx + cy * cy <= r2) tiles[y][x].solid = true;
        }
}

// Espelho de markSolidAt: DESMARCA solidos de cenario num raio (mantem paredes).
// Usado so como fallback da garantia do portal — ver updatePhasePortal.
void Tilemap::clearSolidAt(Vector2 worldPos, float radius) {
    int tx = (int)(worldPos.x / tileSize);
    int ty = (int)(worldPos.y / tileSize);
    int r  = std::max(1, (int)std::ceil(radius / tileSize));
    float r2 = radius * radius;
    for (int y = ty - r; y <= ty + r; ++y)
        for (int x = tx - r; x <= tx + r; ++x) {
            if (x < 0 || x >= width || y < 0 || y >= height) continue;
            float cx = (x + 0.5f) * tileSize - worldPos.x;
            float cy = (y + 0.5f) * tileSize - worldPos.y;
            if (cx * cx + cy * cy <= r2) tiles[y][x].solid = false;
        }
}

void Tilemap::clearSolidFlags() {
    for (auto& row : tiles)
        for (auto& t : row)
            t.solid = false;
}

static void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    rlSetTexture(texture.id);

    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);

        // Front Face
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Right
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left

        // Back Face
        rlNormal3f(0.0f, 0.0f, -1.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Right
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Right
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Left
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Left

        // Top Face
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Bottom Left
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Bottom Right
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right

        // Bottom Face
        rlNormal3f(0.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Top Right
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Top Left
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Left
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Right

        // Right face
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Left
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Left

        // Left Face
        rlNormal3f(-1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Left
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Right
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Right
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left
    rlEnd();

    rlSetTexture(0);
}

// ─── Render 2.5D isométrico — chão (DrawPlane) + paredes (DrawCube) ───────────
// Mapeamento: X3D = X2D, Z3D = Y2D, altura no eixo Y. Culling em janela ao redor
// do alvo da câmera (camTarget em coordenadas de mundo 2D).
// Cor base de chão por bioma — dá identidade ao terreno em 3D (em vez de cinza).
static Color biomeFloorColor(ZoneID z) {
    switch (z) {
        case ZoneID::LARuins:        return { 74, 76, 84, 255};   // concreto
        case ZoneID::Bunker:         return { 58, 64, 72, 255};   // aço
        case ZoneID::DarkForest:     return { 30, 46, 34, 255};   // mato escuro
        case ZoneID::CursedFarm:     return { 78, 64, 40, 255};   // terra
        case ZoneID::Cemetery:       return { 50, 54, 64, 255};   // pedra fria
        case ZoneID::GhostCity:      return { 54, 56, 62, 255};   // asfalto
        case ZoneID::KronosForge:    return { 66, 42, 34, 255};   // vulcânico
        case ZoneID::AbandonedManor: return { 56, 50, 60, 255};   // madeira podre
        case ZoneID::KronosNexus:    return { 44, 36, 62, 255};   // void roxo
        default:                     return { 52, 56, 66, 255};
    }
}
static Color shade(Color c, float f) {
    auto cl = [](float v){ return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v)); };
    return { cl(c.r * f), cl(c.g * f), cl(c.b * f), c.a };
}

// Teste esfera × frustum em espaco de VIEW (raylib: frente = -Z), para tiles no
// chao (y=0). A janela 53x53 e QUADRADA ao redor do alvo, mas o cone de 30° da
// camera cobre bem menos que isso — isto corta os quads fora da tela antes de
// emitir vertice algum.
static bool floorTileVisible(const Matrix& view, const Camera3D& cam, float aspect,
                             float wx, float wz, float radius) {
    float vz = view.m2*wx + view.m10*wz + view.m14;
    float dist = -vz;                                    // >0 = na frente
    if (dist < -radius) return false;                    // totalmente atras
    if (dist < 20.0f) return true;                       // colado na camera: aprova
    float vx = view.m0*wx + view.m8*wz  + view.m12;
    float vy = view.m1*wx + view.m9*wz  + view.m13;
    float tanH = tanf(cam.fovy * 0.5f * (float)DEG2RAD);
    float limY = dist * tanH + radius;
    if (vy < -limY || vy > limY) return false;
    float limX = dist * tanH * aspect + radius;
    return vx >= -limX && vx <= limX;
}

void Tilemap::render3D(Vector2 camTarget, const Camera3D& cam3D, float aspect) const {
    const float TS = (float)tileSize;
    const int   R  = 26; // raio da janela visível em tiles
    int ctx = (int)(camTarget.x / TS);
    int cty = (int)(camTarget.y / TS);
    int x0 = openWorld ? ctx - R : std::max(0, ctx - R);
    int x1 = openWorld ? ctx + R : std::min(width  - 1, ctx + R);
    int y0 = openWorld ? cty - R : std::max(0, cty - R);
    int y1 = openWorld ? cty + R : std::min(height - 1, cty + R);

    SpriteBank& sb = SpriteBank::get();
    ZoneID z = currentZone;
    Color base = biomeFloorColor(z);

    // Frustum da camera 3D (matriz de view), calculado 1x por frame.
    const Matrix floorView = MatrixLookAt(cam3D.position, cam3D.target, cam3D.up);

    // BATCH UNICO do piso: TODOS os tiles usam a MESMA textura (bioma atual),
    // entao um rlBegin/rlEnd so emite os ~2.800 quads de uma vez — antes eram
    // 2.809 rlSetTexture + rlBegin/rlEnd individuais por frame. O rlgl divide o
    // batch internamente (mantendo estado) se o buffer encher, entao e seguro.
    if (sb.ready) {
        rlSetTexture(sb.tileFloor[(int)z].id);
        rlBegin(RL_QUADS);
        rlNormal3f(0.0f, 1.0f, 0.0f);
    }

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            bool inB = (x >= 0 && x < width && y >= 0 && y < height);
            TileType tt = inB ? tiles[y][x].type : TileType::Floor;
            float rx = x * TS, ry = y * TS;
            Vector3 floorCtr = { rx + TS * 0.5f, 0.0f, ry + TS * 0.5f };

            // Bioma por POSIÇÃO — INFINITO: repete o layout 3x3 de zonas pelo mundo
            // UM bioma por FASE. Antes o layout 3x3 se repetia ao infinito e o
            // mundo trocava de tema debaixo dos pes do jogador enquanto ele andava -
            // nao existia sensacao de "passei de fase", so um mosaico continuo.
            // Agora se muda de mundo pelo PORTAL, nao caminhando.
            // (z/base/SpriteBank foram hoistados pra fora do loop: sao invariantes)

            // Variação determinística por tile (textura de terreno, sem cinza liso)
            unsigned int h = (unsigned int)(x * 73856093) ^ (unsigned int)(y * 19349663);
            // ANTES: brilho sorteado POR TILE. Cada quadrado de 64u saia com um tom
            // proprio e o chao inteiro lia como tabuleiro cinza quadriculado. A
            // variacao util e continua (mA/mB, em coordenada de MUNDO), sem aresta.
            float n = 1.0f;
            float fdx = (float)(x - ctx), fdy = (float)(y - cty);
            float fdist = sqrtf(fdx*fdx + fdy*fdy);
            // Fog de profundidade: so esconde a BORDA do mundo infinito. Antes
            // comecava a 42% do raio e caia a 0.22 — isso sozinho tirava 78% da luz
            // do chao em quase toda a tela.
            float fog = 1.0f - (fdist - R * 0.62f) / (R * 0.38f);
            if (fog < 0.58f) fog = 0.58f; if (fog > 1.0f) fog = 1.0f;
            Color floorColor = shade(base, n);
            if (tt == TileType::BrokenFloor) floorColor = shade(base, 0.55f);
            if (tt == TileType::Portal)      floorColor = Color{0, 150, 200, 255};
            if (sb.ready) {
                // Frustum culling por tile: fora do cone da camera, nem calcula cor.
                if (!floorTileVisible(floorView, cam3D, aspect,
                                      rx + TS * 0.5f, ry + TS * 0.5f, TS * 1.5f)) continue;
                // Piso: UV por POSIÇÃO DO MUNDO → textura contínua/seamless entre tiles
                // (sem grade artificial). Textura é tileável (wrap REPEAT).
                // `n` = grao por tile; `macro` = manchas largas (~14 tiles) que
                // quebram a repeticao obvia do tile de 3x3. Sem os dois o chao
                // lia como um plano de cor solida.
                // DUAS escalas de mancha (larga ~30 tiles + media ~8) e um leve
                // desvio de matiz. Uma oitava fraca so nao quebrava a leitura de
                // "folha de linoleo" que o chao tinha.
                float mA = 0.5f + 0.5f * sinf(rx * 0.0031f + ry * 0.0024f);          // manchas largas
                float mB = 0.5f + 0.5f * sinf(rx * 0.0132f - ry * 0.0098f)
                                       * cosf(rx * 0.0087f + ry * 0.0119f);          // manchas medias
                float macro = 0.88f + 0.16f * mA + 0.09f * mB;                       // 0.88 .. 1.13 (era 0.74..1.20: lia como retalho)
                // matiz: areas mais quentes/frias, senao tudo vira o mesmo cinza
                float warm = 0.94f + 0.12f * mA;
                float cool = 0.94f + 0.12f * (1.0f - mB);
                float g = fog * n * macro * 1.12f;   // clareia: textura + mascara ja escurecem muito
                // Dessatura o piso puxando pro cinza: o personagem (que mantem cor
                // cheia) passa a SALTAR do fundo. E a regra de legibilidade dos jogos
                // de arena - fundo apagado, ator saturado.
                float rr2 = 255.0f * g * warm, gg2 = 255.0f * g, bb2 = 255.0f * g * cool;
                float lum = 0.299f * rr2 + 0.587f * gg2 + 0.114f * bb2;
                const float DESAT = 0.45f;
                Color ft = { (unsigned char)fminf(255.0f, rr2 + (lum - rr2) * DESAT),
                             (unsigned char)fminf(255.0f, gg2 + (lum - gg2) * DESAT),
                             (unsigned char)fminf(255.0f, bb2 + (lum - bb2) * DESAT), 255 };
                if (tt == TileType::BrokenFloor) ft = shade(WHITE, 0.62f * fog * macro);
                const float SPAN = 192.0f;   // 1 repetição = 3 tiles
                float u0 = rx / SPAN, u1 = (rx + TS) / SPAN;
                float v0 = ry / SPAN, v1 = (ry + TS) / SPAN;
                rlColor4ub(ft.r, ft.g, ft.b, 255);
                rlTexCoord2f(u0, v0); rlVertex3f(rx,      0.02f, ry);
                rlTexCoord2f(u0, v1); rlVertex3f(rx,      0.02f, ry + TS);
                rlTexCoord2f(u1, v1); rlVertex3f(rx + TS, 0.02f, ry + TS);
                rlTexCoord2f(u1, v0); rlVertex3f(rx + TS, 0.02f, ry);
            } else {
                // Fallback para piso sólido
                DrawPlane(floorCtr, { TS, TS }, shade(base, 0.45f));
                Vector3 tileTop = { floorCtr.x, 0.02f, floorCtr.z };
                DrawPlane(tileTop, { TS - 5.0f, TS - 5.0f }, floorColor);
            }

            // Paredes (tipo Wall ou cenário sólido) = cubos com volume texturizados.
            // Com textura ficam para o SEGUNDO loop, depois do rlEnd() do piso —
            // nao da pra desenhar cubo com um rlBegin de quads aberto.
            if (!sb.ready && inB && tt == TileType::Wall) {
                const float WALL_H = openWorld ? TS * 0.8f : TS * 1.6f;
                Vector3 c = { rx + TS * 0.5f, WALL_H * 0.5f, ry + TS * 0.5f };
                Color wc = shade(base, 1.6f);
                DrawCube(c, TS, WALL_H, TS, wc);
                DrawCubeWires(c, TS, WALL_H, TS, shade(base, 2.0f));
            }
        }
    }
    if (sb.ready) {
        rlEnd();
        rlSetTexture(rlGetTextureIdDefault());
        // Segundo loop: so paredes (poucas), cada uma um DrawCubeTexture proprio.
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                if (!(x >= 0 && x < width && y >= 0 && y < height)) continue;
                if (tiles[y][x].type != TileType::Wall) continue;
                float rx = x * TS, ry = y * TS;
                if (!floorTileVisible(floorView, cam3D, aspect,
                                      rx + TS * 0.5f, ry + TS * 0.5f, TS * 1.5f)) continue;
                float fdx = (float)(x - ctx), fdy = (float)(y - cty);
                float fdist = sqrtf(fdx*fdx + fdy*fdy);
                float fog = 1.0f - (fdist - R * 0.62f) / (R * 0.38f);
                if (fog < 0.58f) fog = 0.58f; if (fog > 1.0f) fog = 1.0f;
                const float WALL_H = openWorld ? TS * 0.8f : TS * 1.6f;
                Vector3 c = { rx + TS * 0.5f, WALL_H * 0.5f, ry + TS * 0.5f };
                DrawCubeTexture(sb.tileWall[(int)z], c, TS, WALL_H, TS, shade(WHITE, fog));
            }
        }
    }
    // P0 fix: religa a textura BRANCA padrão uma vez (rlSetTexture(0) não faz isso)
    // — senão a textura do piso/parede vaza e TINGE todas as primitivas 3D seguintes.
    rlSetTexture(rlGetTextureIdDefault());

    // ── RUAS (geometria, nao tint por tile) ──────────────────────────────────
    // Tentei primeiro tingir o TILE: com 64u de tile nao da para desenhar uma
    // pista de 92u nem uma faixa central de 10u — saiam blocos amarelos jogados
    // pelo chao. Agora a via e um quad proprio, entao a largura e a faixa saem
    // exatas e a cidade ganha direcao: quarteirao, rua, calcada.
    if (openWorld && (currentZone == ZoneID::LARuins || currentZone == ZoneID::GhostCity)) {
        const float SP    = 950.0f;   // mesmo passo da grade de predios
        const float HALFW = 105.0f;   // pista de duas maos
        const float REACH = 1500.0f;  // so o trecho visivel
        float cx0 = camTarget.x - REACH, cx1 = camTarget.x + REACH;
        float cz0 = camTarget.y - REACH, cz1 = camTarget.y + REACH;

        Color asf  = { 78, 78, 84, 255 };    // asfalto
        Color curb = { 122, 120, 116, 255 }; // meio-fio
        Color lane = { 168, 150, 84, 255 };  // faixa central, ja desbotada
        Color hole = {  52,  50,  48, 255 };  // buraco / remendo no asfalto

        auto quad = [](float ax, float az, float bx, float bz, float y, Color c) {
            rlBegin(RL_QUADS);
            rlColor4ub(c.r, c.g, c.b, c.a);
            rlNormal3f(0.0f, 1.0f, 0.0f);
            rlVertex3f(ax, y, az); rlVertex3f(ax, y, bz);
            rlVertex3f(bx, y, bz); rlVertex3f(bx, y, az);
            rlEnd();
        };

        int k0 = (int)floorf((cx0 - 475.0f) / SP), k1 = (int)ceilf((cx1 - 475.0f) / SP);
        for (int k = k0; k <= k1; ++k) {            // vias no eixo X (correm em Z)
            float c = 475.0f + k * SP;
            quad(c - HALFW - 7.0f, cz0, c + HALFW + 7.0f, cz1, 0.60f, curb);
            quad(c - HALFW,        cz0, c + HALFW,        cz1, 1.00f, asf);
            // Faixa APAGADA: 45% dos tracos sumiram com o tempo e os que sobraram
            // sao curtos e sujos. Faixa perfeita e o que fazia a rua parecer nova.
            for (float zd = floorf(cz0 / 90.0f) * 90.0f; zd < cz1; zd += 90.0f) {
                unsigned hz = (unsigned)(zd * 0.37f) * 2654435761u ^ (unsigned)(k * 40503);
                if ((hz >> 7) % 100 < 45) continue;
                float len = 26.0f + (float)((hz >> 3) & 15);
                quad(c - 2.5f, zd, c + 2.5f, zd + len, 1.40f, lane);
            }
            // buracos e remendos no asfalto
            for (float zp = floorf(cz0 / 150.0f) * 150.0f; zp < cz1; zp += 150.0f) {
                unsigned hp = (unsigned)(zp * 0.11f) * 374761393u ^ (unsigned)(k * 19349663);
                if ((hp >> 5) % 100 < 55) continue;
                float off = ((float)((hp >> 9) & 63) / 63.0f - 0.5f) * (HALFW * 1.4f);
                float rw  = 12.0f + (float)((hp >> 2) & 31);
                quad(c + off - rw, zp, c + off + rw, zp + rw * 1.3f, 1.20f, hole);
            }
        }
        int m0 = (int)floorf((cz0 - 475.0f) / SP), m1 = (int)ceilf((cz1 - 475.0f) / SP);
        for (int m = m0; m <= m1; ++m) {            // vias no eixo Z (correm em X)
            float c = 475.0f + m * SP;
            quad(cx0, c - HALFW - 7.0f, cx1, c + HALFW + 7.0f, 0.62f, curb);
            quad(cx0, c - HALFW,        cx1, c + HALFW,        1.02f, asf);
            for (float xd = floorf(cx0 / 90.0f) * 90.0f; xd < cx1; xd += 90.0f) {
                unsigned hz = (unsigned)(xd * 0.37f) * 2246822519u ^ (unsigned)(m * 40503);
                if ((hz >> 7) % 100 < 45) continue;
                float len = 26.0f + (float)((hz >> 3) & 15);
                quad(xd, c - 2.5f, xd + len, c + 2.5f, 1.42f, lane);
            }
            for (float xp = floorf(cx0 / 150.0f) * 150.0f; xp < cx1; xp += 150.0f) {
                unsigned hp = (unsigned)(xp * 0.11f) * 668265263u ^ (unsigned)(m * 19349663);
                if ((hp >> 5) % 100 < 55) continue;
                float off = ((float)((hp >> 9) & 63) / 63.0f - 0.5f) * (HALFW * 1.4f);
                float rw  = 12.0f + (float)((hp >> 2) & 31);
                quad(xp, c + off - rw, xp + rw * 1.3f, c + off + rw, 1.22f, hole);
            }
        }
        rlSetTexture(rlGetTextureIdDefault());
    }
}

bool Tilemap::isWallAtPosition(Vector2 pos) const {
    int x = (int)(pos.x / tileSize);
    int y = (int)(pos.y / tileSize);
    return isWall(x, y);
}

bool Tilemap::isWallAtPosition(Vector2 pos, float radius) const {
    if (radius <= 0.0f) return isWallAtPosition(pos);
    int minX = (int)((pos.x - radius) / tileSize);
    int minY = (int)((pos.y - radius) / tileSize);
    int maxX = (int)((pos.x + radius) / tileSize);
    int maxY = (int)((pos.y + radius) / tileSize);
    float r2 = radius * radius;
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (!isWall(x, y)) continue;
            float nearestX = pos.x < x * tileSize ? x * tileSize : (pos.x > (x + 1) * tileSize ? (x + 1) * tileSize : pos.x);
            float nearestY = pos.y < y * tileSize ? y * tileSize : (pos.y > (y + 1) * tileSize ? (y + 1) * tileSize : pos.y);
            float dx = pos.x - nearestX;
            float dy = pos.y - nearestY;
            if (dx * dx + dy * dy <= r2) return true;
        }
    }
    return false;
}

bool Tilemap::isPortalAtPosition(Vector2 pos, ZoneID& outDest) const {
    int x = (int)(pos.x / tileSize);
    int y = (int)(pos.y / tileSize);
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    if (tiles[y][x].type == TileType::Portal) {
        outDest = (ZoneID)tiles[y][x].portalZone;
        return true;
    }
    return false;
}


