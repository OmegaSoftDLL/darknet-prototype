#pragma once

#include <raylib.h>

class XPOrb {
public:
    Vector2 position;
    int amount = 10;
    float radius = 6.0f;
    float lifetime = 20.0f;
    bool pickedUp = false;
    float magnetSpeed = 0.0f;
    float animTimer   = 0.0f;

    XPOrb(Vector2 pos, int xp);

    void update(float dt, Vector2 playerPos, float playerRadius);
    void render() const;
    bool isExpired() const;
};
