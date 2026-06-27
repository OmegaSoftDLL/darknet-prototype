#include "Item.h"
#include "Effects.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

extern bool g_renderPass3D;

Item Item::createRandom(Vector2 pos) {
    int roll = GetRandomValue(0, 5);
    Item item;
    item.position = pos;
    // Weighted: common drops more often
    if      (roll <= 1) item.type = ItemType::ScrapMetal;
    else if (roll == 2) item.type = ItemType::HealthPack;
    else if (roll == 3) item.type = ItemType::WeaponPart;
    else if (roll == 4) item.type = ItemType::EnergyCore;
    else                item.type = ItemType::PlasmaCell;

    switch (item.type) {
        case ItemType::EnergyCore:
            item.name   = "Nucleo de Energia";
            item.color  = {0, 255, 255, 255};
            item.rarity = ItemRarity::Uncommon;
            item.value  = 25;
            break;
        case ItemType::ScrapMetal:
            item.name   = "Sucata Metalica";
            item.color  = {160,160,170,255};
            item.rarity = ItemRarity::Common;
            item.value  = 8;
            break;
        case ItemType::WeaponPart:
            item.name   = "Peca de Arma";
            item.color  = {255,130,0,255};
            item.rarity = ItemRarity::Uncommon;
            item.value  = 20;
            break;
        case ItemType::HealthPack:
            item.name   = "Kit de Reparo";
            item.color  = {0,210,80,255};
            item.rarity = ItemRarity::Common;
            item.value  = 15;
            break;
        case ItemType::PlasmaCell:
            item.name   = "Celula de Plasma";
            item.color  = {180,0,255,255};
            item.rarity = ItemRarity::Uncommon;
            item.value  = 30;
            break;
        default: break;
    }
    return item;
}

Item Item::createCredits(Vector2 pos, int amount) {
    Item item;
    item.position = pos;
    item.type     = ItemType::Credits;
    item.name     = "Creditos";
    item.color    = {255,210,0,255};
    item.rarity   = ItemRarity::Common;
    item.value    = amount;
    item.radius   = 6.0f;
    return item;
}

Item Item::createTech(Vector2 pos) {
    Item item;
    item.position = pos;
    item.type     = ItemType::TechChip;
    item.name     = "Chip de Tecnologia";
    item.color    = {0,200,255,255};
    item.rarity   = ItemRarity::Rare;
    item.value    = 80;
    item.radius   = 9.0f;
    return item;
}

Item Item::createEliteDrop(Vector2 pos) {
    Item item;
    item.position = pos;
    item.type     = ItemType::NanoCore;
    item.name     = "NanoCore Quantum";
    item.color    = {255,80,200,255};
    item.rarity   = ItemRarity::Epic;
    item.value    = 200;
    item.radius   = 11.0f;
    return item;
}

