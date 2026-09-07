// Game_WorldGen.cpp — scenario of the world open: structures by biome, NPCs urbanos,
// streaming of chunks and block of movement. Extraido of Game.cpp. Same class Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

// Copias file-local of the constantes of scale of Game.cpp (static constexpr):
// usadas here and still usadas in Game.cpp (renderWorld3D) — by isso duplicadas.
static constexpr float FIT_HOUSE    = 175.0f;   // house of 2 andares ~11 m
static constexpr float FIT_BARRACKS = 190.0f;   // barn/galpao ~12 m
static constexpr float FIT_CASTLE   = 340.0f;   // building/castelo ~21 m
static constexpr float FIT_WELL     = 130.0f;   // silo high

// Radius of collision = CIRCUNSCRITO of the that really and drawn (same hash of the
// render in Game.cpp). The inscribed circle there was smaller than the facade: the hero
// afundava ~60u inside the building before bater in the wall invisible.
static float buildingRadius(float x, float z, float sc) {
    unsigned hsh = (unsigned)(x * 0.6f) * 374761393u ^ (unsigned)(z * 0.6f) * 668265263u;
    float W = (250.0f + (float)((hsh >> 5) & 15) * 5.0f) * sc;
    float D = (220.0f + (float)((hsh >> 9) & 15) * 4.0f) * sc;
    return 0.5f * (float)std::sqrt((double)(W * W + D * D));
}

static float vehicleRadius(float x, float z, float sc) {
    unsigned hsh = (unsigned)(x * 0.5f) * 2246822519u ^ (unsigned)(z * 0.5f) * 3266489917u;
    int kind = (int)(hsh % 3);                     // 0 car 1 van 2 caminhao
    float L  = (kind == 2 ? 164.0f : kind == 1 ? 122.0f : 100.0f) * sc;
    float WD = (kind == 2 ?  60.0f : kind == 1 ?  54.0f :  50.0f) * sc;
    float halfW = WD * 0.5f + 5.0f * sc;           // pneu/flange ~5u outside the lataria
    return 0.5f * (float)std::sqrt((double)(L * L + halfW * halfW * 4.0f));
}

// Distance side until the via of draw more next (mesh 475 + k*950, middle-fio
// in 112u). A lane that O PLAYER ENXERGA and essa mesh of quads of the Tilemap, inde-
// pending of the tiles (the streets of tile sao aleatorias). O scenario of city precisa
// respect THIS mesh, ou building/tree nascem in the middle of the street.
static float laneDist(float v) {
    float r = std::fmod(v - 475.0f, 950.0f);
    if (r < 0.0f) r += 950.0f;
    if (r > 475.0f) r = 950.0f - r;
    return r;
}

// Helper file-local (static): usado by buildOpenWorldScenery/updateSceneryChunks.
static const int* structuresFor(ZoneID z, int& outCount) {
    // COHERENCE: castelo (7), well/silo (8) and barn (1) sao MEDIEVAIS/RURAIS —
    // not entram numa city with asphalt and car. City usa building modern (20),
    // house (0) and bunker (15). The castle only remains for the MANOR, which is the only
    // lugar where ele does sentido.
    // Nada of house of roof of telha (0 = house.obj, medieval) numa street with
    // asphalt and car: it was exactly what broke city coherence.
    static const int LA[]      = { 20, 20, 20, 15 };
    static const int GHOST[]   = { 20, 20, 15, 15 };
    static const int FARM[]    = { 1, 1, 0, 8, 18 };
    static const int FOREST[]  = { 18, 18, 19, 1 };
    static const int CEMIT[]   = { 14, 14, 19, 0 };
    static const int BUNKER[]  = { 15, 15, 20 };
    static const int CATAC[]   = { 14, 14, 19 };
    static const int MANOR[]   = { 7, 14, 0, 19 };
    static const int FORGE[]   = { 15, 20, 16 };
    static const int INFERNO[] = { 16, 16, 14 };
    static const int NEXUS[]   = { 17, 17, 16, 20 };
    (void)0;
    switch (z) {
        case ZoneID::GhostCity:      outCount = 4; return GHOST;
        case ZoneID::CursedFarm:     outCount = 5; return FARM;
        case ZoneID::DarkForest:     outCount = 4; return FOREST;
        case ZoneID::Cemetery:       outCount = 4; return CEMIT;
        case ZoneID::Bunker:         outCount = 4; return BUNKER;
        case ZoneID::Catacombs:      outCount = 3; return CATAC;
        case ZoneID::AbandonedManor: outCount = 4; return MANOR;
        case ZoneID::KronosForge:    outCount = 4; return FORGE;
        case ZoneID::InfernoZone:    outCount = 4; return INFERNO;
        case ZoneID::KronosNexus:    outCount = 4; return NEXUS;
        case ZoneID::LARuins:
        default:                     outCount = 5; return LA;
    }
}

