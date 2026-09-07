#include "XPOrb.h"
#include <raymath.h>
#include <cmath>

XPOrb::XPOrb(Vector2 pos, int xp) : position(pos), amount(xp) {}

void XPOrb::update(float dt, Vector2 playerPos, float playerRadius) {
    lifetime -= dt;
    animTimer += dt;

    float dist = Vector2Distance(position, playerPos);
    if (dist < playerRadius + 100.0f) {
        magnetSpeed += dt * 500.0f;
        Vector2 dir = Vector2Normalize(Vector2Subtract(playerPos, position));
        position = Vector2Add(position, Vector2Scale(dir, magnetSpeed * dt));
    }
}

void XPOrb::render() const {
    float pulse = std::sin(animTimer * 6.0f);
    float alpha = (lifetime < 3.0f) ? lifetime / 3.0f : 1.0f;

    // Size scales with XP amount (bigger drops = bigger orb)
    float r = radius + (amount > 50 ? 3.0f : amount > 20 ? 1.5f : 0.0f);

    // Outer glow
    DrawCircleV(position, r + 7.0f + pulse * 3.0f, ColorAlpha({60,160,255,255}, 0.20f * alpha));
    DrawCircleV(position, r + 4.0f + pulse * 2.0f, ColorAlpha({100,200,255,255}, 0.30f * alpha));

    // Body
    DrawCircleV(position, r + pulse * 1.0f, Color{60,180,255,255});
    DrawCircleV(position, r * 0.55f,        Color{180,230,255,255});
    DrawCircleV(position, r * 0.20f,        WHITE);

    // Orbiting micro-spark for large orbs (boss/elite drops)
    if (amount > 40) {
        float the = animTimer * 5.0f;
        DrawCircleV({position.x + std::cos(the) * (r + 6.0f),
                     position.y + std::sin(the) * (r + 6.0f)},
                    2.0f, ColorAlpha(WHITE, 0.85f * alpha));
    }
}

bool XPOrb::isExpired() const {
    return lifetime <= 0.0f;
}
