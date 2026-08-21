#pragma once
#include <string>
#include <raylib.h>

enum class EquipSlot { None, Weapon, Armor, Implant };

struct Equipment {
    std::string name;
    std::string description;
    EquipSlot   slot        = EquipSlot::None;
    float       primary     = 0.0f;   // dmg for weapon, maxHP for armor, speed for implant
    float       secondary   = 0.0f;   // range for weapon, defense% for armor, xpMult for implant
    Color       color       = WHITE;
    int         tier        = 0;

    int upgradeLevel = 0;  // 0-3, each level +30% stats

    bool  isEmpty()            const { return slot == EquipSlot::None; }
    float getEffectivePrimary()   const { return primary   * (1.0f + upgradeLevel * 0.30f); }
    float getEffectiveSecondary() const { return secondary * (1.0f + upgradeLevel * 0.30f); }
    int   upgradeCost()        const {
        if (upgradeLevel >= 3) return 0;
        int costs[3] = {100, 300, 600};
        return costs[upgradeLevel];
    }
    bool  canUpgrade()         const { return !isEmpty() && upgradeLevel < 3; }
};

namespace EDB {
    // ── Armas (Tier 1) ────────────────────────────────────────────────────────
    inline Equipment pistolaPlas()    { return {"Pistola Plasma",     "Dano +15, Alc +20",   EquipSlot::Weapon,  15,  20,  {0,200,255,255},   1}; }
    inline Equipment submetMilitar()  { return {"Submetralhadora Mil","Dano +22, Alc +10",   EquipSlot::Weapon,  22,  10,  {180,200,120,255},  1}; }
    // ── Armas (Tier 2) ────────────────────────────────────────────────────────
    inline Equipment rifleEnergia()   { return {"Rifle de Energia",   "Dano +35, Alc +40",   EquipSlot::Weapon,  35,  40,  {0,255,150,255},    2}; }
    inline Equipment shotgunPlasma()  { return {"Shotgun Plasma",     "Dano +55, Alc -10",   EquipSlot::Weapon,  55, -10,  {255,150,0,255},    2}; }
    inline Equipment espadaEnergia()  { return {"Espada de Energia",  "Dano +50, Alc +25",   EquipSlot::Weapon,  50,  25,  {255,50,200,255},   2}; }
    // ── Armas (Tier 3) ────────────────────────────────────────────────────────
    inline Equipment canhaoEMP()      { return {"Canhao EMP",         "Dano +70, Alc +60",   EquipSlot::Weapon,  70,  60,  {255,200,0,255},    3}; }
    inline Equipment railgunSkynet()  { return {"Railgun Skynet",     "Dano +100, Alc +80",  EquipSlot::Weapon, 100,  80,  {200,0,255,255},    3}; }
    inline Equipment canhaoAnti()     { return {"Canhao Anti-Maquina","Dano +130, Alc +55",  EquipSlot::Weapon, 130,  55,  {255,80,0,255},     3}; }

    // ── Armaduras ─────────────────────────────────────────────────────────────
    inline Equipment coleteMilitar()  { return {"Colete Militar",     "+50 HP, Def 5%",      EquipSlot::Armor,   50,  5,   {120,120,120,255},  1}; }
    inline Equipment armaduraAvan()   { return {"Armadura Avancada",  "+120 HP, Def 15%",    EquipSlot::Armor,  120,  15,  {100,150,220,255},  2}; }
    inline Equipment exoesqueleto()   { return {"Exoesqueleto Titan", "+250 HP, Def 30%",    EquipSlot::Armor,  250,  30,  {200,200,255,255},  3}; }
    inline Equipment nanoMalha()      { return {"Nano Malha T-1000",  "+180 HP, Def 25%",    EquipSlot::Armor,  180,  25,  {0,220,200,255},    3}; }

    // ── Implants ──────────────────────────────────────────────────────────────
    inline Equipment chipVel()        { return {"Chip de Velocidade", "Vel +60",             EquipSlot::Implant, 60,  0,   {255,100,255,255},  1}; }
    inline Equipment neuralLink()     { return {"Neural Link",        "Vel +40, XP x1.5",    EquipSlot::Implant, 40,  1.5f,{150,255,200,255},  2}; }
    inline Equipment quantumCore()    { return {"Quantum Core",       "Vel +80, XP x2.0",    EquipSlot::Implant, 80,  2.0f,{255,255,100,255},  3}; }
    inline Equipment adrenChip()      { return {"Adrenal Override",   "Vel +100, XP x1.8",   EquipSlot::Implant,100,  1.8f,{255,50,100,255},   3}; }

    // Sorteio por TIER — usado pela recompensa de fim de fase. Mantido aqui pra
    // ficar junto do catalogo: quem adicionar item novo ve este sorteio na hora.
    inline Equipment randomForTier(int tier) {
        if (tier < 1) tier = 1;
        if (tier > 3) tier = 3;
        Equipment t1[] = { pistolaPlas(), submetMilitar(), coleteMilitar(), chipVel() };
        Equipment t2[] = { rifleEnergia(), armaduraAvan(), neuralLink() };
        Equipment t3[] = { exoesqueleto(), nanoMalha(), quantumCore(), adrenChip() };
        if (tier == 1) return t1[GetRandomValue(0, 3)];
        if (tier == 2) return t2[GetRandomValue(0, 2)];
        return t3[GetRandomValue(0, 3)];
    }
}
