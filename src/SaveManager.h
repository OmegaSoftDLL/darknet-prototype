#pragma once

#include "Player.h"
#include "Quest.h"
#include "Zone.h"
#include <vector>
#include <string>

static constexpr int SAVE_SLOTS = 3;
static constexpr int SAVE_VERSION = 5;   // V5: equipamento salvo por ID estavel (V4 era nome)

struct SaveSlotInfo {
    bool        exists      = false;
    int         slot        = 0;
    int         playerLevel = 1;
    float       playMinutes = 0.f;
    int         totalKills  = 0;
    int         currentZone = 0;
    std::string saveDate;
};

class SaveManager {
public:
    // Legacy single-file path (kept for backward compat)
    static constexpr const char* saveFile = "darknet_save.txt";

    // Multi-slot API
    static void save(const Player& player, const std::vector<Quest>& quests, ZoneID zone,
                     int slot = 0, float playMinutes = 0.f, int totalKills = 0,
                     int totalDeaths = 0, int bossesKilled = 0, int portalsSealed = 0,
                     int difficultyLevel = 0);
    static bool load(Player& player, std::vector<Quest>& quests, ZoneID& zone, int slot = 0);
    static bool hasSave(int slot = 0);
    static void deleteSave(int slot = 0);
    static SaveSlotInfo getSlotInfo(int slot);
    static void renderSaveSlots(int screenW, int screenH, int highlightSlot = 0);

    // Legacy (single-file)
    static bool exists();

private:
    static std::string slotPath(int slot);
    static void ensureSavesDir();
};
