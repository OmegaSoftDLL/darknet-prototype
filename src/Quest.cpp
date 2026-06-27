#include "Quest.h"

Quest::Quest(const std::string& qid, const std::string& t, const std::string& desc,
             const std::string& npc, QuestType qt, int tgt)
    : id(qid), title(t), description(desc), npcOwner(npc), type(qt), target(tgt) {}

void Quest::updateProgress(int amount) {
    if (completed || !active) return;
    current += amount;
    if (current >= target) {
        current = target;
        completed = true;
    }
}

bool Quest::isComplete() const { return completed; }

std::string Quest::getProgressText() const {
    return std::to_string(current) + "/" + std::to_string(target);
}

void Quest::complete() {
    if (!rewardGiven) rewardGiven = true;
}
