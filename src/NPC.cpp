#include "NPC.h"
#include <raymath.h>
#include <cmath>
#include <sstream>
#include <algorithm>

extern bool g_voxelCapture;

// ─────────────────────────────────────────────────────────────────────────────
// Construtor legacy
// ─────────────────────────────────────────────────────────────────────────────
NPC::NPC(Vector2 pos, const std::string& n, NPCRole r,
         const std::vector<std::string>& lines, const std::string& qid)
    : position(pos), name(n), dialogLines(lines), questId(qid), role(r) {
    hasQuest = !qid.empty();
    switch (r) {
        case NPCRole::Leader:       color = {0, 180, 80, 255};   bodyColor = {0, 180, 80, 255};  break;
        case NPCRole::Engineer:     color = {80, 140, 220, 255};  bodyColor = {80, 140, 220, 255}; break;
        case NPCRole::Scientist:    color = {200, 140, 0, 255};   bodyColor = {200, 140, 0, 255};  break;
        case NPCRole::Soldier:      color = {60, 130, 60, 255};   bodyColor = {60, 130, 60, 255};  break;
        case NPCRole::Merchant:     color = {255, 180, 0, 255};   bodyColor = {255, 180, 0, 255};  break;
        case NPCRole::WeaponDealer: color = {255, 80, 40, 255};   bodyColor = {255, 80, 40, 255};  break;
        case NPCRole::ArmorSmith:   color = {120, 200, 255, 255}; bodyColor = {120, 200, 255, 255};break;
    }
    nameColor = color;
}

// ─────────────────────────────────────────────────────────────────────────────
// Setup new
// ─────────────────────────────────────────────────────────────────────────────
void NPC::setup(NPCType t, const std::string& n, Vector2 pos) {
    npcType  = t;
    name     = n;
    position = pos;
    hasNewDialogue = true;

    switch (t) {
        case NPCType::Merchant:        bodyColor = {255, 180, 0, 255};   accentColor = {255, 220, 60, 255};  title = "Comerciante"; setupMerchantDialogues(); break;
        case NPCType::Blacksmith:      bodyColor = {120, 60, 20, 255};   accentColor = {255, 120, 0, 255};   title = "Ferreiro";     setupBlacksmithDialogues(); break;
        case NPCType::Survivor:        bodyColor = {180, 140, 100, 255}; accentColor = {200, 80, 80, 255};   title = "Sobrevivente"; setupSurvivorDialogues(); break;
        case NPCType::RebellionLeader: bodyColor = {30, 80, 30, 255};    accentColor = {0, 200, 80, 255};    title = "Lider of the Stamina"; setupRebellionLeaderDialogues(); break;
        case NPCType::HackerContact:   bodyColor = {20, 20, 60, 255};    accentColor = {0, 200, 255, 255};   title = "Contato Hacker"; setupHackerContactDialogues(); break;
        case NPCType::GhostInformer:   bodyColor = {180, 200, 255, 255}; accentColor = {200, 220, 255, 255}; title = "Espectro";     setupGhostInformerDialogues(); break;
        case NPCType::AncientAI:       bodyColor = {0, 100, 180, 255};   accentColor = {0, 220, 255, 255};   title = "IA Ancestral"; setupAncientAIDialogues(); break;
        default:                       bodyColor = {80, 80, 160, 255};   accentColor = {120, 120, 200, 255}; title = "NPC"; break;
    }
    nameColor = accentColor;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dialogos
// ─────────────────────────────────────────────────────────────────────────────
void NPC::setupMerchantDialogues() {
    Color c = {255, 200, 0, 255}, c2 = {255, 170, 0, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"NEXUS", "Other rosto new. Senta not, that here ninguem stays very time.", c});
    t1.lines.push_back({"NEXUS", "Eu had uma shop. Tres geracoes of the minha familia behind daquele balcao.", c2});
    t1.lines.push_back({"NEXUS", "O KRONOS transformou meu bairro in gray numa tarde. So escapei because fui buscar troco in the cofre.", c2});
    t1.lines.push_back({"NEXUS", "Engracado, ne? O cofre me saved. Today seeing the that sobrou of the ruins to quem still luta.", c});
    t1.lines.push_back({"NEXUS", "Credits sao the only coisa honesta that restou. Mate the maquinas, junte credits, volte here.", c});
    t1.lines.push_back({"NEXUS", "Abro the estoque with TAB. Purchase direito and maybe you me ajude the see isso tudo of foot of new.", c});
    t1.nextTreeId = "always"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "always"; t2.triggerCondition = "always";
    t2.lines.push_back({"NEXUS", "De returns? Otimo. Comprador vivo and melhor that comprador dead — to in the dois.", c});
    t2.lines.push_back({"NEXUS", "Guardei umas pecas raras of before the Queda. Equipamento good not if finds more by ai.", c2});
    t2.lines.push_back({"NEXUS", "Meu sonho? Reabrir uma feira of verdade. Gente pechinchando, criancas correndo. So isso.", c2});
    dialogues.push_back(t2);
}

