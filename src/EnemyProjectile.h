#pragma once
#include <raylib.h>

class EnemyProjectile {
public:
    Vector2 position;
    Vector2 velocity;
    float   radius           = 5.0f;
    float   speed            = 320.0f;
    float   damage           = 10.0f;
    float   maxRange         = 380.0f;
    float   distanceTraveled = 0.0f;
    bool    active           = true;
    Color   color            = {255, 100, 0, 255};

    EnemyProjectile(Vector2 start, Vector2 dir, float dmg, float spd, Color col);

    void update(float dt);
    void render() const;
    bool isOutOfRange() const;
    bool hitsPlayer(Vector2 playerPos, float playerRadius) const;
};
