#pragma once
#include <raylib.h>
#include <vector>

// ─── InfernoZone: zona vulcanica com lava, geysers e atmosfera de cinzas ─────

struct LavaPool {
    Vector2 position;
    float   radius;
    float   glowPulse;   // random phase for pulsing glow
    Color   color;
};

struct LavaGeyser {
    Vector2 position;
    float   radius;
    float   cooldown;    // seconds until next eruption
    float   erupting;    // > 0 means currently erupting (timer)
    float   phase;       // random phase offset
};

struct AshParticle {
    Vector2 position;
    Vector2 velocity;
    float   life;
    float   maxLife;
    float   size;
};

class InfernoZoneSystem {
public:
    bool active = false;

    void generate(int mapW, int mapH, unsigned int seed);
    void update(float dt, Vector2 playerPos, float& playerHP, float playerMaxHP, bool playerShielded);
    void renderGround(Vector2 camTarget);      // lava pools — draw BEFORE entities
    void renderEffects(Vector2 camTarget);     // geysers + eruptions — draw AFTER entities
    void renderAtmosphere(int screenW, int screenH); // screen-space red haze + ash
    void reset();

    bool isInLava(Vector2 pos) const;
    bool nearGeyser(Vector2 pos, float range) const;

private:
    std::vector<LavaPool>    pools;
    std::vector<LavaGeyser>  geysers;
    std::vector<AshParticle> ash;

    int   mapWidth  = 2000;
    int   mapHeight = 2000;
    float ashSpawnTimer = 0.0f;
    float lavaDmgTimer  = 0.0f;
    unsigned int rngSeed = 0;

    void spawnAsh(Vector2 near);
};
