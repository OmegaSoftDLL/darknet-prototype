#pragma once

#include <raylib.h>
#include <string>

struct Skill {
    std::string name;
    std::string description;
    float cooldown = 1.0f;
    float currentCooldown = 0.0f;
    float damage = 0.0f;
    float range = 0.0f;
    int key;

    Skill(const std::string& n, const std::string& desc, float cd, float dmg, float rng, int k);

    bool isReady() const;
    void use();
    void update(float dt);
    float cooldownPercent() const;
};
