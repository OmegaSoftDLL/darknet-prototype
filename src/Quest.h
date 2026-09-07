#pragma once
#include <string>
#include "Equipment.h"

enum class QuestType {
    Kill,       // kill N enemies
    KillBoss,   // kill the boss
    Collect,    // collect N items
    CollectRare,// collect N items raros ou superiores
    ClosePortal,// close N portals of anomaly
    Zone        // chegar the uma zone especifica
};

struct Quest {
    std::string id;
    std::string title;
    std::string description;
    std::string npcOwner;  // nome of the NPC that of the the quest
    QuestType   type       = QuestType::Kill;
    int         target     = 5;
    int         current    = 0;
    bool        active     = true;
    bool        completed  = false;
    bool        rewardGiven= false;
    float       rewardHP   = 0.0f;
    int         rewardXP   = 0;
    Equipment   rewardEquip;

    Quest() = default;
    Quest(const std::string& id, const std::string& title, const std::string& desc,
          const std::string& npc, QuestType t, int tgt);

    void        updateProgress(int amount);
    bool        isComplete() const;
    std::string getProgressText() const;
    void        complete();
};
