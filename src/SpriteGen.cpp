#include "SpriteGen.h"
#include <cmath>

// ── PRNG deterministic (not usa rand/Math.random) ───────────────────────────
static unsigned int s_rng = 0xC0FFEE11;
static inline int   irnd(int n) { s_rng = s_rng * 1664525u + 1013904223u; return (int)((s_rng >> 8) % (unsigned)n); }
static inline float frnd()      { s_rng = s_rng * 1664525u + 1013904223u; return (float)((s_rng >> 8) & 0xFFFF) / 65535.0f; }

static Color shade(Color c, float f) {
    auto cl = [](float v){ return (unsigned char)(v < 0 ? 0 : v > 255 ? 255 : v); };
    return { cl(c.r * f), cl(c.g * f), cl(c.b * f), c.the };
}

SpriteBank& SpriteBank::get() {
    static SpriteBank inst;
    return inst;
}

void SpriteBank::init() {
    if (ready) return;
    buildTiles();
    buildPlayer();
    buildEnemies();
    buildScenery();
    buildCharAvatars();
    ready = true;
}

void SpriteBank::shutdown() {
    if (!ready) return;
    for (int i = 0; i < NUM_ZONE_TILES; ++i) {
        UnloadTexture(tileFloor[i]);
        UnloadTexture(tileWall[i]);
    }
    for (int d = 0; d < PLAYER_DIRS; ++d)
        for (int f = 0; f < PLAYER_FRAMES; ++f)
            UnloadTexture(player[d][f]);
    for (int and = 0; and < NUM_ENEMY_TYPES; ++and)
        for (int f = 0; f < ENEMY_FRAMES; ++f)
            UnloadTexture(enemy[and][f]);
    for (int s = 0; s < NUM_SCENERY; ++s)
        for (int v = 0; v < SCENERY_VARIANTS; ++v)
            UnloadTexture(scenery[s][v]);
    for (int the = 0; the < NUM_CHAR_AVATARS; ++the)
        UnloadTexture(charAvatar[the]);
    ready = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// TILES — floor and wall texturizados by biome (64x64), with granulado and detalhe
// ─────────────────────────────────────────────────────────────────────────────

// paletas base by ZoneID: {chaoA, chaoB, paredeA, paredeB}
struct ZonePal { Color floorA, floorB, wallA, wallB; };

static ZonePal zonePalette(int zone) {
    switch (zone) {
        // EXPOSICAO: the values antigos (comentados the right) ficavam in ~19% of
        // luminancia and still apanhavam fog * mascara of light * vignette = ~5% in the screen.
        // Subo the base and ABRO the delta floorA/floorB to texture really read.
        case 0:  return {{112,116,124,255},{ 78, 81, 88,255},{134,140,152,255},{ 92, 97,107,255}};  // LARuins asphalt
        case 1:  return {{ 86,102, 94,255},{ 60, 73, 67,255},{114,134,120,255},{ 80, 96, 86,255}};  // Bunker
        case 2:  return {{126, 84, 50,255},{ 88, 58, 34,255},{ 90, 62, 40,255},{ 62, 42, 26,255}};  // KronosForge
        case 3:  return {{ 68, 44,102,255},{ 44, 26, 74,255},{112, 60,158,255},{ 76, 38,116,255}};  // KronosNexus void
        case 4:  return {{ 92, 96, 82,255},{ 62, 66, 55,255},{112,110, 98,255},{ 82, 80, 71,255}};  // Cemetery terra
        case 5:  return {{124,128, 70,255},{ 88, 94, 48,255},{136,102, 58,255},{ 98, 72, 42,255}};  // CursedFarm grass seca
        case 6:  return {{100,104,116,255},{ 70, 74, 85,255},{120,124,138,255},{ 86, 90,104,255}};  // GhostCity concreto
        case 7:  return {{ 68, 92, 58,255},{ 44, 64, 38,255},{ 76, 66, 48,255},{ 52, 46, 34,255}};  // DarkForest
        case 8:  return {{ 80, 76, 88,255},{ 54, 51, 61,255},{100, 92,108,255},{ 70, 64, 78,255}};  // Catacombs
        case 9:  return {{ 92, 76, 96,255},{ 64, 51, 68,255},{114, 92,118,255},{ 82, 64, 88,255}};  // Manor
        case 10: return {{142, 62, 36,255},{102, 40, 24,255},{ 88, 40, 26,255},{ 60, 26, 18,255}};  // Inferno rocha
        default: return {{104,104,112,255},{ 74, 74, 81,255},{128,128,138,255},{ 92, 92,100,255}};
    }
}

static Texture2D makeFloorTex(int zone) {
    const int S = 128;
    Image img = GenImageColor(S, S, BLANK);
    ZonePal p = zonePalette(zone);

    // Base REALISTA: ruido of low frequencia (manchas suaves) + grao fino —
    // without xadrez. Grade 9x9 of values aleatorios interpolada bilinearmente.
    float ng[9][9];
    for (int gi = 0; gi < 8; ++gi) for (int gj = 0; gj < 8; ++gj) ng[gi][gj] = frnd();
    for (int gi = 0; gi < 8; ++gi) ng[gi][8] = ng[gi][0];   // tileavel: edge = start (without seam)
    for (int gj = 0; gj < 9; ++gj) ng[8][gj] = ng[0][gj];
    for (int y = 0; y < S; ++y) {
        for (int x = 0; x < S; ++x) {
            float fxx = x / (float)S * 8.0f, fyy = y / (float)S * 8.0f;
            int ix = (int)fxx, iy = (int)fyy; float txx = fxx - ix, tyy = fyy - iy;
            float blob = ng[iy][ix]   * (1-txx)*(1-tyy) + ng[iy][ix+1]   * txx*(1-tyy)
                       + ng[iy+1][ix] * (1-txx)*tyy     + ng[iy+1][ix+1] * txx*tyy;
            // 2a oitava: detalhe fino by up of the manchas largas. Uma oitava only
            // some in the mipmap the essa distance of camera and the piso vira smooth.
            float o2 = 0.5f + 0.5f * sinf(x * 0.49f) * cosf(y * 0.41f);
            blob = blob * 0.72f + o2 * 0.28f;
            Color base = ColorLerp(p.floorB, p.floorA, blob);   // manchas between 2 tons
            float n    = 0.86f + frnd() * 0.26f;                 // grao fino (more contrast)
            ImageDrawPixel(&img, x, y, shade(base, n));
        }
    }

    // Detalhe by biome
    switch (zone) {
        case 0: case 6: // asphalt/concreto — rachaduras
            for (int i = 0; i < 5; ++i) {
                int x0 = irnd(S), y0 = irnd(S);
                int x1 = x0 + irnd(18) - 9, y1 = y0 + irnd(18) - 9;
                ImageDrawLine(&img, x0, y0, x1, y1, shade(p.floorA, 0.6f));
            }
            break;
        case 4: case 9: // terra — pedrinhas
            for (int i = 0; i < 14; ++i)
                ImageDrawCircle(&img, irnd(S), irnd(S), 1, shade(p.floorA, 1.3f));
            break;
        case 5: case 7: // grass — tufos
            for (int i = 0; i < 22; ++i) {
                int gx = irnd(S), gy = irnd(S);
                ImageDrawLine(&img, gx, gy, gx, gy - 2 - irnd(2), shade(p.floorA, 1.25f));
            }
            break;
        case 10: // lava — veios brilhantes
            for (int i = 0; i < 6; ++i) {
                int x0 = irnd(S), y0 = irnd(S);
                ImageDrawLine(&img, x0, y0, x0 + irnd(10) - 5, y0 + irnd(10), Color{255,140,30,200});
            }
            break;
        case 3: // void — points of energy
            for (int i = 0; i < 8; ++i)
                ImageDrawPixel(&img, irnd(S), irnd(S), Color{160,80,255,220});
            break;
        default: // tech / others — parafusos
            for (int i = 0; i < 6; ++i)
                ImageDrawCircle(&img, 6 + irnd(S-12), 6 + irnd(S-12), 1, shade(p.floorB, 0.7f));
            break;
    }

    // Edge sutil to reading of grade
    // (ground border removed — created grid/seam in 3D; audit P4)

    Texture2D t = LoadTextureFromImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);   // anti-shimmer in perspectiva
    SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    UnloadImage(img);
    return t;
}