static void setupItemByType(Item& item) {
    switch (item.type) {
        // ── Existing base items ──
        case ItemType::EnergyCore:     item.name="Nucleo de Energia"; item.color={0,255,255,255}; item.value=25; break;
        case ItemType::ScrapMetal:     item.name="Sucata Metalica"; item.color={160,160,170,255}; item.value=8; break;
        case ItemType::MetalScrap:     item.name="Sucata de Metal"; item.color={140,140,150,255}; item.value=6; break;
        case ItemType::WeaponPart:     item.name="Peca de Arma"; item.color={255,130,0,255}; item.value=20; break;
        case ItemType::HealthPack:     item.name="Kit de Reparo"; item.color={0,210,80,255}; item.value=15; break;
        case ItemType::TechChip:       item.name="Chip de Tecnologia"; item.color={0,200,255,255}; item.value=80; item.radius=9.0f; break;
        case ItemType::NanoCore:       item.name="NanoCore Quantum"; item.color={255,80,200,255}; item.value=200; item.radius=11.0f; break;
        case ItemType::PlasmaCell:     item.name="Celula de Plasma"; item.color={180,0,255,255}; item.value=30; break;
        case ItemType::Credits:        item.name="Creditos"; item.color={255,210,0,255}; item.value=50; item.radius=6.0f; break;
        case ItemType::AlienCarapace:  item.name="Carapaca Alienigena"; item.color={50,200,50,255}; item.value=40; break;
        case ItemType::PlasmaCore:     item.name="Nucleo de Plasma"; item.color={255,80,0,255}; item.value=60; break;
        case ItemType::NanoFiber:      item.name="Fibra Nano"; item.color={0,200,180,255}; item.value=45; break;
        case ItemType::OmegaEssence:   item.name="Essencia OMEGA"; item.color={200,0,255,255}; item.value=500; item.radius=12.0f; break;
        // ── Weapons ──
        case ItemType::PlasmaRifle:    item.name="Rifle de Plasma"; item.color={0,180,255,255}; item.value=120; item.radius=10.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::VoidBlade:      item.name="Lamina do Void"; item.color={80,0,180,255}; item.value=160; item.radius=10.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::CrystalStaff:   item.name="Cajado de Cristal"; item.color={160,220,255,255}; item.value=140; item.radius=10.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::NanoBow:        item.name="Arco Nano"; item.color={0,220,180,255}; item.value=110; item.radius=10.0f; item.rarity=ItemRarity::Uncommon; break;
        case ItemType::FrostHammer:    item.name="Martelo de Gelo"; item.color={180,230,255,255}; item.value=180; item.radius=11.0f; item.rarity=ItemRarity::Epic; break;
        case ItemType::AcidGun:        item.name="Pistola de Acido"; item.color={100,220,0,255}; item.value=100; item.radius=10.0f; item.rarity=ItemRarity::Uncommon; break;
        case ItemType::SoulScythe:     item.name="Foice da Alma"; item.color={100,0,80,255}; item.value=220; item.radius=11.0f; item.rarity=ItemRarity::Epic; break;
        case ItemType::ChainBlade:     item.name="Lamina de Corrente"; item.color={180,140,0,255}; item.value=150; item.radius=11.0f; item.rarity=ItemRarity::Rare; break;
        // ── Armor ──
        case ItemType::NanoSuit:       item.name="Nano Suit"; item.color={0,180,220,255}; item.value=100; item.radius=11.0f; item.rarity=ItemRarity::Uncommon; break;
        case ItemType::CrystalArmor:   item.name="Armadura de Cristal"; item.color={140,200,255,255}; item.value=160; item.radius=11.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::VoidPlating:    item.name="Blindagem Void"; item.color={60,0,120,255}; item.value=140; item.radius=11.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::DragonScale:    item.name="Escama de Dragao"; item.color={200,80,0,255}; item.value=200; item.radius=12.0f; item.rarity=ItemRarity::Epic; break;
        case ItemType::PhaseCloak:     item.name="Manto de Fase"; item.color={120,0,200,255}; item.value=130; item.radius=11.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::IronBastionArmor: item.name="Armadura Bastiao"; item.color={120,130,140,255}; item.value=240; item.radius=12.0f; item.rarity=ItemRarity::Epic; break;
        case ItemType::BioRegenSuit:   item.name="Traje Bio-Regen"; item.color={0,200,100,255}; item.value=170; item.radius=11.0f; item.rarity=ItemRarity::Rare; break;
        // ── Accessories ──
        case ItemType::QuantumCore:    item.name="Nucleo Quantico"; item.color={200,150,0,255}; item.value=180; item.radius=10.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::SoulCrystal:    item.name="Cristal da Alma"; item.color={160,0,120,255}; item.value=150; item.radius=10.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::VoidFragment:   item.name="Fragmento Void"; item.color={40,0,80,255}; item.value=130; item.radius=10.0f; item.rarity=ItemRarity::Uncommon; break;
        case ItemType::NanoChip:       item.name="Nano Chip"; item.color={0,200,220,255}; item.value=90; item.radius=9.0f; item.rarity=ItemRarity::Uncommon; break;
        case ItemType::TimePiece:      item.name="Relogio do Tempo"; item.color={220,200,0,255}; item.value=220; item.radius=11.0f; item.rarity=ItemRarity::Epic; break;
        case ItemType::FrostRune:      item.name="Runa de Gelo"; item.color={160,220,255,255}; item.value=140; item.radius=10.0f; item.rarity=ItemRarity::Rare; break;
        case ItemType::PlasmaCell2:    item.name="Celula de Plasma+"; item.color={200,50,255,255}; item.value=160; item.radius=10.0f; item.rarity=ItemRarity::Rare; break;
        // ── Consumables ──
        case ItemType::MedKit:         item.name="Kit Medico"; item.color={0,220,80,255}; item.value=60; break;
        case ItemType::EnergyDrink:    item.name="Bebida Energetica"; item.color={255,220,0,255}; item.value=40; break;
        case ItemType::NanoPatch:      item.name="Nano Curativo"; item.color={0,180,160,255}; item.value=50; break;
        case ItemType::VoidEssence2:   item.name="Essencia Void"; item.color={100,0,160,255}; item.value=80; break;
        case ItemType::FrostCrystal2:  item.name="Cristal de Gelo"; item.color={180,230,255,255}; item.value=70; break;
        case ItemType::PlasmaVial:     item.name="Vial de Plasma"; item.color={180,80,255,255}; item.value=65; break;
        case ItemType::SoulFragment2:  item.name="Fragmento da Alma"; item.color={160,60,200,255}; item.value=100; break;
        // ── Legendaries ──
        case ItemType::OmegaWeapon:    item.name="OMEGA WEAPON"; item.color={255,50,0,255}; item.value=1000; item.radius=13.0f; item.rarity=ItemRarity::Omega; break;
        case ItemType::VoidCrown:      item.name="Coroa do Void"; item.color={120,0,255,255}; item.value=800; item.radius=13.0f; item.rarity=ItemRarity::Legendary; break;
        case ItemType::InfinityCore:   item.name="Nucleo Infinito"; item.color={255,200,0,255}; item.value=700; item.radius=12.0f; item.rarity=ItemRarity::Legendary; break;
        case ItemType::DragonSlayer:   item.name="Mata-Dragoes"; item.color={255,100,0,255}; item.value=900; item.radius=13.0f; item.rarity=ItemRarity::Legendary; break;
        default: item.name="Item Desconhecido"; item.color={180,180,180,255}; item.value=10; break;
    }
}

Item Item::createWeaponDrop(Vector2 pos) {
    // Pick a random weapon/armor/accessory from extended list
    static const ItemType weaponPool[] = {
        ItemType::PlasmaRifle, ItemType::VoidBlade, ItemType::CrystalStaff,
        ItemType::NanoBow, ItemType::AcidGun, ItemType::ChainBlade,
        ItemType::NanoSuit, ItemType::CrystalArmor, ItemType::VoidPlating,
        ItemType::PhaseCloak, ItemType::BioRegenSuit,
        ItemType::QuantumCore, ItemType::SoulCrystal, ItemType::VoidFragment,
        ItemType::NanoChip, ItemType::FrostRune
    };
    int idx = GetRandomValue(0, 15);
    Item item;
    item.position = pos;
    item.type     = weaponPool[idx];
    setupItemByType(item);
    item.rarity = rollRarity(false);
    item.applyRarityBonus();
    item.rollAffixes();            // afixos + nome composto + bônus
    item.isNew = true;
    // Itens raros ostentam o feixe por mais tempo
    item.dropBeamTimer = 3.5f + (int)item.rarity * 1.5f;
    return item;
}

void Item::update(float dt) {
    lifetime   -= dt;
    pulseTimer += dt;
    if (pickupDelay  > 0.0f) pickupDelay  -= dt;
    if (dropBeamTimer > 0.0f) dropBeamTimer -= dt;
}

// ── Static rarity helpers ──────────────────────────────────────────────────

ItemRarity Item::rollRarity(bool fromOmegaBoss) {
    if (fromOmegaBoss) return ItemRarity::Omega;
    int r = GetRandomValue(0, 9999);
    if (r >= 9990) return ItemRarity::Legendary; // top 0.1% → ~0.9% (generous)
    if (r >= 9600) return ItemRarity::Epic;       // 9600–9989 = 3.9% ≈ 4%
    if (r >= 8600) return ItemRarity::Rare;       // 8600–9599 = 10%
    if (r >= 6100) return ItemRarity::Uncommon;   // 6100–8599 = 25%
    return ItemRarity::Common;                    // 0–6099 = 61%
}

