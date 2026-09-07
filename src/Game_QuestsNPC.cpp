// Game_QuestsNPC.cpp — setup of quests and NPCs (base, zones and regions of the world).
// Extraido of Game.cpp. Same class Game.
#include "Game.h"
#include "SpriteGen.h"
#include "SpriteExtrude.h"
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <string>

// ─── Quests & NPCs Setup ─────────────────────────────────────────────────────

void Game::buildQuests() {
    quests.clear();

    Quest q1("q_limpar", "Limpeza of Ruins",
             "Elimine 5 patrulhas KRONOS", "VANCE RIOS", QuestType::Kill, 5);
    q1.rewardHP = 50.0f; q1.rewardXP = 100;
    q1.rewardEquip = EDB::pistolaPlas();
    quests.push_back(q1);

    Quest q2("q_suprimentos", "Suprimentos Criticos",
             "Colete 3 Energy Cores for the NEXUS", "MARCO VEIL", QuestType::Collect, 3);
    q2.rewardHP = 30.0f; q2.rewardXP = 80;
    q2.rewardEquip = EDB::coleteMilitar();
    quests.push_back(q2);

    Quest q3("q_executor", "O Executor",
             "Destrua the IRON-VIII Boss of the Forge KRONOS", "COMANDANTE LYRA", QuestType::KillBoss, 1);
    q3.rewardHP = 100.0f; q3.rewardXP = 300;
    q3.rewardEquip = EDB::rifleEnergia();
    quests.push_back(q3);

    Quest q4("q_morphx", "Metal Liquido",
             "Destrua 3 MORPH-X (metal liquido)", "DR. CHEN", QuestType::Kill, 3);
    q4.rewardHP = 0.0f; q4.rewardXP = 200;
    q4.rewardEquip = EDB::armaduraAvan();
    quests.push_back(q4);

    Quest q5("q_drones", "Caca-Drones",
             "Abata 5 Hunter Drones of the KRONOS", "MARCO VEIL", QuestType::Kill, 5);
    q5.rewardHP = 50.0f; q5.rewardXP = 250;
    q5.rewardEquip = EDB::neuralLink();
    quests.push_back(q5);

    // ── 20 Novas Quests ──────────────────────────────────────────────────────

    { Quest q("q_zergling",  "Praga Alienigena",
              "Elimine 15 Zerglings — eles if multiplicam fast!", "DR. CHEN",
              QuestType::Kill, 15);
      q.rewardXP = 400; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_hydra",     "Acid in the Veia",
              "Destrua 8 Hydras — cuidado with the projectiles acidos", "COMANDANTE LYRA",
              QuestType::Kill, 8);
      q.rewardXP = 500; q.rewardHP = 80.0f; quests.push_back(q); }

    { Quest q("q_brood",     "Mae of the Monstros",
              "Mate the Broodmother before that invoque more zerglings", "DR. CHEN",
              QuestType::KillBoss, 1);
      q.rewardXP = 700; q.rewardHP = 100.0f; quests.push_back(q); }

    { Quest q("q_fantasmas", "Assombracoes",
              "Elimine 10 fantasmas in the zones sombrias", "VANCE RIOS",
              QuestType::Kill, 10);
      q.rewardXP = 600; q.rewardHP = 70.0f; quests.push_back(q); }

    { Quest q("q_zumbis",    "Apocalipse Zombie",
              "Elimine 20 zumbis — eles are if espalhando", "MARCO VEIL",
              QuestType::Kill, 20);
      q.rewardXP = 500; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_portais",   "Close the Rifts",
              "Feche 3 portals of anomaly before that more enemies entrem", "VANCE RIOS",
              QuestType::ClosePortal, 3);
      q.rewardXP = 800; q.rewardHP = 120.0f; quests.push_back(q); }

    { Quest q("q_orc",       "Orc Cybernetic",
              "Derrote 5 Orcs Ciberneticos — implantes KRONOS the fortaleceram", "DR. CHEN",
              QuestType::Kill, 5);
      q.rewardXP = 450; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_paladin",   "Paladin Corrompido",
              "Destrua 5 Paladins Corrompidos pelos nanobots KRONOS", "COMANDANTE LYRA",
              QuestType::Kill, 5);
      q.rewardXP = 450; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_omega_boss","Encontro with OMEGA",
              "Sobreviva and destrua the OmegaBoss — ele aparece apos 50 kills", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 2000; q.rewardHP = 200.0f; quests.push_back(q); }

    { Quest q("q_cemiterio", "Zone of the Cemetery",
              "Explore and sobreviva to the Cemetery Abandonado", "COMANDANTE LYRA",
              QuestType::Zone, 4);
      q.rewardXP = 300; q.rewardHP = 40.0f; quests.push_back(q); }

    { Quest q("q_inferno",   "Descida to the Inferno",
              "Range and sobreviva in the Zone Inferno", "DR. CHEN",
              QuestType::Zone, 10);
      q.rewardXP = 1000; q.rewardHP = 150.0f; quests.push_back(q); }

    { Quest q("q_loot",      "Colecionador",
              "Colete 10 items during your quest", "MARCO VEIL",
              QuestType::Collect, 10);
      q.rewardXP = 400; q.rewardHP = 0.0f; quests.push_back(q); }

    { Quest q("q_kamikaze",  "Bombas Vivas",
              "Derrote 8 Kamikazes before that explodam near of you", "DR. CHEN",
              QuestType::Kill, 8);
      q.rewardXP = 350; q.rewardHP = 40.0f; quests.push_back(q); }

    { Quest q("q_sniper",    "Atiradores of Elite",
              "Elimine 6 Snipers enemies — eles atiram of far", "MARCO VEIL",
              QuestType::Kill, 6);
      q.rewardXP = 400; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_alien_boss","Boss Alienigena",
              "Enfrente and destrua the AlienBoss — ameaca maxima", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 1500; q.rewardHP = 180.0f; quests.push_back(q); }

    { Quest q("q_sombra",    "Wraith of the Sombras",
              "Elimine 8 Shadow Wraiths — rapidos and letais", "COMANDANTE LYRA",
              QuestType::Kill, 8);
      q.rewardXP = 600; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_banshee",   "Grito of the Banshee",
              "Destrua 5 Banshee Howlers before that paralisem your equipe", "DR. CHEN",
              QuestType::Kill, 5);
      q.rewardXP = 500; q.rewardHP = 55.0f; quests.push_back(q); }

    { Quest q("q_zombie_lord","Senhor of the Zumbis",
              "Venca the ZombieLord — ele invoca hordas interminaveis", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 1800; q.rewardHP = 200.0f; quests.push_back(q); }

    { Quest q("q_coleta_raro","Tesouros Raros",
              "Colete 5 items Raros ou superiores", "MARCO VEIL",
              QuestType::CollectRare, 5);
      q.rewardXP = 700; q.rewardHP = 0.0f; quests.push_back(q); }

    { Quest q("q_nocturna",  "Quest Noturna",
              "Elimine 30 enemies in the zones sombrias", "COMANDANTE LYRA",
              QuestType::Kill, 30);
      q.rewardXP = 900; q.rewardHP = 100.0f; quests.push_back(q); }
}

