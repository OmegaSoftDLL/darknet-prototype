#pragma once
#include <string>
#include <raylib.h>

enum class ZoneID {
    LARuins       = 0,
    Bunker        = 1,
    KronosForge   = 2,
    KronosNexus   = 3,
    // ── Fases Sombrias ────────────────────────────────────────────────────────
    Cemetery      = 4,   // Cemiterio Abandonado
    CursedFarm    = 5,   // Fazenda Maldita
    GhostCity     = 6,   // Cidade Fantasma
    DarkForest    = 7,   // Floresta Negra
    Catacombs     = 8,   // Catacumbas (subterraneo)
    AbandonedManor= 9,   // Mansao Abandonada (area de boss)
    InfernoZone   = 10,  // Zona Vulcanica — aliens + zumbis + lava
    // Legacy aliases — backward compat while Tilemap.cpp is updated
    SkynetFactory = 2,   // alias KronosForge
    CoreFacility  = 3    // alias KronosNexus
};

struct ZoneInfo {
    std::string name;
    std::string description;
    Color floorColorA;
    Color floorColorB;
    Color wallColor;
    Color wallOutline;
    Color portalColor;
    int   wallDensity;
    float enemyScalar;
    float spawnInterval;
};

inline ZoneInfo getZoneInfo(ZoneID id) {
    switch (id) {
        case ZoneID::LARuins:
            return {"Ruinas de Los Angeles", "Os escombros da humanidade...",
                    {45,45,45,255}, {35,35,35,255}, {80,80,90,255}, {120,120,130,255},
                    {0,200,255,255}, 8, 1.0f, 3.0f};
        case ZoneID::Bunker:
            return {"Bunker NEXUS", "Base de operacoes da resistencia humana",
                    {25,50,25,255}, {18,38,18,255}, {35,70,35,255}, {55,110,55,255},
                    {0,255,100,255}, 5, 0.6f, 4.0f};
        case ZoneID::KronosForge:
            return {"Forja KRONOS", "Producao em massa de Enforcers",
                    {60,30,15,255}, {45,20,8,255}, {90,45,20,255}, {150,75,35,255},
                    {255,100,0,255}, 14, 1.8f, 2.0f};
        case ZoneID::KronosNexus:
            return {"Nucleo KRONOS", "O coracao de KRONOS. Destrua-o!",
                    {25,10,55,255}, {15,5,40,255}, {55,25,110,255}, {90,50,180,255},
                    {255,0,200,255}, 18, 2.5f, 1.5f};
        // ── Fases Sombrias ────────────────────────────────────────────────────
        case ZoneID::Cemetery:
            return {"Cemiterio Abandonado", "Os mortos nao descansam aqui...",
                    {40,42,40,255}, {28,30,28,255}, {55,58,55,255}, {80,85,80,255},
                    {100,255,180,255}, 6, 1.2f, 3.5f};
        case ZoneID::CursedFarm:
            return {"Fazenda Maldita", "A terra esta podre. As colheitas, corrompidas.",
                    {55,45,30,255}, {42,33,18,255}, {75,60,35,255}, {110,88,45,255},
                    {80,200,100,255}, 5, 1.1f, 3.0f};
        case ZoneID::GhostCity:
            return {"Cidade Fantasma", "Ruas vazias. Mas nao desertas.",
                    {35,35,42,255}, {22,22,30,255}, {50,50,65,255}, {75,75,95,255},
                    {150,180,255,255}, 8, 1.4f, 2.5f};
        case ZoneID::DarkForest:
            return {"Floresta Negra", "A nevoa esconde o que mora entre as arvores.",
                    {20,28,20,255}, {12,18,12,255}, {28,40,28,255}, {45,65,45,255},
                    {60,255,120,255}, 10, 1.3f, 3.0f};
        case ZoneID::Catacombs:
            return {"Catacumbas", "Passagens de pedra. Cheiro de morte antiga.",
                    {30,25,30,255}, {20,15,20,255}, {45,35,45,255}, {70,55,70,255},
                    {180,100,255,255}, 12, 1.6f, 2.0f};
        case ZoneID::AbandonedManor:
            return {"Mansao Abandonada", "O boss aguarda nas profundezas.",
                    {28,20,28,255}, {18,12,18,255}, {40,28,40,255}, {65,45,65,255},
                    {200,50,255,255}, 15, 2.0f, 1.8f};
        case ZoneID::InfernoZone:
            return {"Zona Inferno", "Lava, cinzas e criaturas do abismo.",
                    {55,15,5,255}, {40,8,2,255}, {90,30,10,255}, {160,60,20,255},
                    {255,80,0,255}, 10, 2.2f, 1.5f};
    }
    return {};
}

// ── PALETA DE CLUTTER POR BIOMA ─────────────────────────────────────────────
// Grama SEMPRE verde, arvore SEMPRE frondosa e entulho SEMPRE cinza eram o
// maior delator de "mesmo mundo com outra tinta": ~90% dos props da tela sao
// clutter. Cada bioma tem sua propria paleta de grama/folhagem/pedra/entulho.
struct ClutterPalette {
    Color grass;        // cor base da grama
    Color grassDirt;    // mancha de terra sob o tufo
    Color canopy[3];    // folhagem das arvores (3 especies)
    float canopyBulk;   // porte da copa; <= 0 = arvore MORTA (sem copa)
    Color rock;         // pedras/detritos
    Color slabA;        // entulho: laje clara
    Color slabB;        // entulho: laje escura
    Color gravel;       // cascalho claro das marcas no chao
};

