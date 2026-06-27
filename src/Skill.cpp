#include "Skill.h"

Skill::Skill(const std::string& n, const std::string& desc, float cd, float dmg, float rng, int k)
    : name(n), description(desc), cooldown(cd), damage(dmg), range(rng), key(k) {}

bool Skill::isReady() const {
    return currentCooldown <= 0.0f;
}

void Skill::use() {
    currentCooldown = cooldown;
}

void Skill::update(float dt) {
    if (currentCooldown > 0.0f) {
        currentCooldown -= dt;
        if (currentCooldown < 0.0f) currentCooldown = 0.0f;
    }
}

float Skill::cooldownPercent() const {
    if (cooldown <= 0.0f) return 0.0f;
    return currentCooldown / cooldown;
}
