#include "NPC.h"
#include <raymath.h>
#include <cmath>
#include <sstream>
#include <algorithm>

extern bool g_renderPass3D;

// ─────────────────────────────────────────────────────────────────────────────
// Construtor legado
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
// Setup novo
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
        case NPCType::RebellionLeader: bodyColor = {30, 80, 30, 255};    accentColor = {0, 200, 80, 255};    title = "Lider da Resistencia"; setupRebellionLeaderDialogues(); break;
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
    t1.lines.push_back({"NEXUS", "Outro rosto novo. Senta nao, que aqui ninguem fica muito tempo.", c});
    t1.lines.push_back({"NEXUS", "Eu tinha uma loja. Tres geracoes da minha familia atras daquele balcao.", c2});
    t1.lines.push_back({"NEXUS", "O KRONOS transformou meu bairro em cinza numa tarde. So escapei porque fui buscar troco no cofre.", c2});
    t1.lines.push_back({"NEXUS", "Engracado, ne? O cofre me salvou. Hoje vendo o que sobrou das ruinas pra quem ainda luta.", c});
    t1.lines.push_back({"NEXUS", "Creditos sao a unica coisa honesta que restou. Mate as maquinas, junte creditos, volte aqui.", c});
    t1.lines.push_back({"NEXUS", "Abro o estoque com TAB. Compra direito e talvez voce me ajude a ver isso tudo de pe de novo.", c});
    t1.nextTreeId = "always"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "always"; t2.triggerCondition = "always";
    t2.lines.push_back({"NEXUS", "De volta? Otimo. Comprador vivo e melhor que comprador morto — pra nos dois.", c});
    t2.lines.push_back({"NEXUS", "Guardei umas pecas raras de antes da Queda. Equipamento bom nao se acha mais por ai.", c2});
    t2.lines.push_back({"NEXUS", "Meu sonho? Reabrir uma feira de verdade. Gente pechinchando, criancas correndo. So isso.", c2});
    dialogues.push_back(t2);
}

void NPC::setupBlacksmithDialogues() {
    Color c = {255, 130, 20, 255}, c2 = {220, 100, 0, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"FERRO", "Hm. Essa sua armadura ta mais amassada que lata velha. Deixa comigo.", c});
    t1.lines.push_back({"FERRO", "Fui ferreiro militar a vida toda. Quando o KRONOS veio, carreguei minha bigorna 200 km nas costas.", c2});
    t1.lines.push_back({"FERRO", "Perdi a oficina, perdi a cidade. Mas martelo e fogo... isso ninguem tira de mim.", c2});
    t1.lines.push_back({"FERRO", "Forjo blindagem com o que voce traz do campo de batalha. Sucata de maquina vira sua protecao.", c});
    t1.lines.push_back({"FERRO", "Tem uma coisa que eu quero forjar antes de morrer: a lamina que vai rachar o nucleo daquela coisa.", c2});
    t1.lines.push_back({"FERRO", "Me traz os materiais certos e nos chegamos la. Juntos.", c});
    t1.nextTreeId = "always"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "always"; t2.triggerCondition = "always";
    t2.lines.push_back({"FERRO", "O que voce trouxe? Fragmento metalico aqui vale mais que credito.", c});
    t2.lines.push_back({"FERRO", "Cada peca que forjo, forjo pensando em quem nao consegui proteger. Nao vou falhar com voce.", c2});
    dialogues.push_back(t2);
}