Color Item::rarityToColor(ItemRarity r) {
    switch (r) {
        case ItemRarity::Common:    return {180,180,180,255};
        case ItemRarity::Uncommon:  return {80,220,80,255};
        case ItemRarity::Rare:      return {60,130,255,255};
        case ItemRarity::Epic:      return {180,0,255,255};
        case ItemRarity::Legendary: return {255,140,0,255};
        case ItemRarity::Omega:     return {220,20,20,255};
        default:                    return WHITE;
    }
}

const char* Item::rarityToName(ItemRarity r) {
    switch (r) {
        case ItemRarity::Common:    return "Comum";
        case ItemRarity::Uncommon:  return "Incomum";
        case ItemRarity::Rare:      return "Raro";
        case ItemRarity::Epic:      return "Epico";
        case ItemRarity::Legendary: return "LENDARIO";
        case ItemRarity::Omega:     return "!! OMEGA !!";
        default:                    return "";
    }
}

void Item::applyRarityBonus() {
    rarityColor = rarityToColor(rarity);
    float mult = 1.0f;
    switch (rarity) {
        case ItemRarity::Uncommon:  mult = 1.25f; break;
        case ItemRarity::Rare:      mult = 1.60f; break;
        case ItemRarity::Epic:      mult = 2.20f; break;
        case ItemRarity::Legendary: mult = 3.50f; break;
        case ItemRarity::Omega:     mult = 6.00f; break;
        default: break;
    }
    value = (int)(value * mult);
}

// ── Afixos estilo Diablo/PoE — nome composto + bônus numéricos por raridade ──
void Item::rollAffixes() {
    baseName = name;
    int count = (int)rarity;          // Common 0 .. Omega 5
    affixCount = count;
    if (count <= 0) return;

    // stat: 0=dmg 1=hp 2=spd 3=def 4=crit 5=vamp
    struct Affix { const char* word; int stat; float val; };
    static const Affix kPrefix[] = {
        {"Flamejante",0,12}, {"Afiado",0,9},  {"Brutal",0,16},  {"Reforcado",3,8},
        {"Blindado",3,12},   {"Veloz",2,22},  {"Vital",1,45},   {"Preciso",4,8},
        {"Sanguinario",5,9}, {"Furioso",0,13},{"Cromado",3,10}, {"Eletrico",4,10}
    };
    static const Affix kSuffix[] = {
        {"do Tita",1,65},      {"da Furia",0,15},   {"do Vento",2,26},
        {"do Guardiao",3,11},  {"da Precisao",4,11},{"do Vampiro",5,13},
        {"da Tempestade",0,12},{"do Colosso",1,90}, {"da Sombra",2,18},
        {"do Executor",0,18}
    };
    const int NP = (int)(sizeof(kPrefix)/sizeof(kPrefix[0]));
    const int NS = (int)(sizeof(kSuffix)/sizeof(kSuffix[0]));
    float rmult = 1.0f + count * 0.35f;   // bônus maior em raridades altas

    auto apply = [&](int stat, float v) {
        v *= rmult;
        switch (stat) {
            case 0: bonusDamage    += v; break;
            case 1: bonusHealth    += v; break;
            case 2: bonusSpeed     += v; break;
            case 3: bonusDefense   += v; break;
            case 4: bonusCrit      += v; break;
            case 5: bonusVampirism += v; break;
        }
    };

    if (count >= 1) {
        const Affix& a = kPrefix[GetRandomValue(0, NP-1)];
        affixPrefix = a.word; apply(a.stat, a.val);
    }
    if (count >= 2) {
        const Affix& a = kSuffix[GetRandomValue(0, NS-1)];
        affixSuffix = a.word; apply(a.stat, a.val);
    }
    // Afixos extras (Epic+) — bônus ocultos adicionais
    for (int i = 2; i < count; ++i) {
        if (GetRandomValue(0,1)) { const Affix& a = kPrefix[GetRandomValue(0,NP-1)]; apply(a.stat, a.val*0.6f); }
        else                     { const Affix& a = kSuffix[GetRandomValue(0,NS-1)]; apply(a.stat, a.val*0.6f); }
    }

    // Compõe o nome final: "[Prefixo] Base [Sufixo]"
    std::string composed = baseName;
    if (!affixPrefix.empty()) composed = affixPrefix + " " + composed;
    if (!affixSuffix.empty()) composed = composed + " " + affixSuffix;
    name = composed;
}

// ── Drop beam (call from world render, inside camera space) ───────────────