// Popula TODAS the regions of the world open with scenario denso, espalhado by all
// the area of cada region. Chamado uma vez to the start the world open.
void Game::buildOpenWorldScenery() {
    owDecor.scenery.clear();
    m_sceneryBuildQueue.clear();
    m_sceneryPostProcessNeeded = false;
    // Reseta the streaming of chunks (otherwise buildings solidos invisiveis / scenario
    // ghost sobrevivem to the Continue/Novo Game inside the same process).
    m_sceneryChunks.clear();
    m_chunkSolids.clear();
    m_lastChunkX = m_lastChunkY = -999999;

    // Gerador pseudo-random deterministic (not usa Math.random).
    // The seed DERIVES FROM THE PHASE: with fixed seed, grass/entulho/pedras nasciam in the
    // MESMAS positions in all phase ("same world with other tinta"). A realocacao
    // automatic of the portal (anel of candidatos in Game_Phases.cpp) and the network of
    // security caso uma struct new caia in up of the point of the portal.
    unsigned int rng = 0x1234abcd + (unsigned int)owPhase * 0x9e3779b9u;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (float)((rng >> 8) & 0xFFFF) / 65535.0f; // 0..1
    };

    // count and calibrado for the region ANTIGA of 2560x2560; scale by AREA to
    // densidade continue the same when the size of the region muda.
    // The PHASE world ends at the barrier: scenario outside dela only gastava memory and
    // still aparecia behind of the wall of energy, denunciando that the map continuous.
    const float phaseLimit = owPhaseRadius + 140.0f;
    auto insidePhase = [&](Vector2 p) {
        float dx = p.x - safeZoneCenter.x, dy = p.y - safeZoneCenter.y;
        return dx*dx + dy*dy <= phaseLimit * phaseLimit;
    };
    // Zone livre of the HUB (NPCs ficam in the center of the world) — nenhuma struct nasce
    // la. Refuge = safeZoneCenter (the center of the phase/galaxia), not more the center
    // of the zone 0 of the tilemap 3x3 (coincidia in 1280,1280, mas the world now is
    // centrado in the base and the phases cresceram to alem of the grid old).
    const float hubX = safeZoneCenter.x;
    const float hubY = safeZoneCenter.y;
    auto place = [&](Rectangle b, int type, int count,
                     float minScale, float maxScale, float margin, bool city = false) {
        float areaK = (b.width * b.height) / (2560.0f * 2560.0f);
        count = (int)(count * areaK);
        for (int i = 0; i < count; ++i) {
            SceneryObject the;
            the.type     = type;
            the.position = {
                b.x + margin + rnd() * (b.width  - 2 * margin),
                b.y + margin + rnd() * (b.height - 2 * margin)
            };
            if (!insidePhase(the.position)) continue;
            if (type == 6) {   // core of the refuge livre of carcacas
                float dx = the.position.x - hubX, dy = the.position.y - hubY;
                if (dx*dx + dy*dy < 150.0f * 150.0f) continue;
            }
            if (type == 2) {   // tree: praca central livre (area playable of the base)
                float dx = the.position.x - hubX, dy = the.position.y - hubY;
                if (dx*dx + dy*dy < 400.0f * 400.0f) continue;
            }
            if (city) {   // city: nada in up of the lane drawn (quad ±112u)
                float dr = std::fminf(laneDist(the.position.x), laneDist(the.position.y));
                if (type == 2 && dr < 170.0f) continue;                 // tree: only in the quarteirao, far from the middle-fio (copa not invade the lane)
                if (type == 6 && dr > 102.0f) continue;                 // car: NA lane
                if (type == 5 && dr < 118.0f) continue;                 // pole: outside the lane, in the sidewalk
            }
            the.rotation = rnd() * 3.14159f;
            the.scale    = minScale + rnd() * (maxScale - minScale);
            // tint.r > 128 acende luzes (windows/postes)
            the.tint     = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
            m_sceneryBuildQueue.push_back(the);
        }
    };
    std::vector<Vector2> placedB;   // estruturas already colocadas (anti-sobreposicao)
    auto isStruct = [](int t) { return t == 0 || t == 1 || t == 7 || t == 8 || t == 10 ||
                                       (t >= 14 && t <= 20); };
    // Puts 1 objeto. Estruturas grandes: far from the hub + without encostar in other.
    // Distance minima between estruturas PROPORCIONAL to the size. Um value fixed of
    // 400u rejeitava almost all lote of um building of 60u of width: the city saia
    // with uma structure the cada quarteirao and middle, i.and., vazia.
    auto spacingFor = [](int type) {
        switch (type) {
            case 7:  return 420.0f;   // castelo/manor
            case 1:  return 260.0f;   // barn
            case 15: return 210.0f;   // bunker
            case 20: return 320.0f;   // building modern
            case 0:  return 180.0f;   // house
            case 8:  return 170.0f;   // silo
            case 14: return 150.0f;   // crypt
            case 18: return 150.0f;   // cabin
            case 16: return 130.0f;   // espira
            case 17: return 120.0f;   // monolith
            case 19: return 110.0f;   // tower
            default: return 200.0f;
        }
    };
    auto put1 = [&](int type, Vector2 pos, float sc) {
        if (!insidePhase(pos)) return;
        if (isStruct(type)) {
            float hdx = pos.x - hubX, hdy = pos.y - hubY;
            if (hdx*hdx + hdy*hdy < 560.0f * 560.0f) return;   // praca central (NPCs) livre: 560u of folga to structure not espremer the hub
            float md = spacingFor(type);
            for (const auto& q : placedB) { float dx = pos.x-q.x, dy = pos.y-q.y; if (dx*dx + dy*dy < md*md) return; }
            placedB.push_back(pos);
        }
        SceneryObject the; the.type = type; the.position = pos;
        // Building of city if ALINHA the street (rotation in multiples of 90 degrees). Building
        // torto in the middle of the quarteirao denuncia geracao aleatoria in the hour. Carros
        // also: parados in the mesh of streets (04 angles), otherwise ficam atravessados
        // and the "layout" parece bagunca.
        bool aligned = (type == 20 || type == 15 || type == 0 || type == 6);
        the.rotation = aligned ? (float)((int)(rnd() * 4.0f) % 4) * 1.5708f
                             : rnd() * 3.14159f;
        the.scale = sc; the.tint = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
        m_sceneryBuildQueue.push_back(the);
    };
    auto inBnd = [&](Rectangle b, Vector2 p, float m) {
        return p.x >= b.x + m && p.x <= b.x + b.width - m && p.y >= b.y + m && p.y <= b.y + b.height - m;
    };

    for (const auto& r : worldRegions) {
        Rectangle b = r.bounds;
        auto seedIn = [&]() { return Vector2{ b.x + 90 + rnd() * (b.width - 180), b.y + 90 + rnd() * (b.height - 180) }; };
        switch (r.zoneType) {
            case ZoneID::LARuins:    // Ruins of LA — CITY: grade of buildings with praca central
            case ZoneID::GhostCity: {// City ghost
                // Questao of the quarteirao = mesh of VIAS drawn (475 + k*950).
                // Center of the quarteirao stays the middle path between duas vias, in
                // multiples of 950; building the EDGE=210 of the center ≈ colado in the middle-fio.
                // Antes the grade era 220 + k*950 with EDGE=350: the fachada furada
                // ~137u DENTRO of the lane — the "building in the middle of the street" that the user
                // apontou. GhostCity usa the MESMO quarteirao (the vias desenhadas
                // are iguais to the duas cities) and difere in ruina/empty, not
                // in espacamento (before gp=880 desencaixava of the vias of 950).
                bool ghost = (r.zoneType == ZoneID::GhostCity);
                float skip = ghost ? 0.44f : 0.30f;              // lotes vazios
                float gx0 = (float)std::ceil((b.x + 475.0f) / 950.0f) * 950.0f;
                float gy0 = (float)std::ceil((b.y + 475.0f) / 950.0f) * 950.0f;
                for (float gx = gx0; gx < b.x + b.width - 150.0f; gx += 950.0f)
                    for (float gy = gy0; gy < b.y + b.height - 150.0f; gy += 950.0f) {
                        if (rnd() < skip) {                        // lote empty = CENA DE GUERRA, not terreno dead
                            for (int di = 0; di < 3; ++di)
                                put1(21, { gx + (rnd() - 0.5f) * 250.0f, gy + (rnd() - 0.5f) * 250.0f },
                                     0.9f + rnd() * 1.1f);
                            if (rnd() < 0.60f)                     // barril in chamas
                                put1(22, { gx + (rnd() - 0.5f) * 210.0f, gy + (rnd() - 0.5f) * 210.0f },
                                     1.0f + rnd() * 0.4f);
                            if (rnd() < 0.50f)                     // carcaca queimada of veiculo
                                put1(6, { gx + (rnd() - 0.5f) * 240.0f, gy + (rnd() - 0.5f) * 240.0f }, 1.0f);
                            if (rnd() < 0.60f)                     // marcas of fire/tiroteio in the piso
                                put1(13, { gx + (rnd() - 0.5f) * 250.0f, gy + (rnd() - 0.5f) * 250.0f },
                                     1.0f + rnd() * 1.4f);
                            continue;
                        }
                        // PERIMETRO of the quarteirao ocupado: 3 structures by lado,
                        // coladas in the sidewalk, deixando the MIOLO as patio of entulho.
                        // Assim the street vira corredor between massas construidas.
                        int nk2 = 0; const int* kk2 = structuresFor(r.zoneType, nk2);
                        const float EDGE = 210.0f;      // of the center of the quarteirao until the sidewalk
                        for (int side = 0; side < 4; ++side) {
                            for (int slot = 0; slot < 2; ++slot) {
                                // GhostCity: more gaps (street esburacada, empty city)
                                if (rnd() < (ghost ? 0.34f : 0.18f)) continue;  // brecha = beco
                                float along = (slot - 0.5f) * 300.0f + (rnd() - 0.5f) * 50.0f;
                                Vector2 lot;
                                if (side == 0)      lot = { gx + along, gy - EDGE };
                                else if (side == 1) lot = { gx + along, gy + EDGE };
                                else if (side == 2) lot = { gx - EDGE,  gy + along };
                                else                lot = { gx + EDGE,  gy + along };
                                // GhostCity: buildings menores/more degradados
                                float sc2 = ghost ? 0.58f + rnd() * 0.22f : 0.74f + rnd() * 0.26f;
                                put1(kk2[(int)(rnd() * nk2) % nk2], lot, sc2);
                            }
                        }
                        // miolo: entulho and carcacas (patio dark, not descampado)
                        for (int ci = 0; ci < 3; ++ci)
                            put1(21, { gx + (rnd() - 0.5f) * 430.0f,
                                       gy + (rnd() - 0.5f) * 430.0f }, 1.0f + rnd() * 1.0f);
                        // fogueira of sobreviventes of vez in when (light + color)
                        if (rnd() < 0.30f)
                            put1(22, { gx + (rnd() - 0.5f) * 360.0f,
                                       gy + (rnd() - 0.5f) * 360.0f }, 1.0f + rnd() * 0.5f);
                    }
                place(b, 5, 20, 1.0f, 1.0f, 150, true);  // postes in the streets
                place(b, 6, 16, 1.0f, 1.0f, 150, true);  // cars abandonados (only city)
                place(b, 2,  6, 0.8f, 1.2f, 150, true);  // arvores
                // ── CODIGO DE GUERRA: city bombardeada ───────────────────────────
                place(b, 28, 10, 0.9f, 1.7f, 100, true); // crateras (in the floor, junto the vias)
                place(b, 30, 18, 0.9f, 1.9f,  60, true); // asphalt destruido (manchas)
                place(b, 29,  9, 0.8f, 1.5f, 200, true); // building colapsado (ruina with smoke)
                place(b, 31,  7, 1.0f, 1.4f, 160, true); // carcaca in chamas (fire + smoke pesada)
            } break;
            case ZoneID::Bunker: {   // Bunker — military compounds (structures+silos in line)
                for (int bl = 0; bl < 3; ++bl) {
                    Vector2 seed = seedIn(); float ax = (rnd()<0.5f)?1.0f:0.0f, ay = 1.0f-ax; int n = 3 + (int)(rnd()*2.0f);
                    for (int k = 0; k < n; ++k) { Vector2 bp = { seed.x+ax*(k-n*0.5f)*135.0f, seed.y+ay*(k-n*0.5f)*135.0f }; if (inBnd(b,bp,30.0f)) put1((rnd()<0.5f)?7:8, bp, 1.0f+rnd()*0.6f); }
                    for (int k = 0; k < 8; ++k) put1(4, { seed.x+(rnd()-0.5f)*340.0f, seed.y+(rnd()-0.5f)*340.0f }, 1.0f);
                }
                place(b, 5, 8, 1.0f, 1.0f, 150);   // postes
            } break;
            case ZoneID::DarkForest: // Forest densa — MUITA tree: the forest has that abafar
                place(b, 2, 120, 0.9f, 1.8f, 80); place(b, 4, 10, 1.0f, 1.3f, 120); place(b, 3, 8, 0.7f, 1.0f, 120);
                break;
            case ZoneID::CursedFarm: {// Farm — VILLAGES (houses+barn+silo+fence in anel)
                for (int v = 0; v < 3; ++v) {
                    Vector2 seed = seedIn(); put1(0, seed, 1.2f + rnd()*0.4f);
                    int houses = 1 + (int)(rnd()*3.0f);
                    for (int k = 0; k < houses; ++k) { float the = rnd()*6.2832f, d = 110.0f+rnd()*90.0f; Vector2 bp = { seed.x+cosf(the)*d, seed.y+sinf(the)*d }; if (inBnd(b,bp,30.0f)) put1(0, bp, 1.0f+rnd()*0.5f); }
                    Vector2 barn = { seed.x+(rnd()-0.5f)*170.0f, seed.y+(rnd()-0.5f)*170.0f }; if (inBnd(b,barn,30.0f)) put1(1, barn, 1.1f+rnd()*0.4f);
                    Vector2 silo = { seed.x+(rnd()-0.5f)*210.0f, seed.y+(rnd()-0.5f)*210.0f }; if (inBnd(b,silo,30.0f)) put1(8, silo, 1.0f+rnd()*0.4f);
                    for (int k = 0; k < 12; ++k) { float the = k/12.0f*6.2832f; put1(4, { seed.x+cosf(the)*245.0f, seed.y+sinf(the)*245.0f }, 1.0f+rnd()*0.3f); }
                }
                place(b, 2, 16, 0.8f, 1.3f, 120);  // arvores
            } break;
            case ZoneID::Cemetery: { // Cemetery — lapides in FILEIRAS + portao
                for (int g = 0; g < 6; ++g) {
                    Vector2 seed = seedIn(); int rows = 3 + (int)(rnd()*3.0f), cols = 4 + (int)(rnd()*3.0f); float sx = 48.0f, sy = 66.0f;
                    for (int rr = 0; rr < rows; ++rr) for (int c = 0; c < cols; ++c) { Vector2 gp = { seed.x+(c-cols*0.5f)*sx, seed.y+(rr-rows*0.5f)*sy }; if (inBnd(b,gp,30.0f)) put1(3, gp, 0.8f+rnd()*0.4f); }
                    Vector2 gate = { seed.x, seed.y - rows*0.5f*sy - 40.0f }; if (inBnd(b,gate,30.0f)) put1(9, gate, 1.0f);
                }
                place(b, 2, 14, 0.9f, 1.5f, 120); place(b, 10, 4, 1.0f, 1.4f, 250);  // arvores/estatuas
            } break;
            case ZoneID::KronosForge: // Lava forge — silos/estatuas/pedras (without houses)
                place(b, 8, 8, 1.0f, 1.8f, 220); place(b, 10, 5, 1.0f, 1.5f, 250); place(b, 12, 30, 0.7f, 1.3f, 60); place(b, 2, 6, 0.7f, 1.0f, 150);
                break;
            case ZoneID::AbandonedManor: {// Manor — casarao central + estatuas in the cantos
                for (int m = 0; m < 2; ++m) {
                    Vector2 seed = seedIn(); put1(0, seed, 1.8f + rnd()*0.6f);
                    for (int k = 0; k < 4; ++k) { float the = k/4.0f*6.2832f + 0.7f; Vector2 bp = { seed.x+cosf(the)*175.0f, seed.y+sinf(the)*175.0f }; if (inBnd(b,bp,30.0f)) put1(10, bp, 1.1f+rnd()*0.5f); }
                }
                place(b, 2, 16, 0.9f, 1.6f, 120); place(b, 3, 12, 0.7f, 1.1f, 120); place(b, 4, 20, 1.0f, 1.3f, 100);
            } break;
            case ZoneID::KronosNexus: {// Core — struct central + anel of estatuas
                for (int c2 = 0; c2 < 2; ++c2) { Vector2 seed = seedIn(); put1(7, seed, 1.4f+rnd()*0.6f); for (int k = 0; k < 6; ++k) { float the = k/6.0f*6.2832f; Vector2 bp = { seed.x+cosf(the)*165.0f, seed.y+sinf(the)*165.0f }; if (inBnd(b,bp,30.0f)) put1(10, bp, 1.2f+rnd()*0.5f); } }
                place(b, 10, 6, 1.2f, 2.0f, 200);  // estatuas imponentes
            } break;
            case ZoneID::InfernoZone: // Inferno — espiras, rocha vulcanica, only tree MORTA
                place(b, 16, 6, 0.9f, 1.5f, 250); place(b, 12, 40, 0.8f, 1.5f, 60); place(b, 2, 6, 0.8f, 1.1f, 150);
                break;
            case ZoneID::Catacombs:   // Catacumbas — criptas/arcos about floor of stone nua
                place(b, 14, 5, 1.0f, 1.3f, 250); place(b, 9, 3, 1.0f, 1.2f, 250); place(b, 12, 30, 0.7f, 1.3f, 60); place(b, 3, 16, 0.7f, 1.0f, 100);
                break;
            default:
                place(b, 2, 10, 0.8f, 1.4f, 150);
                break;
        }
        // Vegetacao rasteira densa in TODA zone — health in the floor (grass + detritos).
        // QUANTIDADE by biome: the inferno almost not has grass and transborda rocha,
        // the forest abafa in grass, the city ghost afoga in entulho. Antes
        // TODA phase had 300 gramas/120 manchas/90 entulhos — the floor lia igual.
        float grassK = 1.0f, rockK = 1.0f, stainK = 1.0f, rubbleK = 1.0f;
        switch (r.zoneType) {
            case ZoneID::InfernoZone:    grassK = 0.05f; rockK = 2.4f; stainK = 1.3f; rubbleK = 1.6f; break;
            case ZoneID::KronosForge:    grassK = 0.10f; rockK = 2.0f; stainK = 1.2f; rubbleK = 1.5f; break;
            case ZoneID::Catacombs:      grassK = 0.06f; rockK = 1.8f; stainK = 1.1f; rubbleK = 1.4f; break;
            case ZoneID::Cemetery:       grassK = 0.35f; rockK = 1.1f; stainK = 1.0f; rubbleK = 0.9f; break;
            case ZoneID::GhostCity:      grassK = 0.22f; rockK = 1.2f; stainK = 1.2f; rubbleK = 1.8f; break;
            case ZoneID::AbandonedManor: grassK = 0.50f; rockK = 1.0f; stainK = 1.0f; rubbleK = 0.9f; break;
            case ZoneID::KronosNexus:    grassK = 0.15f; rockK = 1.4f; stainK = 1.0f; rubbleK = 1.1f; break;
            case ZoneID::DarkForest:     grassK = 1.7f;  rockK = 0.8f; stainK = 0.8f; rubbleK = 0.4f; break;
            case ZoneID::CursedFarm:     grassK = 1.5f;  rockK = 0.8f; stainK = 0.9f; rubbleK = 0.5f; break;
            case ZoneID::Bunker:         grassK = 0.80f; rockK = 1.0f; stainK = 1.1f; rubbleK = 1.0f; break;
            default: break;
        }
        place(b, 11, (int)(300 * grassK),  0.7f, 1.25f, 20);  // grass
        place(b, 12, (int)( 36 * rockK),   0.6f, 1.2f, 40);   // pedras/detritos
        place(b, 13, (int)(120 * stainK),  0.8f, 2.4f, 20);   // manchas/rachaduras in the floor
        place(b, 21, (int)( 90 * rubbleK), 0.7f, 1.6f, 30);   // ENTULHO: the world fell, has that ter escombro
    }

    // ── Praca central of the REFUGE = PERIMETRO DE GUERRA ──
    // Phase 1 (LA): the vao of 360u in the middle of the world where the NPCs ficam not can
    // read as descampado. Monta um anel of ROADBLOCKS (cars virados to outside,
    // front apontando for the street), burning barrels in the inner ring, debris and
    // marcas of tiroteio. O core (r<150) continuous livre to dar spawn erguer NPC.
    if (currentZone == ZoneID::LARuins) {
        const float R0 = 215.0f, R1 = 320.0f;
        const float openArc = 1.0f;          // vao open to the leste (output livre)
        const int   nCars = 7;
        const float stepA = 6.2832f / (float)nCars;
        float a0 = 0.85f;
        for (int i = 0; i < nCars; ++i) {
            float the = a0 + i * stepA;
            if (the > 2.0f * 3.14159f - openArc) the += stepA;   // pula the vao
            Vector2 p = { hubX + cosf(the) * (R0 + rnd() * (R1 - R0)),
                          hubY + sinf(the) * (R0 + rnd() * (R1 - R0)) };
            if (!insidePhase(p)) continue;
            SceneryObject the;
            the.type = 6; the.position = p;
            the.rotation = the + 3.14159f * 0.5f + (rnd() - 0.5f) * 0.6f;  // front p/ outside
            the.scale = 1.0f + rnd() * 0.25f;
            the.tint = { 200, 200, 200, 255 };
            m_sceneryBuildQueue.push_back(the);
        }
        for (int i = 0; i < 5; ++i) {        // burning barrels in the inner ring
            float the = rnd() * 6.2832f, r = 165.0f + rnd() * 45.0f;
            Vector2 p = { hubX + cosf(the) * r, hubY + sinf(the) * r };
            if (!insidePhase(p)) continue;
            SceneryObject the;
            the.type = 22; the.position = p; the.rotation = rnd() * 6.2832f; the.scale = 1.0f;
            the.tint = { 200, 200, 200, 255 };
            m_sceneryBuildQueue.push_back(the);
        }
        for (int i = 0; i < 20; ++i) {       // debris and detritos
            float the = rnd() * 6.2832f, r = 40.0f + rnd() * 315.0f;
            Vector2 p = { hubX + cosf(the) * r, hubY + sinf(the) * r };
            float dx = p.x - hubX, dy = p.y - hubY;
            if (dx*dx + dy*dy < 52.0f * 52.0f) continue;
            if (!insidePhase(p)) continue;
            SceneryObject the;
            the.type = (rnd() < 0.55f) ? 21 : 12;
            the.position = p; the.rotation = rnd() * 6.2832f; the.scale = 0.8f + rnd() * 1.5f;
            the.tint = { 80, 80, 80, 255 };
            m_sceneryBuildQueue.push_back(the);
        }
        for (int i = 0; i < 16; ++i) {       // marcas of fire/estampidos in the piso
            float the = rnd() * 6.2832f, r = 110.0f + rnd() * 260.0f;
            Vector2 p = { hubX + cosf(the) * r, hubY + sinf(the) * r };
            if (!insidePhase(p)) continue;
            SceneryObject the;
            the.type = 13; the.position = p; the.rotation = rnd() * 6.2832f; the.scale = 0.9f + rnd() * 1.4f;
            the.tint = { 200, 200, 200, 255 };
            m_sceneryBuildQueue.push_back(the);
        }
    }

    // ── Pos-processamento heavy (collision, limpeza of the base, lojas, resources,
    //    animais, civis) NOT roda here: is executado in lotes by streamSceneryBuild()
    //    so that the queue of objetos for totalmente transferida to owDecor.scenery.
    m_sceneryPostProcessNeeded = true;
}

