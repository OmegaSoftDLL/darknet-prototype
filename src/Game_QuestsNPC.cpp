// Game_QuestsNPC.cpp — setup de quests e NPCs (base, zonas e regioes do mundo).
// Extraido de Game.cpp. Mesma classe Game.
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

    Quest q1("q_limpar", "Limpeza de Ruinas",
             "Elimine 5 patrulhas KRONOS", "VANCE RIOS", QuestType::Kill, 5);
    q1.rewardHP = 50.0f; q1.rewardXP = 100;
    q1.rewardEquip = EDB::pistolaPlas();
    quests.push_back(q1);

    Quest q2("q_suprimentos", "Suprimentos Criticos",
             "Colete 3 Energy Cores para o NEXUS", "MARCO VEIL", QuestType::Collect, 3);
    q2.rewardHP = 30.0f; q2.rewardXP = 80;
    q2.rewardEquip = EDB::coleteMilitar();
    quests.push_back(q2);

    Quest q3("q_executor", "O Executor",
             "Destrua o IRON-VIII Boss da Forja KRONOS", "COMANDANTE LYRA", QuestType::KillBoss, 1);
    q3.rewardHP = 100.0f; q3.rewardXP = 300;
    q3.rewardEquip = EDB::rifleEnergia();
    quests.push_back(q3);

    Quest q4("q_morphx", "Metal Liquido",
             "Destrua 3 MORPH-X (metal liquido)", "DR. CHEN", QuestType::Kill, 3);
    q4.rewardHP = 0.0f; q4.rewardXP = 200;
    q4.rewardEquip = EDB::armaduraAvan();
    quests.push_back(q4);

    Quest q5("q_drones", "Caca-Drones",
             "Abata 5 Hunter Drones do KRONOS", "MARCO VEIL", QuestType::Kill, 5);
    q5.rewardHP = 50.0f; q5.rewardXP = 250;
    q5.rewardEquip = EDB::neuralLink();
    quests.push_back(q5);

    // ── 20 Novas Quests ──────────────────────────────────────────────────────

    { Quest q("q_zergling",  "Praga Alienigena",
              "Elimine 15 Zerglings — eles se multiplicam rapido!", "DR. CHEN",
              QuestType::Kill, 15);
      q.rewardXP = 400; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_hydra",     "Acido na Veia",
              "Destrua 8 Hydras — cuidado com os projéteis acidos", "COMANDANTE LYRA",
              QuestType::Kill, 8);
      q.rewardXP = 500; q.rewardHP = 80.0f; quests.push_back(q); }

    { Quest q("q_brood",     "Mae dos Monstros",
              "Mate a Broodmother antes que invoque mais zerglings", "DR. CHEN",
              QuestType::KillBoss, 1);
      q.rewardXP = 700; q.rewardHP = 100.0f; quests.push_back(q); }

    { Quest q("q_fantasmas", "Assombracoes",
              "Elimine 10 fantasmas nas zonas sombrias", "VANCE RIOS",
              QuestType::Kill, 10);
      q.rewardXP = 600; q.rewardHP = 70.0f; quests.push_back(q); }

    { Quest q("q_zumbis",    "Apocalipse Zumbi",
              "Elimine 20 zumbis — eles estao se espalhando", "MARCO VEIL",
              QuestType::Kill, 20);
      q.rewardXP = 500; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_portais",   "Fechar os Rifts",
              "Feche 3 portais de anomalia antes que mais inimigos entrem", "VANCE RIOS",
              QuestType::Kill, 3);
      q.rewardXP = 800; q.rewardHP = 120.0f; quests.push_back(q); }

    { Quest q("q_orc",       "Orc Cibernetico",
              "Derrote 5 Orcs Ciberneticos — implantes KRONOS os fortaleceram", "DR. CHEN",
              QuestType::Kill, 5);
      q.rewardXP = 450; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_paladin",   "Paladin Corrompido",
              "Destrua 5 Paladins Corrompidos pelos nanobots KRONOS", "COMANDANTE LYRA",
              QuestType::Kill, 5);
      q.rewardXP = 450; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_omega_boss","Encontro com OMEGA",
              "Sobreviva e destrua o OmegaBoss — ele aparece apos 50 kills", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 2000; q.rewardHP = 200.0f; quests.push_back(q); }

    { Quest q("q_cemiterio", "Zona do Cemiterio",
              "Explore e sobreviva ao Cemiterio Abandonado", "COMANDANTE LYRA",
              QuestType::Zone, 4);
      q.rewardXP = 300; q.rewardHP = 40.0f; quests.push_back(q); }

    { Quest q("q_inferno",   "Descida ao Inferno",
              "Alcance e sobreviva na Zona Inferno", "DR. CHEN",
              QuestType::Zone, 10);
      q.rewardXP = 1000; q.rewardHP = 150.0f; quests.push_back(q); }

    { Quest q("q_loot",      "Colecionador",
              "Colete 10 itens durante sua missao", "MARCO VEIL",
              QuestType::Collect, 10);
      q.rewardXP = 400; q.rewardHP = 0.0f; quests.push_back(q); }

    { Quest q("q_kamikaze",  "Bombas Vivas",
              "Derrote 8 Kamikazes antes que explodam perto de voce", "DR. CHEN",
              QuestType::Kill, 8);
      q.rewardXP = 350; q.rewardHP = 40.0f; quests.push_back(q); }

    { Quest q("q_sniper",    "Atiradores de Elite",
              "Elimine 6 Snipers inimigos — eles atiram de longe", "MARCO VEIL",
              QuestType::Kill, 6);
      q.rewardXP = 400; q.rewardHP = 50.0f; quests.push_back(q); }

    { Quest q("q_alien_boss","Chefe Alienigena",
              "Enfrente e destrua o AlienBoss — ameaca maxima", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 1500; q.rewardHP = 180.0f; quests.push_back(q); }

    { Quest q("q_sombra",    "Wraith das Sombras",
              "Elimine 8 Shadow Wraiths — rapidos e letais", "COMANDANTE LYRA",
              QuestType::Kill, 8);
      q.rewardXP = 600; q.rewardHP = 60.0f; quests.push_back(q); }

    { Quest q("q_banshee",   "Grito da Banshee",
              "Destrua 5 Banshee Howlers antes que paralisem sua equipe", "DR. CHEN",
              QuestType::Kill, 5);
      q.rewardXP = 500; q.rewardHP = 55.0f; quests.push_back(q); }

    { Quest q("q_zombie_lord","Senhor dos Zumbis",
              "Venca o ZombieLord — ele invoca hordas interminaveis", "VANCE RIOS",
              QuestType::KillBoss, 1);
      q.rewardXP = 1800; q.rewardHP = 200.0f; quests.push_back(q); }

    { Quest q("q_coleta_raro","Tesouros Raros",
              "Colete 5 itens Raros ou superiores", "MARCO VEIL",
              QuestType::Collect, 5);
      q.rewardXP = 700; q.rewardHP = 0.0f; quests.push_back(q); }

    { Quest q("q_nocturna",  "Missao Noturna",
              "Elimine 30 inimigos nas zonas sombrias", "COMANDANTE LYRA",
              QuestType::Kill, 30);
      q.rewardXP = 900; q.rewardHP = 100.0f; quests.push_back(q); }
}

