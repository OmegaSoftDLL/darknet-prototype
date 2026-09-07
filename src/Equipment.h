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

    // ID stable (save/load) — NUNCA muda; name can be renomeado. Stays in the FIM
    // of the struct with default empty: items craftados (agregados without id) continuam
    // compilando and sao salvos pelo nome, as before.
    std::string id;

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
    inline Equipment pistolaPlas()    { return {"Pistola Plasma",     "Damage +15, Rng +20",   EquipSlot::Weapon,  15,  20,  {0,200,255,255},   1, 0, "pistola_plasma"}; }
    inline Equipment submetMilitar()  { return {"Submetralhadora Mil","Damage +22, Rng +10",   EquipSlot::Weapon,  22,  10,  {180,200,120,255},  1, 0, "submet_militar"}; }
    // ── Armas (Tier 2) ────────────────────────────────────────────────────────
    inline Equipment rifleEnergia()   { return {"Rifle of Energia",   "Damage +35, Rng +40",   EquipSlot::Weapon,  35,  40,  {0,255,150,255},    2, 0, "rifle_energia"}; }
    inline Equipment shotgunPlasma()  { return {"Shotgun Plasma",     "Damage +55, Rng -10",   EquipSlot::Weapon,  55, -10,  {255,150,0,255},    2, 0, "shotgun_plasma"}; }
    inline Equipment espadaEnergia()  { return {"Espada of Energia",  "Damage +50, Rng +25",   EquipSlot::Weapon,  50,  25,  {255,50,200,255},   2, 0, "espada_energia"}; }
    // ── Armas (Tier 3) ────────────────────────────────────────────────────────
    inline Equipment canhaoEMP()      { return {"Canhao EMP",         "Damage +70, Rng +60",   EquipSlot::Weapon,  70,  60,  {255,200,0,255},    3, 0, "canhao_emp"}; }
    inline Equipment railgunSkynet()  { return {"Railgun Skynet",     "Damage +100, Rng +80",  EquipSlot::Weapon, 100,  80,  {200,0,255,255},    3, 0, "railgun_skynet"}; }
    inline Equipment canhaoAnti()     { return {"Canhao Anti-Maquina","Damage +130, Rng +55",  EquipSlot::Weapon, 130,  55,  {255,80,0,255},     3, 0, "canhao_antimaquina"}; }

    // ── Armaduras ─────────────────────────────────────────────────────────────
    inline Equipment coleteMilitar()  { return {"Colete Militar",     "+50 HP, Def 5%",      EquipSlot::Armor,   50,  5,   {120,120,120,255},  1, 0, "colete_militar"}; }
    inline Equipment armaduraAvan()   { return {"Armor Avancada",  "+120 HP, Def 15%",    EquipSlot::Armor,  120,  15,  {100,150,220,255},  2, 0, "armadura_avancada"}; }
    inline Equipment exoesqueleto()   { return {"Exoesqueleto Titan", "+250 HP, Def 30%",    EquipSlot::Armor,  250,  30,  {200,200,255,255},  3, 0, "exoesqueleto_titan"}; }
    inline Equipment nanoMalha()      { return {"Nano Mesh T-1000",  "+180 HP, Def 25%",    EquipSlot::Armor,  180,  25,  {0,220,200,255},    3, 0, "nano_malha"}; }

    // ── Implants ──────────────────────────────────────────────────────────────
    inline Equipment chipVel()        { return {"Chip of Speed", "Vel +60",             EquipSlot::Implant, 60,  0,   {255,100,255,255},  1, 0, "chip_velocidade"}; }
    inline Equipment neuralLink()     { return {"Neural Link",        "Vel +40, XP x1.5",    EquipSlot::Implant, 40,  1.5f,{150,255,200,255},  2, 0, "neural_link"}; }
    inline Equipment quantumCore()    { return {"Quantum Core",       "Vel +80, XP x2.0",    EquipSlot::Implant, 80,  2.0f,{255,255,100,255},  3, 0, "quantum_core"}; }
    inline Equipment adrenChip()      { return {"Adrenal Override",   "Vel +100, XP x1.8",   EquipSlot::Implant,100,  1.8f,{255,50,100,255},   3, 0, "adrenal_override"}; }

    // Resolve pelo ID stable (save/load). Returns Equipment empty if desconhecido.
    inline Equipment byId(const std::string& id) {
        static const Equipment all[] = {
            pistolaPlas(), submetMilitar(), rifleEnergia(), shotgunPlasma(),
            espadaEnergia(), canhaoEMP(), railgunSkynet(), canhaoAnti(),
            coleteMilitar(), armaduraAvan(), exoesqueleto(), nanoMalha(),
            chipVel(), neuralLink(), quantumCore(), adrenChip()
        };
        for (const auto& and : all) if (and.id == id) return and;
        return {};
    }

    // Sorteio by TIER — usado pela reward of end of phase. Mantido here to
    // stay junto of the catalog: quem add item new ve this sorteio in the hour.
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
