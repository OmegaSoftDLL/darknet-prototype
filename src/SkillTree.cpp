#include "SkillTree.h"

namespace SkillTree {

static const char* BRANCH_NAMES[3] = { "SIGHT HACK", "NEURAL LINK", "VIRACORP" };

static Color BRANCH_COLORS[3] = {
    {0,  220, 255, 255},
    {90, 140, 255, 255},
    {0,  200, 120, 255}
};

static const PerkInfo PERKS[PERK_COUNT] = {
    {"Hud Balistico",     "Sistema de mira sobreposto: +10% de dano de arma.",        BRANCH_SIGHT,  0},
    {"Laser Duplo",       "Feixe de corte extra: +15% de dano nas habilidades.",      BRANCH_SIGHT,  0},
    {"Perfurador",        "Laser expandido: dispara 5 feixes ao mesmo tempo.",        BRANCH_SIGHT,  1},
    {"Sistema Predador",  "+20% de dano total e Rajada com +4 projeteis.",            BRANCH_SIGHT,  2},
    {"Tempo Neural",      "Sinapses aceleradas: cooldowns das skills -12%.",          BRANCH_NEURAL, 0},
    {"Manto Fantasma",    "Armadura holografica permanente: +10 de defesa.",          BRANCH_NEURAL, 0},
    {"Antena EMP",        "+20% de alcance em todas as skills.",                      BRANCH_NEURAL, 1},
    {"Fantasma Digital",  "Sobrecarga +3s e 12% de chance de evasao.",                BRANCH_NEURAL, 2},
    {"Regenerador",       "Nanobots de reparo regeneram 3 HP por segundo.",           BRANCH_VIRA,   0},
    {"Placa Blindada",    "Revestimento de carapaca: +40 de HP maximo.",              BRANCH_VIRA,   0},
    {"Hack de Sangue",    "+15% de roubo de vida (cura ao derrotar inimigos).",       BRANCH_VIRA,   1},
    {"Protocolo Imortal", "1x a cada 60s, um golpe fatal e anulado.",                 BRANCH_VIRA,   2},
};

const PerkInfo& perk(int i) {
    if (i < 0 || i >= PERK_COUNT) i = 0;
    return PERKS[i];
}

int branchOf(int i) { return perk(i).branch; }
int tierOf(int i)   { return perk(i).tier; }

const char* branchName(int branch) {
    if (branch < 0 || branch > 2) branch = 0;
    return BRANCH_NAMES[branch];
}

Color branchColor(int branch) {
    if (branch < 0 || branch > 2) branch = 0;
    return BRANCH_COLORS[branch];
}

int spentInBranch(uint32_t mask, int branch) {
    int n = 0;
    for (int i = 0; i < PERK_COUNT; ++i)
        if (branchOf(i) == branch && owns(mask, i)) ++n;
    return n;
}

bool canBuy(uint32_t mask, int i) {
    if (i < 0 || i >= PERK_COUNT) return false;
    if (owns(mask, i)) return false;
    int req = branchSpentReq(tierOf(i));
    if (spentInBranch(mask, branchOf(i)) < req) return false;
    return true;
}

PerkStats statsFor(uint32_t mask) {
    PerkStats s;
    if (owns(mask, 0))       s.weaponMult     *= 1.10f;
    if (owns(mask, 1))       s.skillMult      *= 1.15f;
    if (owns(mask, 2))       s.laserBeams     += 1;
    if (owns(mask, 3))       { s.weaponMult *= 1.20f; s.skillMult *= 1.20f; s.burstProj += 2; }
    if (owns(mask, 4))       s.cdMult         *= 0.88f;
    if (owns(mask, 6))       s.rangeMult      *= 1.20f;
    if (owns(mask, 7))       { s.overloadBonus = 3.0f; s.evade = 0.12f; }
    if (owns(mask, 8))       s.regen          = 3.0f;
    if (owns(mask, 10))      s.lifesteal      = 0.15f;
    if (owns(mask, 11))      s.revive         = true;
    return s;
}

int bestNext(uint32_t mask) {
    for (int i = 0; i < PERK_COUNT; ++i)
        if (canBuy(mask, i)) return i;
    return -1;
}

}