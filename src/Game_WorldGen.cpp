// Game_WorldGen.cpp — cenario do mundo aberto: construcoes por bioma, NPCs urbanos,
// streaming de chunks e bloqueio de movimento. Extraido de Game.cpp. Mesma classe Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

// Copias file-local das constantes de escala de Game.cpp (static constexpr):
// usadas aqui e ainda usadas em Game.cpp (renderWorld3D) — por isso duplicadas.
static constexpr float FIT_HOUSE    = 175.0f;   // casa de 2 andares ~11 m
static constexpr float FIT_BARRACKS = 190.0f;   // celeiro/galpao ~12 m
static constexpr float FIT_CASTLE   = 340.0f;   // predio/castelo ~21 m
static constexpr float FIT_WELL     = 130.0f;   // silo alto

// Helper file-local (static): usado por buildOpenWorldScenery/updateSceneryChunks.
static const int* structuresFor(ZoneID z, int& outCount) {
    // COERENCIA: castelo (7), poco/silo (8) e celeiro (1) sao MEDIEVAIS/RURAIS —
    // nao entram numa cidade com asfalto e carro. Cidade usa predio moderno (20),
    // casa (0) e bunker (15). O castelo sobrou so para a MANSAO, que e o unico
    // lugar onde ele faz sentido.
    // Nada de casa de telhado de telha (0 = house.obj, medieval) numa rua com
    // asfalto e carro: era exatamente o que quebrava a coerencia da cidade.
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

// Popula TODAS as regioes do mundo aberto com cenario denso, espalhado por toda
// a area de cada regiao. Chamado uma vez ao iniciar o mundo aberto.
void Game::buildOpenWorldScenery() {
    owDecor.scenery.clear();
    // Reseta o streaming de chunks (senão prédios sólidos invisíveis / cenário
    // fantasma sobrevivem ao Continuar/Novo Jogo dentro do mesmo processo).
    m_sceneryChunks.clear();
    m_chunkSolids.clear();
    m_lastChunkX = m_lastChunkY = -999999;

    // Gerador pseudo-aleatorio deterministico (nao usa Math.random).
    // A seed DERIVA DA FASE: com seed fixa, grama/entulho/pedras nasciam nas
    // MESMAS posicoes em toda fase ("mesmo mundo com outra tinta"). A realocacao
    // automatica do portal (anel de candidatos em Game_Phases.cpp) e a rede de
    // seguranca caso uma estrutura nova caia em cima do ponto do portal.
    unsigned int rng = 0x1234abcd + (unsigned int)owPhase * 0x9e3779b9u;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (float)((rng >> 8) & 0xFFFF) / 65535.0f; // 0..1
    };

    // count e calibrado para a regiao ANTIGA de 2560x2560; escala por AREA pra
    // densidade continuar a mesma quando o tamanho da regiao muda.
    // O mundo da FASE termina na barreira: cenario fora dela so gastava memoria e
    // ainda aparecia atras da parede de energia, denunciando que o mapa continua.
    const float phaseLimit = owPhaseRadius + 140.0f;
    auto insidePhase = [&](Vector2 p) {
        float dx = p.x - safeZoneCenter.x, dy = p.y - safeZoneCenter.y;
        return dx*dx + dy*dy <= phaseLimit * phaseLimit;
    };
    auto place = [&](Rectangle b, int type, int count,
                     float minScale, float maxScale, float margin) {
        float areaK = (b.width * b.height) / (2560.0f * 2560.0f);
        count = (int)(count * areaK);
        for (int i = 0; i < count; ++i) {
            SceneryObject o;
            o.type     = type;
            o.position = {
                b.x + margin + rnd() * (b.width  - 2 * margin),
                b.y + margin + rnd() * (b.height - 2 * margin)
            };
            if (!insidePhase(o.position)) continue;
            o.rotation = rnd() * 3.14159f;
            o.scale    = minScale + rnd() * (maxScale - minScale);
            // tint.r > 128 acende luzes (janelas/postes)
            o.tint     = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
            owDecor.scenery.push_back(o);
        }
    };
    // Zona livre do HUB (NPCs ficam no centro do mundo) — nenhuma estrutura nasce lá.
    const float hubX = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f; // centro do hub (1ª zona)
    const float hubY = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
    std::vector<Vector2> placedB;   // estruturas já colocadas (anti-sobreposição)
    auto isStruct = [](int t) { return t == 0 || t == 1 || t == 7 || t == 8 || t == 10 ||
                                       (t >= 14 && t <= 20); };
    // Coloca 1 objeto. Estruturas grandes: longe do hub + sem encostar em outra.
    // Distancia minima entre estruturas PROPORCIONAL ao tamanho. Um valor fixo de
    // 400u rejeitava quase todo lote de um predio de 60u de largura: a cidade saia
    // com uma construcao a cada quarteirao e meio, ou seja, vazia.
    auto spacingFor = [](int type) {
        switch (type) {
            case 7:  return 420.0f;   // castelo/mansao
            case 1:  return 260.0f;   // celeiro
            case 15: return 210.0f;   // bunker
            case 20: return 320.0f;   // predio moderno
            case 0:  return 180.0f;   // casa
            case 8:  return 170.0f;   // silo
            case 14: return 150.0f;   // cripta
            case 18: return 150.0f;   // cabana
            case 16: return 130.0f;   // espira
            case 17: return 120.0f;   // monolito
            case 19: return 110.0f;   // torre
            default: return 200.0f;
        }
    };
    auto put1 = [&](int type, Vector2 pos, float sc) {
        if (!insidePhase(pos)) return;
        if (isStruct(type)) {
            float hdx = pos.x - hubX, hdy = pos.y - hubY;
            if (hdx*hdx + hdy*hdy < 360.0f * 360.0f) return;   // praça central (NPCs) livre
            float md = spacingFor(type);
            for (const auto& q : placedB) { float dx = pos.x-q.x, dy = pos.y-q.y; if (dx*dx + dy*dy < md*md) return; }
            placedB.push_back(pos);
        }
        SceneryObject o; o.type = type; o.position = pos;
        // Predio de cidade se ALINHA a rua (giro em multiplos de 90 graus). Predio
        // torto no meio do quarteirao denuncia geracao aleatoria na hora.
        bool aligned = (type == 20 || type == 15 || type == 0);
        o.rotation = aligned ? (float)((int)(rnd() * 4.0f) % 4) * 1.5708f
                             : rnd() * 3.14159f;
        o.scale = sc; o.tint = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
        owDecor.scenery.push_back(o);
    };
    auto inBnd = [&](Rectangle b, Vector2 p, float m) {
        return p.x >= b.x + m && p.x <= b.x + b.width - m && p.y >= b.y + m && p.y <= b.y + b.height - m;
    };

    for (const auto& r : worldRegions) {
        Rectangle b = r.bounds;
        auto seedIn = [&]() { return Vector2{ b.x + 90 + rnd() * (b.width - 180), b.y + 90 + rnd() * (b.height - 180) }; };
        switch (r.zoneType) {
            case ZoneID::LARuins:    // Ruínas de LA — CIDADE: grade de prédios com praça central
            case ZoneID::GhostCity: {// Cidade fantasma
                float gp   = 950.0f;                    // quarteirao (fachada 280u + rua 210u)
                // GhostCity = cidade MORTA: muito mais lote vazio e ruina que LA,
                // quarteirao menor e mais degradado. Antes reusava a grade de LA
                // quase identica (skip 0.18 x 0.32) — as duas fases liam igual.
                bool ghost = (r.zoneType == ZoneID::GhostCity);
                float skip = ghost ? 0.44f : 0.30f;              // lotes vazios
                if (ghost) gp = 880.0f;                          // quarteiroes menores
                for (float gx = b.x + 220.0f; gx < b.x + b.width - 220.0f; gx += gp)
                    for (float gy = b.y + 220.0f; gy < b.y + b.height - 220.0f; gy += gp) {
                        if (rnd() < skip) continue;                            // lote vazio = praça/rua larga
                        // PERIMETRO do quarteirao ocupado: 3 construcoes por lado,
                        // coladas na calcada, deixando o MIOLO como patio de entulho.
                        // Assim a rua vira corredor entre massas construidas.
                        int nk2 = 0; const int* kk2 = structuresFor(r.zoneType, nk2);
                        const float EDGE = 350.0f;      // do centro do quarteirao ate a calcada
                        for (int side = 0; side < 4; ++side) {
                            for (int slot = 0; slot < 2; ++slot) {
                                // GhostCity: mais brechas (rua esburacada, cidade vazia)
                                if (rnd() < (ghost ? 0.34f : 0.18f)) continue;  // brecha = beco
                                float along = (slot - 0.5f) * 300.0f + (rnd() - 0.5f) * 50.0f;
                                Vector2 lot;
                                if (side == 0)      lot = { gx + along, gy - EDGE };
                                else if (side == 1) lot = { gx + along, gy + EDGE };
                                else if (side == 2) lot = { gx - EDGE,  gy + along };
                                else                lot = { gx + EDGE,  gy + along };
                                // GhostCity: predios menores/mais degradados
                                float sc2 = ghost ? 0.58f + rnd() * 0.22f : 0.74f + rnd() * 0.26f;
                                put1(kk2[(int)(rnd() * nk2) % nk2], lot, sc2);
                            }
                        }
                        // miolo: entulho e carcacas (patio escuro, nao descampado)
                        for (int ci = 0; ci < 3; ++ci)
                            put1(21, { gx + (rnd() - 0.5f) * 430.0f,
                                       gy + (rnd() - 0.5f) * 430.0f }, 1.0f + rnd() * 1.0f);
                        // fogueira de sobreviventes de vez em quando (luz + cor)
                        if (rnd() < 0.30f)
                            put1(22, { gx + (rnd() - 0.5f) * 360.0f,
                                       gy + (rnd() - 0.5f) * 360.0f }, 1.0f + rnd() * 0.5f);
                    }
                place(b, 5, 20, 1.0f, 1.0f, 150);  // postes nas ruas
                place(b, 6, 16, 1.0f, 1.0f, 150);  // carros abandonados (so cidade)
                place(b, 2,  6, 0.8f, 1.2f, 150);  // árvores
            } break;
            case ZoneID::Bunker: {   // Bunker — compostos militares (estruturas+silos em linha)
                for (int bl = 0; bl < 3; ++bl) {
                    Vector2 seed = seedIn(); float ax = (rnd()<0.5f)?1.0f:0.0f, ay = 1.0f-ax; int n = 3 + (int)(rnd()*2.0f);
                    for (int k = 0; k < n; ++k) { Vector2 bp = { seed.x+ax*(k-n*0.5f)*135.0f, seed.y+ay*(k-n*0.5f)*135.0f }; if (inBnd(b,bp,30.0f)) put1((rnd()<0.5f)?7:8, bp, 1.0f+rnd()*0.6f); }
                    for (int k = 0; k < 8; ++k) put1(4, { seed.x+(rnd()-0.5f)*340.0f, seed.y+(rnd()-0.5f)*340.0f }, 1.0f);
                }
                place(b, 5, 8, 1.0f, 1.0f, 150);   // postes
            } break;
            case ZoneID::DarkForest: // Floresta densa — MUITA arvore: a floresta tem que abafar
                place(b, 2, 120, 0.9f, 1.8f, 80); place(b, 4, 10, 1.0f, 1.3f, 120); place(b, 3, 8, 0.7f, 1.0f, 120);
                break;
            case ZoneID::CursedFarm: {// Fazenda — VILAS (casas+celeiro+silo+cerca em anel)
                for (int v = 0; v < 3; ++v) {
                    Vector2 seed = seedIn(); put1(0, seed, 1.2f + rnd()*0.4f);
                    int houses = 1 + (int)(rnd()*3.0f);
                    for (int k = 0; k < houses; ++k) { float a = rnd()*6.2832f, d = 110.0f+rnd()*90.0f; Vector2 bp = { seed.x+cosf(a)*d, seed.y+sinf(a)*d }; if (inBnd(b,bp,30.0f)) put1(0, bp, 1.0f+rnd()*0.5f); }
                    Vector2 barn = { seed.x+(rnd()-0.5f)*170.0f, seed.y+(rnd()-0.5f)*170.0f }; if (inBnd(b,barn,30.0f)) put1(1, barn, 1.1f+rnd()*0.4f);
                    Vector2 silo = { seed.x+(rnd()-0.5f)*210.0f, seed.y+(rnd()-0.5f)*210.0f }; if (inBnd(b,silo,30.0f)) put1(8, silo, 1.0f+rnd()*0.4f);
                    for (int k = 0; k < 12; ++k) { float a = k/12.0f*6.2832f; put1(4, { seed.x+cosf(a)*245.0f, seed.y+sinf(a)*245.0f }, 1.0f+rnd()*0.3f); }
                }
                place(b, 2, 16, 0.8f, 1.3f, 120);  // árvores
            } break;
            case ZoneID::Cemetery: { // Cemitério — lápides em FILEIRAS + portão
                for (int g = 0; g < 6; ++g) {
                    Vector2 seed = seedIn(); int rows = 3 + (int)(rnd()*3.0f), cols = 4 + (int)(rnd()*3.0f); float sx = 48.0f, sy = 66.0f;
                    for (int rr = 0; rr < rows; ++rr) for (int c = 0; c < cols; ++c) { Vector2 gp = { seed.x+(c-cols*0.5f)*sx, seed.y+(rr-rows*0.5f)*sy }; if (inBnd(b,gp,30.0f)) put1(3, gp, 0.8f+rnd()*0.4f); }
                    Vector2 gate = { seed.x, seed.y - rows*0.5f*sy - 40.0f }; if (inBnd(b,gate,30.0f)) put1(9, gate, 1.0f);
                }
                place(b, 2, 14, 0.9f, 1.5f, 120); place(b, 10, 4, 1.0f, 1.4f, 250);  // árvores/estátuas
            } break;
            case ZoneID::KronosForge: // Forja de lava — silos/estátuas/pedras (sem casas)
                place(b, 8, 8, 1.0f, 1.8f, 220); place(b, 10, 5, 1.0f, 1.5f, 250); place(b, 12, 30, 0.7f, 1.3f, 60); place(b, 2, 6, 0.7f, 1.0f, 150);
                break;
            case ZoneID::AbandonedManor: {// Mansão — casarão central + estátuas nos cantos
                for (int m = 0; m < 2; ++m) {
                    Vector2 seed = seedIn(); put1(0, seed, 1.8f + rnd()*0.6f);
                    for (int k = 0; k < 4; ++k) { float a = k/4.0f*6.2832f + 0.7f; Vector2 bp = { seed.x+cosf(a)*175.0f, seed.y+sinf(a)*175.0f }; if (inBnd(b,bp,30.0f)) put1(10, bp, 1.1f+rnd()*0.5f); }
                }
                place(b, 2, 16, 0.9f, 1.6f, 120); place(b, 3, 12, 0.7f, 1.1f, 120); place(b, 4, 20, 1.0f, 1.3f, 100);
            } break;
            case ZoneID::KronosNexus: {// Núcleo — estrutura central + anel de estátuas
                for (int c2 = 0; c2 < 2; ++c2) { Vector2 seed = seedIn(); put1(7, seed, 1.4f+rnd()*0.6f); for (int k = 0; k < 6; ++k) { float a = k/6.0f*6.2832f; Vector2 bp = { seed.x+cosf(a)*165.0f, seed.y+sinf(a)*165.0f }; if (inBnd(b,bp,30.0f)) put1(10, bp, 1.2f+rnd()*0.5f); } }
                place(b, 10, 6, 1.2f, 2.0f, 200);  // estátuas imponentes
            } break;
            case ZoneID::InfernoZone: // Inferno — espiras, rocha vulcanica, so arvore MORTA
                place(b, 16, 6, 0.9f, 1.5f, 250); place(b, 12, 40, 0.8f, 1.5f, 60); place(b, 2, 6, 0.8f, 1.1f, 150);
                break;
            case ZoneID::Catacombs:   // Catacumbas — criptas/arcos sobre chao de pedra nua
                place(b, 14, 5, 1.0f, 1.3f, 250); place(b, 9, 3, 1.0f, 1.2f, 250); place(b, 12, 30, 0.7f, 1.3f, 60); place(b, 3, 16, 0.7f, 1.0f, 100);
                break;
            default:
                place(b, 2, 10, 0.8f, 1.4f, 150);
                break;
        }
        // Vegetação rasteira densa em TODA zona — vida no chão (grama + detritos).
        // QUANTIDADE por bioma: o inferno quase nao tem grama e transborda rocha,
        // a floresta abafa em grama, a cidade fantasma afoga em entulho. Antes
        // TODA fase tinha 300 gramas/120 manchas/90 entulhos — o chao lia igual.
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
        place(b, 11, (int)(300 * grassK),  0.7f, 1.25f, 20);  // grama
        place(b, 12, (int)( 36 * rockK),   0.6f, 1.2f, 40);   // pedras/detritos
        place(b, 13, (int)(120 * stainK),  0.8f, 2.4f, 20);   // manchas/rachaduras no chao
        place(b, 21, (int)( 90 * rubbleK), 0.7f, 1.6f, 30);   // ENTULHO: o mundo caiu, tem que ter escombro
    }

    // ── Colisao de cenario: estruturas grandes bloqueiam passagem (nao andar em
    //    cima de casas/predios/carros/silos/estatuas). Tipos: 0 casa, 1 celeiro,
    //    6 carro, 7 predio, 8 silo, 9 catacumba, 10 estatua. Arvores/cercas/postes
    //    ficam atravessaveis para nao criar labirintos que prendem o jogador.
    tilemap.clearSolidFlags();
    for (const auto& o : owDecor.scenery) {
        // Pegada de colisao por tipo (predios/casas grandes bloqueiam mais area).
        float rad = 0.0f;
        switch (o.type) {
            // pegada = fracao do tamanho REALMENTE desenhado (fit * escala do obj)
            case 0:  rad = FIT_HOUSE    * o.scale * 0.46f; break; // casa
            case 1:  rad = FIT_BARRACKS * o.scale * 0.46f; break; // celeiro
            case 7:  rad = FIT_CASTLE   * o.scale * 0.40f; break; // predio/castelo
            case 8:  rad = FIT_WELL     * o.scale * 0.42f; break; // silo
            case 9:  rad = 70.0f * o.scale; break;                // catacumba
            case 6:  rad = 78.0f * o.scale; break;                // veiculo (primitivas)
            case 10: rad = 48.0f * o.scale; break;                // estatua
            // estruturas proprias de bioma (cripta/bunker/espira/monolito/cabana/torre)
            case 14: rad = 46.0f * o.scale; break;
            case 15: rad = 58.0f * o.scale; break;
            case 16: rad = 30.0f * o.scale; break;
            case 17: rad = 28.0f * o.scale; break;
            case 18: rad = 46.0f * o.scale; break;
            case 19: rad = 26.0f * o.scale; break;
            case 20: rad = 96.0f * o.scale; break;   // predio moderno (circulo inscrito)
            case 21: continue;   // entulho: decoracao, NAO bloqueia (virava labirinto)
            case 22: continue;   // fogueira: nao bloqueia
            default: continue;                                            // arvores/cercas/postes: atravessavel
        }
        tilemap.markSolidAt(o.position, rad);
    }

    {   // MEDIDA: quantos objetos/estruturas o mundo fixo realmente gerou
        int st = 0, gr = 0;
        for (const auto& o : owDecor.scenery) {
            // estruturas = TUDO que le como construcao/obstaculo, nao so o
            // catalogo medieval — antes reportava estruturas=0 numa cidade
            // cheia de predios modernos (20), carros (6) e estruturas de
            // bioma (14-19), ensinando a ignorar o log.
            if (o.type==0||o.type==1||o.type==6||o.type==7||o.type==8||
                o.type==9||o.type==10||(o.type>=14 && o.type<=20)) ++st;
            if (o.type==11) ++gr;
        }
        TraceLog(LOG_INFO, "SCENERY zona=%s total=%d estruturas=%d grama=%d",
                 getZoneInfo(currentZone).name.c_str(),
                 (int)owDecor.scenery.size(), st, gr);
    }
    owDecorBuilt       = true;
    setupResourceNodes();   // nós de coleta (madeira/pedra/ferro/prata/ouro)
    setupAnimals();         // vida selvagem (veado/coelho/javali/lobo/passaro)
    spawnCityFolk();        // civis que perambulam pela cidade (vida ambiente)
}

