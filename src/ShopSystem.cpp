#include "ShopSystem.h"
#include <raylib.h>
#include <cmath>

// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
//  buildShop the€” monta catalog of items temA¡ticos by type of NPC
// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€

void ShopSystem::buildShop(int npcIdx, const std::string& npcName) {
    npcIndex = npcIdx;
    selected = 0;
    items.clear();
    (void)npcName; // poderia filtrar by nome, mas usamos catA¡soon complete

    // the”€the”€ Armas the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    {
        ShopItem si;
        si.name        = "Pistola Plasma";
        si.description = "Weapon light of plasma. Damage +15, Range +20";
        si.price       = 300;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::pistolaPlas();
        si.color       = {0, 200, 255, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name        = "Rifle Energia";
        si.description = "Rifle of high precisao. Damage +35, Range +40";
        si.price       = 650;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::rifleEnergia();
        si.color       = {0, 255, 150, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name        = "Lancador EMP";
        si.description = "Destroca circuitos enemies. Damage +70, Range +60";
        si.price       = 900;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::canhaoEMP();
        si.color       = {255, 200, 0, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name        = "Espingarda Quantica";
        si.description = "Dispersao quantica devastadora. Damage +55, Rng -10";
        si.price       = 1200;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::shotgunPlasma();
        si.color       = {255, 150, 0, 255};
        items.push_back(si);
    }

    // the”€the”€ Armaduras the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    {
        ShopItem si;
        si.name        = "Colete Tatico";
        si.description = "+50 HP, Def 5%. Protecao basica of combat";
        si.price       = 400;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::coleteMilitar();
        si.color       = {120, 120, 120, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name        = "Armor IRON-VIII";
        si.description = "+120 HP, Def 15%. Exosqueleto of combat";
        si.price       = 800;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::armaduraAvan();
        si.color       = {100, 150, 220, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name        = "Nano-Suit";
        si.description = "+250 HP, Def 30%. Nano-fibras auto-reparantes";
        si.price       = 1500;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::exoesqueleto();
        si.color       = {200, 200, 255, 255};
        items.push_back(si);
    }

    // the”€the”€ Implants the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    {
        ShopItem si;
        si.name        = "Neural Interface";
        si.description = "Vel +40, XP x1.5. Processamento neural aumentado";
        si.price       = 350;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::neuralLink();
        si.color       = {150, 255, 200, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name        = "Servo Boost";
        si.description = "Vel +60. Servomotores hidraulicos of high potencia";
        si.price       = 600;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::chipVel();
        si.color       = {255, 100, 255, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name        = "Shield Cortical";
        si.description = "Vel +80, XP x2.0. Barrier neural of high level";
        si.price       = 1100;
        si.isEquipment = true;
        si.isCosmetic  = false;
        si.equip       = EDB::quantumCore();
        si.color       = {255, 255, 100, 255};
        items.push_back(si);
    }

    // the”€the”€ Consumiveis the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    {
        ShopItem si;
        si.name        = "Potion of Healing";
        si.description = "Restaura 30 HP imediatamente to the usar (key F)";
        si.price       = 50;
        si.isEquipment = false;
        si.isCosmetic  = false;
        Item it{};
        it.type   = ItemType::HealthPack;
        it.name   = "Potion of Healing";
        it.color  = {0, 210, 80, 255};
        it.radius = 8.0f;
        si.item   = it;
        si.color  = {0, 210, 80, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name        = "Energy Core";
        si.description = "Ativa shield protetov by 2.5s to the usar";
        si.price       = 80;
        si.isEquipment = false;
        si.isCosmetic  = false;
        Item it{};
        it.type   = ItemType::EnergyCore;
        it.name   = "Energy Core";
        it.color  = {0, 200, 255, 255};
        it.radius = 8.0f;
        si.item   = it;
        si.color  = {0, 200, 255, 255};
        items.push_back(si);
    }

    // the”€the”€ CosmA©ticos the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    {
        ShopItem si;
        si.name          = "Red Chrome";
        si.description   = "Pintura chrome vermelha. Sem effect of combat.";
        si.price         = 200;
        si.isEquipment   = false;
        si.isCosmetic    = true;
        si.cosmeticColor = {220, 30, 30, 255};
        si.color         = {220, 30, 30, 255};
        items.push_back(si);
    }
    {
        ShopItem si;
        si.name          = "Gold Plating";
        si.description   = "Revestimento golden of luxo. Sem effect of combat.";
        si.price         = 500;
        si.isEquipment   = false;
        si.isCosmetic    = true;
        si.cosmeticColor = {255, 200, 0, 255};
        si.color         = {255, 200, 0, 255};
        items.push_back(si);
    }
}

// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
//  render the€” fullscreen cyberpunk shop
// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€

void ShopSystem::render(int playerCredits, int screenW, int screenH) const {
    if (!open) return;

    // Overlay dark
    DrawRectangle(0, 0, screenW, screenH, ColorAlpha({0, 0, 0, 255}, 0.88f));

    // Edge cyberpunk
    Color C_cyan  = {0, 210, 255, 255};
    Color C_gold  = {255, 190, 0, 255};
    Color C_dark  = {8, 10, 20, 255};
    Color C_sel   = {0, 255, 220, 255};

    int panW = screenW - 80;
    int panH = screenH - 80;
    int panX = 40;
    int panY = 40;

    // Painel of fundo
    DrawRectangle(panX, panY, panW, panH, ColorAlpha(C_dark, 0.97f));
    DrawRectangleLinesEx({(float)panX,(float)panY,(float)panW,(float)panH}, 2, C_cyan);

    // Cantos angulares cyberpunk
    int c = 12;
    DrawLine(panX, panY+c, panX+c, panY, C_cyan);
    DrawLine(panX+panW-c, panY, panX+panW, panY+c, C_cyan);
    DrawLine(panX, panY+panH-c, panX+c, panY+panH, C_cyan);
    DrawLine(panX+panW-c, panY+panH, panX+panW, panY+panH-c, C_cyan);

    // TA­tulo
    const char* title = "SHOP // RESISTANCE";
    int tw = MeasureText(title, 32);
    DrawText(title, screenW/2 - tw/2, panY + 16, 32, C_cyan);

    // Line separadora
    DrawLine(panX+20, panY+58, panX+panW-20, panY+58, ColorAlpha(C_cyan, 0.4f));

    // Subtitulo
    DrawText("CATALOG DE EQUIPAMENTOS E SUPRIMENTOS", panX+24, panY+64, 12,
             ColorAlpha(C_cyan, 0.5f));

    // the”€the”€ List of items (column left) the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    int listX   = panX + 20;
    int listY   = panY + 86;
    int listW   = (int)(panW * 0.55f);
    int listH   = panH - 120;
    int rowH    = 36;
    int maxVis  = listH / rowH;

    // Scroll offset to manter selected visA­vel
    int scrollOff = 0;
    if (selected >= maxVis) scrollOff = selected - maxVis + 1;

    for (int i = 0; i < (int)items.size() && i < maxVis + scrollOff; ++i) {
        int idx = i;
        if (idx < scrollOff) continue;
        int visRow = idx - scrollOff;
        if (visRow >= maxVis) break;

        int ry = listY + visRow * rowH;
        const ShopItem& si = items[idx];

        bool isSel = (idx == selected);

        // Highlight of the selected
        if (isSel) {
            DrawRectangle(listX, ry, listW - 4, rowH - 2, ColorAlpha({0,80,60,255}, 0.5f));
            DrawRectangleLinesEx({(float)listX,(float)ry,(float)(listW-4),(float)(rowH-2)},
                                 1.5f, C_sel);
        } else {
            DrawRectangle(listX, ry, listW - 4, rowH - 2, ColorAlpha({15,20,30,255}, 0.4f));
        }

        // Indicador of color / type
        DrawRectangle(listX + 4, ry + 6, 6, rowH - 14, si.color);

        // Nome of the item
        Color nameCol = isSel ? C_sel : WHITE;
        DrawText(si.name.c_str(), listX + 18, ry + 6, 15, nameCol);

        // Tag of type
        const char* tag = si.isCosmetic ? "[COS]" :
                          si.isEquipment ?
                            (si.equip.slot == EquipSlot::Weapon  ? "[ARM]" :
                             si.equip.slot == EquipSlot::Armor   ? "[DEF]" : "[IMP]")
                          : "[ITM]";
        Color C_item = {0, 210, 80, 255};
        Color tagCol = si.isCosmetic ? C_gold :
                       si.isEquipment ? C_cyan : C_item;
        DrawText(tag, listX + 18, ry + rowH - 18, 11, ColorAlpha(tagCol, 0.8f));

        // PreA§the (yellow golden)
        const char* priceStr = TextFormat("%d cr", si.price);
        int pw = MeasureText(priceStr, 14);
        bool canAfford = (playerCredits >= si.price);
        Color priceCol = canAfford ? C_gold : Color{180,60,60,255};
        DrawText(priceStr, listX + listW - pw - 20, ry + 10, 14, priceCol);
    }

    // Barra of scroll side (if necessA¡rio)
    if ((int)items.size() > maxVis) {
        int sbX = listX + listW - 10;
        int sbH = listH;
        float pct = (float)scrollOff / (float)((int)items.size() - maxVis);
        DrawRectangle(sbX, listY, 4, sbH, ColorAlpha(C_cyan, 0.15f));
        int thumbH = sbH * maxVis / (int)items.size();
        int thumbY = listY + (int)((sbH - thumbH) * pct);
        DrawRectangle(sbX, thumbY, 4, thumbH, ColorAlpha(C_cyan, 0.6f));
    }

    // the”€the”€ Painel direito: descriA§A£the + stats the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    int detX = panX + listW + 30;
    int detY = panY + 86;
    int detW = panW - listW - 50;
    int detH = panH - 140;

    DrawRectangle(detX, detY, detW, detH, ColorAlpha({5,10,25,255}, 0.7f));
    DrawRectangleLinesEx({(float)detX,(float)detY,(float)detW,(float)detH}, 1, ColorAlpha(C_cyan, 0.35f));

    if (selected >= 0 && selected < (int)items.size()) {
        const ShopItem& sel = items[selected];

        // Nome of the item selected
        int snw = MeasureText(sel.name.c_str(), 20);
        DrawText(sel.name.c_str(), detX + detW/2 - snw/2, detY + 14, 20, sel.color);

        // Line separadora interna
        DrawLine(detX+12, detY+40, detX+detW-12, detY+40, ColorAlpha(sel.color, 0.3f));

        // DescriA§A£the
        DrawText(sel.description.c_str(), detX + 12, detY + 50, 13, ColorAlpha(WHITE, 0.85f));

        // EstatA­sticas of the equipment
        if (sel.isEquipment) {
            const Equipment& eq = sel.equip;
            int sy = detY + 80;

            const char* slotName =
                eq.slot == EquipSlot::Weapon  ? "SLOT: ARMA"    :
                eq.slot == EquipSlot::Armor   ? "SLOT: ARMOR" : "SLOT: IMPLANT";
            DrawText(slotName, detX + 12, sy, 13, ColorAlpha(C_cyan, 0.8f));
            sy += 20;

            DrawText(TextFormat("TIER: %d", eq.tier), detX + 12, sy, 13,
                     ColorAlpha(C_gold, 0.8f));
            sy += 20;

            if (eq.slot == EquipSlot::Weapon) {
                DrawText(TextFormat("DMG:    +%.0f", eq.primary),   detX+12, sy, 13, {0,255,100,255}); sy+=18;
                DrawText(TextFormat("RANGE:  +%.0f", eq.secondary), detX+12, sy, 13, {0,200,255,255}); sy+=18;
            } else if (eq.slot == EquipSlot::Armor) {
                DrawText(TextFormat("HP:    +%.0f",  eq.primary),   detX+12, sy, 13, {0,255,100,255}); sy+=18;
                DrawText(TextFormat("DEF:   +%.0f%%", eq.secondary),detX+12, sy, 13, {100,200,255,255}); sy+=18;
            } else { // Implant
                DrawText(TextFormat("SPEED:   +%.0f",  eq.primary),   detX+12, sy, 13, {255,100,255,255}); sy+=18;
                if (eq.secondary > 0.0f)
                    DrawText(TextFormat("XP x%.1f", eq.secondary),  detX+12, sy, 13, {255,220,80,255}); sy+=18;
            }
        } else if (sel.isCosmetic) {
            DrawText("EFEITO: COSMETICO", detX+12, detY+80, 13, C_gold);
            DrawText("Sem bonus of combat.", detX+12, detY+100, 12, ColorAlpha(WHITE,0.6f));
            DrawText("Muda the color of the character.", detX+12, detY+118, 12, ColorAlpha(WHITE,0.6f));

            // Preview of the color
            DrawRectangle(detX+12, detY+140, 40, 40, sel.cosmeticColor);
            DrawRectangleLinesEx({(float)(detX+12),(float)(detY+140), 40,40}, 1.5f, WHITE);
            DrawText("COR", detX+56, detY+154, 12, ColorAlpha(WHITE,0.7f));
        } else {
            // Consumivel
            DrawText("TIPO: CONSUMISPEED", detX+12, detY+80, 13, {0,210,80,255});
            if (sel.item.type == ItemType::HealthPack)
                DrawText("Restaura 30 HP to the collect.", detX+12, detY+100, 12, {0,255,80,255});
            else if (sel.item.type == ItemType::EnergyCore)
                DrawText("Ativa shield by 2.5s to the collect.", detX+12, detY+100, 12, {0,200,255,255});
        }

        // PreA§the in destaque
        DrawLine(detX+12, detY+detH-60, detX+detW-12, detY+detH-60, ColorAlpha(C_gold, 0.3f));
        bool canAfford = (playerCredits >= sel.price);
        Color pCol = canAfford ? C_gold : Color{200,50,50,255};
        DrawText(TextFormat("PRICE: %d cr", sel.price), detX+12, detY+detH-50, 16, pCol);
        if (!canAfford)
            DrawText("CREDITS INSUFICIENTES", detX+12, detY+detH-28, 12, {200,60,60,255});
    }

    // the”€the”€ CrA©ditos of the player (canto direito lower) the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    int credW = 240;
    int credX = panX + panW - credW - 10;
    int credY = panY + panH - 48;
    DrawRectangle(credX, credY, credW, 36, ColorAlpha({0,20,10,255}, 0.85f));
    DrawRectangleLinesEx({(float)credX,(float)credY,(float)credW,36}, 1, C_gold);
    DrawText(TextFormat("CREDITS: %d", playerCredits), credX + 12, credY + 10, 16, C_gold);

    // the”€the”€ Botoes clicaveis: BUY / FECHAR the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    Rectangle buyBtn   = {(float)(screenW/2-170),(float)(panY+panH-58),150,32};
    Rectangle closeBtn = {(float)(screenW/2+20), (float)(panY+panH-58),150,32};
    bool buyHover   = CheckCollisionPointRec(mousePos, buyBtn);
    bool closeHover = CheckCollisionPointRec(mousePos, closeBtn);
    bool canBuy = (selected>=0 && selected<(int)items.size() && playerCredits>=items[selected].price);
    Color buyCol = canBuy ? (buyHover?Color{0,255,150,255}:Color{0,210,120,255}) : Color{90,90,90,255};
    DrawRectangleRec(buyBtn, ColorAlpha(buyCol, buyHover?0.35f:0.20f));
    DrawRectangleLinesEx(buyBtn, buyHover?2.5f:1.5f, buyCol);
    DrawText("BUY", (int)buyBtn.x+30, (int)buyBtn.y+8, 18, buyCol);
    DrawRectangleRec(closeBtn, ColorAlpha(closeHover?Color{255,120,120,255}:Color{200,80,80,255}, closeHover?0.3f:0.18f));
    DrawRectangleLinesEx(closeBtn, closeHover?2.5f:1.5f, Color{255,120,120,255});
    DrawText("FECHAR", (int)closeBtn.x+40, (int)closeBtn.y+8, 18, Color{255,160,160,255});

    // the”€the”€ Controles (rodapA©) the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    const char* ctrl = "Mouse: passe the cursor and click  -  double-click in the item purchase  -  [TAB] close";
    int cw = MeasureText(ctrl, 12);
    DrawText(ctrl, screenW/2 - cw/2, panY + panH - 20, 12, ColorAlpha(C_cyan, 0.6f));

    // the”€the”€ Message of purchase the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
    if (buyMsgTimer > 0.0f) {
        float alpha = buyMsgTimer < 0.5f ? buyMsgTimer / 0.5f : 1.0f;
        int mw = MeasureText(buyMsg.c_str(), 22);
        int mx = screenW/2 - mw/2;
        int my = screenH/2 - 50;
        DrawRectangle(mx - 16, my - 8, mw + 32, 40, ColorAlpha({0,30,20,255}, 0.92f));
        DrawRectangleLinesEx({(float)(mx-16),(float)(my-8),(float)(mw+32),40}, 1.5f,
                             ColorAlpha({0,255,150,255}, alpha));
        DrawText(buyMsg.c_str(), mx, my, 22, ColorAlpha({0,255,150,255}, alpha));
    }
}

// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
//  handleInput the€” navegaA§A£the in the list
// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€

void ShopSystem::handleInput() {
    if (!open) return;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) {
        selected = (selected > 0) ? selected - 1 : (int)items.size() - 1;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        selected = (selected < (int)items.size() - 1) ? selected + 1 : 0;
    }
}

bool ShopSystem::handleMouse(Vector2 m, bool clicked, int screenW, int screenH,
                             int& cr, Equipment& oe, Item& oi,
                             bool& ge, bool& gc, Color& co, bool& closed) {
    closed = false;
    mousePos = m;
    if (!open) return false;

    int panW = screenW-80, panH = screenH-80, panX = 40, panY = 40;
    int listX = panX+20, listY = panY+86, listW = (int)(panW*0.55f), listH = panH-120, rowH = 36;
    int maxVis = listH/rowH; if (maxVis < 1) maxVis = 1;
    int scrollOff = (selected >= maxVis) ? selected - maxVis + 1 : 0;

    // Hover/click in the lines of the list
    for (int visRow = 0; visRow < maxVis; ++visRow) {
        int idx = visRow + scrollOff;
        if (idx >= (int)items.size()) break;
        Rectangle r = {(float)listX,(float)(listY+visRow*rowH),(float)(listW-4),(float)(rowH-2)};
        if (CheckCollisionPointRec(m, r)) {
            selected = idx;  // hover seleciona
            if (clicked) {
                double now = GetTime();
                if (lastClickIdx == idx && now - lastClickT < 0.4) // double-click purchase
                    return tryBuy(cr, oe, oi, ge, gc, co);
                lastClickIdx = idx; lastClickT = now;
            }
            break;
        }
    }

    // Botoes
    Rectangle buyBtn   = {(float)(screenW/2-170),(float)(panY+panH-58),150,32};
    Rectangle closeBtn = {(float)(screenW/2+20), (float)(panY+panH-58),150,32};
    if (clicked && CheckCollisionPointRec(m, closeBtn)) { closed = true; return false; }
    if (clicked && CheckCollisionPointRec(m, buyBtn))   { return tryBuy(cr, oe, oi, ge, gc, co); }
    return false;
}

// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
//  tryBuy the€” tenta buy item selected
// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€

bool ShopSystem::tryBuy(int& playerCredits, Equipment& outEquip, Item& outItem,
                        bool& gotEquip, bool& gotCosmetic, Color& cosmeticOut) {
    if (!open) return false;
    if (selected < 0 || selected >= (int)items.size()) return false;

    const ShopItem& si = items[selected];
    if (playerCredits < si.price) return false;

    playerCredits -= si.price;

    gotEquip    = false;
    gotCosmetic = false;

    if (si.isCosmetic) {
        gotCosmetic  = true;
        cosmeticOut  = si.cosmeticColor;
        hasCosmeticColor = true;
        playerColor  = si.cosmeticColor;
    } else if (si.isEquipment) {
        gotEquip = true;
        outEquip = si.equip;
    } else {
        outItem = si.item;
    }

    buyMsg      = "Comprado: " + si.name + "!";
    buyMsgTimer = 2.0f;
    return true;
}

// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
//  update the€” decrementa timer of the message of purchase
// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€

void ShopSystem::update(float dt) {
    if (buyMsgTimer > 0.0f) buyMsgTimer -= dt;
}

// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€
//  close
// the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€the”€

void ShopSystem::close() {
    open     = false;
    npcIndex = -1;
    selected = 0;
    items.clear();
}