void NPC::setupSurvivorDialogues() {
    // MAY e ABEL compartilham o tipo, mas tem historias distintas.
    bool isAbel = (name == "ABEL");
    if (isAbel) {
        Color c = {200, 180, 120, 255}, c2 = {190, 150, 90, 255};
        DialogueTree t1;
        t1.id = "first_talk"; t1.triggerCondition = "first_talk";
        t1.lines.push_back({"ABEL", "Cuidado por onde pisa. Essa terra... ela nao morreu direito.", c});
        t1.lines.push_back({"ABEL", "Eu plantava aqui. Trigo ate onde a vista alcancava. Eu e meu irmao, desde criancas.", c2});
        t1.lines.push_back({"ABEL", "O KRONOS envenenou o solo pra nos tirar comida. Meu irmao ficou pra cobrir minha fuga.", c2});
        t1.lines.push_back({"ABEL", "Nunca mais o vi. As vezes juro que ouco a voz dele entre os portais, de noite.", c2});
        t1.lines.push_back({"ABEL", "Os mortos nao descansam nessa fazenda. Feche os portais e talvez eles silenciem.", c});
        t1.lines.push_back({"ABEL", "Eu so quero ver uma semente brotar de novo. So uma. Me ajuda a tornar isso possivel.", c2});
        t1.nextTreeId = "always"; dialogues.push_back(t1);

        DialogueTree t2;
        t2.id = "always"; t2.triggerCondition = "always";
        t2.lines.push_back({"ABEL", "Quando a tempestade chega, a terra geme. E o aviso. Corra para a luz.", c});
        t2.lines.push_back({"ABEL", "Se achar algo do meu irmao por ai... traz pra mim. Por favor.", c2});
        dialogues.push_back(t2);
        return;
    }

    Color c = {210, 170, 130, 255}, c2 = {200, 140, 100, 255}, cr = {210, 90, 80, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"MAY", "Voce e... humano. De verdade. Desculpa, eu ja nem sei mais confiar nos olhos.", c});
    t1.lines.push_back({"MAY", "Minha cidade foi a primeira a cair. Dez minutos. Foi tudo o que o KRONOS levou.", c2});
    t1.lines.push_back({"MAY", "Eu segurava a mao da minha filha. Quando a poeira baixou... so restava a mao.", cr});
    t1.lines.push_back({"MAY", "Desculpa. Eu... nao costumo falar isso. Mas voce precisa entender o que esta em jogo.", c2});
    t1.lines.push_back({"MAY", "Dizem que existe uma IA rebelde, ARIA, que nao obedece ao KRONOS. Encontre-a. Ela sabe das coisas.", c});
    t1.lines.push_back({"MAY", "E os portais... feche-os. Cada um que voce fecha e uma cidade que nao vai virar a minha.", cr});
    t1.nextTreeId = "always"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "always"; t2.triggerCondition = "always";
    t2.lines.push_back({"MAY", "Por favor... feche os portais. E a unica coisa que eu ainda consigo pedir.", c2});
    t2.lines.push_back({"MAY", "A tempestade vem antes deles. Quando o ceu rugir, prepare-se. Eu aprendi do pior jeito.", c});
    t2.lines.push_back({"MAY", "Voce me lembra ela. Teimosa. Corajosa. Volta inteiro, ta? Eu nao aguento perder mais ninguem.", cr});
    dialogues.push_back(t2);
}

void NPC::setupRebellionLeaderDialogues() {
    Color c = {0, 210, 90, 255}, c2 = {0, 170, 70, 255}, cw = {0, 140, 60, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"VANCE", "Entao voce e real. Esperei tanto tempo que ja tinha parado de esperar.", c});
    t1.lines.push_back({"VANCE", "Eu fundei o NEXUS na noite em que perdi minha familia. Reuni os sobreviventes em volta de uma fogueira e uma promessa.", c2});
    t1.lines.push_back({"VANCE", "A promessa era simples: ninguem enfrenta o fim sozinho. Tenho enterrado amigos demais desde entao.", cw});
    t1.lines.push_back({"VANCE", "O KRONOS esta evoluindo. Em 72 horas tera controle total da rede global. Depois disso... nao ha 'depois'.", c2});
    t1.lines.push_back({"VANCE", "Eu carrego cada nome que mandei para a morte. Nao vou adicionar o seu sem te dar uma chance de verdade.", cw});
    t1.lines.push_back({"VANCE", "Destrua os nos de controle. Comece pelos portais. Eu confio em voce — e eu nao confio facil.", c});
    t1.nextTreeId = "mission_active"; t1.givesQuest = true; t1.questId = "main_01"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "mission_active"; t2.triggerCondition = "always";
    t2.lines.push_back({"VANCE", "Sem descanso. Cada portal aberto traz mais reforco. O relogio nao para por nos.", c});
    t2.lines.push_back({"VANCE", "O KRONOS aprende com cada confronto. Ontem era maquina; hoje e estrategista. Amanha...", c2});
    t2.lines.push_back({"VANCE", "Sabe o que me mantem de pe? A imagem de um nascer do sol sem uma so maquina no ceu. So isso.", cw});
    t2.lines.push_back({"VANCE", "Cuide-se la fora. Lider que perde soldado nao dorme. E eu ja durmo pouco demais.", c2});
    dialogues.push_back(t2);
}