void Item::drawDropEffect() const {
    if (dropBeamTimer <= 0.0f || rarity < ItemRarity::Uncommon) return;

    const float kPI2 = 3.14159265f;
    float alpha = std::min(dropBeamTimer / 2.0f, 1.0f);
    Color col   = rarityColor;

    float beamW = 8.0f  + (int)rarity * 6.0f;
    float beamH = 200.0f + (int)rarity * 80.0f;

    // Vertical light beam — gradient from item upward
    for (int i = 0; i < 8; i++) {
        float t       = (float)i / 8.0f;
        float segH    = beamH / 8.0f;
        float segAlpha= alpha * (1.0f - t) * 0.55f;
        float segW    = beamW * (1.0f - t * 0.7f);
        DrawRectangle(
            (int)(position.x - segW * 0.5f),
            (int)(position.y - i * segH - segH),
            (int)segW, (int)segH,
            ColorAlpha(col, segAlpha));
    }

    // Glow at origin
    DrawCircle((int)position.x, (int)position.y, beamW * 0.8f,
               ColorAlpha(col, alpha * 0.45f));
    DrawCircle((int)position.x, (int)position.y, beamW * 0.35f,
               ColorAlpha(WHITE, alpha * 0.7f));

    // Particles rising along beam
    float t2 = (float)GetTime();
    for (int p = 0; p < 5; p++) {
        float px = position.x + std::sin(t2 * 2.0f + p * 1.2f) * beamW * 0.55f;
        float py = position.y - std::fmod(t2 * 55.0f + p * 38.0f, beamH);
        DrawCircle((int)px, (int)py, 2.5f, ColorAlpha(col, alpha * 0.85f));
    }

    // Legendary / Omega extras
    if (rarity >= ItemRarity::Legendary) {
        // Expanding shockwave ring at spawn
        float shockR = (2.5f - dropBeamTimer) * 150.0f;
        if (shockR > 0 && shockR < 380.0f) {
            float shockA = (1.0f - shockR / 380.0f) * alpha;
            DrawCircleLines((int)position.x, (int)position.y, shockR,
                            ColorAlpha(col, shockA));
        }

        // Raios caindo do alto sobre o item (drama de lendário)
        for (int b = 0; b < 3; ++b) {
            float phase = std::fmod(t2 * 1.7f + b * 0.7f, 1.0f);
            if (phase > 0.35f) continue;                 // raio "pisca"
            float boltA = alpha * (1.0f - phase / 0.35f);
            float topY  = position.y - beamH;
            float bx    = position.x + std::sin(t2 * 3.0f + b) * beamW * 1.2f;
            Vector2 prev = { bx, topY };
            for (int s = 1; s <= 6; ++s) {
                float ty = topY + (position.y - topY) * (s / 6.0f);
                float jit = (float)GetRandomValue(-7, 7);
                Vector2 nxt = { position.x + (bx - position.x) * (1.0f - s/6.0f) + jit, ty };
                DrawLineEx(prev, nxt, 2.0f, ColorAlpha({255,240,180,255}, boltA));
                prev = nxt;
            }
        }

        // Omega: anel arco-íris girando (sensação de item supremo)
        if (rarity == ItemRarity::Omega) {
            for (int i = 0; i < 12; ++i) {
                float a   = t2 * 2.0f + i * (3.14159f / 6.0f);
                float hue = std::fmod(t2 * 90.0f + i * 30.0f, 360.0f);
                Color rc  = ColorFromHSV(hue, 0.9f, 1.0f);
                float rr  = beamW * 1.6f;
                DrawCircleV({position.x + std::cos(a) * rr, position.y + std::sin(a) * rr},
                            3.0f, ColorAlpha(rc, alpha * 0.9f));
            }
        }

        // Floating tier label above beam
        const char* lbl = (rarity == ItemRarity::Omega) ? "!! OMEGA !!" : "!! LENDARIO !!";
        int tw = MeasureText(lbl, 16);
        float labelY = position.y - beamH * 0.55f;
        float labelAlpha = alpha * (0.6f + 0.4f * std::sin(t2 * 4.0f));
        DrawText(lbl, (int)(position.x) - tw / 2, (int)labelY,
                 16, ColorAlpha(col, labelAlpha));
    }
}