// SAFE ZONE stalls/shops: each service NPC has its OWN STALL,
// proporcional to the oficio, ancorado in the anel in returns of the praca central. Os
// offsets espelham setupBaseNPCs() (Game_QuestsNPC.cpp) and cada tenda is
// deslocada ~52u PRA FORA of the radius: the vendor stays in front of the own shop
// when the player arrives through the center — market in circle, not building espremido.
// Types (only visuals, not bloqueiam): 25 comando · 27 arsenal · 24 forge ·
// 23 estande of market · 26 lab.
void Game::placeBaseShops() {
    float bx = safeZoneCenter.x, by = safeZoneCenter.y;
    auto shop = [&](float ox, float oy, int type, float sc, float rot) {
        float len = std::sqrt(ox * ox + oy * oy);
        if (len < 1.0f) { ox = 1.0f; oy = 0.0f; len = 1.0f; }
        SceneryObject the;
        the.type     = type;
        the.position = { bx + ox + (ox / len) * 52.0f, by + oy + (oy / len) * 52.0f };
        the.rotation = rot;
        the.scale    = sc;
        the.tint     = { 200, 200, 200, 255 };   // light acesa in the estande
        owDecor.scenery.push_back(the);
    };
    // (offset in X/Y, offset of the NPC in setupBaseNPCs and the MESMO)
    shop(-120.0f,  -90.0f, 25, 1.05f, -0.50f);   // VANCE RIOS   -> posto of comando
    shop( 150.0f,  -70.0f, 27, 1.00f, -0.40f);   // ZARA         -> arsenal of weapons
    shop( 180.0f,  100.0f, 24, 1.10f,  0.60f);   // BLACKSMITH KANE-> forge + bigorna
    shop(-180.0f,  110.0f, 23, 0.95f,  2.60f);   // LUNA         -> estande of market
    shop( -30.0f,  150.0f, 26, 0.95f,  1.80f);   // DR. CHEN     -> lab of implantes
}