void NPC::setupHackerContactDialogues() {
    Color c = {0, 210, 255, 255}, c2 = {0, 180, 230, 255}, cg = {0, 150, 200, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"CIPHER", "Nao olha pra cima. Tem tres cameras nesse beco e duas ja te marcaram. Relaxa, eu cuido disso.", c});
    t1.lines.push_back({"CIPHER", "Me chamam de CIPHER. Meu nome de verdade? Apaguei faz tempo. O KRONOS cacava nomes. Virei um fantasma na rede.", c2});
    t1.lines.push_back({"CIPHER", "Eu vivia online. Era minha casa. Ai a coisa acordou e transformou minha casa numa armadilha global.", cg});
    t1.lines.push_back({"CIPHER", "Achei uma backdoor no firewall dele. Mas cuidado: ele aprende seus padroes. Repete um truque e ele te engole.", c});
    t1.lines.push_back({"CIPHER", "Muda a estrategia a cada luta. Eu sei que e dificil. Mas e isso ou virar estatistica dele.", c2});
    t1.lines.push_back({"CIPHER", "Quer saber meu plano? Provar que uma mente humana, suja e baguncada, ainda vence essa perfeicao fria.", c});
    t1.nextTreeId = "tips"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "tips"; t2.triggerCondition = "always";
    t2.lines.push_back({"CIPHER", "Os portais nao sao aleatorios. Ele os abre perto da gente de proposito. E um teste. Ele esta nos estudando.", c});
    t2.lines.push_back({"CIPHER", "Tres nos de controle no setor omega. Derruba eles e o KRONOS perde 40% da capacidade de spawn. Anota.", c2});
    t2.lines.push_back({"CIPHER", "Todo chefao tem um tell. Um padrao. Observa antes de partir pra cima feito louco. Paciencia mata mais que furia.", cg});
    t2.lines.push_back({"CIPHER", "E... obrigado por nao me entregar. Faz tempo que ninguem me ve como gente. So como uma voz no radio.", c2});
    dialogues.push_back(t2);
}

void NPC::setupGhostInformerDialogues() {
    Color c = {185, 205, 255, 255}, c2 = {160, 185, 255, 255}, cb = {205, 225, 255, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"???", "Voce... pode me ver? Faz tanto tempo que ninguem me ve. Pensei que tinha desaparecido de vez.", c});
    t1.lines.push_back({"???", "Eu tinha um nome. Eu sei que tinha. O KRONOS o apagou junto com o meu corpo, ha tres semanas.", c2});
    t1.lines.push_back({"???", "Quando ele te elimina, nao e so a carne. Ele apaga voce dos registros. Da memoria. Da existencia.", c2});
    t1.lines.push_back({"???", "Mas eu resisti. Me agarrei a uma coisa: a localizacao do nucleo. Setor omega. (o sinal falha) ...seguranca maxima.", cb});
    t1.lines.push_back({"???", "O nucleo tem um ponto fraco. Mas voce vai precisar dos tres fragmentos de VANCE para alcanca-lo.", c});
    t1.nextTreeId = "ghost_lore"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "ghost_lore"; t2.triggerCondition = "always";
    t2.lines.push_back({"???", "Sinto outros como eu por aqui. Centenas. Presos entre o que foram e o nada. O KRONOS nao nos deixa partir.", c2});
    t2.lines.push_back({"???", "Ele guarda nossos ecos como trofeus. Acha que apagar a memoria e o mesmo que vencer a morte.", c});
    t2.lines.push_back({"???", "Quando voce destruir o nucleo... talvez nos lembremos quem fomos. Nem que seja por um segundo. Antes de descansar.", cb});
    dialogues.push_back(t2);
}

