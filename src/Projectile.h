#pragma once
#include <raylib.h>

class Projectile {
public:
    Vector2 position;
    Vector2 velocity;
    float   radius           = 6.0f;
    float   speed            = 600.0f;
    float   damage           = 40.0f;
    float   maxRange         = 500.0f;
    float   distanceTraveled = 0.0f;
    bool    active           = true;
    Color   color            = YELLOW;

    bool    isGrenade        = false;
    float   explodeRadius    = 160.0f;
    bool    exploded         = false;

    Projectile(Vector2 start, Vector2 direction, float dmg = 40.0f, float rng = 500.0f,
               float spd = 600.0f, Color col = YELLOW, bool grenade = false);

    void update(float dt);
    void render() const;
    bool isOutOfRange() const;
};