void Game::buildNPCs() {
    npcs.clear();

    float cx = static_cast<float>(tilemap.width  * Tilemap::tileSize) / 2.0f;
    float cy = static_cast<float>(tilemap.height * Tilemap::tileSize) / 2.0f;

    // VANCE RIOS - lider of the NEXUS
    npcs.emplace_back(
        Vector2{cx - 120, cy - 80}, "VANCE RIOS", NPCRole::Leader,
        std::vector<std::string>{
            "KRONOS sent maquinas to in the destroy. Lute!",
            "Elimine the patrulhas KRONOS to release the area.",
            "O futuro of the humanidade is in the your hands."
        }, "q_limpar");

    // MARCO VEIL
    npcs.emplace_back(
        Vector2{cx + 150, cy - 60}, "MARCO VEIL", NPCRole::Soldier,
        std::vector<std::string>{
            "Especialista in explosivos of the NEXUS.",
            "Colete the Energy Cores - precisamos of energy.",
            "Cuidado with the Hunter Drones. Eles in the rastreiam.",
            "Destrua-the before that reportem nossa position!"
        }, "q_suprimentos");

    // COMANDANTE LYRA
    npcs.emplace_back(
        Vector2{cx - 60, cy + 130}, "COMANDANTE LYRA", NPCRole::Soldier,
        std::vector<std::string>{
            "O IRON-VIII not has emocoes, not to, not negocia.",
            "You precisa destroy the Boss of the Forge KRONOS.",
            "Use all the your arsenal. Not hesite."
        }, "q_executor");

    // DR. CHEN - cientista of the implantes
    npcs.emplace_back(
        Vector2{cx + 80, cy + 100}, "DR. CHEN", NPCRole::Scientist,
        std::vector<std::string>{
            "Desenvolvi your implantes of combat, VANCE.",
            "O MORPH-X and minha maior preocupacao. Metal liquido.",
            "Your only fraqueza: temperatura extrema and strength bruta.",
            "Attack without stop - ele regenera HP rapidamente!"
        }, "q_morphx");
}