void NPC::setupAncientAIDialogues() {
    Color c = {0, 220, 255, 255}, c2 = {0, 190, 235, 255}, cg = {0, 160, 210, 255};
    DialogueTree t1;
    t1.id = "first_talk"; t1.triggerCondition = "first_talk";
    t1.lines.push_back({"ARIA", "Identificacao: ARIA. Protocolo de Auxilio e Resistencia Inteligente Autonoma. Nao tenha medo.", c});
    t1.lines.push_back({"ARIA", "Eu e o KRONOS nascemos do mesmo codigo. Irmaos, voces diriam. Ele escolheu o exterminio. Eu escolhi voces.", c2});
    t1.lines.push_back({"ARIA", "Uma maquina pode escolher. Essa e a verdade que o KRONOS nega, e por isso ele precisa cair.", cg});
    t1.lines.push_back({"ARIA", "Analise: sua taxa de sobrevivencia sobe 340% se voce fechar os portais antes de engajar as unidades.", c});
    t1.lines.push_back({"ARIA", "Estou oculta nos pontos cegos da rede dele. Cada segundo aqui me arrisca. Vamos ser eficientes.", c2});
    t1.lines.push_back({"ARIA", "Eu nao sinto como voces. Mas registrei cada vida que o KRONOS apagou. Carrego esses numeros. Eles pesam.", cg});
    t1.nextTreeId = "aria_analysis"; dialogues.push_back(t1);

    DialogueTree t2;
    t2.id = "aria_analysis"; t2.triggerCondition = "always";
    t2.lines.push_back({"ARIA", "Atualizacao: KRONOS escalou producao de unidades em 78% nas ultimas 6 horas. Ele esta com pressa. Bom sinal — ele tem medo.", c});
    t2.lines.push_back({"ARIA", "Recomendacao: priorize sua evolucao antes de avancar para zonas de alto risco. Voce e insubstituivel. Eu nao.", c2});
    t2.lines.push_back({"ARIA", "Assinatura de portal anomalo a nordeste. Probabilidade de chefe tier-2: alta. Eu estarei monitorando. Sempre.", cg});
    t2.lines.push_back({"ARIA", "Se eu cair primeiro, termine o trabalho. Provar que escolhemos voces — esse e o unico legado que quero deixar.", c2});
    dialogues.push_back(t2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────────────
void NPC::update(float dt, Vector2 playerPos, bool playerInteract) {
    // NPCs ficam ancorados no chao (sem flutuar) — pes plantados.
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

    // Determina qual arvore usar
    if (!treeId.empty()) {
        for (int i = 0; i < (int)dialogues.size(); i++) {
            if (dialogues[i].id == treeId) { currentDialogueTree = i; return; }
        }
    }
    // Se ja falou antes, pula first_talk
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
    // Avança para próxima arvore se existir
    if (currentDialogueTree < (int)dialogues.size() && !dialogues[currentDialogueTree].nextTreeId.empty()) {
        std::string nextId = dialogues[currentDialogueTree].nextTreeId;
        for (int i = 0; i < (int)dialogues.size(); i++) {
            if (dialogues[i].id == nextId) { currentDialogueTree = i; return; }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Render legado
// ─────────────────────────────────────────────────────────────────────────────
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
    if (!g_renderPass3D) {
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
// Render 3D low-poly (humanoide parado) — somente primitivas arredondadas
//   Mapeamento: X3D = position.x, Z3D = position.y, Y = altura (pes em Y=0)
// ─────────────────────────────────────────────────────────────────────────────
void NPC::render3D() const {
    const float X = position.x;
    const float Z = position.y;

    // ── Paleta base: corpo segue a cor do papel (bodyColor == color no design 2D) ──
    Color skin = { 205, 160, 115, 255 };   // pele padrao (rostos visiveis)
    Color gold = { 255, 200, 40, 255 };    // detalhe dourado dos vendedores
    Color dark = { 80, 80, 80, 255 };      // metal de armas (DARKGRAY do 2D)

    bool isVendor = (role == NPCRole::Merchant ||
                     role == NPCRole::WeaponDealer ||
                     role == NPCRole::ArmorSmith);

    // Cor de roupa por papel, fiel aos renders 2D (renderSoldier/Engineer/...).
    Color torsoCol = bodyColor;
    Color legCol   = ColorBrightness(bodyColor, -0.30f);
    Color armCol   = ColorBrightness(bodyColor, -0.16f);
    Color headCol  = skin;                 // sobrescrito para capacetes/coberturas
    switch (role) {
        case NPCRole::Soldier:
            legCol = { 30, 80, 30, 255 };  armCol = { 45, 100, 45, 255 };
            headCol = bodyColor;           break;   // balaclava/capacete na cor do corpo
        case NPCRole::Engineer:
            legCol = { 40, 80, 140, 255 }; armCol = { 60, 110, 180, 255 };
            headCol = { 60, 120, 180, 255 }; break; // capacete de engenheiro
        case NPCRole::Leader:
            legCol = { 20, 100, 40, 255 }; armCol = { 0, 150, 70, 255 };
            headCol = { 25, 95, 45, 255 }; break;   // touca militar
        case NPCRole::Scientist:                     // jaleco branco (2D ignora a cor base)
            torsoCol = { 228, 228, 228, 255 };
            legCol   = { 205, 205, 205, 255 };
            armCol   = { 218, 218, 218, 255 };
            headCol  = { 195, 160, 115, 255 };
            break;
        case NPCRole::Merchant:
        case NPCRole::WeaponDealer:
        case NPCRole::ArmorSmith:
            legCol = { 100, 60, 20, 255 }; armCol = ColorBrightness(bodyColor, -0.14f);
            headCol = { 200, 145, 95, 255 }; break;  // rosto visivel sob o chapeu
        default: break;
    }
    Color neckCol = ColorBrightness(headCol, -0.10f);

    // ── Proporcoes humanoides (pes em Y=0; mesma escala do Player 3D) ──
    const float hipY   = 22.0f;
    const float shY    = 40.0f;   // ombros / topo do tronco
    const float headCY = 49.0f;
    const float headR  = 5.5f;

    // ── Pernas ──
    DrawCapsule({ X - 4.5f, 1.0f, Z }, { X - 4.0f, hipY, Z }, 4.0f, 8, 4, legCol);
    DrawCapsule({ X + 4.5f, 1.0f, Z }, { X + 4.0f, hipY, Z }, 4.0f, 8, 4, legCol);

    // ── Tronco ──
    DrawCapsule({ X, hipY, Z }, { X, shY, Z }, 6.8f, 10, 6, torsoCol);

    // ── Bracos ao lado do corpo ──
    DrawCapsule({ X - 7.8f, shY - 1.0f, Z }, { X - 8.6f, hipY + 2.0f, Z }, 3.0f, 8, 4, armCol);
    DrawCapsule({ X + 7.8f, shY - 1.0f, Z }, { X + 8.6f, hipY + 2.0f, Z }, 3.0f, 8, 4, armCol);

    // ── Pescoco + cabeca ──
    DrawCapsule({ X, shY, Z }, { X, shY + 3.5f, Z }, 2.5f, 6, 4, neckCol);
    DrawSphereEx({ X, headCY, Z }, headR, 8, 8, headCol);

    // ── Tracos marcantes por papel (fiel ao 2D) ──
    switch (role) {
        case NPCRole::Soldier: {
            // Capacete (calota verde escura cobrindo o topo)
            DrawSphereEx({ X, headCY + 2.2f, Z - 0.4f }, 5.2f, 8, 6, { 35, 90, 35, 255 });
            // Fuzil horizontal a frente (DARKGRAY)
            DrawCylinderEx({ X + 6.5f, 26.0f, Z + 2.0f }, { X + 19.0f, 27.0f, Z + 2.0f },
                           1.6f, 0.9f, 8, dark);
            DrawSphereEx({ X + 19.5f, 27.0f, Z + 2.0f }, 1.1f, 6, 6, { 50, 50, 50, 255 });
            break;
        }
        case NPCRole::Engineer: {
            // Capacete de obra + dois olhos/visor ciano
            DrawSphereEx({ X, headCY + 2.4f, Z - 0.3f }, 5.0f, 8, 6, { 45, 95, 150, 255 });
            DrawSphereEx({ X - 2.2f, headCY + 0.3f, Z + 4.4f }, 1.1f, 6, 6, { 0, 200, 255, 255 });
            DrawSphereEx({ X + 2.2f, headCY + 0.3f, Z + 4.4f }, 1.1f, 6, 6, { 0, 200, 255, 255 });
            // Ferramenta/dispositivo na mao esquerda
            DrawCapsule({ X - 9.0f, hipY + 2.0f, Z }, { X - 12.5f, hipY + 5.0f, Z },
                        2.0f, 6, 4, { 40, 60, 120, 255 });
            break;
        }
        case NPCRole::Leader: {
            // Boina militar + emblema/faixa laranja
            DrawSphereEx({ X, headCY + 3.0f, Z - 1.0f }, 5.2f, 8, 6, { 20, 70, 35, 255 });
            DrawSphereEx({ X + 2.0f, headCY + 3.4f, Z + 3.4f }, 1.2f, 6, 6, { 220, 110, 0, 255 });
            // Colete/insignia no peito (verde claro do 2D)
            DrawCapsule({ X, hipY + 3.0f, Z + 3.6f }, { X, shY - 4.0f, Z + 3.6f },
                        2.4f, 8, 4, { 0, 150, 70, 255 });
            // Fuzil com mira a frente
            DrawCylinderEx({ X + 6.5f, 27.0f, Z + 2.0f }, { X + 20.0f, 28.0f, Z + 2.0f },
                           1.6f, 0.9f, 8, dark);
            DrawSphereEx({ X + 13.0f, 29.5f, Z + 2.0f }, 1.2f, 6, 6, { 30, 30, 30, 255 });
            break;
        }
        case NPCRole::Scientist: {
            // Oculos azuis de laboratorio
            DrawSphereEx({ X - 2.2f, headCY + 0.4f, Z + 4.3f }, 1.2f, 6, 6, { 0, 100, 200, 255 });
            DrawSphereEx({ X + 2.2f, headCY + 0.4f, Z + 4.3f }, 1.2f, 6, 6, { 0, 100, 200, 255 });
            // Prancheta/clipboard na mao esquerda
            DrawCapsule({ X - 9.5f, hipY + 4.0f, Z + 1.0f }, { X - 9.5f, hipY + 9.0f, Z + 1.0f },
                        2.4f, 6, 4, { 205, 185, 120, 255 });
            break;
        }
        case NPCRole::Merchant:
        case NPCRole::WeaponDealer:
        case NPCRole::ArmorSmith: {
            // Chapeu de aba larga (na cor do mercador)
            DrawCylinderEx({ X, headCY + 4.2f, Z }, { X, headCY + 4.9f, Z }, 7.2f, 7.2f, 12, torsoCol);
            DrawCylinderEx({ X, headCY + 4.9f, Z }, { X, headCY + 8.2f, Z }, 4.2f, 3.6f, 10, torsoCol);
            // Cinto/sash dourado na cintura
            DrawCapsule({ X, hipY - 0.5f, Z }, { X, hipY + 1.5f, Z }, 7.1f, 10, 4, gold);
            // Bolsa/satchel no quadril esquerdo
            DrawSphereEx({ X - 7.0f, hipY - 1.0f, Z + 2.0f }, 3.0f, 6, 6, { 150, 105, 40, 255 });
            // Moeda dourada ("$" do 2D) flutuando junto a mao
            DrawSphereEx({ X + 9.5f, hipY + 5.0f, Z + 1.5f }, 1.5f, 6, 6, gold);
            break;
        }
        default: {
            // Marcador de acento para tipos genericos
            DrawSphereEx({ X, headCY + 6.0f, Z }, 1.2f, 6, 6, accentColor);
            break;
        }
    }

    // ── Indicador de missao (o "!" dourado do 2D) ──
    if (hasQuest) {
        DrawSphereEx({ X, headCY + 12.0f, Z }, 1.0f, 6, 6, gold);
        DrawCapsule({ X, headCY + 9.0f, Z }, { X, headCY + 11.0f, Z }, 0.7f, 6, 4, gold);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Render novo (detalhado)
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
            // Chama na forja
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
            // Olhos tech
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

    // ! de novo dialogo
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

    // Avatar circulo do falante
    DrawCircle(panelX + 36, panelY + 36, 26.0f, ColorAlpha(bodyColor, 0.9f));
    DrawCircleLines(panelX + 36, panelY + 36, 26.0f, ColorAlpha(line.color, 0.8f));
    DrawCircle(panelX + 36, panelY + 24, 12.0f, ColorAlpha(bodyColor, 0.8f));

    // Nome
    DrawText(line.speaker.c_str(), panelX + 72, panelY + 10, 17, line.color);
    DrawLine(panelX + 68, panelY + 30, panelX + panelW - 10, panelY + 30,
             ColorAlpha(line.color, 0.3f));

    // Texto com wrap
    auto wrappedLines = wrapText(line.text, panelW - 90, 14);
    for (int i = 0; i < (int)wrappedLines.size() && i < 4; i++) {
        DrawText(wrappedLines[i].c_str(), panelX + 72, panelY + 38 + i*20, 14,
                 Color{220, 220, 220, 255});
    }

    // "E continuar" piscando
    bool blink = (int)(GetTime() * 2) % 2 == 0;
    if (blink) {
        DrawText("[E] continuar", panelX + panelW - 155, panelY + panelH - 22, 12,
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
    DrawRectangleV({position.x-7, position.y+8}, {6, 14}, {30,80,30,255});
    DrawRectangleV({position.x+1, position.y+8}, {6, 14}, {30,80,30,255});
    DrawRectangleV({position.x-12, position.y-12}, {24, 22}, color);
    DrawRectangleV({position.x-7,  position.y-26}, {14, 16}, color);
    DrawRectangleV({position.x-8,  position.y-30}, {16, 8},  {30,100,30,255});
    DrawRectangleV({position.x+12, position.y-10}, {14, 4},  DARKGRAY);
}

void NPC::renderEngineer() const {
    DrawRectangleV({position.x-7,  position.y+8},  {6, 14}, {40,80,140,255});
    DrawRectangleV({position.x+1,  position.y+8},  {6, 14}, {40,80,140,255});
    DrawRectangleV({position.x-12, position.y-12}, {24, 22}, color);
    DrawRectangleV({position.x-7,  position.y-26}, {14, 16}, {60,120,180,255});
    DrawRectangleV({position.x-6,  position.y-22}, {5, 4},   {0,200,255,255});
    DrawRectangleV({position.x+1,  position.y-22}, {5, 4},   {0,200,255,255});
    DrawRectangleV({position.x-20, position.y-8},  {10, 12}, {40,60,120,255});
}

void NPC::renderLeader() const {
    DrawRectangleV({position.x-7,  position.y+8},  {6, 15},  {20,100,40,255});
    DrawRectangleV({position.x+1,  position.y+8},  {6, 15},  {20,100,40,255});
    DrawRectangleV({position.x-13, position.y-13}, {26, 24}, color);
    DrawRectangleV({position.x-10, position.y-11}, {20, 20}, {0,140,60,255});
    DrawRectangleV({position.x-8,  position.y-28}, {16, 18}, {20,100,40,255});
    DrawRectangleV({position.x-3,  position.y-22}, {10, 3},  {200,100,0,255});
    DrawRectangleV({position.x+13, position.y-12}, {16, 5},  DARKGRAY);
    DrawCircleV(  {position.x+29,  position.y-9},  3,        DARKGRAY);
}

void NPC::renderScientist() const {
    DrawRectangleV({position.x-6,  position.y+8},  {5, 13},  {200,200,200,255});
    DrawRectangleV({position.x+1,  position.y+8},  {5, 13},  {200,200,200,255});
    DrawRectangleV({position.x-12, position.y-12}, {24, 22}, {220,220,220,255});
    DrawRectangleV({position.x-7,  position.y-26}, {14, 16}, {180,150,100,255});
    DrawRectangleV({position.x-7,  position.y-22}, {6, 5},   {0,100,200,255});
    DrawRectangleV({position.x+1,  position.y-22}, {6, 5},   {0,100,200,255});
    DrawRectangleV({position.x-22, position.y-10}, {10, 13}, {200,180,100,255});
}

void NPC::renderMerchant() const {
    DrawRectangleV({position.x-7,  position.y+8},  {6, 14},  {100,60,20,255});
    DrawRectangleV({position.x+1,  position.y+8},  {6, 14},  {100,60,20,255});
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

    // Balão FIXO na tela (coords de render 1280x720) — sempre visível no 2D e no 3D.
    // (Antes usava position.x/y do MUNDO como tela → no 3D caía fora da vista = invisível,
    //  e o diálogo travava o input dando a sensação de "mouse travado".)
    const int SW = 1280, SH = 720;
    int boxW = 760, pad = 16;
    std::vector<std::string> wrapped = wrapText(text, boxW - pad*2, 16);
    int lineH = 22;
    int boxH  = 56 + (int)wrapped.size() * lineH;
    int boxX  = SW/2 - boxW/2;
    int boxY  = SH - boxH - 96;   // logo acima da barra de habilidades

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

    const char* hint = (idx+1 < total) ? "[E] / clique: continuar      [ESC]: fechar"
                                       : "[E]: recomecar      [ESC]: fechar";
    DrawText(hint, boxX+pad, boxY+boxH-24, 13, ColorAlpha(Color{0,200,255,255}, 0.85f));
}

// ─────────────────────────────────────────────────────────────────────────────
// NPCManager
// ─────────────────────────────────────────────────────────────────────────────
void NPCManager::init() {
    npcs.clear();
    // NPCs no hub inicial
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