void Item::render() const {
    const float kPI    = 3.14159265f;
    float pulse        = std::sin(pulseTimer * 3.5f);
    float pulse2       = std::sin(pulseTimer * 5.0f + 1.0f);
    float alpha        = (lifetime < 3.0f) ? lifetime / 3.0f : 1.0f;
    Vector2 pos        = position;
    int ix             = (int)pos.x;
    int iy             = (int)pos.y;

    // ---- Ground shadow ------------------------------------------------
    if (!g_renderPass3D) {
        DrawCircleV({pos.x, pos.y + 2.0f}, radius * 1.1f, ColorAlpha({0,0,0,255}, 0.35f * alpha));
    }

    // ---- Rarity outer glow ring ---------------------------------------
    float glowR = radius;
    switch (rarity) {
        case ItemRarity::Common:
            glowR = radius + 4.0f + pulse * 1.5f;
            DrawCircleV(pos, glowR, ColorAlpha(color, 0.18f * alpha));
            break;
        case ItemRarity::Uncommon:
            glowR = radius + 6.0f + pulse * 2.5f;
            DrawCircleV(pos, glowR, ColorAlpha(color, 0.25f * alpha));
            DrawCircleLines(ix, iy, glowR, ColorAlpha(color, 0.45f * alpha));
            break;
        case ItemRarity::Rare:
            glowR = radius + 10.0f + pulse * 4.0f;
            DrawCircleV(pos, glowR,        ColorAlpha(color, 0.22f * alpha));
            DrawCircleV(pos, glowR - 4.0f, ColorAlpha(color, 0.30f * alpha));
            DrawCircleLines(ix, iy, glowR, ColorAlpha(color, 0.60f * alpha));
            {
                float a = pulseTimer * 3.0f;
                DrawCircleV({pos.x + std::cos(a)*glowR*0.7f,
                             pos.y + std::sin(a)*glowR*0.7f},
                            2.5f, ColorAlpha(WHITE, 0.8f * alpha));
            }
            break;
        case ItemRarity::Epic:
            glowR = radius + 14.0f + pulse * 5.0f;
            DrawCircleV(pos, glowR,        ColorAlpha(color, 0.20f * alpha));
            DrawCircleV(pos, glowR - 5.0f, ColorAlpha(color, 0.28f * alpha));
            DrawCircleLines(ix, iy, glowR, ColorAlpha(color, 0.70f * alpha));
            for (int i = 0; i < 2; ++i) {
                float a = pulseTimer * 4.0f + i * kPI;
                DrawCircleV({pos.x + std::cos(a)*(glowR*0.65f),
                             pos.y + std::sin(a)*(glowR*0.65f)},
                            3.0f, ColorAlpha(WHITE, 0.9f * alpha));
            }
            break;
        case ItemRarity::Legendary: {
            glowR = radius + 18.0f + pulse * 7.0f;
            Color legCol = {255,140,0,255};
            DrawCircleV(pos, glowR,        ColorAlpha(legCol, 0.22f * alpha));
            DrawCircleV(pos, glowR - 6.0f, ColorAlpha(legCol, 0.32f * alpha));
            DrawCircleLines(ix, iy, glowR, ColorAlpha(legCol, 0.85f * alpha));
            // 3 orbiting dots
            for (int i = 0; i < 3; ++i) {
                float a = pulseTimer * 3.5f + i * (2.0f * kPI / 3.0f);
                DrawCircleV({pos.x + std::cos(a)*(glowR*0.75f),
                             pos.y + std::sin(a)*(glowR*0.75f)},
                            3.5f, ColorAlpha({255,220,80,255}, 0.95f * alpha));
            }
            break;
        }
        case ItemRarity::Omega: {
            glowR = radius + 24.0f + pulse * 9.0f;
            Color omgCol = {220,20,20,255};
            DrawCircleV(pos, glowR,        ColorAlpha(omgCol, 0.25f * alpha));
            DrawCircleV(pos, glowR - 8.0f, ColorAlpha(omgCol, 0.35f * alpha));
            DrawCircleLines(ix, iy, glowR,        ColorAlpha(omgCol, 0.90f * alpha));
            DrawCircleLines(ix, iy, glowR + 5.0f, ColorAlpha({255,80,0,255}, 0.5f * alpha));
            for (int i = 0; i < 4; ++i) {
                float a = pulseTimer * 5.0f + i * (kPI / 2.0f);
                DrawCircleV({pos.x + std::cos(a)*(glowR*0.8f),
                             pos.y + std::sin(a)*(glowR*0.8f)},
                            4.5f, ColorAlpha({255,50,0,255}, alpha));
            }
            break;
        }
    }

    // ==================================================================
    // Per-type distinct visual
    // ==================================================================
    switch (type) {

    // ------------------------------------------------------------------
    // WeaponPart — mini-arma: cano + coronha + muzzle neon
    // ------------------------------------------------------------------
    case ItemType::WeaponPart: {
        Color wCol  = {255, 130, 0, 255};
        Color muzz  = {255, 220, 80, 255};
        // Coronha (retangulo esquerdo)
        Rectangle stock = { pos.x - 10.0f, pos.y - 3.5f, 8.0f, 7.0f };
        DrawRectangleRec(stock, ColorAlpha({90,60,30,255}, alpha));
        DrawRectangleLinesEx(stock, 1.0f, ColorAlpha(wCol, 0.7f * alpha));
        // Receiver (retangulo central)
        Rectangle recv = { pos.x - 3.0f, pos.y - 3.0f, 7.0f, 6.0f };
        DrawRectangleRec(recv, ColorAlpha({60,50,50,255}, alpha));
        DrawRectangleLinesEx(recv, 1.0f, ColorAlpha(wCol, alpha));
        // Cano (retangulo estreito direito)
        Rectangle barrel = { pos.x + 4.0f, pos.y - 1.5f, 8.0f, 3.0f };
        DrawRectangleRec(barrel, ColorAlpha({40,40,40,255}, alpha));
        DrawRectangleLinesEx(barrel, 1.0f, ColorAlpha(wCol, alpha));
        // Muzzle neon
        DrawGlowCircle({pos.x + 12.5f, pos.y}, 2.5f, ColorAlpha(muzz, alpha), 2.2f);
        // Trigger
        DrawLineEx({pos.x, pos.y + 3.0f}, {pos.x + 2.0f, pos.y + 5.5f},
                   1.5f, ColorAlpha(wCol, alpha));
        break;
    }

    // ------------------------------------------------------------------
    // ScrapMetal — placa de armadura: retangulo + rivets + brilho metalico
    // ------------------------------------------------------------------
    case ItemType::ScrapMetal: {
        Color mCol  = {160, 160, 170, 255};
        Color shine = {220, 220, 240, 255};
        float shiftY = pulse * 0.5f;
        // Placa principal
        Rectangle plate = { pos.x - 8.0f, pos.y - 7.0f + shiftY, 16.0f, 14.0f };
        DrawRectangleRec(plate, ColorAlpha({70,75,80,255}, alpha));
        DrawRectangleLinesEx(plate, 1.5f, ColorAlpha(mCol, alpha));
        // Brilho diagonal (highlight)
        DrawLineEx({pos.x - 6.0f, pos.y - 5.0f + shiftY},
                   {pos.x - 2.0f, pos.y - 1.0f + shiftY},
                   2.0f, ColorAlpha(shine, 0.55f * alpha));
        // Rivets (parafusos nos cantos)
        float r2 = plate.x; float t2 = plate.y;
        float corners[4][2] = {
            {r2 + 2.5f, t2 + 2.5f}, {r2 + 13.5f, t2 + 2.5f},
            {r2 + 2.5f, t2 + 11.5f},{r2 + 13.5f, t2 + 11.5f}
        };
        for (auto& c : corners)
            DrawCircleV({c[0], c[1]}, 1.5f, ColorAlpha(shine, alpha));
        break;
    }

    // ------------------------------------------------------------------
    // TechChip — chip eletrônico: quadrado + trilhas de circuito + pulsacao ciano/roxo
    // ------------------------------------------------------------------
    case ItemType::TechChip: {
        Color chipCol   = {0, 200, 255, 255};
        Color traceCol  = {120, 0, 255, 255};
        float chipPulse = 0.5f + 0.5f * std::sin(pulseTimer * 6.0f);
        // Corpo do chip
        Rectangle chip = { pos.x - 8.0f, pos.y - 8.0f, 16.0f, 16.0f };
        DrawRectangleRec(chip, ColorAlpha({10,10,30,255}, alpha));
        DrawRectangleLinesEx(chip, 1.5f, ColorAlpha(chipCol, alpha));
        // Trilhas horizontais
        for (int row = -2; row <= 2; row += 2) {
            DrawLineEx({pos.x - 7.0f, pos.y + row},
                       {pos.x + 7.0f, pos.y + row},
                       1.0f, ColorAlpha(traceCol, 0.5f * alpha));
        }
        // Trilhas verticais
        for (int col = -2; col <= 2; col += 2) {
            DrawLineEx({pos.x + col, pos.y - 7.0f},
                       {pos.x + col, pos.y + 7.0f},
                       1.0f, ColorAlpha(chipCol, 0.5f * alpha));
        }
        // Nucleo central pulsante
        DrawGlowCircle(pos, 3.0f + chipPulse,
                       ColorAlpha(chipCol, alpha), 1.8f);
        // Pinos laterais (pins)
        for (int p = -1; p <= 1; p++) {
            DrawLineEx({pos.x - 8.0f, pos.y + p * 4.0f},
                       {pos.x - 11.0f, pos.y + p * 4.0f},
                       1.5f, ColorAlpha(chipCol, alpha));
            DrawLineEx({pos.x + 8.0f, pos.y + p * 4.0f},
                       {pos.x + 11.0f, pos.y + p * 4.0f},
                       1.5f, ColorAlpha(chipCol, alpha));
        }
        break;
    }

    // ------------------------------------------------------------------
    // Credits — moeda dourada: circulo amarelo + "$" rotacionando + brilho
    // ------------------------------------------------------------------
    case ItemType::Credits: {
        Color gold   = {255, 210, 0, 255};
        Color goldDk = {180, 130, 0, 255};
        float spin   = pulseTimer * 2.5f; // moeda "girando"
        float scaleX = std::abs(std::cos(spin)); // efeito de rotacao 3D
        float coinW  = radius * scaleX;
        if (coinW < 1.0f) coinW = 1.0f;
        // Sombra da moeda
        DrawCircleV({pos.x, pos.y + 1.5f}, radius + 1.0f, ColorAlpha(goldDk, 0.5f * alpha));
        // Face da moeda (ellipse via DrawCircleV escalado em x)
        DrawCircleV(pos, radius, ColorAlpha(goldDk, alpha));
        // Face frontal (estreita quando girando)
        Rectangle face = { pos.x - coinW, pos.y - radius,
                           coinW * 2.0f, radius * 2.0f };
        DrawEllipse(ix, iy, coinW, radius, ColorAlpha(gold, alpha));
        // Brilho neon
        DrawGlowCircle(pos, radius * 0.35f, ColorAlpha({255,240,120,255}, alpha), 2.0f);
        // Simbolo "$"
        if (scaleX > 0.3f) {
            DrawText("$", ix - 4, iy - 5, 10, ColorAlpha(WHITE, scaleX * alpha));
        }
        // Valor
        {
            const char* vt = TextFormat("+%d", value);
            int tw = MeasureText(vt, 10);
            DrawText(vt, ix - tw/2, iy - (int)radius - 12,
                     10, ColorAlpha({255,220,80,255}, alpha));
        }
        break;
    }

    // ------------------------------------------------------------------
    // HealthPack — cruz vermelha + fundo escuro + brilho verde pulsante
    // ------------------------------------------------------------------
    case ItemType::HealthPack: {
        Color red    = {220, 30, 30, 255};
        Color green  = {0, 230, 100, 255};
        float gPulse = 0.5f + 0.5f * std::sin(pulseTimer * 4.0f);
        // Fundo quadrado escuro
        Rectangle bg = { pos.x - 8.0f, pos.y - 8.0f, 16.0f, 16.0f };
        DrawRectangleRec(bg, ColorAlpha({20,20,20,255}, alpha));
        DrawRectangleLinesEx(bg, 1.0f, ColorAlpha(red, 0.7f * alpha));
        // Cruz vermelha
        Rectangle crossH = { pos.x - 7.0f, pos.y - 2.5f, 14.0f, 5.0f };
        Rectangle crossV = { pos.x - 2.5f, pos.y - 7.0f, 5.0f, 14.0f };
        DrawRectangleRec(crossH, ColorAlpha(red, alpha));
        DrawRectangleRec(crossV, ColorAlpha(red, alpha));
        // Brillo verde pulsante
        DrawCircleV(pos, radius * 1.5f + gPulse * 4.0f,
                    ColorAlpha(green, 0.12f * gPulse * alpha));
        DrawCircleLines(ix, iy, radius * 1.5f + gPulse * 4.0f,
                        ColorAlpha(green, 0.4f * gPulse * alpha));
        break;
    }

    // ------------------------------------------------------------------
    // EnergyCore — hexagono brilhante azul/branco pulsando + raios
    // ------------------------------------------------------------------
    case ItemType::EnergyCore: {
        Color cyan  = {0, 255, 255, 255};
        Color white = {200, 240, 255, 255};
        float ePulse = 0.6f + 0.4f * std::sin(pulseTimer * 5.0f);
        float hexR   = radius * ePulse;
        // Hexagono (6 triangulos)
        for (int i = 0; i < 6; ++i) {
            float a0 = (i)     * (kPI / 3.0f) + pulseTimer * 0.4f;
            float a1 = (i + 1) * (kPI / 3.0f) + pulseTimer * 0.4f;
            Vector2 v0 = { pos.x + std::cos(a0) * hexR,
                           pos.y + std::sin(a0) * hexR };
            Vector2 v1 = { pos.x + std::cos(a1) * hexR,
                           pos.y + std::sin(a1) * hexR };
            DrawTriangle(pos, v0, v1, ColorAlpha(cyan, 0.35f * alpha));
            DrawLineEx(v0, v1, 1.5f, ColorAlpha(white, 0.9f * alpha));
        }
        // Glow central
        DrawGlowCircle(pos, 4.0f * ePulse, ColorAlpha(white, alpha), 2.0f);
        // Raios eletricos (4 linhas saindo do centro em angulos aleatorios fixos)
        for (int i = 0; i < 4; ++i) {
            float a   = pulseTimer * 3.0f + i * (kPI / 2.0f);
            float len = hexR * 0.8f + pulse2 * 2.0f;
            Vector2 tip = { pos.x + std::cos(a) * len,
                            pos.y + std::sin(a) * len };
            DrawLineEx(pos, tip, 1.0f, ColorAlpha(white, 0.6f * alpha));
        }
        break;
    }

    // ------------------------------------------------------------------
    // NanoCore — nucleo raro: esfera magenta pulsante + orbita
    // ------------------------------------------------------------------
    case ItemType::NanoCore: {
        Color magenta = {255, 80, 200, 255};
        float nPulse  = 0.7f + 0.3f * std::sin(pulseTimer * 4.5f);
        DrawGlowCircle(pos, radius * nPulse, ColorAlpha(magenta, alpha), 2.5f);
        // Anel orbitante duplo
        for (int ring = 0; ring < 2; ++ring) {
            float angle = pulseTimer * (2.0f + ring) + ring * (kPI / 2.0f);
            float rR    = radius * 1.6f;
            DrawCircleLines(ix, iy, rR, ColorAlpha(magenta, 0.5f * alpha));
            DrawCircleV({ pos.x + std::cos(angle) * rR,
                          pos.y + std::sin(angle) * rR },
                        2.5f, ColorAlpha(WHITE, 0.9f * alpha));
        }
        break;
    }

    // ------------------------------------------------------------------
    // PlasmaCell — celula ciano + linhas de plasma
    // ------------------------------------------------------------------
    case ItemType::PlasmaCell: {
        Color plasma = {180, 0, 255, 255};
        Color glow2  = {100, 200, 255, 255};
        float pPulse = 0.5f + 0.5f * std::sin(pulseTimer * 6.5f);
        // Corpo oval
        DrawEllipse(ix, iy, radius, radius * 0.75f,
                    ColorAlpha({40,0,60,255}, alpha));
        DrawEllipse(ix, iy, radius * 0.75f, radius * 0.55f,
                    ColorAlpha(plasma, 0.6f * alpha));
        // Linhas de plasma internas
        for (int i = 0; i < 3; ++i) {
            float a = pulseTimer * 4.0f + i * (2.0f * kPI / 3.0f);
            Vector2 p1 = { pos.x + std::cos(a) * radius * 0.5f,
                           pos.y + std::sin(a) * radius * 0.35f };
            Vector2 p2 = { pos.x - std::cos(a) * radius * 0.5f,
                           pos.y - std::sin(a) * radius * 0.35f };
            DrawLineEx(p1, p2, 1.5f, ColorAlpha(glow2, pPulse * alpha));
        }
        DrawGlowCircle(pos, 3.0f * pPulse, ColorAlpha(glow2, alpha), 1.8f);
        break;
    }

    // ------------------------------------------------------------------
    // MetalScrap — fragmento metalico cinza
    // ------------------------------------------------------------------
    case ItemType::MetalScrap: {
        float r = radius * 0.9f;
        DrawPoly(pos, 5, r, pulseTimer * 20.0f, ColorAlpha({120,120,130,255}, alpha));
        DrawPoly(pos, 5, r * 0.6f, pulseTimer * 20.0f, ColorAlpha({180,180,190,255}, alpha));
        DrawPolyLines(pos, 5, r, pulseTimer * 20.0f, ColorAlpha({220,220,230,255}, alpha * 0.8f));
        DrawGlowCircle(pos, r * 0.4f, ColorAlpha({160,160,180,255}, alpha * 0.5f), 1.5f);
        break;
    }

    // ------------------------------------------------------------------
    // AlienCarapace — placas organicas alienigenas verdes
    // ------------------------------------------------------------------
    case ItemType::AlienCarapace: {
        float pulse = 0.5f + 0.5f * std::sin(pulseTimer * 4.0f);
        DrawCircleV(pos, radius, ColorAlpha({20,60,20,255}, alpha));
        DrawCircleV(pos, radius * 0.75f, ColorAlpha({30,150,50,255}, alpha));
        for (int i = 0; i < 5; ++i) {
            float a = i * (2.0f * kPI / 5.0f) + pulseTimer;
            float ox = std::cos(a) * radius * 0.55f;
            float oy = std::sin(a) * radius * 0.55f;
            DrawCircleV({pos.x + ox, pos.y + oy}, 2.5f,
                        ColorAlpha({80, 220, 80, 255}, pulse * alpha));
        }
        DrawGlowCircle(pos, radius * 0.4f * pulse, ColorAlpha({0,200,60,255}, alpha), 1.6f);
        break;
    }

    // ------------------------------------------------------------------
    // PlasmaCore — nucleo de plasma laranja incandescente
    // ------------------------------------------------------------------
    case ItemType::PlasmaCore: {
        float pulse = 0.5f + 0.5f * std::sin(pulseTimer * 7.0f);
        DrawCircleV(pos, radius, ColorAlpha({60,20,0,255}, alpha));
        DrawCircleV(pos, radius * 0.7f, ColorAlpha({255,80,0,255}, alpha));
        DrawCircleV(pos, radius * 0.4f * (0.8f + 0.2f * pulse),
                    ColorAlpha({255,200,80,255}, alpha));
        DrawGlowCircle(pos, radius * pulse, ColorAlpha({255,120,0,255}, alpha), 2.0f);
        break;
    }

    // ------------------------------------------------------------------
    // NanoFiber — fios nanotecnologicos turquesa
    // ------------------------------------------------------------------
    case ItemType::NanoFiber: {
        float pulse = 0.5f + 0.5f * std::sin(pulseTimer * 5.5f);
        DrawCircleV(pos, radius * 0.6f, ColorAlpha({0,50,50,255}, alpha));
        for (int i = 0; i < 4; ++i) {
            float a = i * (kPI / 2.0f) + pulseTimer * 2.0f;
            Vector2 p1 = {pos.x, pos.y};
            Vector2 p2 = {pos.x + std::cos(a) * radius,
                          pos.y + std::sin(a) * radius};
            DrawLineEx(p1, p2, 2.0f, ColorAlpha({0,200,180,255}, pulse * alpha));
        }
        DrawCircleV(pos, 3.0f, ColorAlpha({0,255,220,255}, alpha));
        DrawGlowCircle(pos, radius * 0.5f * pulse, ColorAlpha({0,200,180,255}, alpha), 1.5f);
        break;
    }

    // ------------------------------------------------------------------
    // OmegaEssence — essencia roxa pulsante
    // ------------------------------------------------------------------
    case ItemType::OmegaEssence: {
        float pulse = 0.5f + 0.5f * std::sin(pulseTimer * 8.0f);
        float r = radius * (0.85f + 0.15f * pulse);
        DrawCircleV(pos, r, ColorAlpha({60,0,80,255}, alpha));
        DrawCircleV(pos, r * 0.65f, ColorAlpha({160,0,220,255}, alpha));
        DrawCircleV(pos, r * 0.3f, ColorAlpha({255,100,255,255}, alpha));
        for (int i = 0; i < 6; ++i) {
            float a = i * (kPI / 3.0f) + pulseTimer * 3.0f;
            float ox = std::cos(a) * r * 0.75f;
            float oy = std::sin(a) * r * 0.75f;
            DrawCircleV({pos.x + ox, pos.y + oy}, 1.5f + pulse,
                        ColorAlpha({220,80,255,255}, pulse * alpha));
        }
        DrawGlowCircle(pos, r * pulse, ColorAlpha({180,0,255,255}, alpha), 2.2f);
        break;
    }

    } // end switch(type)

    // ---- Name label (uncommon+) com cor da raridade + bônus principal -----
    if (rarity >= ItemRarity::Uncommon) {
        Color labelCol = rarityToColor(rarity);
        std::string displayName = (rarity >= ItemRarity::Legendary)
            ? (std::string("[") + rarityToName(rarity) + "] " + name)
            : name;
        int tw = MeasureText(displayName.c_str(), 10);
        // Fundo escuro para legibilidade
        DrawRectangle(ix - tw/2 - 3, iy - (int)radius - 16, tw + 6, 13,
                      ColorAlpha(BLACK, 0.55f * alpha));
        DrawText(displayName.c_str(), ix - tw/2, iy - (int)radius - 14,
                 10, ColorAlpha(labelCol, alpha));

        // Linha de bônus principal (Epic+) — mostra a vantagem do afixo
        if (rarity >= ItemRarity::Epic) {
            std::string stat;
            if      (bonusDamage    > 0) stat = TextFormat("+%.0f DANO", bonusDamage);
            else if (bonusHealth    > 0) stat = TextFormat("+%.0f HP",   bonusHealth);
            else if (bonusDefense   > 0) stat = TextFormat("+%.0f DEF",  bonusDefense);
            else if (bonusSpeed     > 0) stat = TextFormat("+%.0f VEL",  bonusSpeed);
            if (!stat.empty()) {
                int sw = MeasureText(stat.c_str(), 9);
                DrawText(stat.c_str(), ix - sw/2, iy - (int)radius - 28, 9,
                         ColorAlpha({255,230,150,255}, alpha));
            }
        }
    }
}