static Texture2D makeWallTex(int zone) {
    const int S = 64;
    Image img = GenImageColor(S, S, BLANK);
    ZonePal p = zonePalette(zone);

    // Block of stone/metal with volume (topo clear, base dark)
    for (int y = 0; y < S; ++y) {
        for (int x = 0; x < S; ++x) {
            float v    = 1.0f - (float)y / S * 0.35f;     // shadow vertical
            Color base = ((x >> 4) + (y >> 4)) % 2 == 0 ? p.wallA : p.wallB;
            float n    = 0.9f + frnd() * 0.2f;
            ImageDrawPixel(&img, x, y, shade(base, v * n));
        }
    }
    // Juntas of tijolo
    for (int by = 0; by < S; by += 16) {
        ImageDrawLine(&img, 0, by, S, by, shade(p.wallB, 0.55f));
        int off = (by / 16) % 2 ? 16 : 0;
        for (int bx = off; bx < S; bx += 32)
            ImageDrawLine(&img, bx, by, bx, by + 16, shade(p.wallB, 0.55f));
    }
    // Highlight upper
    ImageDrawLine(&img, 0, 0, S, 0, shade(p.wallA, 1.3f));

    Texture2D t = LoadTextureFromImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    UnloadImage(img);
    return t;
}

void SpriteBank::buildTiles() {
    s_rng = 0xC0FFEE11;
    for (int z = 0; z < NUM_ZONE_TILES; ++z) {
        tileFloor[z] = makeFloorTex(z);
        tileWall [z] = makeWallTex(z);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PLAYER — soldier cybernetic 24x32, 4 directions, frames of caminhada
// ─────────────────────────────────────────────────────────────────────────────

static void drawPlayerFrame(Image* img, int dir, int frame) {
    // paleta
    Color armor   = {40, 70, 110, 255};
    Color armorLt = {70, 120, 180, 255};
    Color armorDk = {26, 44, 70, 255};
    Color skin    = {210, 170, 140, 255};
    Color visor   = {0, 230, 255, 255};
    Color metal   = {150, 160, 175, 255};
    Color boot    = {30, 32, 38, 255};

    int cx = 12; // center horizontal (img 24 of width)

    // bob/passada: frame 0 idle, 1 and 3 = legs alternadas, 2 = neutral
    int legPhase = (frame == 1) ? 1 : (frame == 3) ? -1 : 0;
    int bob      = (frame == 1 || frame == 3) ? 1 : 0;

    int top = 4 + bob;

    // ── Legs ──
    if (dir == 2 || dir == 3) { // side
        ImageDrawRectangle(img, cx - 3, 24, 4, 7 - 1, boot);
        ImageDrawRectangle(img, cx + 0, 24 - legPhase, 4, 7, boot);
    } else {
        ImageDrawRectangle(img, cx - 5, 24, 4, 7 + legPhase, boot);
        ImageDrawRectangle(img, cx + 1, 24, 4, 7 - legPhase, boot);
    }

    // ── Torso (armor) ──
    ImageDrawRectangle(img, cx - 6, top + 6, 12, 14, armor);
    ImageDrawRectangle(img, cx - 6, top + 6, 3, 14, armorDk);          // shadow side
    ImageDrawRectangle(img, cx + 3, top + 6, 3, 14, armorDk);
    ImageDrawRectangle(img, cx - 2, top + 8, 4, 10, armorLt);          // peitoral highlight
    // ombreiras
    ImageDrawRectangle(img, cx - 8, top + 6, 3, 4, metal);
    ImageDrawRectangle(img, cx + 5, top + 6, 3, 4, metal);

    // ── Arms ──
    int armSwing = legPhase; // arms acompanham the passada
    ImageDrawRectangle(img, cx - 8, top + 9 + armSwing, 3, 9, armorDk);
    ImageDrawRectangle(img, cx + 5, top + 9 - armSwing, 3, 9, armorDk);

    // ── Head / capacete ──
    ImageDrawRectangle(img, cx - 4, top, 8, 8, metal);
    ImageDrawRectangle(img, cx - 4, top, 8, 2, shade(metal, 1.2f));    // topo of the capacete
    if (dir == 0) { // virado to down — rosto visible
        ImageDrawRectangle(img, cx - 3, top + 3, 6, 4, skin);
        ImageDrawRectangle(img, cx - 3, top + 4, 6, 2, visor);        // view glowing
    } else if (dir == 1) { // virado to up — nuca
        ImageDrawRectangle(img, cx - 3, top + 3, 6, 4, shade(metal, 0.8f));
    } else { // side — middle rosto + view
        int fx = (dir == 3) ? cx : cx - 3;
        ImageDrawRectangle(img, fx, top + 3, 3, 4, skin);
        ImageDrawRectangle(img, fx, top + 4, 3, 2, visor);
    }

    // ── Weapon (side) ──
    if (dir == 3)      ImageDrawRectangle(img, cx + 6, top + 12, 7, 2, metal);
    else if (dir == 2) ImageDrawRectangle(img, cx - 13, top + 12, 7, 2, metal);
}

void SpriteBank::buildPlayer() {
    const int W = 24, H = 34;
    for (int d = 0; d < PLAYER_DIRS; ++d) {
        for (int f = 0; f < PLAYER_FRAMES; ++f) {
            Image img = GenImageColor(W, H, BLANK);
            drawPlayerFrame(&img, d, f);
            player[d][f] = LoadTextureFromImage(img);
            SetTextureFilter(player[d][f], TEXTURE_FILTER_POINT);
            UnloadImage(img);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// INIMIGOS — sprites pixel-art by categoria visual, 2 frames of anim idle
// ─────────────────────────────────────────────────────────────────────────────

enum ECat { CAT_ROBOT, CAT_ALIEN, CAT_GHOST, CAT_ZOMBIE, CAT_DEMON, CAT_HUMANOID };

static ECat enemyCategory(int t) {
    switch (t) {
        // Robos / Mecas
        case 0: case 1: case 2: case 3: case 5: case 6: case 13: case 16:
        case 26: case 27: case 28: case 29: case 44:
            return CAT_ROBOT;
        // Aliens / Zerg
        case 4: case 9: case 10: case 11: case 12: case 30: case 37: case 46:
            return CAT_ALIEN;
        // Fantasmas / Sombras
        case 17: case 18: case 22: case 24: case 25: case 45: case 47: case 48:
            return CAT_GHOST;
        // Zumbis / Mortos-vivos
        case 19: case 20: case 21: case 23: case 15: case 43:
            return CAT_ZOMBIE;
        // Demonios / Inferno
        case 34: case 36: case 38: case 39: case 40: case 50:
            return CAT_DEMON;
        // Humanoides corrompidos (resto)
        default:
            return CAT_HUMANOID;
    }
}

static bool enemyIsBoss(int t) {
    switch (t) {
        case 3: case 12: case 16: case 22: case 23:
        case 28: case 35: case 38: case 40: case 50: return true;
        default: return false;
    }
}

struct EPal { Color main, dark, light, accent, accent2; };

static EPal enemyPalette(int t) {
    ECat c = enemyCategory(t);
    // light variacao of matiz by type p/ diferenciar enemies of the same categoria
    int v = (t * 37) % 36 - 18;
    auto vary = [&](Color col) {
        auto cl = [](int x){ return (unsigned char)(x < 0 ? 0 : x > 255 ? 255 : x); };
        return Color{ cl(col.r + v), cl(col.g + v/2), cl(col.b - v/2), col.the };
    };
    EPal p;
    switch (c) {
        case CAT_ROBOT:
            p = {{96,102,116,255},{52,56,66,255},{150,158,172,255},{255,60,40,255},{255,180,40,255}};
            break;
        case CAT_ALIEN:
            p = {{116,58,150,255},{60,28,80,255},{170,110,200,255},{150,235,70,255},{220,80,255,255}};
            break;
        case CAT_GHOST:
            p = {{150,180,220,180},{70,90,130,160},{210,230,255,200},{120,255,255,255},{180,140,255,255}};
            break;
        case CAT_ZOMBIE:
            p = {{96,128,72,255},{50,68,38,255},{150,180,110,255},{150,40,40,255},{120,90,40,255}};
            break;
        case CAT_DEMON:
            p = {{72,34,28,255},{38,18,16,255},{120,60,40,255},{255,120,20,255},{255,210,60,255}};
            break;
        default: // HUMANOID
            p = {{72,84,120,255},{38,44,66,255},{120,135,180,255},{210,180,120,255},{120,220,255,255}};
            break;
    }
    p.main  = vary(p.main);
    p.light = vary(p.light);
    return p;
}

// draws the silhueta of the enemy in the Image (feet ~ H-2)
static void drawEnemySprite(Image* im, int W, int H, int type, int frame) {
    ECat cat  = enemyCategory(type);
    EPal p    = enemyPalette(type);
    bool boss = enemyIsBoss(type);
    int cx    = W / 2;
    int gy    = H - 2;                 // floor
    int bob   = (frame == 1) ? 1 : 0;  // respiracao
    float fs  = boss ? 1.0f : 1.0f;
    (void)fs;

    switch (cat) {
        case CAT_ROBOT: {
            int bw = boss ? 22 : 12;
            int bh = boss ? 22 : 14;
            int topY = gy - bh - 9 + bob;
            // legs
            int legSh = (frame == 1) ? 1 : 0;
            ImageDrawRectangle(im, cx - bw/3 - 1, gy - 9 + legSh, 4, 9, p.dark);
            ImageDrawRectangle(im, cx + bw/3 - 2, gy - 9 - legSh, 4, 9, p.dark);
            // torso angular
            ImageDrawRectangle(im, cx - bw/2, topY, bw, bh, p.main);
            ImageDrawRectangle(im, cx - bw/2, topY, 3, bh, p.dark);          // shadow
            ImageDrawRectangle(im, cx + bw/2 - 3, topY, 3, bh, p.dark);
            ImageDrawRectangle(im, cx - bw/4, topY + 3, bw/2, bh - 6, p.light); // peitoral
            // ombreiras
            ImageDrawRectangle(im, cx - bw/2 - 3, topY, 4, 5, p.light);
            ImageDrawRectangle(im, cx + bw/2 - 1, topY, 4, 5, p.light);
            // arms
            ImageDrawRectangle(im, cx - bw/2 - 3, topY + 5, 3, bh - 6, p.dark);
            ImageDrawRectangle(im, cx + bw/2,     topY + 5, 3, bh - 6, p.dark);
            // head + view red
            int hw = boss ? 12 : 8;
            ImageDrawRectangle(im, cx - hw/2, topY - 7, hw, 7, p.light);
            ImageDrawRectangle(im, cx - hw/2, topY - 5, hw, 2, p.accent);    // view
            if (boss) { // chifres/antenas of boss
                ImageDrawLine(im, cx - hw/2, topY - 7, cx - hw/2 - 3, topY - 12, p.dark);
                ImageDrawLine(im, cx + hw/2, topY - 7, cx + hw/2 + 3, topY - 12, p.dark);
            }
            break;
        }
        case CAT_ALIEN: {
            int bw = boss ? 22 : 13;
            int bh = boss ? 18 : 12;
            int cyB = gy - bh/2 - 6 + bob;
            // legs/garras
            int sw = (frame == 1) ? 2 : 0;
            ImageDrawLine(im, cx - 3, gy - 7, cx - 6 - sw, gy, p.dark);
            ImageDrawLine(im, cx + 3, gy - 7, cx + 6 + sw, gy, p.dark);
            ImageDrawLine(im, cx - 1, gy - 7, cx - 2, gy, p.dark);
            ImageDrawLine(im, cx + 1, gy - 7, cx + 2, gy, p.dark);
            // body organic (oval)
            ImageDrawCircle(im, cx, cyB, bh/2 + 1, p.dark);
            ImageDrawCircle(im, cx, cyB, bh/2 - 1, p.main);
            ImageDrawCircle(im, cx - 1, cyB - 1, bh/4, p.light);
            // head/cupula
            ImageDrawCircle(im, cx, cyB - bh/2 - 2, boss ? 6 : 4, p.dark);
            ImageDrawCircle(im, cx, cyB - bh/2 - 2, boss ? 4 : 3, p.accent2);
            // eyes acidos
            ImageDrawPixel(im, cx - 2, cyB - bh/2 - 2, p.accent);
            ImageDrawPixel(im, cx + 2, cyB - bh/2 - 2, p.accent);
            // garras laterais (presas)
            ImageDrawLine(im, cx - bw/2, cyB, cx - bw/2 - 4, cyB - 4 - sw, p.accent);
            ImageDrawLine(im, cx + bw/2, cyB, cx + bw/2 + 4, cyB - 4 + sw, p.accent);
            break;
        }
        case CAT_GHOST: {
            int r = boss ? 12 : 8;
            int cyB = gy - r - 6 + bob;
            // body float translucido
            ImageDrawCircle(im, cx, cyB, r, p.main);
            ImageDrawCircle(im, cx, cyB, r - 2, p.light);
            // cauda esfumacada (ondulante by frame)
            int waves = boss ? 5 : 3;
            for (int i = 0; i < waves; ++i) {
                int wy = cyB + r - 1 + i * 3;
                int off = ((i + frame) % 2 == 0) ? -2 : 2;
                int rr = r - i - 1; if (rr < 2) rr = 2;
                Color tail = p.main; tail.the = (unsigned char)(120 - i * 25);
                ImageDrawCircle(im, cx + off, wy, rr, tail);
            }
            // eyes brilhantes
            ImageDrawCircle(im, cx - r/2, cyB - 1, 2, p.accent);
            ImageDrawCircle(im, cx + r/2, cyB - 1, 2, p.accent);
            ImageDrawPixel(im, cx - r/2, cyB - 1, WHITE);
            ImageDrawPixel(im, cx + r/2, cyB - 1, WHITE);
            break;
        }
        case CAT_ZOMBIE: {
            int bw = boss ? 18 : 11;
            int bh = boss ? 18 : 13;
            int lean = 2; // curvado to front
            int topY = gy - bh - 8 + bob;
            // legs arrastando
            int legSh = (frame == 1) ? 2 : 0;
            ImageDrawRectangle(im, cx - 4, gy - 8 + legSh, 3, 8, p.dark);
            ImageDrawRectangle(im, cx + 2, gy - 8, 3, 8, p.dark);
            // torso curvado
            ImageDrawRectangle(im, cx - bw/2 + lean, topY, bw, bh, p.main);
            ImageDrawRectangle(im, cx - bw/2 + lean, topY, bw, bh, p.main);
            // rasgos
            ImageDrawLine(im, cx - 2 + lean, topY + 3, cx - 1 + lean, topY + bh - 2, p.dark);
            ImageDrawLine(im, cx + 3 + lean, topY + 2, cx + 3 + lean, topY + bh - 4, p.dark);
            // arm esticado to front
            int armSh = (frame == 1) ? 1 : 0;
            ImageDrawRectangle(im, cx + bw/2 - 2 + lean, topY + 3 + armSh, 8, 3, p.light);
            // head pendendo
            ImageDrawCircle(im, cx + lean + 2, topY - 1, boss ? 6 : 4, p.light);
            ImageDrawPixel(im, cx + lean + 1, topY - 1, p.accent); // eye sangrento
            ImageDrawPixel(im, cx + lean + 3, topY - 1, p.accent);
            break;
        }
        case CAT_DEMON: {
            int bw = boss ? 24 : 14;
            int bh = boss ? 22 : 14;
            int topY = gy - bh - 9 + bob;
            // legs/cascos
            ImageDrawRectangle(im, cx - bw/3, gy - 9, 5, 9, p.dark);
            ImageDrawRectangle(im, cx + bw/3 - 4, gy - 9, 5, 9, p.dark);
            // torso massudo
            ImageDrawRectangle(im, cx - bw/2, topY, bw, bh, p.main);
            ImageDrawRectangle(im, cx - bw/2, topY, 3, bh, p.dark);
            // rachaduras of ember (acende in the frame 1)
            Color ember = (frame == 1) ? p.accent2 : p.accent;
            ImageDrawLine(im, cx - 3, topY + 2, cx - 1, topY + bh - 3, ember);
            ImageDrawLine(im, cx + 2, topY + 3, cx + 4, topY + bh - 4, ember);
            ImageDrawLine(im, cx - 4, topY + bh/2, cx + 3, topY + bh/2 + 1, ember);
            // ombros/musculos
            ImageDrawRectangle(im, cx - bw/2 - 2, topY + 2, 4, 6, p.dark);
            ImageDrawRectangle(im, cx + bw/2 - 2, topY + 2, 4, 6, p.dark);
            // head + chifres
            int hw = boss ? 12 : 8;
            ImageDrawRectangle(im, cx - hw/2, topY - 7, hw, 7, p.dark);
            ImageDrawLine(im, cx - hw/2, topY - 7, cx - hw/2 - 3, topY - 13, p.light); // chifre L
            ImageDrawLine(im, cx + hw/2, topY - 7, cx + hw/2 + 3, topY - 13, p.light); // chifre R
            // eyes in ember
            ImageDrawPixel(im, cx - 2, topY - 4, p.accent2);
            ImageDrawPixel(im, cx + 2, topY - 4, p.accent2);
            break;
        }
        default: { // HUMANOID
            int bw = boss ? 16 : 10;
            int bh = boss ? 20 : 14;
            int topY = gy - bh - 9 + bob;
            // legs
            int legSh = (frame == 1) ? 1 : 0;
            ImageDrawRectangle(im, cx - 4, gy - 9 + legSh, 3, 9, p.dark);
            ImageDrawRectangle(im, cx + 1, gy - 9 - legSh, 3, 9, p.dark);
            // manto/torso
            ImageDrawRectangle(im, cx - bw/2, topY, bw, bh, p.main);
            ImageDrawRectangle(im, cx - bw/2, topY, bw, 3, p.light);   // ombro
            ImageDrawRectangle(im, cx - 1, topY + 3, 2, bh - 4, p.dark); // dobra of the manto
            // arms
            ImageDrawRectangle(im, cx - bw/2 - 2, topY + 3, 3, bh - 5, p.dark);
            ImageDrawRectangle(im, cx + bw/2 - 1, topY + 3, 3, bh - 5, p.dark);
            // capuz + rosto sombrio
            ImageDrawCircle(im, cx, topY - 2, boss ? 6 : 4, p.dark);
            ImageDrawPixel(im, cx - 1, topY - 2, p.accent2);
            ImageDrawPixel(im, cx + 1, topY - 2, p.accent2);
            // weapon/lamina (acento)
            ImageDrawLine(im, cx + bw/2 + 1, topY + bh - 2, cx + bw/2 + 5, topY - 2, p.accent);
            break;
        }
    }
}

void SpriteBank::buildEnemies() {
    for (int t = 0; t < NUM_ENEMY_TYPES; ++t) {
        bool boss = enemyIsBoss(t);
        int W = boss ? 48 : 32;
        int H = boss ? 52 : 40;
        for (int f = 0; f < ENEMY_FRAMES; ++f) {
            Image img = GenImageColor(W, H, BLANK);
            drawEnemySprite(&img, W, H, t, f);
            enemy[t][f] = LoadTextureFromImage(img);
            SetTextureFilter(enemy[t][f], TEXTURE_FILTER_POINT);
            UnloadImage(img);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CENARIO — houses, lapides, arvores, buildings, etc. (pixel-art atmospheric)
// ─────────────────────────────────────────────────────────────────────────────

// Telhado/triangle preenchido by scanlines (raylib 5.0 not has ImageDrawTriangle)
static void imgRoof(Image* img, int cx, int topY, int baseY, int halfBase, Color c) {
    int hgt = baseY - topY;
    if (hgt <= 0) return;
    for (int y = 0; y <= hgt; ++y) {
        float tt = (float)y / (float)hgt;        // 0 in the apice, 1 in the base
        int half = (int)(halfBase * tt);
        ImageDrawRectangle(img, cx - half, topY + y, half * 2 + 1, 1, c);
    }
}

// Window: moldura dark + vidro (lit = yellow, dim = blue dark)
static void imgWindow(Image* img, int x, int y, int w, int h, bool lit) {
    ImageDrawRectangle(img, x, y, w, h, Color{18,16,22,255});
    Color glass = lit ? Color{255,205,90,255} : Color{40,46,70,255};
    ImageDrawRectangle(img, x + 1, y + 1, w - 2, h - 2, glass);
    // cruz of the window
    ImageDrawRectangle(img, x + w/2, y, 1, h, Color{18,16,22,255});
    ImageDrawRectangle(img, x, y + h/2, w, 1, Color{18,16,22,255});
}

static void drawSceneryImg(Image* img, int type, int W, int H, int variant) {
    switch (type) {
        case 0: { // HOUSE — walls + roof + door + windows + chamine
            Color wallV[3]  = {{120,108,92,255},{96,100,108,255},{110,90,78,255}};
            Color roofV[3]  = {{86,40,32,255},{60,58,66,255},{70,48,36,255}};
            Color wall = wallV[variant], wallDk = shade(wall,0.7f);
            Color roof = roofV[variant], roofDk = shade(roof,0.7f);
            int wallTop = 26, wallH = H - wallTop;
            // body
            ImageDrawRectangle(img, 6, wallTop, W-12, wallH, wall);
            ImageDrawRectangle(img, 6, wallTop, 4, wallH, shade(wall,1.15f));   // light esq
            ImageDrawRectangle(img, W-10, wallTop, 4, wallH, wallDk);           // shadow dir
            // roof
            imgRoof(img, W/2, 2, wallTop+2, W/2-2, roof);
            imgRoof(img, W/2, 4, wallTop, W/2-6, roofDk);
            ImageDrawRectangle(img, 4, wallTop, W-8, 3, shade(roof,0.5f));      // beiral
            // chamine
            ImageDrawRectangle(img, W-22, 6, 7, 16, shade(wall,0.6f));
            ImageDrawRectangle(img, W-24, 4, 11, 4, shade(wall,0.5f));
            // door
            ImageDrawRectangle(img, W/2-7, H-18, 14, 18, Color{55,38,24,255});
            ImageDrawRectangle(img, W/2-6, H-17, 12, 16, Color{70,48,30,255});
            ImageDrawCircle(img, W/2+3, H-9, 1, Color{200,180,90,255});         // macaneta
            // windows (acesas in the variante 0)
            bool lit = (variant == 0);
            imgWindow(img, 12, wallTop+8, 12, 12, lit);
            imgWindow(img, W-24, wallTop+8, 12, 12, lit && variant!=2);
            // rachaduras
            ImageDrawLine(img, 16, wallTop+24, 20, H-6, shade(wall,0.5f));
            break;
        }
        case 1: { // BARN — red with tabuas and door dupla
            Color red[3] = {{120,42,28,255},{104,36,24,255},{92,46,30,255}};
            Color barn = red[variant], barnDk = shade(barn,0.65f);
            int top = 22;
            ImageDrawRectangle(img, 4, top, W-8, H-top, barn);
            for (int r = top+4; r < H; r += 7)                    // tabuas
                ImageDrawLine(img, 4, r, W-4, r, shade(barn,0.6f));
            ImageDrawRectangle(img, 4, top, 3, H-top, shade(barn,1.2f));
            ImageDrawRectangle(img, W-7, top, 3, H-top, barnDk);
            // roof gambrel (duas inclinacoes)
            imgRoof(img, W/2, 2, top, W/2-2, Color{50,40,30,255});
            ImageDrawRectangle(img, 2, top, W-4, 3, Color{34,26,18,255});
            // window redonda at the top
            ImageDrawCircle(img, W/2, top+8, 5, barnDk);
            ImageDrawCircle(img, W/2, top+8, 3, Color{200,170,90,255});
            // doors duplas
            ImageDrawRectangle(img, W/2-16, H-26, 14, 26, barnDk);
            ImageDrawRectangle(img, W/2+2, H-26, 14, 26, barnDk);
            ImageDrawLine(img, W/2-9, H-26, W/2-9, H-1, shade(barn,1.3f));
            ImageDrawLine(img, W/2+9, H-26, W/2+9, H-1, shade(barn,1.3f));
            break;
        }
        case 2: { // TREE — variante 0 viva (copa green), 1-2 mortas/secas
            Color barkV[3] = {{72,54,38,255},{54,46,40,255},{46,40,30,255}};
            Color bark = barkV[variant], barkL = shade(bark,1.4f), barkD = shade(bark,0.65f);
            int cx = W/2;
            // tronco with texture
            ImageDrawRectangle(img, cx-4, 24, 8, H-24, bark);
            ImageDrawRectangle(img, cx-2, 26, 2, H-28, barkL);          // light
            ImageDrawRectangle(img, cx+2, 26, 2, H-28, barkD);          // shadow
            for (int ty = 30; ty < H-4; ty += 9)                        // veios of the casca
                ImageDrawLine(img, cx-3, ty, cx+3, ty+2, barkD);
            // raizes salientes
            ImageDrawLine(img, cx, H-2, cx-9, H-1, bark);
            ImageDrawLine(img, cx, H-2, cx+8, H-1, bark);
            ImageDrawLine(img, cx-3, H-3, cx-7, H-1, barkD);

            if (variant == 0) {
                // TREE VIVA — galhos curtos + copa of folhagem in camadas
                ImageDrawLine(img, cx, 28, cx-9, 20, bark);
                ImageDrawLine(img, cx, 26, cx+9, 18, bark);
                Color leafD = {26,72,34,255}, leafM = {38,104,46,255}, leafL = {58,140,62,255};
                // massa of copa (varios circles sobrepostos)
                ImageDrawCircle(img, cx,    16, 15, leafD);
                ImageDrawCircle(img, cx-10, 20, 10, leafD);
                ImageDrawCircle(img, cx+10, 20, 10, leafD);
                ImageDrawCircle(img, cx-4,  12, 11, leafM);
                ImageDrawCircle(img, cx+6,  14, 10, leafM);
                ImageDrawCircle(img, cx,    10,  9, leafM);
                // highlights (light vinda of up-esq)
                ImageDrawCircle(img, cx-6,  9,  5, leafL);
                ImageDrawCircle(img, cx+3,  8,  4, leafL);
                // points of folha clear
                for (int k = 0; k < 10; ++k) {
                    int lx = cx - 13 + (k * 137) % 26;
                    int ly = 6  + (k * 71)  % 22;
                    ImageDrawPixel(img, lx, ly, leafL);
                }
            } else {
                // TREE SECA/MORTA — galhos retorcidos without folhas
                ImageDrawLine(img, cx, 30, cx-14, 14, bark);
                ImageDrawLine(img, cx-14, 14, cx-20, 8, bark);
                ImageDrawLine(img, cx, 26, cx+13, 12, bark);
                ImageDrawLine(img, cx+13, 12, cx+18, 6, bark);
                ImageDrawLine(img, cx, 38, cx-10, 30, bark);
                ImageDrawLine(img, cx, 34, cx+9, 24, bark);
                if (variant == 1) // corvo in the galho
                    ImageDrawCircle(img, cx-19, 8, 2, Color{20,20,24,255});
                else // poucas folhas mortas alaranjadas
                    for (int k=0;k<4;k++) ImageDrawPixel(img, cx-8+k*5, 12+(k%2)*4, Color{130,80,30,255});
            }
            break;
        }
        case 3: { // LAPIDE — stone arredondada with cruz, base, musgo
            Color stoneV[3] = {{120,120,128,255},{104,100,96,255},{96,104,112,255}};
            Color stone = stoneV[variant], stoneDk = shade(stone,0.65f);
            int cx = W/2;
            ImageDrawRectangle(img, 4, H-6, W-8, 6, stoneDk);              // base
            ImageDrawRectangle(img, cx-8, 8, 16, H-12, stone);            // body
            ImageDrawCircle(img, cx, 8, 8, stone);                        // topo arredondado
            ImageDrawRectangle(img, cx-8, 8, 3, H-14, shade(stone,1.2f)); // light
            ImageDrawRectangle(img, cx+5, 8, 3, H-14, stoneDk);
            if (variant != 2) { // cruz recorded
                ImageDrawRectangle(img, cx-1, 6, 2, 12, stoneDk);
                ImageDrawRectangle(img, cx-4, 9, 8, 2, stoneDk);
            } else { // "RIP"
                ImageDrawRectangle(img, cx-5, 12, 10, 2, stoneDk);
                ImageDrawRectangle(img, cx-5, 16, 10, 2, stoneDk);
            }
            ImageDrawRectangle(img, cx-7, H-9, 5, 3, Color{40,70,30,255}); // musgo
            break;
        }
        case 4: { // FENCE — postes + travessas
            Color woodV[3] = {{90,68,44,255},{72,72,78,255},{80,60,40,255}};
            Color wood = woodV[variant], woodDk = shade(wood,0.65f);
            for (int px = 4; px < W-2; px += 12) {
                ImageDrawRectangle(img, px, 4, 4, H-6, wood);
                ImageDrawRectangle(img, px, 4, 1, H-6, shade(wood,1.3f));
                ImageDrawRectangle(img, px, 2, 4, 2, woodDk);            // ponta
            }
            ImageDrawRectangle(img, 2, 10, W-4, 3, woodDk);              // travessa sup
            ImageDrawRectangle(img, 2, H-12, W-4, 3, woodDk);           // travessa inf
            break;
        }
        case 5: { // POLE DE LIGHT — haste + luminaria with glow
            Color pole = (variant==1)?Color{70,74,82,255}:Color{60,58,54,255};
            int cx = W/2;
            ImageDrawRectangle(img, cx-2, 10, 4, H-10, pole);            // haste
            ImageDrawRectangle(img, cx-1, 12, 1, H-12, shade(pole,1.4f));
            ImageDrawRectangle(img, cx-6, H-3, 12, 3, shade(pole,0.6f)); // base
            // arm + luminaria
            ImageDrawRectangle(img, cx, 8, 8, 2, pole);
            ImageDrawRectangle(img, cx+6, 8, 6, 4, Color{40,40,46,255});
            bool on = (variant != 2);
            Color glow = on ? Color{255,210,120,255} : Color{60,60,70,255};
            ImageDrawRectangle(img, cx+7, 10, 4, 3, glow);
            if (on) {                                                    // halo
                ImageDrawCircle(img, cx+9, 13, 5, Color{255,210,120,70});
                ImageDrawCircle(img, cx+9, 13, 3, Color{255,220,140,120});
            }
            break;
        }
        case 6: { // CAR ABANDONADO — body enferrujado, windows quebradas
            Color bodyV[3] = {{90,70,50,255},{70,80,80,255},{96,60,52,255}};
            Color body = bodyV[variant], bodyDk = shade(body,0.6f);
            int top = 8;
            ImageDrawRectangle(img, 4, H-14, W-8, 10, body);            // carroceria
            ImageDrawRectangle(img, 12, top, W-26, 8, body);           // cabine
            ImageDrawRectangle(img, 4, H-14, W-8, 2, shade(body,1.2f));
            ImageDrawRectangle(img, 4, H-6, W-8, 2, bodyDk);
            // windows quebradas
            ImageDrawRectangle(img, 14, top+1, 10, 6, Color{30,34,40,255});
            ImageDrawRectangle(img, W-22, top+1, 8, 6, Color{30,34,40,255});
            ImageDrawLine(img, 14, top+1, 24, top+7, Color{120,130,140,255});
            // ferrugem
            ImageDrawCircle(img, 20, H-9, 2, Color{120,60,20,255});
            ImageDrawCircle(img, W-16, H-10, 2, Color{120,60,20,255});
            // rodas (uma faltando in the variante 0)
            ImageDrawCircle(img, 14, H-3, 3, Color{20,20,22,255});
            if (variant != 0) ImageDrawCircle(img, W-14, H-3, 3, Color{20,20,22,255});
            break;
        }
        case 7: { // BUILDING — varios andares of windows, silhueta urbana
            Color concV[3] = {{70,72,80,255},{60,58,64,255},{78,74,70,255}};
            Color conc = concV[variant], concDk = shade(conc,0.7f);
            ImageDrawRectangle(img, 2, 4, W-4, H-4, conc);
            ImageDrawRectangle(img, 2, 4, 3, H-4, shade(conc,1.15f));   // light
            ImageDrawRectangle(img, W-5, 4, 3, H-4, concDk);            // shadow
            ImageDrawRectangle(img, 2, 2, W-4, 3, concDk);             // topo
            // grade of windows
            for (int wy = 10; wy < H-8; wy += 12) {
                for (int wx = 8; wx < W-10; wx += 12) {
                    bool lit = (((wx*7 + wy*13 + variant*5) >> 2) & 3) == 0;
                    imgWindow(img, wx, wy, 7, 8, lit);
                }
            }
            // antena at the top of the variante 1
            if (variant == 1) {
                ImageDrawRectangle(img, W/2, 0, 1, 6, Color{40,40,46,255});
                ImageDrawCircle(img, W/2, 0, 1, Color{255,60,60,255});
            }
            break;
        }
        case 8: { // SILO — cilindro metallic with topo conico
            Color metV[3] = {{120,124,130,255},{104,106,110,255},{110,116,126,255}};
            Color met = metV[variant], metDk = shade(met,0.6f);
            int cx = W/2, bodyTop = 18;
            ImageDrawRectangle(img, 6, bodyTop, W-12, H-bodyTop, met);
            ImageDrawRectangle(img, 6, bodyTop, 4, H-bodyTop, shade(met,1.2f));
            ImageDrawRectangle(img, W-10, bodyTop, 4, H-bodyTop, metDk);
            for (int r = bodyTop+6; r < H; r += 10)                    // aneis
                ImageDrawLine(img, 6, r, W-6, r, metDk);
            imgRoof(img, cx, 2, bodyTop, W/2-6, shade(met,0.8f));      // topo conico
            ImageDrawCircle(img, cx, bodyTop+4, 2, metDk);
            if (variant == 2)                                         // ferrugem
                ImageDrawRectangle(img, 8, H-16, 4, 10, Color{110,55,20,255});
            break;
        }
        case 9: { // ARCO DE CATACOMB — arco of stone with tijolos
            Color stV[3] = {{96,92,98,255},{84,80,86,255},{100,96,90,255}};
            Color st = stV[variant], stDk = shade(st,0.6f);
            int legW = 12;
            ImageDrawRectangle(img, 4, 16, legW, H-16, st);            // leg esq
            ImageDrawRectangle(img, W-4-legW, 16, legW, H-16, st);     // leg dir
            // arco (meia returns) by scanlines
            int cx = W/2, rad = (W-8)/2;
            for (int y = 0; y <= 18; ++y) {
                float tt = (float)y/18.0f;
                int half = (int)(rad * std::sqrt(1.0f - tt*tt));
                ImageDrawRectangle(img, cx-half, 16-y, 2, 1, st);
                ImageDrawRectangle(img, cx+half-1, 16-y, 2, 1, st);
                ImageDrawRectangle(img, cx-half, 16-y, half*2, 1, (y<4)?st:BLANK);
            }
            // block upper
            for (int yy=0; yy<16; yy++)
                ImageDrawRectangle(img, cx-rad, yy, rad*2, 1, (yy<3)?st:BLANK);
            ImageDrawRectangle(img, cx-rad, 0, rad*2, 4, st);
            // juntas of tijolo
            for (int by=20; by<H; by+=8) {
                ImageDrawLine(img, 4, by, 4+legW, by, stDk);
                ImageDrawLine(img, W-4-legW, by, W-4, by, stDk);
            }
            break;
        }
        case 10: { // STATUE — figura/anjo in stone with pedestal
            Color stV[3] = {{140,138,132,255},{120,124,130,255},{132,126,120,255}};
            Color st = stV[variant], stDk = shade(st,0.65f), stL = shade(st,1.2f);
            int cx = W/2;
            // pedestal
            ImageDrawRectangle(img, cx-12, H-12, 24, 12, stDk);
            ImageDrawRectangle(img, cx-10, H-14, 20, 4, st);
            // body (manto)
            ImageDrawRectangle(img, cx-7, 24, 14, H-36, st);
            ImageDrawRectangle(img, cx-7, 24, 3, H-36, stL);
            ImageDrawRectangle(img, cx+4, 24, 3, H-36, stDk);
            // head
            ImageDrawCircle(img, cx, 18, 6, st);
            ImageDrawCircle(img, cx-2, 16, 1, stL);
            // arms / asas conforme variante
            if (variant == 0) { // asas of anjo
                ImageDrawLine(img, cx-7, 28, cx-16, 20, stL);
                ImageDrawLine(img, cx-7, 32, cx-15, 30, stL);
                ImageDrawLine(img, cx+7, 28, cx+16, 20, stL);
                ImageDrawLine(img, cx+7, 32, cx+15, 30, stL);
            } else { // arms
                ImageDrawRectangle(img, cx-11, 28, 4, 14, st);
                ImageDrawRectangle(img, cx+7, 28, 4, 14, st);
            }
            // musgo/desgaste
            ImageDrawRectangle(img, cx-6, H-20, 3, 6, Color{50,70,40,255});
            break;
        }
        default:
            ImageDrawRectangle(img, 4, 4, W-8, H-8, Color{100,100,100,255});
            break;
    }
}

void SpriteBank::buildScenery() {
    // Tamanhos by type (width, height)
    static const int dims[NUM_SCENERY][2] = {
        {64,72},  // 0 house
        {64,64},  // 1 barn
        {48,72},  // 2 tree morta
        {28,40},  // 3 lapide
        {44,28},  // 4 fence
        {20,64},  // 5 pole
        {52,32},  // 6 car
        {64,112}, // 7 building
        {40,80},  // 8 silo
        {56,56},  // 9 arco
        {36,66},  // 10 statue
    };
    s_rng = 0x5CE0E12A;
    for (int t = 0; t < NUM_SCENERY; ++t) {
        for (int v = 0; v < SCENERY_VARIANTS; ++v) {
            int W = dims[t][0], H = dims[t][1];
            Image img = GenImageColor(W, H, BLANK);
            drawSceneryImg(&img, t, W, H, v);
            scenery[t][v] = LoadTextureFromImage(img);
            SetTextureFilter(scenery[t][v], TEXTURE_FILTER_POINT);
            UnloadImage(img);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AVATARES of the classes — retratos (busto) 96x120 for the screen of selecao
// index = CharacterClass { Soldier, Guerreira, Robo, Mage, Bruxa, HomemFera }
// ─────────────────────────────────────────────────────────────────────────────

static void avatarBackground(Image* img, int W, int H, Color halo) {
    for (int y = 0; y < H; ++y) {
        float t = (float)y / H;
        Color base = { (unsigned char)(10 + 8*t), (unsigned char)(12 + 8*t),
                       (unsigned char)(20 + 10*t), 255 };
        for (int x = 0; x < W; ++x) ImageDrawPixel(img, x, y, base);
    }
    int hx = W/2, hy = H*38/100;
    for (int r = 40; r > 0; r -= 2) {
        float the = (1.0f - r/40.0f) * 0.22f;
        ImageDrawCircle(img, hx, hy, r, ColorAlpha(halo, the));
    }
    ImageDrawRectangleLines(img, {0,0,(float)W,(float)H}, 2, shade(halo, 0.8f));
}

static void buildOneAvatar(Image* img, int cls, int W, int H) {
    int cx = W/2;
    switch (cls) {
        case 0: { // SOLDADO — cyborg middle-homem middle-maquina
            avatarBackground(img, W, H, Color{60,110,190,255});
            Color skin={210,170,140,255}, skinDk={170,130,105,255};
            Color steel={120,135,160,255}, steelLt={170,185,210,255};
            Color visor={0,230,255,255}, armor={45,75,120,255};
            ImageDrawRectangle(img, cx-34, H-34, 68, 34, armor);
            ImageDrawRectangle(img, cx-34, H-34, 68, 4, steelLt);
            ImageDrawRectangle(img, cx-8, H-44, 16, 14, skinDk);
            ImageDrawRectangle(img, cx-30, 18, 30, 60, skin);
            ImageDrawCircle(img, cx-2, 48, 28, skin);
            ImageDrawRectangle(img, cx, 20, 28, 56, steel);
            ImageDrawRectangle(img, cx, 20, 4, 56, steelLt);
            ImageDrawRectangle(img, cx-32, 16, 64, 14, armor);
            ImageDrawRectangle(img, cx-32, 16, 64, 4, steelLt);
            ImageDrawRectangle(img, cx-16, 44, 8, 5, Color{30,30,40,255});
            ImageDrawCircle(img, cx-13, 46, 2, Color{255,255,255,255});
            ImageDrawRectangle(img, cx+6, 44, 18, 5, visor);
            ImageDrawRectangle(img, cx+6, 44, 18, 2, Color{200,255,255,255});
            ImageDrawRectangle(img, cx-14, 64, 10, 2, skinDk);
            break;
        }
        case 1: { // GUERREIRA — mulher esguia, rabo of cavalo, magenta
            avatarBackground(img, W, H, Color{200,70,130,255});
            Color skin={225,180,150,255}, skinDk={190,145,120,255};
            Color hair={90,40,60,255}, hairLt={140,70,100,255};
            Color cloth={170,50,95,255}, clothLt={210,90,130,255};
            Color band={255,80,160,255};
            ImageDrawRectangle(img, cx-26, H-30, 52, 30, cloth);
            ImageDrawRectangle(img, cx-26, H-30, 52, 3, clothLt);
            ImageDrawRectangle(img, cx-6, H-40, 12, 12, skinDk);
            ImageDrawRectangle(img, cx+18, 30, 10, 50, hair);
            ImageDrawRectangle(img, cx+20, 34, 5, 44, hairLt);
            ImageDrawCircle(img, cx, 40, 28, hair);
            ImageDrawCircle(img, cx, 50, 24, skin);
            ImageDrawRectangle(img, cx-24, 30, 48, 14, hair);
            ImageDrawRectangle(img, cx-24, 40, 48, 5, band);
            ImageDrawRectangle(img, cx-14, 52, 7, 4, Color{40,20,30,255});
            ImageDrawRectangle(img, cx+7, 52, 7, 4, Color{40,20,30,255});
            ImageDrawPixel(img, cx-11, 53, Color{255,255,255,255});
            ImageDrawPixel(img, cx+10, 53, Color{255,255,255,255});
            ImageDrawRectangle(img, cx-5, 64, 10, 2, Color{170,60,80,255});
            break;
        }
        case 2: { // ROBO — head retangular, view varredura red
            avatarBackground(img, W, H, Color{0,200,255,255});
            Color steel={110,120,135,255}, steelLt={165,178,195,255}, steelDk={60,68,82,255};
            Color visor={255,60,60,255};
            ImageDrawRectangle(img, cx-32, H-36, 64, 36, steelDk);
            ImageDrawRectangle(img, cx-32, H-36, 64, 5, steelLt);
            ImageDrawRectangle(img, cx-30, H-30, 6, 6, Color{0,200,255,255});
            ImageDrawRectangle(img, cx-7, H-46, 14, 12, steelDk);
            ImageDrawRectangle(img, cx-26, 22, 52, 52, steel);
            ImageDrawRectangle(img, cx-26, 22, 52, 5, steelLt);
            ImageDrawRectangle(img, cx-26, 22, 4, 52, steelLt);
            ImageDrawRectangle(img, cx+22, 22, 4, 52, steelDk);
            ImageDrawRectangle(img, cx-20, 42, 40, 9, Color{40,10,10,255});
            ImageDrawRectangle(img, cx-20, 44, 40, 4, visor);
            ImageDrawRectangle(img, cx+4, 44, 8, 4, Color{255,200,200,255});
            ImageDrawRectangle(img, cx-1, 10, 2, 14, steelLt);
            ImageDrawCircle(img, cx, 9, 3, Color{0,255,255,255});
            ImageDrawCircle(img, cx-20, 28, 2, steelDk);
            ImageDrawCircle(img, cx+20, 28, 2, steelDk);
            for (int i=0;i<5;i++) ImageDrawRectangle(img, cx-15+i*7, 60, 4, 8, steelDk);
            break;
        }
        case 3: { // MAGO — capuz, eyes brilhantes, barba, staff
            avatarBackground(img, W, H, Color{150,90,220,255});
            Color robe={70,45,120,255}, robeLt={110,75,170,255}, robeDk={45,28,80,255};
            Color shadow={20,15,35,255}, beard={225,225,235,255}, beardDk={180,180,200,255};
            Color eye={120,210,255,255};
            ImageDrawRectangle(img, cx-32, H-40, 64, 40, robe);
            ImageDrawRectangle(img, cx-32, H-40, 64, 4, robeLt);
            for (int y=14; y<58; ++y) {
                int hw = 8 + (y-14)*32/44;
                ImageDrawRectangle(img, cx-hw, y, hw*2, 1, (y<18)?robeDk:robe);
            }
            ImageDrawRectangle(img, cx-2, 12, 4, 6, robeLt);
            ImageDrawCircle(img, cx, 44, 16, shadow);
            ImageDrawRectangle(img, cx-10, 42, 6, 4, eye);
            ImageDrawRectangle(img, cx+4, 42, 6, 4, eye);
            ImageDrawPixel(img, cx-8, 43, Color{255,255,255,255});
            ImageDrawPixel(img, cx+6, 43, Color{255,255,255,255});
            for (int y=50; y<74; ++y) {
                int bw = 14 - (y-50)*10/24;
                ImageDrawRectangle(img, cx-bw, y, bw*2, 1, (y%3==0)?beardDk:beard);
            }
            ImageDrawRectangle(img, W-16, 24, 4, H-24, Color{90,60,30,255});
            ImageDrawCircle(img, W-14, 22, 6, Color{150,230,255,255});
            ImageDrawCircle(img, W-14, 22, 3, Color{255,255,255,255});
            break;
        }
        case 4: { // BRUXA — chapeu pontudo, cabelo, capa, varinha
            avatarBackground(img, W, H, Color{160,70,200,255});
            Color hat={50,20,70,255}, hatLt={90,45,120,255}, hatBand={200,90,230,255};
            Color skin={225,185,160,255}, hair={60,30,55,255}, hairLt={100,55,95,255};
            Color cape={80,40,110,255}, capeLt={120,70,160,255};
            ImageDrawRectangle(img, cx-32, H-34, 64, 34, cape);
            ImageDrawRectangle(img, cx-32, H-34, 64, 4, capeLt);
            ImageDrawCircle(img, cx, 54, 26, hair);
            ImageDrawRectangle(img, cx-26, 54, 8, 30, hair);
            ImageDrawRectangle(img, cx+18, 54, 8, 30, hairLt);
            ImageDrawCircle(img, cx, 56, 21, skin);
            for (int y=4; y<34; ++y) {
                int hw = (y-4)*16/30;
                ImageDrawRectangle(img, cx-hw, y, hw*2, 1, (y<8)?hat:((y%6==0)?hatLt:hat));
            }
            ImageDrawRectangle(img, cx-34, 32, 68, 7, hat);
            ImageDrawRectangle(img, cx-34, 32, 68, 2, hatLt);
            ImageDrawRectangle(img, cx-16, 28, 32, 4, hatBand);
            ImageDrawRectangle(img, cx-11, 52, 6, 4, Color{60,30,60,255});
            ImageDrawRectangle(img, cx+5, 52, 6, 4, Color{60,30,60,255});
            ImageDrawPixel(img, cx-9, 53, Color{255,255,255,255});
            ImageDrawPixel(img, cx+7, 53, Color{255,255,255,255});
            ImageDrawRectangle(img, cx-6, 64, 12, 2, Color{170,80,110,255});
            ImageDrawRectangle(img, W-15, 40, 3, 40, Color{40,30,50,255});
            ImageDrawCircle(img, W-13, 38, 5, Color{220,140,255,255});
            ImageDrawCircle(img, W-13, 38, 2, Color{255,255,255,255});
            break;
        }
        default: { // HOMEM-FERA — focinho, orelhas, presas, pelos
            avatarBackground(img, W, H, Color{200,120,40,255});
            Color fur={150,95,50,255}, furLt={185,130,80,255}, furDk={100,62,32,255};
            Color snout={120,75,45,255}, eye={255,210,40,255};
            ImageDrawRectangle(img, cx-34, H-34, 68, 34, furDk);
            for (int i=0;i<18;i++) ImageDrawLine(img, cx-30+i*4, H-34, cx-30+i*4-2, H-40, fur);
            ImageDrawCircle(img, cx, 50, 30, fur);
            ImageDrawLine(img, cx-26, 30, cx-32, 8, furDk);
            ImageDrawLine(img, cx-26, 30, cx-18, 14, furDk);
            ImageDrawLine(img, cx-30, 18, cx-22, 16, furLt);
            ImageDrawLine(img, cx+26, 30, cx+32, 8, furDk);
            ImageDrawLine(img, cx+26, 30, cx+18, 14, furDk);
            ImageDrawLine(img, cx+30, 18, cx+22, 16, furLt);
            for (int i=0;i<14;i++) ImageDrawLine(img, cx-24+i*4, 24, cx-26+i*4, 18, furLt);
            ImageDrawCircle(img, cx, 62, 13, snout);
            ImageDrawCircle(img, cx, 60, 5, Color{30,20,15,255});
            ImageDrawRectangle(img, cx-15, 46, 9, 5, eye);
            ImageDrawRectangle(img, cx+6, 46, 9, 5, eye);
            ImageDrawRectangle(img, cx-12, 47, 2, 3, Color{20,15,10,255});
            ImageDrawRectangle(img, cx+9, 47, 2, 3, Color{20,15,10,255});
            ImageDrawRectangle(img, cx-7, 68, 3, 6, Color{245,245,235,255});
            ImageDrawRectangle(img, cx+4, 68, 3, 6, Color{245,245,235,255});
            break;
        }
    }
}

void SpriteBank::buildCharAvatars() {
    const int W = 96, H = 120;
    for (int c = 0; c < NUM_CHAR_AVATARS; ++c) {
        Image img = GenImageColor(W, H, BLANK);
        buildOneAvatar(&img, c, W, H);
        charAvatar[c] = LoadTextureFromImage(img);
        SetTextureFilter(charAvatar[c], TEXTURE_FILTER_POINT);
        UnloadImage(img);
    }
}