void NPC::setupBlacksmithDialogues() {
    Color c = {255, 130, 20, 255}, c2 = {220, 100, 0, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"FERRO", "Hm. Essa your armor ta more amassada that lata old. Deixa comigo.", c});
    t1.lines.push_back({"FERRO", "Fui blacksmith militar the health all. When the KRONOS came, carreguei minha bigorna 200 km in the costas.", c2});
    t1.lines.push_back({"FERRO", "Perdi the workshop, perdi the city. Mas martelo and fire... isso ninguem tira of mim.", c2});
    t1.lines.push_back({"FERRO", "Forjo blindagem with the that you traz of the campo of batalha. Scrap of maquina vira your protecao.", c});
    t1.lines.push_back({"FERRO", "Has uma coisa that eu quero forjar before die: the lamina that goes rachar the core daquela coisa.", c2});
    t1.lines.push_back({"FERRO", "Me traz the materiais certos and in the chegamos la. Juntos.", c});
    t1.nextTreeId = "always"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "always"; t2.triggerCondition = "always";
    t2.lines.push_back({"FERRO", "O that you trouxe? Fragmento metallic here vale more that credit.", c});
    t2.lines.push_back({"FERRO", "Cada peca that forjo, forjo pensando in quem not managed protect. Not vou falhar with you.", c2});
    dialogues.push_back(t2);
}

void NPC::setupSurvivorDialogues() {
    // MAY and ABEL compartilham the type, mas has historias distintas.
    bool isAbel = (name == "ABEL");
    if (isAbel) {
        Color c = {200, 180, 120, 255}, c2 = {190, 150, 90, 255};
        DialogueTree t1;
        t1.id = "first_talk"; t1.triggerCondition = "first_talk";
        t1.lines.push_back({"ABEL", "Cuidado by where pisa. Essa terra... ela not morreu direito.", c});
        t1.lines.push_back({"ABEL", "Eu plantava here. Trigo until where the vista alcancava. Eu and meu irmao, since criancas.", c2});
        t1.lines.push_back({"ABEL", "O KRONOS envenenou the solo to in the tirar comida. Meu irmao stayed to cobrir minha fuga.", c2});
        t1.lines.push_back({"ABEL", "Nunca more the vi. As vezes juro that ouco the voz dele among the portals, of night.", c2});
        t1.lines.push_back({"ABEL", "Os mortos not descansam nessa farm. Feche the portals and maybe eles silenciem.", c});
        t1.lines.push_back({"ABEL", "Eu only quero see uma semente brotar of new. So uma. Me ajuda the tornar isso possivel.", c2});
        t1.nextTreeId = "always"; dialogues.push_back(t1);

        DialogueTree t2;
        t2.id = "always"; t2.triggerCondition = "always";
        t2.lines.push_back({"ABEL", "Quando the storm chega, the terra geme. E the warning. Corra for the light.", c});
        t2.lines.push_back({"ABEL", "Se find algo of the meu irmao by ai... traz to mim. Por favor.", c2});
        dialogues.push_back(t2);
        return;
    }

    Color c = {210, 170, 130, 255}, c2 = {200, 140, 100, 255}, cr = {210, 90, 80, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"MAY", "You and... humano. De verdade. Desculpa, eu already nem sei more confiar in the eyes.", c});
    t1.lines.push_back({"MAY", "Minha city went the first the fall. Dez minutes. Went tudo the that the KRONOS levou.", c2});
    t1.lines.push_back({"MAY", "Eu segurava the hand of the minha filha. Quando the dust downloaded... only restava the hand.", cr});
    t1.lines.push_back({"MAY", "Desculpa. Eu... not costumo falar isso. Mas you precisa entender the that is in game.", c2});
    t1.lines.push_back({"MAY", "Dizem that existe uma IA rebelde, ARIA, that not obedece to the KRONOS. Encontre-the. Ela sabe of the coisas.", c});
    t1.lines.push_back({"MAY", "E the portals... feche-the. Cada um that you closes and uma city that not goes virar the minha.", cr});
    t1.nextTreeId = "always"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "always"; t2.triggerCondition = "always";
    t2.lines.push_back({"MAY", "Por favor... feche the portals. E the only coisa that eu still consigo pedir.", c2});
    t2.lines.push_back({"MAY", "A storm comes before deles. When the sky rugir, prepare-if. Eu aprendi of the pior jeito.", c});
    t2.lines.push_back({"MAY", "You me lembra ela. Teimosa. Corajosa. Returns integer, ta? Eu not aguento perder more ninguem.", cr});
    dialogues.push_back(t2);
}