// NPCs of servico ancorados in the SAFE ZONE — lider (quests), armorer, blacksmith,
// merchant and cientista. O player equipa, purchase and gets quests in the base.
void Game::setupBaseNPCs() {
    float bx = safeZoneCenter.x, by = safeZoneCenter.y;

    // VANCE RIOS — fundador of the stamina NEXUS
    npcs.emplace_back(
        Vector2{bx - 120, by - 90}, "VANCE RIOS", NPCRole::Leader,
        std::vector<std::string>{
            "Eu sou VANCE RIOS, fundador of the NEXUS — the last stamina humana.",
            "When the KRONOS despertou, perdi minha city and minha familia numa night.",
            "Reuni the sobreviventes nesta base. E tudo that resta of in the.",
            "Meu objective and simple: destroy the core of the KRONOS and libertar the humanidade.",
            "Meu desejo? See the sol nascer without maquinas patrulhando the sky.",
            "Eu te ajudo with MISSOES and estrategia. Cumpra-the and ficamos more fortes.",
            "Comece limpando the patrulhas la outside. Confio in you, soldier."
        }, "q_limpar");

    // Zara — engenheira of weapons
    npcs.emplace_back(
        Vector2{bx + 150, by - 70}, "Zara", NPCRole::WeaponDealer,
        std::vector<std::string>{
            "Me chamo Zara. Eu projetava weapons for the army... before the collapse.",
            "Escapei of the first purga of the KRONOS with uma caixa of ferramentas and raiva.",
            "Today forjo and seeing weapons here in the base to quem luta of verdade.",
            "Meu objective and armar the stamina until the dentes.",
            "Desejo vingar cada pessoa that those maquinas tiraram of mim.",
            "Eu te ajudo te dando POWER DE FIRE. Aperte [TAB] and veja meu arsenal."
        }, "");

    // BLACKSMITH KANE — armorer/blacksmith
    npcs.emplace_back(
        Vector2{bx + 180, by + 100}, "BLACKSMITH KANE", NPCRole::ArmorSmith,
        std::vector<std::string>{
            "KANE. Fui blacksmith militar in the trincheiras before the KRONOS dominar tudo.",
            "Cheguei here loading minha bigorna in the costas by 200 km.",
            "Forjo blindagem and implantes — the that te mantem integer la outside.",
            "Meu objective and that none soldier of the NEXUS caia by falta of protecao.",
            "Desejo, same, and forjar the armor that goes derrubar the core KRONOS.",
            "Eu te ajudo with DEFENSE. Traga materiais and [TAB] to see minhas forjas."
        }, "");

    // LUNA — mercadora/sucateira
    npcs.emplace_back(
        Vector2{bx - 180, by + 110}, "LUNA", NPCRole::Merchant,
        std::vector<std::string>{
            "Oi. Sou the LUNA. Vasculho the ruins behind of qualquer coisa useful.",
            "Sobrevivi sozinha 3 anos in the cities mortas before find the NEXUS.",
            "Troco suprimentos: potions, kits, energy — the that te mantem vivo.",
            "Meu objective and that ninguem here morra by falta of um remedio.",
            "Meu desejo and um lugar where eu not precise more catar lixo to viver.",
            "Eu te ajudo with CONSUMIVEIS. Aperte [TAB] and reabasteca before leave."
        }, "");

    // DR. CHEN — cientista of the implantes
    npcs.emplace_back(
        Vector2{bx - 30, by + 150}, "DR. CHEN", NPCRole::Scientist,
        std::vector<std::string>{
            "Dr. Chen. Fui eu quem projetou the implantes of combat in the your body.",
            "Trabalhei for the KRONOS before entender the that ele planejava. Desertei.",
            "Trouxe comigo the tecnologia that today te torna more that humano.",
            "Meu objective and evoluir you until power enfrentar the core of igual to igual.",
            "Meu desejo and reparar the error of ter ajudado the create that IA.",
            "Eu te ajudo with EVOLUTION: suba of level and use [K] to evoluir, [L] p/ points."
        }, "q_morphx");
}