// Populate the CITY with civilians that HAVE TASKS (not wander aimlessly): guards patrol
// rotas, trabalhadores ficam numa obra (martelando), grupos conversam in rodas,
// vendedores ficam in the posto. Reusam the modelos voxel of the NPCs (by NPCRole).
void Game::spawnCityFolk() {
    cityFolk.clear();
    if (!openWorldMode) return;

    // Rodas of conversa (points fixos where grupos if juntam)
    Vector2 meet[4];
    for (int i = 0; i < 4; ++i) {
        float the = GetRandomValue(0, 359) * DEG2RAD;
        float d = (float)GetRandomValue(220, (int)(safeZoneRadius * 0.7f));
        meet[i] = { safeZoneCenter.x + std::cos(the) * d, safeZoneCenter.y + std::sin(the) * d };
    }
    // List of buildings of the city (postos of trabalho)
    std::vector<Vector2> builds;
    for (const auto& the : owDecor.scenery)
        if ((the.type==0||the.type==1||the.type==7||the.type==8) &&
            Vector2Distance(the.position, safeZoneCenter) < safeZoneRadius + 200.0f)
            builds.push_back(the.position);

    int n = 26;
    for (int i = 0; i < n; ++i) {
        CityFolk f;
        float the = GetRandomValue(0, 359) * DEG2RAD;
        float d = (float)GetRandomValue(170, (int)(safeZoneRadius * 0.9f));
        f.home = { safeZoneCenter.x + std::cos(the) * d, safeZoneCenter.y + std::sin(the) * d };
        int tries = 0;
        while (isBlocked(f.home) && tries < 10) {
            the = GetRandomValue(0, 359) * DEG2RAD; d = (float)GetRandomValue(170, (int)(safeZoneRadius * 0.9f));
            f.home = { safeZoneCenter.x + std::cos(the) * d, safeZoneCenter.y + std::sin(the) * d }; tries++;
        }
        f.position = f.home; f.role = GetRandomValue(0, 6); f.speed = 40.0f + (float)GetRandomValue(0, 38);

        int jr = GetRandomValue(0, 99);
        if      (jr < 28) f.job = FolkJob::Guard;
        else if (jr < 60) f.job = FolkJob::Worker;
        else if (jr < 86) f.job = FolkJob::Chatter;
        else              f.job = FolkJob::Vendor;

        if (f.job == FolkJob::Guard) {                         // ronda between 2 points
            float ga = GetRandomValue(0, 359) * DEG2RAD, gd = 200.0f + GetRandomValue(0, 180);
            f.anchor = { f.home.x + std::cos(ga) * gd, f.home.y + std::sin(ga) * gd };
            f.target = f.anchor;
        } else if (f.job == FolkJob::Worker) {                 // posto = near of um building
            if (!builds.empty()) {
                Vector2 b = builds[GetRandomValue(0, (int)builds.size()-1)];
                Vector2 dir = { f.home.x - b.x, f.home.y - b.y };
                float l = std::sqrt(dir.x*dir.x + dir.y*dir.y); if (l < 1.0f) { dir = {1,0}; l = 1; }
                f.anchor = { b.x + dir.x/l * 95.0f, b.y + dir.y/l * 95.0f };  // in FRENTE to the building
            } else f.anchor = f.home;
            f.target = f.anchor;
        } else if (f.job == FolkJob::Chatter) {                // junta-if the uma roda
            f.anchor = meet[GetRandomValue(0, 3)];
            f.anchor.x += (float)GetRandomValue(-28, 28); f.anchor.y += (float)GetRandomValue(-28, 28);
            f.target = f.anchor;
        } else { f.anchor = f.home; f.target = f.home; }       // vendor stays in the posto
        cityFolk.push_back(f);
    }
}