void Game::buildNPCs() {
    npcs.clear();

    float cx = static_cast<float>(tilemap.width  * Tilemap::tileSize) / 2.0f;
    float cy = static_cast<float>(tilemap.height * Tilemap::tileSize) / 2.0f;

    // VANCE RIOS - lider do NEXUS
    npcs.emplace_back(
        Vector2{cx - 120, cy - 80}, "VANCE RIOS", NPCRole::Leader,
        std::vector<std::string>{
            "KRONOS enviou maquinas para nos destruir. Lute!",
            "Elimine as patrulhas KRONOS para liberar a area.",
            "O futuro da humanidade esta nas suas maos."
        }, "q_limpar");

    // MARCO VEIL
    npcs.emplace_back(
        Vector2{cx + 150, cy - 60}, "MARCO VEIL", NPCRole::Soldier,
        std::vector<std::string>{
            "Especialista em explosivos do NEXUS.",
            "Colete os Energy Cores - precisamos de energia.",
            "Cuidado com os Hunter Drones. Eles nos rastreiam.",
            "Destrua-os antes que reportem nossa posicao!"
        }, "q_suprimentos");

    // COMANDANTE LYRA
    npcs.emplace_back(
        Vector2{cx - 60, cy + 130}, "COMANDANTE LYRA", NPCRole::Soldier,
        std::vector<std::string>{
            "O IRON-VIII nao tem emocoes, nao para, nao negocia.",
            "Voce precisa destruir o Boss da Forja KRONOS.",
            "Use todo o seu arsenal. Nao hesite."
        }, "q_executor");

    // DR. CHEN - cientista dos implantes
    npcs.emplace_back(
        Vector2{cx + 80, cy + 100}, "DR. CHEN", NPCRole::Scientist,
        std::vector<std::string>{
            "Desenvolvi seus implantes de combate, VANCE.",
            "O MORPH-X e minha maior preocupacao. Metal liquido.",
            "Sua unica fraqueza: temperatura extrema e forca bruta.",
            "Ataque sem parar - ele regenera HP rapidamente!"
        }, "q_morphx");
}