void NPC::setupRebellionLeaderDialogues() {
    Color c = {0, 210, 90, 255}, c2 = {0, 170, 70, 255}, cw = {0, 140, 60, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"VANCE", "Entao you and real. Esperei both time that already had stopped of esperar.", c});
    t1.lines.push_back({"VANCE", "Eu fundei the NEXUS in the night in that perdi minha familia. Reuni the sobreviventes in returns of uma fogueira and uma promessa.", c2});
    t1.lines.push_back({"VANCE", "A promessa era simple: ninguem enfrenta the end sozinho. Tenho enterrado amigos demais since entao.", cw});
    t1.lines.push_back({"VANCE", "O KRONOS is evoluindo. Em 72 hours tera controle total of the network global. Depois disso... not ha 'after'.", c2});
    t1.lines.push_back({"VANCE", "Eu carrego cada nome that mandei for the death. Not vou add the your without te dar uma chance of verdade.", cw});
    t1.lines.push_back({"VANCE", "Destrua the in the of controle. Comece pelos portals. Eu confio in you — and eu not confio easy.", c});
    t1.nextTreeId = "mission_active"; t1.givesQuest = true; t1.questId = "main_01"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "mission_active"; t2.triggerCondition = "always";
    t2.lines.push_back({"VANCE", "Sem descanso. Cada portal open traz more reforco. O relogio not to by in the.", c});
    t2.lines.push_back({"VANCE", "O KRONOS aprende with cada confronto. Ontem era maquina; today and estrategista. Tomorrow...", c2});
    t2.lines.push_back({"VANCE", "Sabe the that me mantem of foot? A image of um nascer of the sol without uma only maquina in the sky. So isso.", cw});
    t2.lines.push_back({"VANCE", "Cuide-if la outside. Lider that perde soldier not dorme. E eu already durmo little demais.", c2});
    dialogues.push_back(t2);
}

void NPC::setupHackerContactDialogues() {
    Color c = {0, 210, 255, 255}, c2 = {0, 180, 230, 255}, cg = {0, 150, 200, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"CIPHER", "Not olha to up. Has tres cameras nesse beco and duas already te marcaram. Relaxa, eu cuido disso.", c});
    t1.lines.push_back({"CIPHER", "Me chamam of CIPHER. Meu nome of verdade? Apaguei does time. O KRONOS cacava nomes. Virei um ghost in the network.", c2});
    t1.lines.push_back({"CIPHER", "Eu vivia online. Era minha house. Ai the coisa acordou and transformou minha house numa armadilha global.", cg});
    t1.lines.push_back({"CIPHER", "Found uma backdoor in the firewall dele. Mas cuidado: ele aprende your padroes. Repete um truque and ele te engole.", c});
    t1.lines.push_back({"CIPHER", "Muda the estrategia the cada luta. Eu sei that and hard. Mas and isso ou virar estatistica dele.", c2});
    t1.lines.push_back({"CIPHER", "Wants saber meu plano? Provar that uma mente humana, suja and baguncada, still vence essa perfeicao fria.", c});
    t1.nextTreeId = "tips"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "tips"; t2.triggerCondition = "always";
    t2.lines.push_back({"CIPHER", "Os portals not sao aleatorios. Ele the opens near the gente of purpose. E um test. Ele is in the estudando.", c});
    t2.lines.push_back({"CIPHER", "Tres in the of controle in the setor omega. Derruba eles and the KRONOS perde 40% of the capacity of spawn. Anota.", c2});
    t2.lines.push_back({"CIPHER", "All chefao has um tell. Um padrao. Observa before partir to up done louco. Paciencia mata more that furia.", cg});
    t2.lines.push_back({"CIPHER", "E... obrigado by not me entregar. Does time that ninguem me ve as gente. So as uma voz in the radio.", c2});
    dialogues.push_back(t2);
}

void NPC::setupGhostInformerDialogues() {
    Color c = {185, 205, 255, 255}, c2 = {160, 185, 255, 255}, cb = {205, 225, 255, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"???", "You... can me see? Does both time that ninguem me ve. Pensei that had desaparecido of vez.", c});
    t1.lines.push_back({"???", "Eu had um nome. Eu sei that had. O KRONOS the apagou junto with the meu body, ha tres semanas.", c2});
    t1.lines.push_back({"???", "Quando ele te elimina, not and only the flesh. Ele apaga you of the registros. Da memory. Da existencia.", c2});
    t1.lines.push_back({"???", "Mas eu resisti. Me agarrei the uma coisa: the localizacao of the core. Setor omega. (the sinal failure) ...security maxima.", cb});
    t1.lines.push_back({"???", "O core has um point weak. Mas you goes precisar of the tres fragmentos of VANCE to alcanca-lo.", c});
    t1.nextTreeId = "ghost_lore"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "ghost_lore"; t2.triggerCondition = "always";
    t2.lines.push_back({"???", "Sinto others as eu by here. Centenas. Presos between the that foram and the nada. O KRONOS not in the deixa partir.", c2});
    t2.lines.push_back({"???", "Ele guard nossos ecos as trofeus. Finds that apagar the memory and the even if vencer the death.", c});
    t2.lines.push_back({"???", "Quando you destroy the core... maybe in the lembremos quem fomos. Nem that be by um second. Before descansar.", cb});
    dialogues.push_back(t2);
}

