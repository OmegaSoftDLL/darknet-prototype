#include "CraftingSystem.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>

// ─── Helpers ─────────────────────────────────────────────────────────────────

int CraftingSystem::countMaterial(const std::vector<Item>& bag, ItemType t) {
    int count = 0;
    for (const auto& it : bag) if (it.type == t) ++count;
    return count;
}

bool CraftingSystem::canCraft(const std::vector<Item>& bag, int idx) const {
    if (idx < 0 || idx >= (int)recipes.size()) return false;
    for (const auto& [itype, icount] : recipes[idx].ingredients)
        if (countMaterial(bag, itype) < icount) return false;
    return true;
}

std::vector<int> CraftingSystem::getFilteredIndices() const {
    std::vector<int> out;
    for (int i = 0; i < (int)recipes.size(); ++i) {
        if (selectedCategory == CraftCategory::All || recipes[i].category == selectedCategory)
            out.push_back(i);
    }
    return out;
}

// ─── Build Recipes ────────────────────────────────────────────────────────────

void CraftingSystem::buildRecipes() {
    recipes.clear();

    // ── ARMAS ─────────────────────────────────────────────────────────────────
    {   CraftingRecipe r; r.name="Faca de Combate"; r.category=CraftCategory::Weapons;
        r.description="Lâmina de sucata. Dano +15"; r.ingredients={{ItemType::MetalScrap,3}};
        r.resultEquip={"Faca de Combate","Dano +15",EquipSlot::Weapon,15,0,{200,200,200,255},1};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Lancador Acido"; r.category=CraftCategory::Weapons;
        r.description="Arma alien. Dano +30, Alc +20";
        r.ingredients={{ItemType::AlienCarapace,3},{ItemType::PlasmaCore,1}};
        r.resultEquip={"Lancador Acido","Dano +30, Alc +20",EquipSlot::Weapon,30,20,{80,255,60,255},2};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Rifle de Plasma"; r.category=CraftCategory::Weapons;
        r.description="Alta energia. Dano +45, Alc +50";
        r.ingredients={{ItemType::PlasmaCore,2},{ItemType::NanoFiber,2}};
        r.resultEquip={"Rifle de Plasma","Dano +45, Alc +50",EquipSlot::Weapon,45,50,{0,150,255,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Lamina Void"; r.category=CraftCategory::Weapons;
        r.description="Dano +60 | Vampirismo 10%";
        r.ingredients={{ItemType::PlasmaCore,3},{ItemType::OmegaEssence,1}};
        r.resultEquip={"Lamina Void","Dano +60",EquipSlot::Weapon,60,30,{120,0,200,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Martelo do Gelo"; r.category=CraftCategory::Weapons;
        r.description="Dano +55 | Congela inimigos";
        r.ingredients={{ItemType::MetalScrap,6},{ItemType::AlienCarapace,3}};
        r.resultEquip={"Martelo do Gelo","Dano +55, Slow AoE",EquipSlot::Weapon,55,15,{100,200,255,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Foice da Alma"; r.category=CraftCategory::Weapons;
        r.description="Dano +65 | Drena HP dos inimigos";
        r.ingredients={{ItemType::OmegaEssence,2},{ItemType::NanoFiber,3}};
        r.resultEquip={"Foice da Alma","Dano +65, Drain HP",EquipSlot::Weapon,65,25,{200,50,255,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Canhao EMP Elite"; r.category=CraftCategory::Weapons;
        r.description="Dano +80, Alc +70 | Paralisa mecas";
        r.ingredients={{ItemType::PlasmaCore,4},{ItemType::MetalScrap,5}};
        r.resultEquip={"Canhao EMP Elite","Dano +80, Alc +70",EquipSlot::Weapon,80,70,{255,200,0,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Lamina de Corrente"; r.category=CraftCategory::Weapons;
        r.description="Combo 3-hit. Dano +50 por acerto";
        r.ingredients={{ItemType::MetalScrap,4},{ItemType::NanoFiber,2}};
        r.resultEquip={"Lamina de Corrente","Dano +50, Combo x3",EquipSlot::Weapon,50,10,{255,150,50,255},2};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Cajado de Cristal"; r.category=CraftCategory::Weapons;
        r.description="Dano +40, Alc +80 | Magico";
        r.ingredients={{ItemType::AlienCarapace,4},{ItemType::PlasmaCore,2}};
        r.resultEquip={"Cajado de Cristal","Dano +40, Alc +80",EquipSlot::Weapon,40,80,{150,255,255,255},2};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="OMEGA-7 [LENDARIO]"; r.category=CraftCategory::Weapons; r.isLegendary=true;
        r.description="Arma suprema. Dano +120 | Todas as classes";
        r.ingredients={{ItemType::OmegaEssence,3},{ItemType::PlasmaCore,5},{ItemType::NanoFiber,5}};
        r.resultEquip={"OMEGA-7","Dano +120, Alc +100 [LENDARIA]",EquipSlot::Weapon,120,100,{255,215,0,255},3};
        r.isLegendary=true; recipes.push_back(r); }

    // ── ARMADURAS ─────────────────────────────────────────────────────────────
    {   CraftingRecipe r; r.name="Armadura Hibrida"; r.category=CraftCategory::Armor;
        r.description="+80 HP, Def +25%";
        r.ingredients={{ItemType::MetalScrap,5},{ItemType::AlienCarapace,2}};
        r.resultEquip={"Armadura Hibrida","+80 HP, Def 25%",EquipSlot::Armor,80,25,{60,180,100,255},2};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Nano-Armadura"; r.category=CraftCategory::Armor;
        r.description="+120 HP, Def +40%, regeneracao";
        r.ingredients={{ItemType::NanoFiber,4}};
        r.resultEquip={"Nano-Armadura","+120 HP, Def 40%, Regen",EquipSlot::Armor,120,40,{0,220,200,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Traje de Cristal"; r.category=CraftCategory::Armor;
        r.description="+150 HP, Def +30% | Reflete 10%";
        r.ingredients={{ItemType::AlienCarapace,3},{ItemType::PlasmaCore,2}};
        r.resultEquip={"Traje de Cristal","+150 HP, Def 30%",EquipSlot::Armor,150,30,{150,200,255,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Exo-Suit de Combate"; r.category=CraftCategory::Armor;
        r.description="+200 HP, Def +50% | Exoesqueleto";
        r.ingredients={{ItemType::MetalScrap,6},{ItemType::NanoFiber,4},{ItemType::PlasmaCore,2}};
        r.resultEquip={"Exo-Suit","+ 200 HP, Def 50%",EquipSlot::Armor,200,50,{180,180,255,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Armadura Bio-Regen"; r.category=CraftCategory::Armor;
        r.description="+100 HP, regeneracao 3 HP/s";
        r.ingredients={{ItemType::NanoFiber,3},{ItemType::AlienCarapace,2}};
        r.resultEquip={"Bio-Regen Suit","+100 HP, +3 regen/s",EquipSlot::Armor,100,20,{0,255,150,255},2};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="VOID CROWN [LENDARIO]"; r.category=CraftCategory::Armor; r.isLegendary=true;
        r.description="+300 HP, Def +60% | Capacete Void";
        r.ingredients={{ItemType::OmegaEssence,3},{ItemType::PlasmaCore,4},{ItemType::MetalScrap,5}};
        r.resultEquip={"Void Crown","+300 HP, Def 60% [LENDARIO]",EquipSlot::Armor,300,60,{180,0,255,255},3};
        r.isLegendary=true; recipes.push_back(r); }

    // ── ACESSORIOS ────────────────────────────────────────────────────────────
    {   CraftingRecipe r; r.name="Chip de Velocidade+"; r.category=CraftCategory::Accessories;
        r.description="Vel +80, reflexos aumentados";
        r.ingredients={{ItemType::NanoFiber,2},{ItemType::MetalScrap,2}};
        r.resultEquip={"Chip Vel+","Vel +80",EquipSlot::Implant,80,0,{255,100,200,255},2};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Neural Link Avancado"; r.category=CraftCategory::Accessories;
        r.description="Vel +60, XP x2.0";
        r.ingredients={{ItemType::NanoFiber,3},{ItemType::PlasmaCore,1}};
        r.resultEquip={"Neural Link Adv","Vel +60, XP x2.0",EquipSlot::Implant,60,2.0f,{150,255,200,255},2};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Quantum Core Elite"; r.category=CraftCategory::Accessories;
        r.description="Vel +100, XP x2.5, CD -20%";
        r.ingredients={{ItemType::PlasmaCore,3},{ItemType::NanoFiber,3}};
        r.resultEquip={"Quantum Core Elite","Vel +100, XP x2.5",EquipSlot::Implant,100,2.5f,{255,255,100,255},3};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Chip de Combate"; r.category=CraftCategory::Accessories;
        r.description="Vel +50, Dano +20%";
        r.ingredients={{ItemType::MetalScrap,3},{ItemType::AlienCarapace,1}};
        r.resultEquip={"Chip Combate","Vel +50, Dmg+20%",EquipSlot::Implant,50,1.2f,{255,80,50,255},2};
        recipes.push_back(r); }
    {   CraftingRecipe r; r.name="INFINITY CORE [LENDARIO]"; r.category=CraftCategory::Accessories; r.isLegendary=true;
        r.description="Vel +120, XP x3.0 PERMANENTE";
        r.ingredients={{ItemType::OmegaEssence,4},{ItemType::PlasmaCore,4},{ItemType::NanoFiber,4}};
        r.resultEquip={"Infinity Core","Vel +120, XP x3.0 [LENDARIO]",EquipSlot::Implant,120,3.0f,{255,215,0,255},3};
        r.isLegendary=true; recipes.push_back(r); }

    // ── CONSUMIVEIS ──────────────────────────────────────────────────────────
    {   CraftingRecipe r; r.name="MedKit"; r.category=CraftCategory::Consumables;
        r.description="Cura 50 HP imediatamente"; r.makesEquipment=false;
        r.ingredients={{ItemType::NanoFiber,1},{ItemType::MetalScrap,1}};
        r.resultItem.type=ItemType::MedKit; r.resultItem.name="MedKit";
        r.resultItem.color={0,220,100,255}; recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Nano Patch"; r.category=CraftCategory::Consumables;
        r.description="Cura 30 HP ao longo de 5s"; r.makesEquipment=false;
        r.ingredients={{ItemType::NanoFiber,2}};
        r.resultItem.type=ItemType::NanoPatch; r.resultItem.name="Nano Patch";
        r.resultItem.color={0,180,255,255}; recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Elixir de Energia"; r.category=CraftCategory::Consumables;
        r.description="Skills recarregam 2x mais rapido por 30s"; r.makesEquipment=false;
        r.ingredients={{ItemType::PlasmaCore,1},{ItemType::NanoFiber,1}};
        r.resultItem.type=ItemType::EnergyDrink; r.resultItem.name="Elixir de Energia";
        r.resultItem.color={255,200,0,255}; recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Essencia Void"; r.category=CraftCategory::Consumables;
        r.description="Invulneravel por 2s"; r.makesEquipment=false;
        r.ingredients={{ItemType::OmegaEssence,1}};
        r.resultItem.type=ItemType::VoidEssence2; r.resultItem.name="Essencia Void";
        r.resultItem.color={180,0,255,255}; recipes.push_back(r); }
    {   CraftingRecipe r; r.name="Celula de Plasma (Consumivel)"; r.category=CraftCategory::Consumables;
        r.description="Reduz cooldowns por 15s"; r.makesEquipment=false;
        r.ingredients={{ItemType::PlasmaCore,1}};
        r.resultItem.type=ItemType::PlasmaVial; r.resultItem.name="Celula Plasma";
        r.resultItem.color={0,150,255,255}; recipes.push_back(r); }

    // ── ESPADA LENDARIA ───────────────────────────────────────────────────────
    {   CraftingRecipe r; r.name="ESPADA DO EXECUTOR"; r.category=CraftCategory::Weapons; r.isLegendary=true;
        r.description="Arma lendaria. Dano +80 | +50% vs Boss";
        r.ingredients={{ItemType::OmegaEssence,1},{ItemType::MetalScrap,3},{ItemType::AlienCarapace,3}};
        r.resultEquip={"Espada do Executor","Dano +80, +50% boss [LENDARIA]",EquipSlot::Weapon,80,35,{255,215,0,255},3};
        r.isLegendary=true; recipes.push_back(r); }
}

// ─── Update ──────────────────────────────────────────────────────────────────

void CraftingSystem::update(float dt) {
    if (msgTimer > 0.f) msgTimer -= dt;
    if (crafting) {
        craftTimer -= dt;
        if (craftTimer <= 0.f) crafting = false;
    }
}

void CraftingSystem::handleInput() {
    if (!open) return;
    auto filtered = getFilteredIndices();
    int  visCount = (int)filtered.size();

    // Category change: Q/E or A/D
    if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_A)) {
        int cat = (int)selectedCategory - 1;
        if (cat < 0) cat = (int)CraftCategory::COUNT - 1;
        selectedCategory = (CraftCategory)cat;
        selected = 0; categoryScroll = 0;
    }
    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_D)) {
        int cat = ((int)selectedCategory + 1) % (int)CraftCategory::COUNT;
        selectedCategory = (CraftCategory)cat;
        selected = 0; categoryScroll = 0;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        if (selected > 0) { --selected; if (selected < categoryScroll) --categoryScroll; }
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        if (selected < visCount - 1) {
            ++selected;
            if (selected >= categoryScroll + 9) ++categoryScroll;
        }
    }
}

bool CraftingSystem::handleMouse(Vector2 m, bool clicked, std::vector<Item>& bag,
                                 int screenW, int screenH, Equipment& oe, Item& oi,
                                 bool& ge, bool& closed) {
    closed = false;
    mousePos = m;
    if (!open) return false;

    int panW = 920, panH = 580;
    int panX = screenW/2 - panW/2, panY = screenH/2 - panH/2;

    // Abas de categoria
    int tabY = panY+38, tabX = panX+12, tabW = 120, tabH = 22;
    for (int c = 0; c < (int)CraftCategory::COUNT; ++c) {
        Rectangle r = {(float)(tabX+c*tabW),(float)tabY,(float)(tabW-4),(float)tabH};
        if (clicked && CheckCollisionPointRec(m, r)) {
            selectedCategory = (CraftCategory)c; selected = 0; categoryScroll = 0;
            return false;
        }
    }

    // Lista de receitas
    auto filtered = getFilteredIndices();
    int visMax = 9, leftX = panX+12, leftW = 450, listY = panY+68+18;
    for (int vi = 0; vi < visMax; ++vi) {
        int fi = vi + categoryScroll;
        if (fi >= (int)filtered.size()) break;
        Rectangle r = {(float)leftX,(float)(listY+vi*48),(float)leftW,44.f};
        if (CheckCollisionPointRec(m, r)) {
            selected = fi;  // hover seleciona
            if (clicked) {
                double now = GetTime();
                if (lastClickIdx == fi && now - lastClickT < 0.4)  // duplo-clique fabrica
                    return tryCraft(bag, oe, oi, ge);
                lastClickIdx = fi; lastClickT = now;
            }
            break;
        }
    }

    // Botoes
    int botY = panY + panH - 46;
    Rectangle craftBtn = {(float)(panX+panW-340),(float)(botY+6),150,30};
    Rectangle closeBtn = {(float)(panX+panW-170),(float)(botY+6),150,30};
    if (clicked && CheckCollisionPointRec(m, closeBtn)) { closed = true; return false; }
    if (clicked && CheckCollisionPointRec(m, craftBtn)) { return tryCraft(bag, oe, oi, ge); }
    return false;
}

// ─── Render ──────────────────────────────────────────────────────────────────

static const char* materialName(ItemType t) {
    switch (t) {
        case ItemType::MetalScrap:    return "Sucata Metal";
        case ItemType::AlienCarapace: return "Carapaca Alien";
        case ItemType::PlasmaCore:    return "Nucleo Plasma";
        case ItemType::NanoFiber:     return "Fibra Nano";
        case ItemType::OmegaEssence:  return "Essencia Omega";
        default:                      return "?";
    }
}

static Color materialColor(ItemType t) {
    switch (t) {
        case ItemType::MetalScrap:    return {180, 180, 180, 255};
        case ItemType::AlienCarapace: return {60,  255, 80,  255};
        case ItemType::PlasmaCore:    return {0,   180, 255, 255};
        case ItemType::NanoFiber:     return {0,   220, 200, 255};
        case ItemType::OmegaEssence:  return {255, 215, 0,   255};
        default:                      return WHITE;
    }
}

static const char* categoryName(CraftCategory c) {
    switch(c) {
        case CraftCategory::All:         return "TODOS";
        case CraftCategory::Weapons:     return "ARMAS";
        case CraftCategory::Armor:       return "ARMADURA";
        case CraftCategory::Accessories: return "ACESSORIO";
        case CraftCategory::Consumables: return "CONSUMIVEL";
        default: return "?";
    }
}

void CraftingSystem::render(const std::vector<Item>& bag, int screenW, int screenH) {
    if (!open) return;

    float t = (float)GetTime();
    Color borderCol = {0, 200, 255, 255};

    // Full-screen dark overlay
    DrawRectangle(0, 0, screenW, screenH, ColorAlpha(BLACK, 0.88f));

    // Main panel
    int panW = 920, panH = 580;
    int panX = screenW / 2 - panW / 2;
    int panY = screenH / 2 - panH / 2;

    DrawRectangle(panX, panY, panW, panH, ColorAlpha(Color{6,10,20,255}, 0.97f));
    DrawRectangleLinesEx({(float)panX,(float)panY,(float)panW,(float)panH}, 2.f, ColorAlpha(borderCol,0.85f));
    int cc = 12;
    DrawLine(panX,panY+cc,panX+cc,panY,borderCol);
    DrawLine(panX+panW-cc,panY,panX+panW,panY+cc,borderCol);
    DrawLine(panX,panY+panH-cc,panX+cc,panY+panH,borderCol);
    DrawLine(panX+panW-cc,panY+panH,panX+panW,panY+panH-cc,borderCol);

    // Title
    const char* title = "[ WORKBENCH - FABRICACAO ]";
    int tw = MeasureText(title, 22);
    DrawText(title, screenW/2 - tw/2, panY + 10, 22, borderCol);

    // ── Category tabs ─────────────────────────────────────────────────────────
    int tabY = panY + 38;
    int tabX = panX + 12;
    int tabW = 120, tabH = 22;
    for (int c = 0; c < (int)CraftCategory::COUNT; ++c) {
        bool isCur = (c == (int)selectedCategory);
        Color bg  = isCur ? Color{0,60,120,220} : Color{10,16,30,150};
        Color col = isCur ? borderCol : ColorAlpha(WHITE, 0.5f);
        DrawRectangle(tabX + c*tabW, tabY, tabW-4, tabH, bg);
        if (isCur) DrawRectangleLinesEx({(float)(tabX+c*tabW),(float)tabY,(float)(tabW-4),(float)tabH},1.f,borderCol);
        int nw = MeasureText(categoryName((CraftCategory)c), 11);
        DrawText(categoryName((CraftCategory)c), tabX+c*tabW+(tabW-4)/2-nw/2, tabY+5, 11, col);
    }
    DrawText("[Q/E] mudar categoria", panX+panW-230, tabY+4, 10, ColorAlpha(WHITE,0.4f));

    DrawLine(panX+10, tabY+tabH+2, panX+panW-10, tabY+tabH+2, ColorAlpha(borderCol,0.3f));

    // ── Left panel: recipe list ────────────────────────────────────────────────
    auto filtered = getFilteredIndices();
    int visMax  = 9;
    int leftX   = panX + 12;
    int leftW   = 450;
    int listY   = panY + 68;

    DrawText("RECEITAS", leftX, listY, 13, ColorAlpha(borderCol, 0.7f));
    listY += 18;

    for (int vi = 0; vi < visMax; ++vi) {
        int fi = vi + categoryScroll;
        if (fi >= (int)filtered.size()) break;
        int  recIdx = filtered[fi];
        const auto& r = recipes[recIdx];
        bool avail   = canCraft(bag, recIdx);
        bool isSel   = (vi == selected - categoryScroll || fi == selected);

        int rowY = listY + vi * 48;
        int rowH = 44;

        Color rowBg = isSel ? ColorAlpha(Color{0,60,120,255},0.9f) : ColorAlpha(Color{10,16,30,255},0.7f);
        DrawRectangle(leftX, rowY, leftW, rowH, rowBg);

        Color rowBorder = r.isLegendary ? Color{255,215,0,255}
                        : avail         ? Color{0,210,80,255}
                                        : Color{80,30,30,255};
        float borderAlpha = isSel ? (0.6f+0.4f*sinf(t*4.f)) : 0.4f;
        DrawRectangleLinesEx({(float)leftX,(float)rowY,(float)leftW,(float)rowH},
                              isSel?2.f:1.f, ColorAlpha(rowBorder,borderAlpha));

        // Legendary glow
        Color nameCol = r.isLegendary ? ColorAlpha(Color{255,240,100,255}, 0.6f+0.4f*sinf(t*4.f))
                      : avail         ? WHITE
                                      : Color{140,140,140,255};
        DrawText(r.name.c_str(), leftX+8, rowY+5, 13, nameCol);
        DrawText(r.description.c_str(), leftX+8, rowY+22, 10, ColorAlpha(WHITE,avail?0.7f:0.4f));

        // Status badge
        if (avail) {
            DrawRectangle(leftX+leftW-72, rowY+5, 64, 16, ColorAlpha(Color{0,180,60,255},0.25f));
            DrawText("POSSIVEL", leftX+leftW-70, rowY+8, 10, Color{0,220,80,255});
        } else {
            DrawRectangle(leftX+leftW-60, rowY+5, 52, 16, ColorAlpha(Color{180,30,30,255},0.25f));
            DrawText("FALTA", leftX+leftW-58, rowY+8, 10, Color{200,60,60,255});
        }
    }

    // Scrollbar hint
    if ((int)filtered.size() > visMax) {
        int total = (int)filtered.size();
        DrawText(TextFormat("%d/%d", (int)filtered.size(), (int)recipes.size()),
                 leftX, listY + visMax*48 + 4, 10, ColorAlpha(WHITE,0.4f));
    }

    // ── Right panel: materials + selected detail ───────────────────────────────
    int rightX = panX + 12 + leftW + 20;
    int rightY = panY + 68;
    int rightW = panW - leftW - 44;

    DrawLine(rightX-10, panY+62, rightX-10, panY+panH-54, ColorAlpha(borderCol,0.25f));
    DrawText("MATERIAIS", rightX, rightY, 13, ColorAlpha(borderCol,0.7f));
    rightY += 18;

    static const ItemType matTypes[] = {
        ItemType::MetalScrap, ItemType::AlienCarapace,
        ItemType::PlasmaCore, ItemType::NanoFiber, ItemType::OmegaEssence
    };
    for (int i = 0; i < 5; ++i) {
        ItemType mt  = matTypes[i];
        int      cnt = countMaterial(bag, mt);
        Color    mc  = materialColor(mt);
        int my = rightY + i * 46;
        int mh = 42, mw = rightW - 8;
        DrawRectangle(rightX, my, mw, mh, ColorAlpha(Color{10,16,30,255},0.7f));
        DrawRectangleLinesEx({(float)rightX,(float)my,(float)mw,(float)mh},1.f,ColorAlpha(mc,0.35f));
        DrawCircleV({(float)(rightX+16),(float)(my+mh/2)}, 8.f, ColorAlpha(mc,0.9f));
        DrawText(materialName(mt), rightX+32, my+5, 12, ColorAlpha(mc,0.95f));
        Color cntCol = cnt > 0 ? WHITE : Color{100,100,100,255};
        DrawText(TextFormat("x%d", cnt), rightX+32, my+22, 13, cntCol);
    }

    // Selected recipe detail
    {
        int detY = rightY + 5*46 + 12;
        DrawLine(rightX, detY-4, rightX+rightW-8, detY-4, ColorAlpha(borderCol,0.2f));

        auto visFiltered = getFilteredIndices();
        if (selected >= 0 && selected < (int)visFiltered.size()) {
            int recIdx = visFiltered[selected];
            const auto& r = recipes[recIdx];
            DrawText("SELECIONADO:", rightX, detY, 11, ColorAlpha(borderCol,0.65f));
            Color nameC = r.isLegendary ? Color{255,215,0,255} : WHITE;
            DrawText(r.name.c_str(), rightX, detY+14, 13, nameC);
            for (int j = 0; j < (int)r.ingredients.size(); ++j) {
                auto [itype,icount] = r.ingredients[j];
                int have = countMaterial(bag, itype);
                bool ok  = (have >= icount);
                DrawText(TextFormat("%s: %d/%d", materialName(itype), have, icount),
                         rightX, detY+32+j*14, 11, ok?Color{0,210,80,255}:Color{210,60,60,255});
            }
            // Result preview
            if (r.makesEquipment) {
                DrawText(TextFormat("Resultado: %s", r.resultEquip.name.c_str()),
                         rightX, detY+90, 11, ColorAlpha(Color{0,200,255,255},0.8f));
            }
        }
    }

    // ── Bottom bar ────────────────────────────────────────────────────────────
    int botY = panY + panH - 46;
    DrawLine(panX+10, botY, panX+panW-10, botY, ColorAlpha(borderCol,0.25f));

    if (crafting) {
        float pct   = 1.f - (craftTimer / 1.5f);
        float pulse = 0.5f + 0.5f*sinf(t*10.f);
        DrawText("  FABRICANDO...", panX+12, botY+10, 18, ColorAlpha(Color{255,220,0,255},pulse));
        int barW = panW - 24;
        DrawRectangle(panX+12, botY+30, barW, 8, ColorAlpha(Color{30,30,60,255},0.8f));
        DrawRectangle(panX+12, botY+30, (int)(barW*pct), 8, ColorAlpha(Color{0,200,255,255},0.9f));
    } else {
        DrawText("Mouse: clique nas abas e receitas  -  duplo-clique fabrica",
                 panX+12, botY+10, 12, ColorAlpha(WHITE,0.5f));
        if (msgTimer > 0.f)
            DrawText(lastMsg.c_str(), panX+12, botY+28, 13,
                     ColorAlpha(Color{0,220,100,255}, std::min(msgTimer,1.f)));

        // Botoes clicaveis CRAFTAR / FECHAR
        Rectangle craftBtn = {(float)(panX+panW-340),(float)(botY+6),150,30};
        Rectangle closeBtn = {(float)(panX+panW-170),(float)(botY+6),150,30};
        auto vf = getFilteredIndices();
        bool canC = (selected>=0 && selected<(int)vf.size() && canCraft(bag, vf[selected]));
        bool ch = CheckCollisionPointRec(mousePos, craftBtn);
        bool xh = CheckCollisionPointRec(mousePos, closeBtn);
        Color cCol = canC ? (ch?Color{0,255,150,255}:Color{0,210,120,255}) : Color{90,90,90,255};
        DrawRectangleRec(craftBtn, ColorAlpha(cCol, ch?0.35f:0.20f));
        DrawRectangleLinesEx(craftBtn, ch?2.5f:1.5f, cCol);
        DrawText("CRAFTAR", (int)craftBtn.x+34, (int)craftBtn.y+7, 17, cCol);
        DrawRectangleRec(closeBtn, ColorAlpha(xh?Color{255,120,120,255}:Color{200,80,80,255}, xh?0.3f:0.18f));
        DrawRectangleLinesEx(closeBtn, xh?2.5f:1.5f, Color{255,120,120,255});
        DrawText("FECHAR", (int)closeBtn.x+40, (int)closeBtn.y+7, 17, Color{255,160,160,255});
    }
}

// ─── Try Craft ───────────────────────────────────────────────────────────────

bool CraftingSystem::tryCraft(std::vector<Item>& bag, Equipment& outEquip,
                               Item& outItem, bool& gotEquip) {
    auto filtered = getFilteredIndices();
    if (selected < 0 || selected >= (int)filtered.size()) return false;
    int recIdx = filtered[selected];
    if (!canCraft(bag, recIdx)) {
        lastMsg = "MATERIAIS INSUFICIENTES!"; msgTimer = 2.5f; return false;
    }
    if (crafting) return false;

    crafting = true; craftTimer = 1.5f; craftingIdx = recIdx;

    const auto& r = recipes[recIdx];
    for (const auto& [itype,icount] : r.ingredients) {
        int toRemove = icount;
        for (auto it = bag.begin(); it != bag.end() && toRemove > 0;) {
            if (it->type == itype) { it = bag.erase(it); --toRemove; } else ++it;
        }
    }
    gotEquip = r.makesEquipment;
    if (gotEquip) outEquip = r.resultEquip; else outItem = r.resultItem;
    lastMsg = "FABRICADO: " + r.name + "!"; msgTimer = 3.f;
    return true;
}