// ============================================================================
// render3D() — low-poly 3D model floating slightly over the ground.
// World mapping: X3D = position.x, Z3D = position.y, Y = height.
// ONLY rounded primitives are used (no cubes).
// ============================================================================
void Item::render3D() const {
    const float kPI = 3.14159265f;
    float t      = (float)GetTime();
    // Gentle sinusoidal bob between ~8 and ~14 of height.
    float baseY  = 11.0f + std::sin(t * 2.0f) * 3.0f;
    float spin   = t * 1.2f;
    Vector3 base = { position.x, baseY, position.y };

    // Whether this is a high-rarity drop (Rare and above) -> translucent halo.
    bool isRare  = (rarity >= ItemRarity::Rare);

    // ── Per-type model selection ───────────────────────────────────────────
    switch (type) {

    // HealthPack / MedKit — capsule with a bright cross.
    case ItemType::HealthPack:
    case ItemType::MedKit: {
        Color shell = {230, 235, 245, 255};
        Color cross = {235, 40, 40, 255};
        Vector3 top = { base.x, base.y + 3.0f, base.z };
        Vector3 bot = { base.x, base.y - 3.0f, base.z };
        DrawCapsule(top, bot, 3.6f, 10, 10, shell);
        // Red cross — two thin perpendicular capsules on the front face.
        float zf = base.z;
        DrawCapsule({base.x - 2.4f, base.y, zf}, {base.x + 2.4f, base.y, zf},
                    0.9f, 6, 6, cross);
        DrawCapsule({base.x, base.y - 2.4f, zf}, {base.x, base.y + 2.4f, zf},
                    0.9f, 6, 6, cross);
        break;
    }

    // Credits / coins — flat golden cylinder, slowly spinning.
    case ItemType::Credits: {
        Color gold     = {255, 205, 60, 255};
        Color goldEdge = {200, 150, 20, 255};
        float wob = std::sin(spin) * 0.6f;
        Vector3 c0 = { base.x, base.y - 0.8f, base.z };
        Vector3 c1 = { base.x, base.y + 0.8f + wob, base.z };
        DrawCylinderEx(c0, c1, 4.2f, 4.2f, 14, gold);
        DrawCylinderEx(c0, c1, 4.4f, 4.4f, 14, ColorAlpha(goldEdge, 0.6f));
        // tiny center stud
        DrawSphere(base, 1.4f, ColorAlpha({255,245,200,255}, 0.9f));
        break;
    }

    // Crafting materials — colored spheres / capsules using the material color.
    case ItemType::MetalScrap:
    case ItemType::AlienCarapace:
    case ItemType::PlasmaCore:
    case ItemType::NanoFiber:
    case ItemType::OmegaEssence: {
        // Capsule core with a brighter inner sphere, tinted by item color.
        Vector3 top = { base.x, base.y + 2.5f, base.z };
        Vector3 bot = { base.x, base.y - 2.5f, base.z };
        DrawCapsule(top, bot, 3.0f, 10, 10, color);
        Color inner = { (unsigned char)std::min(255, color.r + 60),
                        (unsigned char)std::min(255, color.g + 60),
                        (unsigned char)std::min(255, color.b + 60), 255 };
        DrawSphereEx(base, 2.0f, 8, 8, ColorAlpha(inner, 0.85f));
        break;
    }

    // Default — sphere with the item's own color, plus a subtle core highlight.
    default: {
        DrawSphereEx(base, 3.4f, 10, 10, color);
        Color hi = { (unsigned char)std::min(255, color.r + 70),
                     (unsigned char)std::min(255, color.g + 70),
                     (unsigned char)std::min(255, color.b + 70), 255 };
        DrawSphereEx({ base.x, base.y + 0.8f, base.z }, 1.6f, 8, 8, ColorAlpha(hi, 0.8f));
        break;
    }
    }

    // ── Rare-drop translucent glow sphere ──────────────────────────────────
    if (isRare) {
        float pulse  = 0.5f + 0.5f * std::sin(t * 3.0f);
        float glowR  = 5.5f + pulse * 1.5f;
        DrawSphereEx(base, glowR, 10, 10, ColorAlpha(rarityColor, 0.18f + 0.10f * pulse));
    }
}
