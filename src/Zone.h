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
