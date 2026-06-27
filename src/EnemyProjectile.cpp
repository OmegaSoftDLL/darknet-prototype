#include "EnemyProjectile.h"
#include "Effects.h"
#include <raymath.h>
#include <cmath>

EnemyProjectile::EnemyProjectile(Vector2 start, Vector2 dir, float dmg, float spd, Color col)
    : position(start), damage(dmg), speed(spd), color(col) {
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.0f) velocity = {dir.x / len * speed, dir.y / len * speed};
}

void EnemyProjectile::update(float dt) {
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    distanceTraveled += speed * dt;
}

void EnemyProjectile::render() const {
    // Glow
    DrawCircleV(position, radius * 4.0f, ColorAlpha(color, 0.04f));
    DrawCircleV(position, radius * 2.5f, ColorAlpha(color, 0.12f));
    DrawCircleV(position, radius * 1.5f, ColorAlpha(color, 0.28f));
    // Core
    DrawCircleV(position, radius, color);
    DrawCircleV(position, radius * 0.45f, ColorAlpha(WHITE, 0.8f));
    // Trail
    Vector2 tail = {position.x - velocity.x * 0.05f, position.y - velocity.y * 0.05f};
    DrawLineEx(tail, position, radius * 1.2f, ColorAlpha(color, 0.3f));
}

bool EnemyProjectile::isOutOfRange() const { return distanceTraveled >= maxRange; }

bool EnemyProjectile::hitsPlayer(Vector2 playerPos, float playerRadius) const {
    return active && Vector2Distance(position, playerPos) <= radius + playerRadius;
}