// Popula a CIDADE com civis que TÊM TAREFAS (não vagam à toa): guardas patrulham
// rotas, trabalhadores ficam numa obra (martelando), grupos conversam em rodas,
// vendedores ficam no posto. Reusam os modelos voxel dos NPCs (por NPCRole).
void Game::spawnCityFolk() {
    cityFolk.clear();
    if (!openWorldMode) return;

    // Rodas de conversa (pontos fixos onde grupos se juntam)
    Vector2 meet[4];
    for (int i = 0; i < 4; ++i) {
        float a = GetRandomValue(0, 359) * DEG2RAD;
        float d = (float)GetRandomValue(220, (int)(safeZoneRadius * 0.7f));
        meet[i] = { safeZoneCenter.x + std::cos(a) * d, safeZoneCenter.y + std::sin(a) * d };
    }
    // Lista de prédios da cidade (postos de trabalho)
    std::vector<Vector2> builds;
    for (const auto& o : owDecor.scenery)
        if ((o.type==0||o.type==1||o.type==7||o.type==8) &&
            Vector2Distance(o.position, safeZoneCenter) < safeZoneRadius + 200.0f)
            builds.push_back(o.position);

    int n = 26;
    for (int i = 0; i < n; ++i) {
        CityFolk f;
        float a = GetRandomValue(0, 359) * DEG2RAD;
        float d = (float)GetRandomValue(170, (int)(safeZoneRadius * 0.9f));
        f.home = { safeZoneCenter.x + std::cos(a) * d, safeZoneCenter.y + std::sin(a) * d };
        int tries = 0;
        while (isBlocked(f.home) && tries < 10) {
            a = GetRandomValue(0, 359) * DEG2RAD; d = (float)GetRandomValue(170, (int)(safeZoneRadius * 0.9f));
            f.home = { safeZoneCenter.x + std::cos(a) * d, safeZoneCenter.y + std::sin(a) * d }; tries++;
        }
        f.position = f.home; f.role = GetRandomValue(0, 6); f.speed = 40.0f + (float)GetRandomValue(0, 38);

        int jr = GetRandomValue(0, 99);
        if      (jr < 28) f.job = FolkJob::Guard;
        else if (jr < 60) f.job = FolkJob::Worker;
        else if (jr < 86) f.job = FolkJob::Chatter;
        else              f.job = FolkJob::Vendor;

        if (f.job == FolkJob::Guard) {                         // ronda entre 2 pontos
            float ga = GetRandomValue(0, 359) * DEG2RAD, gd = 200.0f + GetRandomValue(0, 180);
            f.anchor = { f.home.x + std::cos(ga) * gd, f.home.y + std::sin(ga) * gd };
            f.target = f.anchor;
        } else if (f.job == FolkJob::Worker) {                 // posto = perto de um prédio
            if (!builds.empty()) {
                Vector2 b = builds[GetRandomValue(0, (int)builds.size()-1)];
                Vector2 dir = { f.home.x - b.x, f.home.y - b.y };
                float l = std::sqrt(dir.x*dir.x + dir.y*dir.y); if (l < 1.0f) { dir = {1,0}; l = 1; }
                f.anchor = { b.x + dir.x/l * 95.0f, b.y + dir.y/l * 95.0f };  // em FRENTE ao prédio
            } else f.anchor = f.home;
            f.target = f.anchor;
        } else if (f.job == FolkJob::Chatter) {                // junta-se a uma roda
            f.anchor = meet[GetRandomValue(0, 3)];
            f.anchor.x += (float)GetRandomValue(-28, 28); f.anchor.y += (float)GetRandomValue(-28, 28);
            f.target = f.anchor;
        } else { f.anchor = f.home; f.target = f.home; }       // vendedor fica no posto
        cityFolk.push_back(f);
    }
}

