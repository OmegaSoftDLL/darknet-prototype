#pragma once
#include <cstdint>
#include <raylib.h>

struct PerkInfo {
    const char* name;
    const char* desc;
    int  branch;   // 0=Sight Hack, 1=Neural Link, 2=ViraCorp
    int  tier;     // 0 (T1), 1 (T2), 2 (T3)
};

struct PerkStats {
    float weaponMult    = 1.0f;   // dano de arma (O1 x O4)
    float skillMult     = 1.0f;   // dano de habilidades (O2 x O4)
    float cdMult        = 1.0f;   // cooldown das skills (C1)
    float rangeMult     = 1.0f;   // alcance das skills (C3)
    int   laserBeams    = 0;      // feixes extras do laser (O3)
    int   burstProj     = 0;      // projeteis extras da rajada (O4)
    float overloadBonus = 0.0f;   // segundos extra de sobrecarga (C4)
    float regen         = 0.0f;   // HP/s (S1)
    float lifesteal     = 0.0f;   // roubo de vida na morte (S3)
    float evade         = 0.0f;   // chance de evasao (C4)
    bool  revive        = false;  // Protocolo Imortal (S4)
};

namespace SkillTree {

    constexpr int PERK_COUNT      = 12;
    constexpr int BRANCH_SIGHT   = 0;
    constexpr int BRANCH_NEURAL  = 1;
    constexpr int BRANCH_VIRA    = 2;

    constexpr int PERK_C2_MANTO  = 5;   // +10 baseDefense na compra
    constexpr int PERK_S2_PLACA  = 9;   // +40 baseMaxHealth na compra

    const PerkInfo& perk(int i);
    inline uint32_t bit(int i) { return 1u << i; }
    inline bool     owns(uint32_t mask, int i) { return (mask & bit(i)) != 0; }
    int             branchOf(int i);
    int             tierOf(int i);
    int             spentInBranch(uint32_t mask, int branch);
    inline int      branchSpentReq(int tier) { return tier == 2 ? 3 : tier * 2; }
    bool            canBuy(uint32_t mask, int i);
    PerkStats       statsFor(uint32_t mask);
    int             bestNext(uint32_t mask);
    const char*     branchName(int branch);
    Color           branchColor(int branch);

}