// NPCs de servico ancorados na ZONA SEGURA — lider (quests), armeiro, ferreiro,
// mercador e cientista. O jogador equipa, compra e pega missoes na base.
void Game::setupBaseNPCs() {
    float bx = safeZoneCenter.x, by = safeZoneCenter.y;

    // VANCE RIOS — fundador da resistencia NEXUS
    npcs.emplace_back(
        Vector2{bx - 120, by - 90}, "VANCE RIOS", NPCRole::Leader,
        std::vector<std::string>{
            "Eu sou VANCE RIOS, fundador do NEXUS — a ultima resistencia humana.",
            "Quando o KRONOS despertou, perdi minha cidade e minha familia numa noite.",
            "Reuni os sobreviventes nesta base. E tudo que resta de nos.",
            "Meu objetivo e simples: destruir o nucleo do KRONOS e libertar a humanidade.",
            "Meu desejo? Ver o sol nascer sem maquinas patrulhando o ceu.",
            "Eu te ajudo com MISSOES e estrategia. Cumpra-as e ficamos mais fortes.",
            "Comece limpando as patrulhas la fora. Confio em voce, soldado."
        }, "q_limpar");

    // Zara — engenheira de armas
    npcs.emplace_back(
        Vector2{bx + 150, by - 70}, "Zara", NPCRole::WeaponDealer,
        std::vector<std::string>{
            "Me chamo Zara. Eu projetava armas para o exercito... antes do colapso.",
            "Escapei da primeira purga do KRONOS com uma caixa de ferramentas e raiva.",
            "Hoje forjo e vendo armas aqui na base para quem luta de verdade.",
            "Meu objetivo e armar a resistencia ate os dentes.",
            "Desejo vingar cada pessoa que aquelas maquinas tiraram de mim.",
            "Eu te ajudo te dando PODER DE FOGO. Aperte [TAB] e veja meu arsenal."
        }, "");

    // FERREIRO KANE — armeiro/ferreiro
    npcs.emplace_back(
        Vector2{bx + 180, by + 100}, "FERREIRO KANE", NPCRole::ArmorSmith,
        std::vector<std::string>{
            "KANE. Fui ferreiro militar nas trincheiras antes do KRONOS dominar tudo.",
            "Cheguei aqui carregando minha bigorna nas costas por 200 km.",
            "Forjo blindagem e implantes — o que te mantem inteiro la fora.",
            "Meu objetivo e que nenhum soldado do NEXUS caia por falta de protecao.",
            "Desejo, mesmo, e forjar a armadura que vai derrubar o nucleo KRONOS.",
            "Eu te ajudo com DEFESA. Traga materiais e [TAB] para ver minhas forjas."
        }, "");

    // LUNA — mercadora/sucateira
    npcs.emplace_back(
        Vector2{bx - 180, by + 110}, "LUNA", NPCRole::Merchant,
        std::vector<std::string>{
            "Oi. Sou a LUNA. Vasculho as ruinas atras de qualquer coisa util.",
            "Sobrevivi sozinha 3 anos nas cidades mortas antes de achar o NEXUS.",
            "Troco suprimentos: pocoes, kits, energia — o que te mantem vivo.",
            "Meu objetivo e que ninguem aqui morra por falta de um remedio.",
            "Meu desejo e um lugar onde eu nao precise mais catar lixo pra viver.",
            "Eu te ajudo com CONSUMIVEIS. Aperte [TAB] e reabasteca antes de sair."
        }, "");

    // DR. CHEN — cientista dos implantes
    npcs.emplace_back(
        Vector2{bx - 30, by + 150}, "DR. CHEN", NPCRole::Scientist,
        std::vector<std::string>{
            "Dr. Chen. Fui eu quem projetou os implantes de combate no seu corpo.",
            "Trabalhei para o KRONOS antes de entender o que ele planejava. Desertei.",
            "Trouxe comigo a tecnologia que hoje te torna mais que humano.",
            "Meu objetivo e evoluir voce ate poder enfrentar o nucleo de igual pra igual.",
            "Meu desejo e reparar o erro de ter ajudado a criar aquela IA.",
            "Eu te ajudo com EVOLUCAO: suba de nivel e use [K] para evoluir, [L] p/ pontos."
        }, "q_morphx");
}

