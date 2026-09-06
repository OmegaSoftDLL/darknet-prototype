#include "Achievement.h"
#include "Player.h"
#include <cmath>
#include <algorithm>

static Achievement makeAch(const char* id, const char* title, const char* desc,
                            const char* icon, int req, int xp, int credits) {
    Achievement a;
    a.id          = id;
    a.title       = title;
    a.description = desc;
    a.icon        = icon;
    a.requirement = req;
    a.rewardXP    = xp;
    a.rewardCredits = credits;
    return a;
}

void AchievementSystem::init() {
    achievements.clear();

    // Kill milestones
    achievements.push_back(makeAch("kill_1",     "Primeiro Sangue",     "Eliminar 1 inimigo",       "[X]",  1,     50,   20));
    achievements.push_back(makeAch("kill_10",    "Cacador",             "Eliminar 10 inimigos",     "[X]",  10,    100,  40));
    achievements.push_back(makeAch("kill_50",    "Combatente",          "Eliminar 50 inimigos",     "[X]",  50,    200,  80));
    achievements.push_back(makeAch("kill_100",   "Guerreiro",           "Eliminar 100 inimigos",    "[X]",  100,   400,  150));
    achievements.push_back(makeAch("kill_250",   "Exterminador",        "Eliminar 250 inimigos",    "[X]",  250,   800,  300));
    achievements.push_back(makeAch("kill_500",   "Maquina de Guerra",   "Eliminar 500 inimigos",    "[X]",  500,   1500, 600));
    achievements.push_back(makeAch("kill_1000",  "Mestre da Morte",     "Eliminar 1000 inimigos",   "[X]",  1000,  3000, 1200));
    achievements.push_back(makeAch("kill_5000",  "Lenda Sombria",       "Eliminar 5000 inimigos",   "[X]",  5000,  10000,5000));

    // Level milestones
    achievements.push_back(makeAch("level_5",    "Iniciado",            "Atingir Level 5",          "[L]",  5,     200,  100));
    achievements.push_back(makeAch("level_10",   "Combatente Treinado", "Atingir Level 10",         "[L]",  10,    500,  250));
    achievements.push_back(makeAch("level_20",   "Veterano",            "Atingir Level 20",         "[L]",  20,    1000, 500));
    achievements.push_back(makeAch("level_30",   "Elite DARKNET",       "Atingir Level 30",         "[L]",  30,    2000, 1000));
    achievements.push_back(makeAch("level_50",   "Executor OMEGA",      "Atingir Level 50",         "[L]",  50,    5000, 2500));

    // Boss kills
    achievements.push_back(makeAch("boss_1",     "Caçador de Bosses",   "Derrotar 1 boss",          "[B]",  1,     300,  150));
    achievements.push_back(makeAch("boss_5",     "Boss Slayer",         "Derrotar 5 bosses",        "[B]",  5,     800,  400));
    achievements.push_back(makeAch("boss_10",    "Aniquilador",         "Derrotar 10 bosses",       "[B]",  10,    2000, 1000));
    achievements.push_back(makeAch("boss_20",    "Caçador Lendario",    "Derrotar 20 bosses",       "[B]",  20,    5000, 2500));

    // Portal closure
    achievements.push_back(makeAch("portal_1",   "Fechador de Portais", "Fechar 1 portal",          "[P]",  1,     200,  100));
    achievements.push_back(makeAch("portal_5",   "Guardiao",            "Fechar 5 portais",         "[P]",  5,     500,  250));
    achievements.push_back(makeAch("portal_20",  "Sentinela OMEGA",     "Fechar 20 portais",        "[P]",  20,    1500, 750));
    achievements.push_back(makeAch("portal_50",  "Exterminador de Rifts","Fechar 50 portais",       "[P]",  50,    4000, 2000));

    // Item rarity
    achievements.push_back(makeAch("item_rare",    "Bom Gosto",         "Encontrar item Raro",      "[I]",  1,     300,  150));
    achievements.push_back(makeAch("item_epic",    "Colecionador",      "Encontrar item Epico",     "[I]",  1,     600,  300));
    achievements.push_back(makeAch("item_legend",  "Sortudo de Verdade","Encontrar item Lendario",  "[I]",  1,     2000, 1000));
    achievements.push_back(makeAch("item_omega",   "OMEGA COLETADO",    "Encontrar item OMEGA",     "[I]",  1,     5000, 2500));

    // Zone exploration
    achievements.push_back(makeAch("zone_dark",    "Explorador das Trevas","Entrar em zona sombria", "[Z]",  1,     400,  200));
    achievements.push_back(makeAch("zone_all",     "Mapeador do Mundo",  "Visitar 5 zonas distintas","[Z]", 5,     1000, 500));
    achievements.push_back(makeAch("zone_inferno", "Sobrevivente do Inferno","Sobreviver na Zona Inferno","[Z]",1,600,300));

    // Playtime
    achievements.push_back(makeAch("time_10",    "Comprometido",        "Jogar 10 minutos",         "[T]",  10,    100,  50));
    achievements.push_back(makeAch("time_60",    "Viciado",             "Jogar 60 minutos",         "[T]",  60,    500,  250));
    achievements.push_back(makeAch("time_300",   "Sem Vida Social",     "Jogar 5 horas",            "[T]",  300,   2000, 1000));

    // Credits
    achievements.push_back(makeAch("credits_1000",  "Milionario?",      "Acumular 1000 creditos",   "[C]",  1000,  200,  100));
    achievements.push_back(makeAch("credits_10000", "Tycoon",           "Acumular 10000 creditos",  "[C]",  10000, 1000, 500));

    // Deaths
    achievements.push_back(makeAch("no_death",    "Intocavel",          "Completar uma sessão sem morrer","[S]",0,1000,500));

    // Kill streak
    achievements.push_back(makeAch("streak_10",   "Em Chamas",          "Kill streak de 10",        "[K]",  10,    300,  150));
    achievements.push_back(makeAch("streak_20",   "Imparavel!",         "Kill streak de 20",        "[K]",  20,    800,  400));
}

