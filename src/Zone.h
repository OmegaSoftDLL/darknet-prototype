#pragma once
#include <string>
#include <raylib.h>

enum class ZoneID {
    LARuins       = 0,
    Bunker        = 1,
    KronosForge   = 2,
    KronosNexus   = 3,
    // ── Phases Sombrias ────────────────────────────────────────────────────────
    Cemetery      = 4,   // Cemetery Abandonado
    CursedFarm    = 5,   // Farm Maldita
    GhostCity     = 6,   // City Fantasma
    DarkForest    = 7,   // Forest Negra
    Catacombs     = 8,   // Catacumbas (subterraneo)
    AbandonedManor= 9,   // Manor Abandonada (area of boss)
    InfernoZone   = 10,  // Zone Vulcanica — aliens + zumbis + lava
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
            return {"Ruins of Los Angeles", "Os debris of the humanidade...",
                    {72,72,72,255}, {58,58,58,255}, {118,118,126,255}, {158,158,164,255},
                    {0,200,255,255}, 8, 1.0f, 3.0f};
        case ZoneID::Bunker:
            return {"Bunker NEXUS", "Base of operacoes of the stamina humana",
                    {48,82,44,255}, {34,62,32,255}, {66,104,62,255}, {92,140,86,255},
                    {0,255,100,255}, 5, 0.6f, 4.0f};
        case ZoneID::KronosForge:
            return {"Forge KRONOS", "Production in massa of Enforcers",
                    {96,56,34,255}, {76,40,22,255}, {136,80,44,255}, {196,116,64,255},
                    {255,100,0,255}, 14, 1.8f, 2.0f};
        case ZoneID::KronosNexus:
            return {"Core KRONOS", "O coracao of KRONOS. Destrua-the!",
                    {66,44,104,255}, {46,26,78,255}, {104,70,160,255}, {148,102,220,255},
                    {255,0,200,255}, 18, 2.5f, 1.5f};
        // ── Phases Sombrias ────────────────────────────────────────────────────
        case ZoneID::Cemetery:
            return {"Cemetery Abandonado", "Os mortos not descansam here...",
                    {78,78,72,255}, {58,58,54,255}, {92,94,88,255}, {128,132,124,255},
                    {100,255,180,255}, 6, 1.2f, 3.5f};
        case ZoneID::CursedFarm:
            return {"Farm Maldita", "A terra is podre. As colheitas, corrompidas.",
                    {84,66,42,255}, {64,50,28,255}, {108,86,48,255}, {148,120,62,255},
                    {80,200,100,255}, 5, 1.1f, 3.0f};
        case ZoneID::GhostCity:
            return {"City Fantasma", "Ruas vazias. Mas not desertas.",
                    {74,74,88,255}, {52,52,64,255}, {92,92,114,255}, {126,126,150,255},
                    {150,180,255,255}, 8, 1.4f, 2.5f};
        case ZoneID::DarkForest:
            return {"Forest Negra", "A fog esconde the that mora among the arvores.",
                    {54,72,50,255}, {34,46,32,255}, {66,88,62,255}, {92,118,84,255},
                    {60,255,120,255}, 10, 1.3f, 3.0f};
        case ZoneID::Catacombs:
            return {"Catacumbas", "Passagens of stone. Cheiro of death antiga.",
                    {70,58,66,255}, {48,40,46,255}, {88,70,88,255}, {128,102,126,255},
                    {180,100,255,255}, 12, 1.6f, 2.0f};
        case ZoneID::AbandonedManor:
            return {"Manor Abandonada", "O boss aguarda in the profundezas.",
                    {66,48,62,255}, {44,32,42,255}, {86,62,84,255}, {122,86,118,255},
                    {200,50,255,255}, 15, 2.0f, 1.8f};
        case ZoneID::InfernoZone:
            return {"Zone Inferno", "Lava, cinzas and criaturas of the abismo.",
                    {96,40,18,255}, {72,26,10,255}, {140,62,28,255}, {210,100,40,255},
                    {255,80,0,255}, 10, 2.2f, 1.5f};
    }
    return {};
}

// ── PALETA DE CLUTTER POR BIOMA ─────────────────────────────────────────────
// Grama SEMPRE green, tree SEMPRE frondosa and entulho SEMPRE gray eram the
// maior delator of "same world with other tinta": ~90% of the props of the screen sao
// clutter. Cada biome has your own paleta of grass/folhagem/stone/entulho.
struct ClutterPalette {
    Color grass;        // color base of the grass
    Color grassDirt;    // mancha of terra sob the tufo
    Color canopy[3];    // folhagem of the arvores (3 especies)
    float canopyBulk;   // porte of the copa; <= 0 = tree MORTA (without copa)
    Color rock;         // pedras/detritos
    Color slabA;        // entulho: laje clear
    Color slabB;        // entulho: laje dark
    Color gravel;       // cascalho clear of the marcas in the floor
};

inline const ClutterPalette& clutterPaletteFor(ZoneID z) {
    //                                       grass              terra             copa x3                                  bulk  stone              laje A            laje B             cascalho
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

// Color of the CEU/HORIZONTE by phase: not and only "environment escurecido" — the sky red
// of the inferno and the blue-night of the city ghost sao metade of the reading of the biome.
// A same color alimenta the fog of the shader, entao the horizonte morre nela.
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