void Game::updateCityFolk(float dt) {
    if (!openWorldMode) return;
    for (auto& f : cityFolk) {
        Vector2 d = { f.target.x - f.position.x, f.target.y - f.position.y };
        float dist = std::sqrt(d.x*d.x + d.y*d.y);

        if (f.job == FolkJob::Guard) {                         // PATROL: goes-and-returns without stop
            if (dist < 20.0f) {
                bool atB = Vector2Distance(f.target, f.anchor) < 5.0f;
                f.target = atB ? f.home : f.anchor;            // troca the ponta of the ronda
            } else {
                Vector2 s = { d.x/dist * f.speed * dt, d.y/dist * f.speed * dt };
                Vector2 np = { f.position.x + s.x, f.position.y + s.y };
                if (!isBlocked(np)) { f.position = np; f.facing = (s.x>=0)?1:-1;
                                      f.walkPhase += dt * 7.0f; }   // passo anda with the civilian
                else { Vector2 tmp = f.target; f.target = f.home; f.home = tmp; }  // contorna: inverte route
            }
            continue;
        }

        if (dist > 16.0f) {                                    // going for the posto
            Vector2 s = { d.x/dist * f.speed * dt, d.y/dist * f.speed * dt };
            Vector2 np = { f.position.x + s.x, f.position.y + s.y };
            if (!isBlocked(np)) { f.position = np; f.facing = (s.x>=0)?1:-1; f.atStation = false;
                                  f.walkPhase += dt * 7.0f; }
            else f.target = f.home;
        } else {                                               // CHEGOU → executes the TASK
            f.atStation = true; f.work += dt;
            if (f.job == FolkJob::Worker) {                    // martela; of vez in when muda of foot
                f.timer -= dt;
                if (f.timer <= 0.0f) {
                    float the = GetRandomValue(0, 359) * DEG2RAD;
                    f.target = { f.anchor.x + std::cos(the)*26.0f, f.anchor.y + std::sin(the)*26.0f };
                    f.timer = 2.5f + (float)GetRandomValue(0, 300)/100.0f;
                }
            } else if (f.job == FolkJob::Chatter) {            // stays in the roda; rarely troca of group
                f.timer -= dt;
                if (f.timer <= 0.0f) { f.timer = 5.0f + (float)GetRandomValue(0, 600)/100.0f; }
            }
            // Vendor: stays stopped in the posto.
        }
    }
}