void Game::setupZoneNPCs(ZoneID zone) {
    npcs.clear();
    float cx = static_cast<float>(tilemap.width  * Tilemap::tileSize) / 2.0f;
    float cy = static_cast<float>(tilemap.height * Tilemap::tileSize) / 2.0f;

    switch (zone) {
        case ZoneID::LARuins:
            // Regiao inicial = base. Em mundo aberto, NPCs ficam na ZONA SEGURA.
            if (openWorldMode) {
                setupBaseNPCs();
            } else {
                buildNPCs();
                npcs.emplace_back(
                    Vector2{cx + 200, cy + 160}, "Zara", NPCRole::WeaponDealer,
                    std::vector<std::string>{
                        "Tenho armas da resistencia. [E] Abrir loja",
                        "Compre logo — KRONOS ta chegando.",
                        "Melhores armas do NEXUS!"
                    }, "");
            }
            break;
        case ZoneID::Bunker:
            npcs.emplace_back(
                Vector2{cx, cy - 100}, "VANCE RIOS", NPCRole::Leader,
                std::vector<std::string>{
                    "Bem-vindo ao Bunker NEXUS!",
                    "Aqui voce pode recuperar forcas.",
                    "A Forja KRONOS fica ao leste. Seja cuidadoso."
                }, "");
            npcs.emplace_back(
                Vector2{cx - 150, cy}, "MARCO VEIL", NPCRole::Soldier,
                std::vector<std::string>{
                    "Abata os drones que patrulham o perimetro.",
                    "Cinco Hunter Drones destruidos e nossa rota fica livre."
                }, "q_drones");
            // Armeiro no bunker — WeaponDealer
            npcs.emplace_back(
                Vector2{cx + 180, cy + 80}, "Rex", NPCRole::WeaponDealer,
                std::vector<std::string>{
                    "Tenho itens para vender. [E] Abrir loja",
                    "Armas e implantes de ponta. Preco justo.",
                    "Cada credito importa contra KRONOS."
                }, "");
            // Mercador de suprimentos militares
            npcs.emplace_back(
                Vector2{cx - 200, cy + 120}, "LUNA", NPCRole::Merchant,
                std::vector<std::string>{
                    "Suprimentos militares disponíveis. [E] Abrir loja",
                    "Tudo que sobrou das bases NEXUS destruidas.",
                    "Pague e sobreviva — nessa ordem."
                }, "");
            break;
        case ZoneID::KronosForge:
            npcs.emplace_back(
                Vector2{cx, cy + 120}, "COMANDANTE LYRA", NPCRole::Soldier,
                std::vector<std::string>{
                    "Esta forja produz IRON-VIII a cada hora.",
                    "Destrua o IRON-VIII Boss no centro da forja!",
                    "O implante neural ajuda a esquivar dos projetos."
                }, "q_executor");
            npcs.emplace_back(
                Vector2{cx + 130, cy - 80}, "DR. CHEN", NPCRole::Scientist,
                std::vector<std::string>{
                    "Esta e minha chance de corrigir o que KRONOS corrompeu.",
                    "Os MORPH-X patrulham os andares superiores.",
                    "Use a Barreira de Escudo contra o MORPH-X!"
                }, "q_morphx");
            // Ferreiro de armaduras na forja
            npcs.emplace_back(
                Vector2{cx - 160, cy + 50}, "KOBA-7", NPCRole::ArmorSmith,
                std::vector<std::string>{
                    "Tenho itens para vender. [E] Abrir loja",
                    "Armaduras forjadas com metal KRONOS capturado.",
                    "So os mais fortes sobrevivem aqui dentro."
                }, "");
            break;
        case ZoneID::KronosNexus:
            npcs.emplace_back(
                Vector2{cx, cy - 150}, "VANCE RIOS", NPCRole::Leader,
                std::vector<std::string>{
                    "Este e o coracao de KRONOS. Tudo termina aqui.",
                    "Destrua o Nucleo Principal para libertar a humanidade!",
                    "Forca, soldado. O futuro e nosso."
                }, "");
            // Ultimo vendedor antes do final
            npcs.emplace_back(
                Vector2{cx + 140, cy - 180}, "Nyx", NPCRole::Merchant,
                std::vector<std::string>{
                    "Tenho itens para vender. [E] Abrir loja",
                    "Ultima chance antes do nucleo. Equipese bem.",
                    "Se voce falhar, todos morremos. Sem desconto."
                }, "");
            break;
    }
}