void Game::updateCityFolk(float dt) {
    if (!openWorldMode) return;
    for (auto& f : cityFolk) {
        Vector2 d = { f.target.x - f.position.x, f.target.y - f.position.y };
        float dist = std::sqrt(d.x*d.x + d.y*d.y);

        if (f.job == FolkJob::Guard) {                         // PATRULHA: vai-e-volta sem parar
            if (dist < 20.0f) {
                bool atB = Vector2Distance(f.target, f.anchor) < 5.0f;
                f.target = atB ? f.home : f.anchor;            // troca a ponta da ronda
            } else {
                Vector2 s = { d.x/dist * f.speed * dt, d.y/dist * f.speed * dt };
                Vector2 np = { f.position.x + s.x, f.position.y + s.y };
                if (!isBlocked(np)) { f.position = np; f.facing = (s.x>=0)?1:-1;
                                      f.walkPhase += dt * 7.0f; }   // passo anda com o civil
                else { Vector2 tmp = f.target; f.target = f.home; f.home = tmp; }  // contorna: inverte rota
            }
            continue;
        }

        if (dist > 16.0f) {                                    // indo para o posto
            Vector2 s = { d.x/dist * f.speed * dt, d.y/dist * f.speed * dt };
            Vector2 np = { f.position.x + s.x, f.position.y + s.y };
            if (!isBlocked(np)) { f.position = np; f.facing = (s.x>=0)?1:-1; f.atStation = false;
                                  f.walkPhase += dt * 7.0f; }
            else f.target = f.home;
        } else {                                               // CHEGOU → executa a TAREFA
            f.atStation = true; f.work += dt;
            if (f.job == FolkJob::Worker) {                    // martela; de vez em quando muda de pé
                f.timer -= dt;
                if (f.timer <= 0.0f) {
                    float a = GetRandomValue(0, 359) * DEG2RAD;
                    f.target = { f.anchor.x + std::cos(a)*26.0f, f.anchor.y + std::sin(a)*26.0f };
                    f.timer = 2.5f + (float)GetRandomValue(0, 300)/100.0f;
                }
            } else if (f.job == FolkJob::Chatter) {            // fica na roda; raramente troca de grupo
                f.timer -= dt;
                if (f.timer <= 0.0f) { f.timer = 5.0f + (float)GetRandomValue(0, 600)/100.0f; }
            }
            // Vendor: fica parado no posto.
        }
    }
}