// World INFINITO: generates scenario in CHUNKS around of the player conforme explora and
// descarrega chunks distantes. Deterministic by chunk (auto-structure).
void Game::updateSceneryChunks(Vector2 playerPos) {
    if (!openWorldMode) return;
    const float CH   = 1280.0f;                 // size of the chunk (~20 tiles)
    const int   RAD  = 2;                        // radius in chunks (5x5 carregados)
    int pcx = (int)floorf(playerPos.x / CH);
    int pcy = (int)floorf(playerPos.y / CH);

    auto keyOf = [](int cx, int cy) -> long long {
        return ((long long)(cx + 100000) << 21) | (long long)(cy + 100000);
    };
    std::set<long long> want;
    for (int cy = pcy - RAD; cy <= pcy + RAD; ++cy)
        for (int cx = pcx - RAD; cx <= pcx + RAD; ++cx) want.insert(keyOf(cx, cy));

    // To the CRUZAR of chunk: descarrega the that left of the radius (poda). Mantem the fixed (-1).
    if (pcx != m_lastChunkX || pcy != m_lastChunkY) {
        m_lastChunkX = pcx; m_lastChunkY = pcy;
        owDecor.scenery.erase(std::remove_if(owDecor.scenery.begin(), owDecor.scenery.end(),
            [&](const SceneryObject& the){ return the.chunk != -1 && want.find(the.chunk) == want.end(); }),
            owDecor.scenery.end());
        for (auto it = m_sceneryChunks.begin(); it != m_sceneryChunks.end();)
            it = (want.find(*it) == want.end()) ? m_sceneryChunks.erase(it) : std::next(it);
    }

    // AMORTIZADO: generates in the maximum 1 chunk by frame (evita engasgo to the cruzar fronteira).
    long long toGen = -1; int gcx = 0, gcy = 0;
    for (int cy = pcy - RAD; cy <= pcy + RAD && toGen < 0; ++cy)
        for (int cx = pcx - RAD; cx <= pcx + RAD; ++cx) {
            long long k = keyOf(cx, cy);
            if (!m_sceneryChunks.count(k)) { toGen = k; gcx = cx; gcy = cy; break; }
        }
    if (toGen < 0) return;                       // all the chunks of the radius already existem
    m_sceneryChunks.insert(toGen);

    float ox = gcx * CH, oy = gcy * CH;
    // WORLD CENTRADO NA BASE: eager regions (worldRegions) cover the whole phase,
    // incluindo coordenadas negativas now (grid dynamic centrado in the refuge).
    // Chunk procedural SO ativa where not existe region — i.and., alem of the phase.
    // O guard old (ORIG fixed 0..24576) permitia generate in the quadrante negative, and
    // with the phase cobrindo the margem oeste/norte the scenario saia DUPLICADO in up of the
    // region eager (the bug that the grade 3x3 ancorada in the origem never had).
    bool inEager = false;
    for (const auto& r : worldRegions) {
        if (ox >= r.bounds.x && oy >= r.bounds.y &&
            ox < r.bounds.x + r.bounds.width && oy < r.bounds.y + r.bounds.height) {
            inEager = true;
            break;
        }
    }
    if (!inEager) {   // generates chunk procedural outside the area eager
        unsigned int rng = (unsigned int)(gcx * 73856093) ^ (unsigned int)(gcy * 19349663) ^ 0x5151u;
        auto rnd = [&]() { rng = rng * 1664525u + 1013904223u; return (float)((rng >> 8) & 0xFFFF) / 65535.0f; };
        auto add = [&](int type, int count, float mn, float mx) {
            for (int i = 0; i < count; ++i) {
                SceneryObject the;
                the.type = type;
                the.position = { ox + rnd() * CH, oy + rnd() * CH };
                the.rotation = rnd() * 3.14159f;
                the.scale = mn + rnd() * (mx - mn);
                the.tint = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
                the.chunk = toGen;
                owDecor.scenery.push_back(the);
            }
        };
        // ── Scenario with AGRUPAMENTO coerente + contexto by biome of the POSICAO ──
        // Props pequenos espalhados; estruturas grandes in CLUSTERS (quarteiroes,
        // villages, compounds) only in the right biome. Castelo/house never solto outside of tema.
        auto put = [&](int type, Vector2 pos, float sc) {
            {   // outside the barrier of the phase not existe world
                float dx = pos.x - safeZoneCenter.x, dy = pos.y - safeZoneCenter.y;
                float lim = owPhaseRadius + 140.0f;
                if (dx*dx + dy*dy > lim*lim) return;
            }
            {   // tree never in the lane, car always in the lane. A lane E drawn when
                // currentZone and uma zone of city (Tilemap draws the mesh 475+k*950
                // only nesses biomas); the guard segue currentZone, not biomeAtWorld — the
                // biome of the point can divergir in the bordas of phase and tree nascia in the
                // middle of the asphalt.
                if (currentZone == ZoneID::LARuins || currentZone == ZoneID::GhostCity) {
                    float dr = std::fminf(laneDist(pos.x), laneDist(pos.y));
                    if (type == 2 && dr < 170.0f) return;
                    if (type == 6 && dr > 102.0f) return;
                    if (type == 5 && dr < 118.0f) return;   // pole: in the sidewalk, never in the lane
                }
            }
            SceneryObject the;
            the.type = type; the.position = pos; the.rotation = rnd() * 3.14159f;
            the.scale = sc; the.tint = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
            if (type == 6) the.rotation = (float)((int)(rnd() * 4.0f) % 4) * 1.5708f;   // car alinhado the street
            the.chunk = toGen; owDecor.scenery.push_back(the);
        };
        // Struct only enters if: the biome DA POSICAO bate (without vazar to the vizinho),
        // is far from the hub (NPCs) and NOT encosta in other struct (anti-amontoado).
        std::vector<Vector2> placedB;
        const float hubX = safeZoneCenter.x;   // refuge = center of the phase/base (see buildOpenWorldScenery)
        const float hubY = safeZoneCenter.y;
        auto putB = [&](int type, Vector2 pos, float sc, ZoneID want, float minSp = 400.0f) {
            if (tilemap.biomeAtWorld(pos.x, pos.y) != want) return;
            float hdx = pos.x - hubX, hdy = pos.y - hubY;
            if (hdx*hdx + hdy*hdy < 360.0f * 360.0f) return;   // praca central (NPCs) livre
            for (const auto& q : placedB) { float dx = pos.x-q.x, dy = pos.y-q.y; if (dx*dx + dy*dy < minSp*minSp) return; }
            placedB.push_back(pos);
            put(type, pos, sc);
        };

        add(11, 165, 0.7f, 1.25f);   // grass base (universal, without collision)
        add(13,  34, 0.8f, 2.4f);    // marcas in the floor (same densidade of the world fixed)

        // 1) Props pequenos espalhados pela biome local (SEM buildings grandes here).
        for (int i = 0; i < 38; ++i) {
            Vector2 p  = { ox + rnd() * CH, oy + rnd() * CH };
            ZoneID  lz = tilemap.biomeAtWorld(p.x, p.y);
            float   roll = rnd(); int t = -1; float mn = 0.7f, mx = 1.6f;
            switch (lz) {
                case ZoneID::DarkForest:     t = (roll < 0.84f) ? 2 : 12; mx = 1.9f; break;  // arvores/pedras
                case ZoneID::Cemetery:       t = (roll < 0.70f) ? 2 : 12; break;             // arvores/pedras (lapides in the cluster)
                case ZoneID::GhostCity:      t = (roll < 0.4f) ? 5 : (roll < 0.65f) ? 6 : (roll < 0.85f) ? 12 : 28; break; // postes/cars/detritos/cratera
                case ZoneID::KronosForge:
                case ZoneID::InfernoZone:    t = (roll < 0.72f) ? 12 : 2; break;             // pedras/tree queimada
                case ZoneID::CursedFarm:     t = (roll < 0.55f) ? 4 : 2; break;              // cercas/arvores
                case ZoneID::Bunker:         t = (roll < 0.6f) ? 4 : 5; break;               // cercas/postes
                case ZoneID::AbandonedManor: t = (roll < 0.6f) ? 2 : 3; break;               // arvores/lapides
                case ZoneID::KronosNexus:    t = 12; break;                                  // detritos
                case ZoneID::LARuins:        t = (roll < 0.50f) ? 5 : (roll < 0.68f) ? 6 : (roll < 0.78f) ? 2 :
                                (roll < 0.88f) ? 28 : (roll < 0.95f) ? 30 : 31; break; // postes/cars/tree/cratera/asphalt/carcaca
                default:                     t = (roll < 0.7f) ? 2 : 12; break;
            }
            if (t >= 0) put(t, p, mn + rnd() * (mx - mn));
        }

        // 2) CLUSTERS estruturais (2 sementes by chunk) — only in the biome of the seed.
        for (int s = 0; s < 2; ++s) {
            Vector2 seed = { ox + (0.22f + rnd() * 0.56f) * CH, oy + (0.22f + rnd() * 0.56f) * CH };
            ZoneID  bz   = tilemap.biomeAtWorld(seed.x, seed.y);
            switch (bz) {
                case ZoneID::GhostCity:
                case ZoneID::LARuins: {   // quarteirao: buildings in the grade of the vias + cars in the lane
                    // Seed and PONTO DE COLAGEM: encaixa the quarteirao in the mesh real
                    // (lanes at 475+k*950, block center at 950*m). Building the
                    // 210u of the center = colado in the middle-fio, never inside the lane.
                    float bx = (float)std::floor(seed.x / 950.0f) * 950.0f + 950.0f;
                    float by = (float)std::floor(seed.y / 950.0f) * 950.0f + 950.0f;
                    for (int k = 0; k < 4; ++k) {
                        if (rnd() < 0.18f) continue;            // brecha = beco / lote empty
                        float px = (k & 1) ? bx + 210.0f : bx - 210.0f;
                        float py = (k & 2) ? by + 210.0f : by - 210.0f;
                        if (rnd() < 0.30f) {                    // lote empty = debris of the guerra
                            put(21, { px + (rnd() - 0.5f) * 130.0f, py + (rnd() - 0.5f) * 130.0f },
                                0.9f + rnd() * 0.9f);
                            if (rnd() < 0.5f)
                                put(6, { px + (rnd() - 0.5f) * 260.0f, py + (rnd() - 0.5f) * 260.0f }, 1.0f);
                            if (rnd() < 0.55f)
                                put(22, { px + (rnd() - 0.5f) * 260.0f, py + (rnd() - 0.5f) * 260.0f }, 1.0f);
                            continue;
                        }
                        int nKinds = 0;
                        const int* kinds = structuresFor(bz, nKinds);
                        // minSp 220: torres vizinhas in the same sidewalk sao adjacentes
                        // same (285u in the quarteirao); the 400 generico matava the canto.
                        putB(kinds[(int)(rnd() * nKinds) % nKinds], { px, py },
                             0.85f + rnd() * 0.35f, bz, 220.0f);
                    }
                    // miolo of the quarteirao: entulho (patio of guerra, not descampado)
                    for (int mi = 0; mi < 3; ++mi)
                        put(21, { bx + (rnd() - 0.5f) * 430.0f, by + (rnd() - 0.5f) * 430.0f },
                            1.0f + rnd() * 0.9f);
                    // postes in the meios-fios of the via (x in 338..352 of the center = sidewalk,
                    // 12..26u to outside the lane of 112u — never inside the asphalt)
                    for (int k = 0; k < 3; ++k) {
                        float px = bx + ((rnd() < 0.5f) ? -1.0f : 1.0f) * (338.0f + rnd() * 14.0f);
                        put(5, { px, by + (rnd() - 0.5f) * 600.0f }, 1.0f);
                    }
                } break;
                case ZoneID::CursedFarm: {                    // village: houses + barn + silo + fence in anel
                    putB(0, seed, 1.2f + rnd() * 0.4f, bz);
                    int houses = 1 + (int)(rnd() * 3.0f);
                    for (int k = 0; k < houses; ++k) {
                        float the = rnd() * 6.2832f, d = 110.0f + rnd() * 90.0f;
                        putB(0, { seed.x + cosf(the) * d, seed.y + sinf(the) * d }, 1.0f + rnd() * 0.5f, bz);
                    }
                    putB(1, { seed.x + (rnd() - 0.5f) * 170.0f, seed.y + (rnd() - 0.5f) * 170.0f }, 1.1f + rnd() * 0.4f, bz);
                    putB(8, { seed.x + (rnd() - 0.5f) * 210.0f, seed.y + (rnd() - 0.5f) * 210.0f }, 1.0f + rnd() * 0.4f, bz);
                    for (int k = 0; k < 12; ++k) { float the = k / 12.0f * 6.2832f; put(4, { seed.x + cosf(the) * 245.0f, seed.y + sinf(the) * 245.0f }, 1.0f + rnd() * 0.3f); }
                } break;
                case ZoneID::Bunker: {                        // composto militar: structures+silos in line + cercas
                    float ax = (rnd() < 0.5f) ? 1.0f : 0.0f, ay = 1.0f - ax;
                    int n = 3 + (int)(rnd() * 2.0f);
                    for (int k = 0; k < n; ++k) {
                        Vector2 bp = { seed.x + ax * (k - n * 0.5f) * 135.0f, seed.y + ay * (k - n * 0.5f) * 135.0f };
                        putB((rnd() < 0.5f) ? 7 : 8, bp, 1.0f + rnd() * 0.6f, bz);
                    }
                    for (int k = 0; k < 10; ++k) put(4, { seed.x + (rnd() - 0.5f) * 360.0f, seed.y + (rnd() - 0.5f) * 360.0f }, 1.0f);
                } break;
                case ZoneID::AbandonedManor: {                // manor: casarao central + estatuas in the cantos
                    putB(0, seed, 1.8f + rnd() * 0.6f, bz);
                    for (int k = 0; k < 4; ++k) { float the = k / 4.0f * 6.2832f + 0.7f; putB(10, { seed.x + cosf(the) * 175.0f, seed.y + sinf(the) * 175.0f }, 1.1f + rnd() * 0.5f, bz); }
                } break;
                case ZoneID::KronosNexus: {                   // core: struct central + anel of estatuas
                    putB(7, seed, 1.4f + rnd() * 0.6f, bz);
                    for (int k = 0; k < 6; ++k) { float the = k / 6.0f * 6.2832f; putB(10, { seed.x + cosf(the) * 165.0f, seed.y + sinf(the) * 165.0f }, 1.2f + rnd() * 0.5f, bz); }
                } break;
                case ZoneID::Cemetery: {                      // cemetery: lapides in FILEIRAS regulares
                    int rows = 3 + (int)(rnd() * 3.0f), cols = 4 + (int)(rnd() * 3.0f);
                    float sx = 48.0f, sy = 66.0f;
                    for (int r = 0; r < rows; ++r) for (int c = 0; c < cols; ++c) {
                        Vector2 gp = { seed.x + (c - cols * 0.5f) * sx, seed.y + (r - rows * 0.5f) * sy };
                        if (tilemap.biomeAtWorld(gp.x, gp.y) == bz) put(3, gp, 0.8f + rnd() * 0.4f);
                    }
                    putB(9, { seed.x, seed.y - rows * 0.5f * sy - 40.0f }, 1.0f, bz);  // arco/portao in the input
                } break;
                default: break;   // forest/forge: without cluster estrutural (only props espalhados)
            }
        }
    }

    // Reconstroi the collision of the estruturas of chunk (buildings/houses/silos) — circles.
    m_chunkSolids.clear();
    for (const auto& the : owDecor.scenery) {
        if (the.chunk == -1) continue;
        float rad = 0.0f;
        switch (the.type) {
            case 0:  rad = FIT_HOUSE    * the.scale * 0.46f; break;
            case 1:  rad = FIT_BARRACKS * the.scale * 0.46f; break;
            case 7:  rad = FIT_CASTLE   * the.scale * 0.40f; break;
            case 8:  rad = FIT_WELL     * the.scale * 0.42f; break;
            case 6:  rad = vehicleRadius(the.position.x, the.position.y, the.scale); break;
            case 9:  rad = 70.0f * the.scale; break;
            case 10: rad = 48.0f * the.scale; break;
            case 14: rad = 46.0f * the.scale; break;
            case 15: rad = 58.0f * the.scale; break;
            case 16: rad = 30.0f * the.scale; break;
            case 17: rad = 28.0f * the.scale; break;
            case 18: rad = 46.0f * the.scale; break;
            case 19: rad = 26.0f * the.scale; break;
            case 20: rad = buildingRadius(the.position.x, the.position.y, the.scale); break;
            default: continue;
        }
        m_chunkSolids.push_back({ the.position.x, the.position.y, rad });
    }
}

