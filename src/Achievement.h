#pragma once
#include <raylib.h>
#include <string>
#include <vector>

struct Achievement {
    std::string id;
    std::string title;
    std::string description;
    std::string icon;
    bool        unlocked       = false;
    float       displayTimer   = 0.0f;  // popup countdown
    int         requirement    = 1;
    int         progress       = 0;
    int         rewardXP       = 0;
    int         rewardCredits  = 0;
};

class AchievementSystem {
public:
    std::vector<Achievement> achievements;
    std::string              popupText;
    float                    popupTimer  = 0.0f;
    Color                    popupColor  = {255, 200, 0, 255};

    void init();
    void update(float dt);
    void checkProgress(const std::string& id, int value);
    void unlock(const std::string& id);
    void renderPopup(int screenW, int screenH) const;

    // Progress reporters called from Game
    void onKill(int totalKills);
    void onLevelUp(int level);
    void onPortalClosed(int total);
    void onBossKilled(int totalBosses);
    void onItemFound(int rarityInt);
    void onZoneVisited(int zoneId);
    void onPlaytime(float minutes);
    void onCreditsEarned(int total);
    void onDeathCount(int deaths);
    void onKillStreak(int streak);

private:
    Achievement* findById(const std::string& id);
};
