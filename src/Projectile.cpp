#include "Projectile.h"
#include "Effects.h"
#include <raymath.h>
#include <cmath>

Projectile::Projectile(Vector2 start, Vector2 direction, float dmg, float rng,
                        float spd, Color col, bool grenade)
    : position(start), speed(spd), damage(dmg), maxRange(rng), color(col), isGrenade(grenade) {
    if (direction.x == 0.0f && direction.y == 0.0f) {
        active = false; // direcao invalida: projétil nao deve existir
    } else {
        velocity = Vector2Scale(Vector2Normalize(direction), speed);
    }
}

void Projectile::update(float dt) {
    Vector2 delta = Vector2Scale(velocity, dt);
    position = Vector2Add(position, delta);
    distanceTraveled += Vector2Length(delta);
    if (distanceTraveled >= maxRange) active = false;
}

void Projectile::render() const {
    if (isGrenade) {
        float pulse = std::sin(distanceTraveled * 0.08f) * 3.0f;
        float r = radius + pulse;
        // Glow
        DrawCircleV(position, r * 4.0f, ColorAlpha({255,80,0,255}, 0.05f));
        DrawCircleV(position, r * 2.5f, ColorAlpha({255,120,0,255}, 0.12f));
        DrawCircleV(position, r * 1.4f, ColorAlpha({255,180,0,255}, 0.25f));
        // Core
        DrawCircleV(position, r, {255, 100, 0, 255});
        DrawCircleV(position, r * 0.5f, {255, 220, 50, 255});
        DrawCircleV(position, r * 0.2f, WHITE);
        // Trail
        Vector2 tail = {position.x - velocity.x * 0.05f, position.y - velocity.y * 0.05f};
        DrawLineEx(tail, position, r * 1.5f, ColorAlpha({255, 160, 0, 255}, 0.3f));
    } else {
        // Glow layers
        DrawCircleV(position, radius * 4.5f, ColorAlpha(color, 0.04f));
        DrawCircleV(position, radius * 3.0f, ColorAlpha(color, 0.10f));
        DrawCircleV(position, radius * 1.8f, ColorAlpha(color, 0.25f));
        // Core
        DrawCircleV(position, radius, color);
        DrawCircleV(position, radius * 0.45f, ColorAlpha(WHITE, 0.9f));
        // Trail
        Vector2 tail = {position.x - velocity.x * 0.055f, position.y - velocity.y * 0.055f};
        DrawLineEx(tail, position, radius * 1.4f, ColorAlpha(color, 0.35f));
        Vector2 tail2 = {position.x - velocity.x * 0.1f, position.y - velocity.y * 0.1f};
        DrawLineEx(tail2, position, radius * 0.6f, ColorAlpha(color, 0.15f));
    }
}

bool Projectile::isOutOfRange() const {
    return distanceTraveled >= maxRange;
}