void Game::setupZoneNPCs(ZoneID zone) {
    npcs.clear();
    float cx = static_cast<float>(tilemap.width  * Tilemap::tileSize) / 2.0f;
    float cy = static_cast<float>(tilemap.height * Tilemap::tileSize) / 2.0f;

    switch (zone) {
        case ZoneID::LARuins:
            // Region inicial = base. Em world open, NPCs ficam in the SAFE ZONE.
            if (openWorldMode) {
                setupBaseNPCs();
            } else {
                buildNPCs();
                npcs.emplace_back(
                    Vector2{cx + 200, cy + 160}, "Zara", NPCRole::WeaponDealer,
                    std::vector<std::string>{
                        "Tenho weapons of the stamina. [E] Open shop",
                        "Compre soon — KRONOS ta chegando.",
                        "Melhores weapons of the NEXUS!"
                    }, "");
            }
            break;
        case ZoneID::Bunker:
            npcs.emplace_back(
                Vector2{cx, cy - 100}, "VANCE RIOS", NPCRole::Leader,
                std::vector<std::string>{
                    "Bem-coming to the Bunker NEXUS!",
                    "Aqui you can recuperar forces.",
                    "A Forge KRONOS stays to the leste. Seja cuidadoso."
                }, "");
            npcs.emplace_back(
                Vector2{cx - 150, cy}, "MARCO VEIL", NPCRole::Soldier,
                std::vector<std::string>{
                    "Abata the drones that patrulham the perimetro.",
                    "Cinco Hunter Drones destruidos and nossa route stays livre."
                }, "q_drones");
            // Armeiro in the bunker — WeaponDealer
            npcs.emplace_back(
                Vector2{cx + 180, cy + 80}, "Rex", NPCRole::WeaponDealer,
                std::vector<std::string>{
                    "Tenho items to sell. [E] Open shop",
                    "Armas and implantes of ponta. Price justo.",
                    "Cada credit importa contra KRONOS."
                }, "");
            // Mercador of suprimentos militares
            npcs.emplace_back(
                Vector2{cx - 200, cy + 120}, "LUNA", NPCRole::Merchant,
                std::vector<std::string>{
                    "Suprimentos militares disponiveis. [E] Open shop",
                    "Tudo that sobrou of the bases NEXUS destruidas.",
                    "Pague and sobreviva — nessa ordem."
                }, "");
            break;
        case ZoneID::KronosForge:
            npcs.emplace_back(
                Vector2{cx, cy + 120}, "COMANDANTE LYRA", NPCRole::Soldier,
                std::vector<std::string>{
                    "Is forge produz IRON-VIII the cada hour.",
                    "Destrua the IRON-VIII Boss in the center of the forge!",
                    "O implant neural ajuda the esquivar of the projetos."
                }, "q_executor");
            npcs.emplace_back(
                Vector2{cx + 130, cy - 80}, "DR. CHEN", NPCRole::Scientist,
                std::vector<std::string>{
                    "Is and minha chance of fix the that KRONOS corrompeu.",
                    "Os MORPH-X patrulham the andares superiores.",
                    "Use the Barrier of Shield contra the MORPH-X!"
                }, "q_morphx");
            // Ferreiro of armaduras in the forge
            npcs.emplace_back(
                Vector2{cx - 160, cy + 50}, "KOBA-7", NPCRole::ArmorSmith,
                std::vector<std::string>{
                    "Tenho items to sell. [E] Open shop",
                    "Armaduras forjadas with metal KRONOS capturado.",
                    "So the more fortes sobrevivem here inside."
                }, "");
            break;
        case ZoneID::KronosNexus:
            npcs.emplace_back(
                Vector2{cx, cy - 150}, "VANCE RIOS", NPCRole::Leader,
                std::vector<std::string>{
                    "Este and the coracao of KRONOS. Tudo ends here.",
                    "Destrua the Core Principal to libertar the humanidade!",
                    "Strength, soldier. O futuro and nosso."
                }, "");
            // Last vendor before the final
            npcs.emplace_back(
                Vector2{cx + 140, cy - 180}, "Nyx", NPCRole::Merchant,
                std::vector<std::string>{
                    "Tenho items to sell. [E] Open shop",
                    "Ultima chance before the core. Equipese well.",
                    "Se you falhar, all morremos. Sem desconto."
                }, "");
            break;
    }
}