// ─── Open World ──────────────────────────────────────────────────────────────

void Game::setupWorldRegions() {
    worldRegions.clear();
    // Each region = OW_ZONE_W * tileSize pixels wide/tall
    float sz = (float)(Tilemap::OW_ZONE_W * Tilemap::tileSize); // 2560

    auto add = [&](int col, int row, ZoneID z, const char* n, Color c) {
        WorldRegion r;
        r.bounds     = {col * sz, row * sz, sz, sz};
        r.zoneType   = z;
        r.name       = n;
        r.discovered = false;
        r.mapColor   = c;
        worldRegions.push_back(r);
    };

    // Uma FASE = um mundo inteiro: TODAS as regioes assumem o bioma da fase
    // (currentZone), nao uma grade 3x3 fixa com LARuins no hub. Antes o refugio
    // nascia com grade de predios modernos (structuresFor(LARuins)) mesmo na
    // fase do Cemiterio — cidade de LA sobre chao de cemiterio (auditoria P2).
    // O catalogo de estruturas ja e keyado por zoneType, entao a regiao seguindo
    // a fase faz o cenario inteiro acompanhar o piso (biomeAtWorld = fase).
    // As POSICOES 3x3 continuam casando com Tilemap::generateOpenWorld().
    ZoneID hz = currentZone;
    add(0, 0, hz, "Refugio",             {80,120,80,255});
    add(1, 0, hz, "Setor Leste",         {60,80,120,255});
    add(2, 0, hz, "Setor Extremo Leste", {20,50,20,255});
    add(0, 1, hz, "Setor Sul",           {100,80,40,255});
    add(1, 1, hz, "Setor Sudeste",       {60,60,80,255});
    add(2, 1, hz, "Setor Leste 2",       {40,60,80,255});
    add(0, 2, hz, "Setor Extremo Sul",   {120,40,20,255});
    add(1, 2, hz, "Setor Sul 2",         {80,40,80,255});
    add(2, 2, hz, "Confins",             {80,0,120,255});

    // O nome do HUB acompanha a fase (e o nome que aparece no mapa).
    {
        const PhaseDef& pd = phaseDef(owPhase);
        if (!pd.title.empty()) worldRegions[0].name = pd.title;
    }

    // First region starts discovered
    if (!worldRegions.empty()) worldRegions[0].discovered = true;
}

ZoneID Game::getRegionAt(Vector2 pos) const {
    // Bioma por POSIÇÃO — MESMA regra (período 2560 e owLayout 3x3 por módulo) que
    // Tilemap::render3D usa, para que INIMIGOS, ÁUDIO e ZONA concordem com o CHÃO
    // em TODO o mundo infinito (não só na área fixa central).
    (void)pos;
    // Uma FASE = um mundo inteiro. Trocar de mundo e pelo PORTAL (ver
    // advanceOpenWorldPhase), nunca por atravessar uma linha invisivel no chao.
    return currentZone;
}