// Mundo INFINITO: gera cenário em CHUNKS ao redor do player conforme explora e
// descarrega chunks distantes. Determinístico por chunk (auto-construção).
void Game::updateSceneryChunks(Vector2 playerPos) {
    if (!openWorldMode) return;
    const float CH   = 1280.0f;                 // tamanho do chunk (~20 tiles)
    const int   RAD  = 2;                        // raio em chunks (5x5 carregados)
    const float ORIG = (float)(Tilemap::OW_COLS * Tilemap::OW_ZONE_W * 64); // região fixa original
    int pcx = (int)floorf(playerPos.x / CH);
    int pcy = (int)floorf(playerPos.y / CH);

    auto keyOf = [](int cx, int cy) -> long long {
        return ((long long)(cx + 100000) << 21) | (long long)(cy + 100000);
    };
    std::set<long long> want;
    for (int cy = pcy - RAD; cy <= pcy + RAD; ++cy)
        for (int cx = pcx - RAD; cx <= pcx + RAD; ++cx) want.insert(keyOf(cx, cy));

    // Ao CRUZAR de chunk: descarrega o que saiu do raio (poda). Mantém o fixo (-1).
    if (pcx != m_lastChunkX || pcy != m_lastChunkY) {
        m_lastChunkX = pcx; m_lastChunkY = pcy;
        owDecor.scenery.erase(std::remove_if(owDecor.scenery.begin(), owDecor.scenery.end(),
            [&](const SceneryObject& o){ return o.chunk != -1 && want.find(o.chunk) == want.end(); }),
            owDecor.scenery.end());
        for (auto it = m_sceneryChunks.begin(); it != m_sceneryChunks.end();)
            it = (want.find(*it) == want.end()) ? m_sceneryChunks.erase(it) : std::next(it);
    }

    // AMORTIZADO: gera no máximo 1 chunk por frame (evita engasgo ao cruzar fronteira).
    long long toGen = -1; int gcx = 0, gcy = 0;
    for (int cy = pcy - RAD; cy <= pcy + RAD && toGen < 0; ++cy)
        for (int cx = pcx - RAD; cx <= pcx + RAD; ++cx) {
            long long k = keyOf(cx, cy);
            if (!m_sceneryChunks.count(k)) { toGen = k; gcx = cx; gcy = cy; break; }
        }
    if (toGen < 0) return;                       // todos os chunks do raio já existem
    m_sceneryChunks.insert(toGen);

    float ox = gcx * CH, oy = gcy * CH;
    if (!(ox >= 0 && oy >= 0 && ox < ORIG && oy < ORIG)) {   // não gera na região fixa
        unsigned int rng = (unsigned int)(gcx * 73856093) ^ (unsigned int)(gcy * 19349663) ^ 0x5151u;
        auto rnd = [&]() { rng = rng * 1664525u + 1013904223u; return (float)((rng >> 8) & 0xFFFF) / 65535.0f; };
        auto add = [&](int type, int count, float mn, float mx) {
            for (int i = 0; i < count; ++i) {
                SceneryObject o;
                o.type = type;
                o.position = { ox + rnd() * CH, oy + rnd() * CH };
                o.rotation = rnd() * 3.14159f;
                o.scale = mn + rnd() * (mx - mn);
                o.tint = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
                o.chunk = toGen;
                owDecor.scenery.push_back(o);
            }
        };
        // ── Cenário com AGRUPAMENTO coerente + contexto por bioma da POSIÇÃO ──
        // Props pequenos espalhados; estruturas grandes em CLUSTERS (quarteirões,
        // vilas, compostos) só na bioma certa. Castelo/casa nunca solto fora de tema.
        auto put = [&](int type, Vector2 pos, float sc) {
            {   // fora da barreira da fase nao existe mundo
                float dx = pos.x - safeZoneCenter.x, dy = pos.y - safeZoneCenter.y;
                float lim = owPhaseRadius + 140.0f;
                if (dx*dx + dy*dy > lim*lim) return;
            }
            SceneryObject o;
            o.type = type; o.position = pos; o.rotation = rnd() * 3.14159f;
            o.scale = sc; o.tint = (rnd() > 0.5f) ? Color{200,200,200,255} : Color{80,80,80,255};
            o.chunk = toGen; owDecor.scenery.push_back(o);
        };
        // Estrutura só entra se: a bioma DA POSIÇÃO bate (sem vazar pro vizinho),
        // está longe do hub (NPCs) e NÃO encosta em outra estrutura (anti-amontoado).
        std::vector<Vector2> placedB;
        const float hubX = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize) / 2.0f; // centro do hub (1ª zona)
        const float hubY = (float)(Tilemap::OW_ZONE_H * Tilemap::tileSize) / 2.0f;
        auto putB = [&](int type, Vector2 pos, float sc, ZoneID want) {
            if (tilemap.biomeAtWorld(pos.x, pos.y) != want) return;
            float hdx = pos.x - hubX, hdy = pos.y - hubY;
            if (hdx*hdx + hdy*hdy < 360.0f * 360.0f) return;   // praça central (NPCs) livre
            for (const auto& q : placedB) { float dx = pos.x-q.x, dy = pos.y-q.y; if (dx*dx + dy*dy < 400.0f*400.0f) return; }
            placedB.push_back(pos);
            put(type, pos, sc);
        };

        add(11, 165, 0.7f, 1.25f);   // grama base (universal, sem colisão)
        add(13,  34, 0.8f, 2.4f);    // marcas no chao (mesma densidade do mundo fixo)

        // 1) Props pequenos espalhados pela bioma local (SEM prédios grandes aqui).
        for (int i = 0; i < 38; ++i) {
            Vector2 p  = { ox + rnd() * CH, oy + rnd() * CH };
            ZoneID  lz = tilemap.biomeAtWorld(p.x, p.y);
            float   roll = rnd(); int t = -1; float mn = 0.7f, mx = 1.6f;
            switch (lz) {
                case ZoneID::DarkForest:     t = (roll < 0.84f) ? 2 : 12; mx = 1.9f; break;  // árvores/pedras
                case ZoneID::Cemetery:       t = (roll < 0.70f) ? 2 : 12; break;             // árvores/pedras (lápides no cluster)
                case ZoneID::GhostCity:      t = (roll < 0.5f) ? 5 : (roll < 0.8f) ? 6 : 12; break; // postes/carros/detritos
                case ZoneID::KronosForge:
                case ZoneID::InfernoZone:    t = (roll < 0.72f) ? 12 : 2; break;             // pedras/árvore queimada
                case ZoneID::CursedFarm:     t = (roll < 0.55f) ? 4 : 2; break;              // cercas/árvores
                case ZoneID::Bunker:         t = (roll < 0.6f) ? 4 : 5; break;               // cercas/postes
                case ZoneID::AbandonedManor: t = (roll < 0.6f) ? 2 : 3; break;               // árvores/lápides
                case ZoneID::KronosNexus:    t = 12; break;                                  // detritos
                case ZoneID::LARuins:        t = (roll < 0.5f) ? 5 : (roll < 0.8f) ? 6 : 2; break; // postes/carros/árvore
                default:                     t = (roll < 0.7f) ? 2 : 12; break;
            }
            if (t >= 0) put(t, p, mn + rnd() * (mx - mn));
        }

        // 2) CLUSTERS estruturais (2 sementes por chunk) — só na bioma do seed.
        for (int s = 0; s < 2; ++s) {
            Vector2 seed = { ox + (0.22f + rnd() * 0.56f) * CH, oy + (0.22f + rnd() * 0.56f) * CH };
            ZoneID  bz   = tilemap.biomeAtWorld(seed.x, seed.y);
            switch (bz) {
                case ZoneID::GhostCity:
                case ZoneID::LARuins: {                       // quarteirão: GRADE de prédios + postes nas ruas
                    int cols = 2 + (int)(rnd() * 2.0f), rows = 2 + (int)(rnd() * 2.0f);
                    float sp = 340.0f;
                    for (int r = 0; r < rows; ++r) for (int c = 0; c < cols; ++c) {
                        if (rnd() < 0.18f) continue;          // lote vazio (variedade)
                        Vector2 bp = { seed.x + (c - cols * 0.5f) * sp + (rnd() - 0.5f) * 26.0f,
                                       seed.y + (r - rows * 0.5f) * sp + (rnd() - 0.5f) * 26.0f };
                        float tr = rnd();
                        // tipos vindos do catalogo do BIOMA (cada fase tem outra cara)
                        int nKinds = 0;
                        const int* kinds = structuresFor(bz, nKinds);   // bioma do chunk
                        int bt = kinds[(int)(tr * nKinds) % nKinds];
                        putB(bt, bp, 0.85f + rnd() * 0.35f, bz);
                    }
                    for (int k = 0; k < 4; ++k)
                        put(5, { seed.x + (rnd() - 0.5f) * sp * cols, seed.y + (rnd() - 0.5f) * sp * rows }, 1.0f);
                } break;
                case ZoneID::CursedFarm: {                    // vila: casas + celeiro + silo + cerca em anel
                    putB(0, seed, 1.2f + rnd() * 0.4f, bz);
                    int houses = 1 + (int)(rnd() * 3.0f);
                    for (int k = 0; k < houses; ++k) {
                        float a = rnd() * 6.2832f, d = 110.0f + rnd() * 90.0f;
                        putB(0, { seed.x + cosf(a) * d, seed.y + sinf(a) * d }, 1.0f + rnd() * 0.5f, bz);
                    }
                    putB(1, { seed.x + (rnd() - 0.5f) * 170.0f, seed.y + (rnd() - 0.5f) * 170.0f }, 1.1f + rnd() * 0.4f, bz);
                    putB(8, { seed.x + (rnd() - 0.5f) * 210.0f, seed.y + (rnd() - 0.5f) * 210.0f }, 1.0f + rnd() * 0.4f, bz);
                    for (int k = 0; k < 12; ++k) { float a = k / 12.0f * 6.2832f; put(4, { seed.x + cosf(a) * 245.0f, seed.y + sinf(a) * 245.0f }, 1.0f + rnd() * 0.3f); }
                } break;
                case ZoneID::Bunker: {                        // composto militar: estruturas+silos em linha + cercas
                    float ax = (rnd() < 0.5f) ? 1.0f : 0.0f, ay = 1.0f - ax;
                    int n = 3 + (int)(rnd() * 2.0f);
                    for (int k = 0; k < n; ++k) {
                        Vector2 bp = { seed.x + ax * (k - n * 0.5f) * 135.0f, seed.y + ay * (k - n * 0.5f) * 135.0f };
                        putB((rnd() < 0.5f) ? 7 : 8, bp, 1.0f + rnd() * 0.6f, bz);
                    }
                    for (int k = 0; k < 10; ++k) put(4, { seed.x + (rnd() - 0.5f) * 360.0f, seed.y + (rnd() - 0.5f) * 360.0f }, 1.0f);
                } break;
                case ZoneID::AbandonedManor: {                // mansão: casarão central + estátuas nos cantos
                    putB(0, seed, 1.8f + rnd() * 0.6f, bz);
                    for (int k = 0; k < 4; ++k) { float a = k / 4.0f * 6.2832f + 0.7f; putB(10, { seed.x + cosf(a) * 175.0f, seed.y + sinf(a) * 175.0f }, 1.1f + rnd() * 0.5f, bz); }
                } break;
                case ZoneID::KronosNexus: {                   // núcleo: estrutura central + anel de estátuas
                    putB(7, seed, 1.4f + rnd() * 0.6f, bz);
                    for (int k = 0; k < 6; ++k) { float a = k / 6.0f * 6.2832f; putB(10, { seed.x + cosf(a) * 165.0f, seed.y + sinf(a) * 165.0f }, 1.2f + rnd() * 0.5f, bz); }
                } break;
                case ZoneID::Cemetery: {                      // cemitério: lápides em FILEIRAS regulares
                    int rows = 3 + (int)(rnd() * 3.0f), cols = 4 + (int)(rnd() * 3.0f);
                    float sx = 48.0f, sy = 66.0f;
                    for (int r = 0; r < rows; ++r) for (int c = 0; c < cols; ++c) {
                        Vector2 gp = { seed.x + (c - cols * 0.5f) * sx, seed.y + (r - rows * 0.5f) * sy };
                        if (tilemap.biomeAtWorld(gp.x, gp.y) == bz) put(3, gp, 0.8f + rnd() * 0.4f);
                    }
                    putB(9, { seed.x, seed.y - rows * 0.5f * sy - 40.0f }, 1.0f, bz);  // arco/portão na entrada
                } break;
                default: break;   // floresta/forja: sem cluster estrutural (só props espalhados)
            }
        }
    }

    // Reconstrói a colisão das estruturas de chunk (prédios/casas/silos) — círculos.
    m_chunkSolids.clear();
    for (const auto& o : owDecor.scenery) {
        if (o.chunk == -1) continue;
        float rad = 0.0f;
        switch (o.type) {
            case 0:  rad = FIT_HOUSE    * o.scale * 0.46f; break;
            case 1:  rad = FIT_BARRACKS * o.scale * 0.46f; break;
            case 7:  rad = FIT_CASTLE   * o.scale * 0.40f; break;
            case 8:  rad = FIT_WELL     * o.scale * 0.42f; break;
            default: continue;
        }
        m_chunkSolids.push_back({ o.position.x, o.position.y, rad });
    }
}

// Bloqueio de movimento: parede do grid OU estrutura gerada em chunk no infinito.
bool Game::isBlocked(Vector2 pos) const {
    if (tilemap.isWallAtPosition(pos)) return true;
    for (const auto& s : m_chunkSolids) {
        float dx = pos.x - s.x, dy = pos.y - s.y;
        if (dx * dx + dy * dy < s.z * s.z) return true;
    }
    return false;
}

// FALLBACK da garantia do portal (updatePhasePortal): remove TODA colisao de
// cenario num raio — tiles solidos E estruturas de chunk. So roda quando a
// busca de ponto livre falhou por completo: melhor um predio atravessavel
// perto do portal que um portal inalcancavel (soft-lock de fase).
void Game::clearBlockingAt(Vector2 pos, float radius) {
    tilemap.clearSolidAt(pos, radius);
    float r2 = radius * radius;
    m_chunkSolids.erase(std::remove_if(m_chunkSolids.begin(), m_chunkSolids.end(),
        [&](const Vector3& s) {
            float dx = s.x - pos.x, dy = s.y - pos.y;
            return dx * dx + dy * dy < r2;
        }), m_chunkSolids.end());
}