inline const ClutterPalette& clutterPaletteFor(ZoneID z) {
    //                                       grama              terra             copa x3                                  bulk  pedra              laje A            laje B             cascalho
    static const ClutterPalette LA      = { {140,138, 70,255}, { 58, 52, 36,255}, {{ 96,116, 52,255},{110,124, 58,255},{ 88,104, 60,255}}, 0.85f, {118,116,112,255}, {118,114,108,255}, { 92, 88, 84,255}, {176,172,162,255} };
    static const ClutterPalette BUNKER  = { { 84,120, 60,255}, { 40, 50, 30,255}, {{ 74,118, 52,255},{ 90,132, 58,255},{ 66,106, 62,255}}, 0.90f, {112,116,108,255}, {112,116,108,255}, { 84, 88, 82,255}, {168,172,160,255} };
    static const ClutterPalette FORGE   = { {110, 70, 50,255}, { 48, 32, 26,255}, {{ 64, 44, 36,255},{ 74, 50, 38,255},{ 56, 40, 34,255}}, 0.40f, { 96, 64, 54,255}, { 96, 70, 60,255}, { 70, 50, 44,255}, {150,120,104,255} };
    static const ClutterPalette NEXUS   = { { 64,140,140,255}, { 28, 44, 50,255}, {{ 52,120,124,255},{ 60,132,120,255},{ 44,108,116,255}}, 0.80f, { 96,110,124,255}, { 92,106,120,255}, { 68, 80, 94,255}, {140,170,180,255} };
    static const ClutterPalette CEMIT   = { { 92, 86,104,255}, { 40, 36, 44,255}, {{ 58, 52, 62,255},{ 64, 58, 68,255},{ 52, 48, 56,255}}, 0.00f, {110,110,118,255}, {106,106,116,255}, { 78, 78, 88,255}, {160,158,170,255} };
    static const ClutterPalette FARM    = { { 96,128, 48,255}, { 52, 44, 26,255}, {{100,128, 50,255},{112,136, 56,255},{ 88,116, 52,255}}, 1.00f, {120,108, 92,255}, {116,104, 86,255}, { 88, 78, 64,255}, {172,160,138,255} };
    static const ClutterPalette GHOST   = { {126,122, 84,255}, { 48, 46, 40,255}, {{ 66, 62, 56,255},{ 72, 68, 60,255},{ 58, 56, 50,255}}, 0.00f, {108,110,116,255}, {104,106,112,255}, { 80, 82, 88,255}, {160,162,170,255} };
    static const ClutterPalette FOREST  = { { 54,110, 46,255}, { 26, 38, 22,255}, {{ 56,104, 44,255},{ 70,118, 50,255},{ 48, 92, 52,255}}, 1.15f, {100,110, 96,255}, { 96,104, 90,255}, { 70, 78, 66,255}, {150,160,144,255} };
    static const ClutterPalette CATAC   = { { 76, 64, 88,255}, { 30, 24, 36,255}, {{ 56, 48, 66,255},{ 62, 54, 72,255},{ 50, 44, 58,255}}, 0.00f, { 96, 86,102,255}, { 94, 84,100,255}, { 70, 62, 76,255}, {140,128,152,255} };
    static const ClutterPalette MANOR   = { { 84, 76, 96,255}, { 34, 28, 38,255}, {{ 62, 54, 74,255},{ 68, 60, 80,255},{ 54, 48, 64,255}}, 0.30f, {102, 96,110,255}, {100, 94,108,255}, { 74, 68, 82,255}, {152,144,164,255} };
    static const ClutterPalette INFERNO = { {120, 58, 40,255}, { 44, 24, 18,255}, {{ 48, 36, 32,255},{ 54, 40, 34,255},{ 42, 32, 30,255}}, 0.00f, { 88, 52, 42,255}, { 90, 56, 46,255}, { 64, 40, 34,255}, {170,110, 90,255} };
    switch (z) {
        case ZoneID::Bunker:         return BUNKER;
        case ZoneID::KronosForge:    return FORGE;
        case ZoneID::KronosNexus:    return NEXUS;
        case ZoneID::Cemetery:       return CEMIT;
        case ZoneID::CursedFarm:     return FARM;
        case ZoneID::GhostCity:      return GHOST;
        case ZoneID::DarkForest:     return FOREST;
        case ZoneID::Catacombs:      return CATAC;
        case ZoneID::AbandonedManor: return MANOR;
        case ZoneID::InfernoZone:    return INFERNO;
        case ZoneID::LARuins:
        default:                     return LA;
    }
}

// Cor do CEU/HORIZONTE por fase: nao e so "ambiente escurecido" — o ceu vermelho
// do inferno e o azul-noite da cidade fantasma sao metade da leitura do bioma.
// A mesma cor alimenta o fog do shader, entao o horizonte morre nela.
inline Color skyColorFor(ZoneID z) {
    switch (z) {
        case ZoneID::Bunker:         return {  96, 120, 116, 255 };
        case ZoneID::KronosForge:    return { 176,  96,  52, 255 };
        case ZoneID::KronosNexus:    return {  72, 140, 178, 255 };
        case ZoneID::Cemetery:       return {  86,  96, 140, 255 };
        case ZoneID::CursedFarm:     return { 150, 132,  88, 255 };
        case ZoneID::GhostCity:      return {  78,  90, 122, 255 };
        case ZoneID::DarkForest:     return {  62,  96,  72, 255 };
        case ZoneID::Catacombs:      return {  64,  50,  80, 255 };
        case ZoneID::AbandonedManor: return {  90,  64, 108, 255 };
        case ZoneID::InfernoZone:    return { 168,  54,  26, 255 };
        case ZoneID::LARuins:
        default:                     return { 126, 142, 168, 255 };
    }
}