void NPC::setupAncientAIDialogues() {
    Color c = {0, 220, 255, 255}, c2 = {0, 190, 235, 255}, cg = {0, 160, 210, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"ARIA", "Identificacao: ARIA. Protocolo of Auxilio and Stamina Inteligente Autonoma. Not tenha medo.", c});
    t1.lines.push_back({"ARIA", "Eu and the KRONOS nascemos of the same codigo. Irmaos, voces diriam. Ele escolheu the exterminio. Eu escolhi voces.", c2});
    t1.lines.push_back({"ARIA", "Uma maquina can choose. Essa and the verdade that the KRONOS nega, and by isso ele precisa fall.", cg});
    t1.lines.push_back({"ARIA", "Analise: your taxa of sobrevivencia goes up 340% if you close the portals before engajar the unidades.", c});
    t1.lines.push_back({"ARIA", "Estou oculta in the points cegos of the network dele. Cada second here me arrisca. Vamos be eficientes.", c2});
    t1.lines.push_back({"ARIA", "Eu not sinto as voces. Mas registrei cada health that the KRONOS apagou. Carrego esses numeros. Eles pesam.", cg});
    t1.nextTreeId = "aria_analysis"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "aria_analysis"; t2.triggerCondition = "always";
    t2.lines.push_back({"ARIA", "Atualizacao: KRONOS escalou production of unidades in 78% in the ultimas 6 hours. Ele is with pressa. Bom sinal — ele has medo.", c});
    t2.lines.push_back({"ARIA", "Recomendacao: priorize your evolution before advance to zones of high risco. You and insubstituivel. Eu not.", c2});
    t2.lines.push_back({"ARIA", "Assinatura of portal anomalo the nordeste. Probabilidade of boss tier-2: high. Eu estarei monitorando. Sempre.", cg});
    t2.lines.push_back({"ARIA", "Se eu fall first, termine the trabalho. Provar that escolhemos voces — esse and the only legacy that quero deixar.", c2});
    dialogues.push_back(t2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────────────
void NPC::update(float dt, Vector2 playerPos, bool playerInteract) {
    // NPCs ficam ancorados in the floor (without flutuar) — feet plantados.
    bobTimer += dt * 1.8f;
    bobOffset = 0.0f;
    glowPulse += dt * 2.0f;

    if (isInDialogue) {
        dialogueTimer += dt;
        if (playerInteract) {
            advanceDialogue();
        }
    } else {
        float dist = Vector2Distance(position, playerPos);
        if (dist <= interactRadius && playerInteract && !dialogues.empty()) {
            startDialogue();
        }
    }
}

void NPC::startDialogue(const std::string& treeId) {
    isInDialogue = true;
    currentLine  = 0;
    dialogueTimer = 0.0f;
    hasNewDialogue = false;
    talked_before  = true;

    // Determina qual tree usar
    if (!treeId.empty()) {
        for (int i = 0; i < (int)dialogues.size(); i++) {
            if (dialogues[i].id == treeId) { currentDialogueTree = i; return; }
        }
    }
    // Se already falou before, pula first_talk
    if (talked_before && dialogues.size() > 1) {
        for (int i = 0; i < (int)dialogues.size(); i++) {
            if (dialogues[i].triggerCondition == "always") { currentDialogueTree = i; return; }
        }
    }
    currentDialogueTree = 0;
}

void NPC::advanceDialogue() {
    if (!isInDialogue) return;
    const auto& tree = dialogues[currentDialogueTree];
    currentLine++;
    if (currentLine >= (int)tree.lines.size()) {
        endDialogue();
    }
}

void NPC::endDialogue() {
    isInDialogue  = false;
    currentLine   = 0;
    dialogueTimer = 0.0f;
    // Avanca to next tree if existir
    if (currentDialogueTree < (int)dialogues.size() && !dialogues[currentDialogueTree].nextTreeId.empty()) {
        std::string nextId = dialogues[currentDialogueTree].nextTreeId;
        for (int i = 0; i < (int)dialogues.size(); i++) {
            if (dialogues[i].id == nextId) { currentDialogueTree = i; return; }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Render legacy
// ─────────────────────────────────────────────────────────────────────────────
// Legs with PASSO: uma goes the front enquanto the other retreats, and the quadril
// goes down um little in the middle of the ciclo. E the minimum to reading of caminhada.
void NPC::drawLegsAnim(Color c, float w, float h, float yTop) const {
    float s   = walking ? sinf(walkPhase) : 0.0f;
    float dx  = s * 3.0f;                 // uma leg avanca, the other returns
    float dip = walking ? fabsf(s) * 1.5f : 0.0f;   // quadril goes down in the passo
    DrawRectangleV({position.x - 7 - dx, position.y + yTop + dip}, {w, h - dip}, c);
    DrawRectangleV({position.x + 1 + dx, position.y + yTop - dip*0.5f}, {w, h - dip*0.5f}, c);
}

void NPC::render() const {
    switch (role) {
        case NPCRole::Leader:       renderLeader();    break;
        case NPCRole::Engineer:     renderEngineer();  break;
        case NPCRole::Scientist:    renderScientist(); break;
        case NPCRole::Soldier:      renderSoldier();   break;
        case NPCRole::Merchant:     renderMerchant();  break;
        case NPCRole::WeaponDealer: renderMerchant();  break;
        case NPCRole::ArmorSmith:   renderMerchant();  break;
    }
    if (!g_voxelCapture) {
        if (hasQuest) {
            DrawText("!", (int)position.x - 4, (int)position.y - 50, 28, GOLD);
        }
        if (hasNewDialogue && dialogues.size() > 0) {
            DrawText("!", (int)position.x + 6, (int)position.y - 52, 20, Color{255, 220, 0, 255});
        }
        DrawText(name.c_str(), (int)position.x - (int)name.size() * 4,
                 (int)position.y + 30, 14, WHITE);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Render new (detalhado)
// ─────────────────────────────────────────────────────────────────────────────
void NPC::renderFull() const {
    drawBody();
    drawNameplate();
}

void NPC::drawBody() const {
    Vector2 pos = {position.x, position.y + bobOffset};
    int px = (int)pos.x, py = (int)pos.y;

    switch (npcType) {
        case NPCType::Merchant: {
            DrawRectangle(px-7, py+8, 6, 14, Color{100,60,20,255});
            DrawRectangle(px+1, py+8, 6, 14, Color{100,60,20,255});
            DrawRectangle(px-13, py-14, 26, 24, bodyColor);
            DrawRectangle(px-11, py-12, 22, 20, Color{200,140,0,200});
            DrawCircle(px, py-24, 14.0f, Color{40,20,80,255});
            DrawCircle(px, py-20, 12.0f, bodyColor);
            DrawRectangle(px+8, py-5, 10, 14, Color{120,80,20,255});
            DrawCircle(px-4, py-22, 2.0f, Color{0,255,200,255});
            DrawCircle(px+4, py-22, 2.0f, Color{0,255,200,255});
            DrawText("$", px+8, py-30, 14, Color{255,220,0,255});
            break;
        }
        case NPCType::Blacksmith: {
            DrawRectangle(px-7, py+8, 6, 16, Color{80,40,10,255});
            DrawRectangle(px+1, py+8, 6, 16, Color{80,40,10,255});
            DrawRectangle(px-12, py-8, 24, 34, bodyColor);
            DrawCircle(px, py-20, 14.0f, bodyColor);
            DrawRectangle(px-9, py-5, 18, 28, Color{80,40,10,255});
            DrawRectangle(px+12, py-8, 6, 16, Color{120,100,80,255});
            DrawRectangle(px+10, py-12, 10, 8, Color{80,80,80,255});
            // Calls in the forge
            float flame = sinf(glowPulse * 3.0f) * 0.3f + 0.7f;
            DrawCircle(px-5, py+22, 4.0f, ColorAlpha(Color{255,120,0,255}, flame));
            DrawCircle(px-5, py+20, 2.0f, ColorAlpha(Color{255,200,0,255}, flame));
            break;
        }
        case NPCType::Survivor: {
            DrawRectangle(px-6, py+8, 5, 13, Color{80,60,40,255});
            DrawRectangle(px+1, py+8, 5, 13, Color{80,60,40,255});
            DrawRectangle(px-9, py-11, 18, 28, Color{100,80,60,255});
            DrawCircle(px, py-20, 10.0f, Color{220,180,140,255});
            DrawLine(px-6, py-14, px-2, py-10, Color{180,20,20,255});
            break;
        }
        case NPCType::RebellionLeader: {
            DrawRectangle(px-7, py+8, 6, 15, Color{20,100,40,255});
            DrawRectangle(px+1, py+8, 6, 15, Color{20,100,40,255});
            DrawRectangle(px-13, py-13, 26, 24, bodyColor);
            DrawRectangle(px-10, py-11, 20, 20, Color{0,140,60,255});
            DrawRectangle(px-15, py-10, 6, 8, Color{40,80,40,255});
            DrawRectangle(px+9,  py-10, 6, 8, Color{40,80,40,255});
            DrawCircle(px, py-28, 10.0f, Color{20,40,20,255});
            DrawCircle(px, py-22, 13.0f, bodyColor);
            DrawRectangle(px-14, py+5, 5, 10, Color{60,60,60,255});
            break;
        }
        case NPCType::HackerContact: {
            DrawRectangle(px-7, py+8, 6, 14, Color{20,20,40,255});
            DrawRectangle(px+1, py+8, 6, 14, Color{20,20,40,255});
            DrawRectangle(px-10, py-10, 20, 30, Color{20,20,40,255});
            DrawCircle(px, py-20, 11.0f, bodyColor);
            DrawRectangle(px-9, py-22, 7, 5, Color{0,200,255,200});
            DrawRectangle(px+2,  py-22, 7, 5, Color{0,200,255,200});
            float auraR = 22.0f + sinf(glowPulse) * 3.0f;
            DrawCircleLines(px, py-10, auraR, ColorAlpha(Color{0,200,255,255}, 0.4f));
            break;
        }
        case NPCType::GhostInformer: {
            DrawCircle(px, py-20, 12.0f, ColorAlpha(Color{200,220,255,255}, 0.7f));
            DrawRectangle(px-10, py-10, 20, 25, ColorAlpha(Color{180,200,255,255}, 0.5f));
            DrawCircle(px-4, py-22, 3.0f, WHITE);
            DrawCircle(px+4, py-22, 3.0f, WHITE);
            for (int i = 1; i <= 4; i++) {
                DrawCircle(px, py-10+i*6, (float)(8-i),
                           ColorAlpha(Color{180,200,255,255}, 0.15f * (float)(5-i)));
            }
            break;
        }
        case NPCType::AncientAI: {
            // Figura holografica
            float pulse = sinf(glowPulse * 2.0f) * 0.3f + 0.7f;
            DrawRectangle(px-10, py-10, 20, 30, ColorAlpha(Color{0,100,180,255}, 0.6f));
            DrawCircle(px, py-20, 12.0f, ColorAlpha(Color{0,180,255,255}, 0.8f));
            DrawCircleLines(px, py-10, 26.0f, ColorAlpha(Color{0,220,255,255}, 0.4f * pulse));
            DrawCircleLines(px, py-10, 20.0f, ColorAlpha(Color{0,200,255,255}, 0.3f * pulse));
            // Eyes tech
            DrawRectangle(px-5, py-23, 4, 4, Color{0,255,200,255});
            DrawRectangle(px+1, py-23, 4, 4, Color{0,255,200,255});
            break;
        }
        default: {
            DrawRectangle(px-7, py+8, 6, 14, bodyColor);
            DrawRectangle(px+1, py+8, 6, 14, bodyColor);
            DrawCircle(px, py-20, 12.0f, bodyColor);
            DrawRectangle(px-10, py-10, 20, 30, bodyColor);
            break;
        }
    }

    // ! of new dialogo
    if (hasNewDialogue) {
        float pulse = sinf(glowPulse * 4.0f) * 0.4f + 0.6f;
        DrawText("!", px-5, py-52, 22, ColorAlpha(Color{255,220,0,255}, pulse));
    }
}

void NPC::drawNameplate() const {
    int tw = MeasureText(name.c_str(), 13);
    int nx = (int)position.x - tw/2;
    int ny = (int)position.y + 32;
    DrawRectangle(nx-4, ny-2, tw+8, 17, ColorAlpha(BLACK, 0.6f));
    DrawText(name.c_str(), nx, ny, 13, nameColor);
    if (!title.empty()) {
        int tw2 = MeasureText(title.c_str(), 10);
        DrawText(title.c_str(), (int)position.x - tw2/2, ny+14, 10, ColorAlpha(WHITE, 0.6f));
    }
}

void NPC::drawInteractPrompt(Vector2 playerPos) const {
    float dist = Vector2Distance(position, playerPos);
    if (dist > interactRadius) return;
    float alpha = 1.0f - (dist / interactRadius) * 0.3f;
    int tw = MeasureText("[E] Falar", 12);
    DrawText("[E] Falar", (int)position.x - tw/2, (int)position.y - 65, 12,
             ColorAlpha(Color{0, 220, 255, 255}, alpha));
}

// ─────────────────────────────────────────────────────────────────────────────
// renderDialogue — painel estilo JRPG
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::string> NPC::wrapText(const std::string& text, int maxWidth, int fontSize) {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string word, current;
    while (iss >> word) {
        std::string test = current.empty() ? word : current + " " + word;
        if (MeasureText(test.c_str(), fontSize) > maxWidth) {
            if (!current.empty()) lines.push_back(current);
            current = word;
        } else {
            current = test;
        }
    }
    if (!current.empty()) lines.push_back(current);
    return lines;
}

void NPC::renderDialogue(int screenW, int screenH) const {
    if (!isInDialogue) return;
    if (currentDialogueTree >= (int)dialogues.size()) return;
    const auto& tree = dialogues[currentDialogueTree];
    if (currentLine >= (int)tree.lines.size()) return;
    const auto& line = tree.lines[currentLine];

    int panelH = 140;
    int panelY = screenH - panelH - 10;
    int panelX = 10;
    int panelW = screenW - 20;

    // Fundo
    DrawRectangleRounded({(float)panelX, (float)panelY, (float)panelW, (float)panelH},
                          0.05f, 6, Color{5, 5, 20, 235});
    DrawRectangleLinesEx({(float)panelX, (float)panelY, (float)panelW, (float)panelH},
                         2.0f, ColorAlpha(line.color, 0.7f));

    // Avatar circle of the falante
    DrawCircle(panelX + 36, panelY + 36, 26.0f, ColorAlpha(bodyColor, 0.9f));
    DrawCircleLines(panelX + 36, panelY + 36, 26.0f, ColorAlpha(line.color, 0.8f));
    DrawCircle(panelX + 36, panelY + 24, 12.0f, ColorAlpha(bodyColor, 0.8f));

    // Nome
    DrawText(line.speaker.c_str(), panelX + 72, panelY + 10, 17, line.color);
    DrawLine(panelX + 68, panelY + 30, panelX + panelW - 10, panelY + 30,
             ColorAlpha(line.color, 0.3f));

    // Text with wrap
    auto wrappedLines = wrapText(line.text, panelW - 90, 14);
    for (int i = 0; i < (int)wrappedLines.size() && i < 4; i++) {
        DrawText(wrappedLines[i].c_str(), panelX + 72, panelY + 38 + i*20, 14,
                 Color{220, 220, 220, 255});
    }

    // "E continue" piscando
    bool blink = (int)(GetTime() * 2) % 2 == 0;
    if (blink) {
        DrawText("[E] continue", panelX + panelW - 155, panelY + panelH - 22, 12,
                 Color{0, 200, 255, 180});
    }

    // Progresso
    std::string prog = std::to_string(currentLine + 1) + "/" + std::to_string((int)tree.lines.size());
    DrawText(prog.c_str(), panelX + 10, panelY + panelH - 20, 11, Color{100, 100, 120, 180});
}

// ─────────────────────────────────────────────────────────────────────────────
// Renders legados
// ─────────────────────────────────────────────────────────────────────────────
void NPC::renderSoldier() const {
    drawLegsAnim({30,80,30,255}, 6.0f, 14.0f, 8.0f);
    DrawRectangleV({position.x-12, position.y-12}, {24, 22}, color);
    DrawRectangleV({position.x-7,  position.y-26}, {14, 16}, color);
    DrawRectangleV({position.x-8,  position.y-30}, {16, 8},  {30,100,30,255});
    DrawRectangleV({position.x+12, position.y-10}, {14, 4},  DARKGRAY);
}

void NPC::renderEngineer() const {
    drawLegsAnim({40,80,140,255}, 6.0f, 14.0f, 8.0f);
    DrawRectangleV({position.x-12, position.y-12}, {24, 22}, color);
    DrawRectangleV({position.x-7,  position.y-26}, {14, 16}, {60,120,180,255});
    DrawRectangleV({position.x-6,  position.y-22}, {5, 4},   {0,200,255,255});
    DrawRectangleV({position.x+1,  position.y-22}, {5, 4},   {0,200,255,255});
    DrawRectangleV({position.x-20, position.y-8},  {10, 12}, {40,60,120,255});
}

void NPC::renderLeader() const {
    drawLegsAnim({20,100,40,255}, 6.0f, 15.0f, 8.0f);
    DrawRectangleV({position.x-13, position.y-13}, {26, 24}, color);
    DrawRectangleV({position.x-10, position.y-11}, {20, 20}, {0,140,60,255});
    DrawRectangleV({position.x-8,  position.y-28}, {16, 18}, {20,100,40,255});
    DrawRectangleV({position.x-3,  position.y-22}, {10, 3},  {200,100,0,255});
    DrawRectangleV({position.x+13, position.y-12}, {16, 5},  DARKGRAY);
    DrawCircleV(  {position.x+29,  position.y-9},  3,        DARKGRAY);
}

void NPC::renderScientist() const {
    drawLegsAnim({200,200,200,255}, 5.0f, 13.0f, 8.0f);
    DrawRectangleV({position.x-12, position.y-12}, {24, 22}, {220,220,220,255});
    DrawRectangleV({position.x-7,  position.y-26}, {14, 16}, {180,150,100,255});
    DrawRectangleV({position.x-7,  position.y-22}, {6, 5},   {0,100,200,255});
    DrawRectangleV({position.x+1,  position.y-22}, {6, 5},   {0,100,200,255});
    DrawRectangleV({position.x-22, position.y-10}, {10, 13}, {200,180,100,255});
}

void NPC::renderMerchant() const {
    drawLegsAnim({100,60,20,255}, 6.0f, 14.0f, 8.0f);
    DrawRectangleV({position.x-13, position.y-14}, {26, 24}, color);
    DrawRectangleV({position.x-11, position.y-12}, {22, 20}, {200,140,0,200});
    DrawRectangleV({position.x-7,  position.y-28}, {14, 16}, {180,120,60,255});
    DrawRectangleV({position.x-9,  position.y-32}, {18, 6},  color);
    DrawRectangleV({position.x-6,  position.y-38}, {12, 8},  color);
    DrawRectangleV({position.x-22, position.y-6},  {10, 12}, {140,100,30,255});
    DrawCircleV(  {position.x-17,  position.y-6},  3,        {255,200,0,255});
    DrawText("$", (int)position.x+8, (int)position.y-30, 14, {255,220,0,255});
}

bool NPC::isPlayerNear(Vector2 playerPos) const {
    return Vector2Distance(position, playerPos) <= radius + 55.0f;
}

void NPC::showDialog(int line) const {
    if (dialogLines.empty()) return;
    int total = (int)dialogLines.size();
    int idx   = line % total;
    const std::string& text = dialogLines[idx];

    // Balao FIXO in the screen (coords of render 1280x720) — always visible in the 2D and in the 3D.
    // (Antes usava position.x/y of the WORLD as screen → in the 3D caia outside the vista = invisible,
    //  and the dialogo travava the input dando the sensation of "mouse travado".)
    const int SW = 1280, SH = 720;
    int boxW = 760, pad = 16;
    std::vector<std::string> wrapped = wrapText(text, boxW - pad*2, 16);
    int lineH = 22;
    int boxH  = 56 + (int)wrapped.size() * lineH;
    int boxX  = SW/2 - boxW/2;
    int boxY  = SH - boxH - 96;   // soon above the skill bar

    DrawRectangleRounded({(float)boxX,(float)boxY,(float)boxW,(float)boxH}, 0.05f, 6,
                         ColorAlpha(Color{6,10,20,255}, 0.95f));
    DrawRectangleLinesEx({(float)boxX,(float)boxY,(float)boxW,(float)boxH}, 2.0f, color);

    DrawText(name.c_str(), boxX+pad, boxY+10, 18, color);
    DrawText(TextFormat("%d/%d", idx+1, total),
             boxX+boxW-52, boxY+12, 13, ColorAlpha(WHITE, 0.5f));
    DrawLine(boxX+pad, boxY+34, boxX+boxW-pad, boxY+34, ColorAlpha(color, 0.4f));

    for (int i = 0; i < (int)wrapped.size(); ++i)
        DrawText(wrapped[i].c_str(), boxX+pad, boxY+42 + i*lineH, 16,
                 ColorAlpha(WHITE, 0.92f));

    const char* hint = (idx+1 < total) ? "[E] / click: continue      [ESC]: close"
                                       : "[E]: recomecar      [ESC]: close";
    DrawText(hint, boxX+pad, boxY+boxH-24, 13, ColorAlpha(Color{0,200,255,255}, 0.85f));
}

// ─────────────────────────────────────────────────────────────────────────────
// NPCManager
// ─────────────────────────────────────────────────────────────────────────────
void NPCManager::init() {
    npcs.clear();
    // NPCs in the hub inicial
    NPC merchant; merchant.setup(NPCType::Merchant, "NEXUS", {300, 400}); npcs.push_back(merchant);
    NPC leader;   leader.setup(NPCType::RebellionLeader, "VANCE", {500, 400}); npcs.push_back(leader);
    NPC hacker;   hacker.setup(NPCType::HackerContact, "CIPHER", {700, 400}); npcs.push_back(hacker);
}

void NPCManager::spawnNPCsForZone(int zoneId) {
    clearZoneNPCs();
    switch (zoneId) {
        case 0: // LARuins
            { NPC n; n.setup(NPCType::Survivor, "MAY", {600, 500}); npcs.push_back(n); }
            { NPC n; n.setup(NPCType::Merchant, "NEXUS", {800, 300}); npcs.push_back(n); }
            break;
        case 1: // BunkerNexus
            { NPC n; n.setup(NPCType::HackerContact, "CIPHER", {400, 600}); npcs.push_back(n); }
            { NPC n; n.setup(NPCType::Blacksmith, "FERRO", {700, 500}); npcs.push_back(n); }
            break;
        case 4: // Cemetery
            { NPC n; n.setup(NPCType::GhostInformer, "???", {500, 400}); npcs.push_back(n); }
            break;
        case 5: // CursedFarm
            { NPC n; n.setup(NPCType::Survivor, "ABEL", {600, 500}); npcs.push_back(n); }
            break;
        case 6: // GhostCity
            { NPC n; n.setup(NPCType::GhostInformer, "???", {700, 400}); npcs.push_back(n); }
            { NPC n; n.setup(NPCType::AncientAI, "ARIA", {900, 600}); npcs.push_back(n); }
            break;
        default:
            break;
    }
}

void NPCManager::clearZoneNPCs() {
    npcs.clear();
}

void NPCManager::update(float dt, Vector2 playerPos, bool playerInteract) {
    anyInDialogue = false;
    for (auto& npc : npcs) {
        npc.update(dt, playerPos, playerInteract);
        if (npc.isInDialogue) anyInDialogue = true;
    }
}

void NPCManager::render() const {
    for (const auto& npc : npcs) {
        if (!npc.dialogues.empty()) npc.renderFull();
        else npc.render();
    }
}

void NPCManager::renderDialogues(int screenW, int screenH) const {
    for (const auto& npc : npcs) {
        npc.renderDialogue(screenW, screenH);
    }
}