Achievement* AchievementSystem::findById(const std::string& id) {
    for (auto& a : achievements) if (a.id == id) return &a;
    return nullptr;
}

void AchievementSystem::unlock(const std::string& id) {
    Achievement* a = findById(id);
    if (!a || a->unlocked) return;
    a->unlocked     = true;
    a->displayTimer = 4.0f;
    popupText  = std::string("[CONQUISTA] ") + a->icon + " " + a->title;
    popupTimer = 4.0f;
    popupColor = {255, 200, 0, 255};
    if (playerPtr) {
        playerPtr->xp       += a->rewardXP;
        playerPtr->credits  += a->rewardCredits;
    }
}

void AchievementSystem::checkProgress(const std::string& id, int value) {
    Achievement* a = findById(id);
    if (!a || a->unlocked) return;
    a->progress = value;
    if (a->progress >= a->requirement) unlock(id);
}

void AchievementSystem::update(float dt) {
    if (popupTimer > 0.0f) popupTimer -= dt;
}

void AchievementSystem::renderPopup(int screenW, int screenH) const {
    if (popupTimer <= 0.0f || popupText.empty()) return;

    float alpha = 1.0f;
    if (popupTimer < 0.5f) alpha = popupTimer / 0.5f;
    if (4.0f - popupTimer < 0.2f) alpha = (4.0f - popupTimer) / 0.2f;

    // Slide in from right
    float slideX = 0.0f;
    if (4.0f - popupTimer < 0.3f) {
        float t = (4.0f - popupTimer) / 0.3f;
        slideX = (1.0f - t) * 300.0f;
    }

    const char* txt = popupText.c_str();
    int tw  = MeasureText(txt, 14);
    int bw  = tw + 24;
    int bh  = 32;
    int bx  = screenW - bw - 10 + (int)slideX;
    int by  = 50;

    DrawRectangle(bx, by, bw, bh, ColorAlpha({10, 20, 10, 255}, 0.92f * alpha));
    DrawRectangleLinesEx({(float)bx,(float)by,(float)bw,(float)bh}, 2.0f,
                         ColorAlpha(popupColor, 0.95f * alpha));
    DrawText(txt, bx + 12, by + 9, 14, ColorAlpha(popupColor, alpha));
}

void AchievementSystem::onKill(int totalKills) {
    checkProgress("kill_1",    totalKills);
    checkProgress("kill_10",   totalKills);
    checkProgress("kill_50",   totalKills);
    checkProgress("kill_100",  totalKills);
    checkProgress("kill_250",  totalKills);
    checkProgress("kill_500",  totalKills);
    checkProgress("kill_1000", totalKills);
    checkProgress("kill_5000", totalKills);
}

void AchievementSystem::onLevelUp(int level) {
    checkProgress("level_5",  level);
    checkProgress("level_10", level);
    checkProgress("level_20", level);
    checkProgress("level_30", level);
    checkProgress("level_50", level);
}

void AchievementSystem::onPortalClosed(int total) {
    checkProgress("portal_1",  total);
    checkProgress("portal_5",  total);
    checkProgress("portal_20", total);
    checkProgress("portal_50", total);
}

void AchievementSystem::onBossKilled(int totalBosses) {
    checkProgress("boss_1",  totalBosses);
    checkProgress("boss_5",  totalBosses);
    checkProgress("boss_10", totalBosses);
    checkProgress("boss_20", totalBosses);
}

void AchievementSystem::onItemFound(int rarityInt) {
    if (rarityInt >= 2) checkProgress("item_rare",   1);
    if (rarityInt >= 3) checkProgress("item_epic",   1);
    if (rarityInt >= 4) checkProgress("item_legend", 1);
    if (rarityInt >= 5) checkProgress("item_omega",  1);
}

void AchievementSystem::onZoneVisited(int zoneId) {
    if (zoneId >= 4) checkProgress("zone_dark", 1);
    if (zoneId == 10) checkProgress("zone_inferno", 1);

    // Count distinct zones by tracking highest
    Achievement* za = findById("zone_all");
    if (za && !za->unlocked) {
        if (zoneId + 1 > za->progress) {
            za->progress = zoneId + 1;
            if (za->progress >= za->requirement) unlock("zone_all");
        }
    }
}

void AchievementSystem::onPlaytime(float minutes) {
    checkProgress("time_10",  (int)minutes);
    checkProgress("time_60",  (int)minutes);
    checkProgress("time_300", (int)minutes);
}

void AchievementSystem::onCreditsEarned(int total) {
    checkProgress("credits_1000",  total);
    checkProgress("credits_10000", total);
}

void AchievementSystem::onDeathCount(int deaths) {
    if (deaths == 0) {
        Achievement* a = findById("no_death");
        if (a && !a->unlocked) unlock("no_death");
    }
}

void AchievementSystem::onKillStreak(int streak) {
    checkProgress("streak_10", streak);
    checkProgress("streak_20", streak);
}