// Bloqueio of movement: wall of the grid OU struct generated in chunk in the infinito.
bool Game::isBlocked(Vector2 pos) const {
    if (tilemap.isWallAtPosition(pos)) return true;
    for (const auto& s : m_chunkSolids) {
        float dx = pos.x - s.x, dy = pos.y - s.y;
        if (dx * dx + dy * dy < s.z * s.z) return true;
    }
    return false;
}

// FALLBACK of the guarantee of the portal (updatePhasePortal): removes TODA collision of
// scenario num radius — tiles solidos E estruturas of chunk. So roda when the
// free-point search completely failed: melhor um building atravessavel
// near the portal that um portal inalcancavel (soft-lock of phase).
void Game::clearBlockingAt(Vector2 pos, float radius) {
    tilemap.clearSolidAt(pos, radius);
    float r2 = radius * radius;
    m_chunkSolids.erase(std::remove_if(m_chunkSolids.begin(), m_chunkSolids.end(),
        [&](const Vector3& s) {
            float dx = s.x - pos.x, dy = s.y - pos.y;
            return dx * dx + dy * dy < r2;
        }), m_chunkSolids.end());
}


void Game::streamSceneryBuild() {
    if (m_sceneryBuildQueue.empty() && !m_sceneryPostProcessNeeded) return;

    // Transfere until SCENERY_BUILD_BUDGET objetos by frame to evitar engasgo.
    const size_t n = std::min(m_sceneryBuildQueue.size(), (size_t)SCENERY_BUILD_BUDGET);
    owDecor.scenery.insert(owDecor.scenery.end(),
                           m_sceneryBuildQueue.begin(),
                           m_sceneryBuildQueue.begin() + (ptrdiff_t)n);
    m_sceneryBuildQueue.erase(m_sceneryBuildQueue.begin(),
                              m_sceneryBuildQueue.begin() + (ptrdiff_t)n);

    if (!m_sceneryBuildQueue.empty() || !m_sceneryPostProcessNeeded) return;

    // Queue esvaziou: roda the pos-processamento heavy UMA vez.

    // ── Collision of scenario: estruturas grandes bloqueiam passagem (not andar in
    //    up of houses/buildings/cars/silos/estatuas). Types: 0 house, 1 barn,
    //    6 car, 7 building, 8 silo, 9 catacomb, 10 statue. Trees/fences/poles
    //    ficam atravessaveis to not create labirintos that prendem the player.
    tilemap.clearSolidFlags();
    for (const auto& the : owDecor.scenery) {
        // Pegada of collision by type (buildings/houses grandes bloqueiam more area).
        float rad = 0.0f;
        switch (the.type) {
            case 0:  rad = FIT_HOUSE    * the.scale * 0.46f; break; // house
            case 1:  rad = FIT_BARRACKS * the.scale * 0.46f; break; // barn
            case 7:  rad = FIT_CASTLE   * the.scale * 0.40f; break; // building/castelo
            case 8:  rad = FIT_WELL     * the.scale * 0.42f; break; // silo
            case 9:  rad = 70.0f * the.scale; break;                // catacomb
            case 6:  rad = vehicleRadius(the.position.x, the.position.y, the.scale); break; // veiculo
            case 10: rad = 48.0f * the.scale; break;                // statue
            case 14: rad = 46.0f * the.scale; break;
            case 15: rad = 58.0f * the.scale; break;
            case 16: rad = 30.0f * the.scale; break;
            case 17: rad = 28.0f * the.scale; break;
            case 18: rad = 46.0f * the.scale; break;
            case 19: rad = 26.0f * the.scale; break;
            case 20: rad = buildingRadius(the.position.x, the.position.y, the.scale); break;   // building modern
            case 21: continue;   // entulho: decoracao, NOT bloqueia (virava labirinto)
            case 22: continue;   // fogueira: not bloqueia
            case 28: rad = 40.0f * the.scale; break;               // cratera: edge bloqueia
            case 29: rad = 60.0f * the.scale; break;               // building colapsado: debris bloqueiam
            case 31: rad = 30.0f * the.scale; break;               // carcaca queimada: casco bloqueia
            default: continue;                                            // arvores/cercas/postes: atravessavel
        }
        tilemap.markSolidAt(the.position, rad);
    }

    {   // MEDIDA: quantos objetos/estruturas the world fixed really generated
        int st = 0, gr = 0;
        for (const auto& the : owDecor.scenery) {
            if (the.type==0||the.type==1||the.type==6||the.type==7||the.type==8||
                the.type==9||the.type==10||(the.type>=14 && the.type<=20)) ++st;
            if (the.type==11) ++gr;
        }
        TraceLog(LOG_INFO, "SCENERY zone=%s total=%d estruturas=%d grass=%d",
                 getZoneInfo(currentZone).name.c_str(),
                 (int)owDecor.scenery.size(), st, gr);
    }
    owDecorBuilt = true;
    placeBaseShops();   // barracas/lojas of the NPCs of the zone segura (after of the solidas: the anel delas not colide)

    // AREA PROTEGIDA PURA: the base is lugar of NPC and SHOP, not of pular between
    // carcacas. O lixo urbano that falls inside the radius protected (cars, entulho,
    // marcas of fire, fogueiras, panelas of stone) leaves of the scenario; grass, arvores
    // and postes of light ficam (iluminacao/vegetacao not are refugo).
    {
        const float cleanR = safeZoneRadius;
        auto isDebris = [](int t) {
            return t == 6 || t == 12 || t == 13 || t == 21 || t == 22;
        };
        owDecor.scenery.erase(
            std::remove_if(owDecor.scenery.begin(), owDecor.scenery.end(),
                [&](const SceneryObject& the) {
                    if (!isDebris(the.type)) return false;
                    float dx = the.position.x - safeZoneCenter.x;
                    float dy = the.position.y - safeZoneCenter.y;
                    return dx * dx + dy * dy <= cleanR * cleanR;
                }),
            owDecor.scenery.end());
    }
    setupResourceNodes();   // in the of coleta (wood/stone/iron/silver/gold)
    setupAnimals();         // health selvagem (veado/coelho/javali/wolf/passaro)
    spawnCityFolk();        // civis that perambulam pela city (health environment)

    m_sceneryPostProcessNeeded = false;
}