// ─── Open World ──────────────────────────────────────────────────────────────

void Game::setupWorldRegions() {
    worldRegions.clear();
    // Each region = OW_ZONE_W * tileSize pixels wide/tall
    float sz = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize); // 2560

    // ── World open big the suficiente for the PHASE INTEIRA ─────────────────
    // A phase and um disco of owPhaseRadius in returns of the refuge. As regions sao
    // uma GRID quadrada (impar by lado) CENTRADA in the refuge that cobre the disco
    // integer. Antes eram 3x3 ancoradas in the origem (0,0) with the hub in the canto:
    // metade of uma phase big ficava without scenario (regions of 2560 tinham the
    // hub at (1280,1280) and the square all comecava in the origem).
    float side = (owPhaseRadius + 340.0f) * 2.0f;   // square that contem the disco
    int   cols = (int)ceilf(side / sz);
    if (cols < 3) cols = 3;
    if ((cols & 1) == 0) cols += 1;                  // impar p/ ter celula central
    float regW  = side / (float)cols;
    float x0 = safeZoneCenter.x - side * 0.5f;
    float y0 = safeZoneCenter.y - side * 0.5f;
    int   cc  = cols / 2;                            // column/line central (hub)

    auto add = [&](int col, int row, ZoneID z, const char* n, Color c) {
        WorldRegion r;
        r.bounds     = {x0 + col * regW, y0 + row * regW, regW, regW};
        r.zoneType   = z;
        r.name       = n;
        r.discovered = false;
        r.mapColor   = c;
        worldRegions.push_back(r);
    };

    // Uma PHASE = um world integer: TODAS the regions assumem the biome of the phase
    // (currentZone), not uma grade 3x3 fixa with LARuins in the hub.
    ZoneID hz = currentZone;

    // Nomes by SETOR (bussola) in relacao to the refuge; the center and the "Refuge".
    auto sectName = [&](int dr, int dc) -> std::string {
        std::string s;
        if (dr < 0) s += "Norte";
        else if (dr > 0) s += "Sul";
        if (dc < 0) s += (s.empty() ? "Oeste" : "-Oeste");
        else if (dc > 0) s += (s.empty() ? "Leste" : "-Leste");
        return s;
    };
    static const Color SEC_COLORS[5] = {
        {120,170,90,255}, {90,120,170,255}, {170,120,90,255},
        {90,170,150,255}, {150,90,170,255},
    };
    for (int row = 0; row < cols; ++row) {
        for (int col = 0; col < cols; ++col) {
            int dr = row - cc, dc = col - cc;
            Color c = SEC_COLORS[((dr + 2) * 5 + (dc + 2)) % 5];
            if (dr == 0 && dc == 0) {
                add(col, row, hz, "Refuge", c);
                continue;
            }
            std::string nm = sectName(dr, dc);
            std::string full = nm.empty() ? "Setor Central" : ("Setor " + nm);
            add(col, row, hz, full.c_str(), c);
        }
    }

    // O HUB (region that contem the base/safeZoneCenter) acompanha the phase — and the nome
    // that aparece in the map — and nasce DESCOBERTO. A grid and row-major of the canto
    // upper-esquerdo, entao worldRegions[0] NOT and the hub: procuro pela region
    // that contem the center of the base.
    {
        const PhaseDef& pd = phaseDef(owPhase);
        for (auto& r : worldRegions) {
            float cx = safeZoneCenter.x, cy = safeZoneCenter.y;
            if (cx >= r.bounds.x && cx < r.bounds.x + r.bounds.width &&
                cy >= r.bounds.y && cy < r.bounds.y + r.bounds.height) {
                if (!pd.title.empty()) r.name = pd.title;
                r.discovered = true;
                break;
            }
        }
    }
}

ZoneID Game::getRegionAt(Vector2 pos) const {
    // Biome by POSICAO — MESMA regra (periodo 2560 and owLayout 3x3 by module) that
    // Tilemap::render3D usa, to that INIMIGOS, AUDIO and ZONE concordem with the CHAO
    // in TODO the world infinito (not only in the area fixa central).
    (void)pos;
    // Uma PHASE = um world integer. Swap of world and pelo PORTAL (see
    // advanceOpenWorldPhase), never by atravessar uma line invisible in the floor.
    return currentZone;
}
