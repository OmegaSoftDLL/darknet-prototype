#pragma once
#include <raylib.h>
#include <vector>
#include <cmath>
#include <functional>

enum class PortalState {
    Opening,  // opening animation (1.5s)
    Active,   // open — spawns enemies and can be attacked
    Closing,  // player closed it — collapse animation (2.0s)
    Closed    // dead
};

struct AnomalyPortal {
    Vector2     position      = {0, 0};
    PortalState state         = PortalState::Opening;
    float       health        = 500.0f;
    float       maxHealth     = 500.0f;
    float       stateTimer    = 0.0f;
    float       spawnTimer    = 0.0f;
    float       spawnRate     = 4.0f;
    float       pulseTimer    = 0.0f;
    float       arcTimer      = 0.0f;
    float       particleAngle = 0.0f;
    float       baseRadius    = 45.0f;
    int         tier          = 1;
    bool        playerInRange = false;
    int         totalSpawned  = 0;
    Color       portalColor   = {120, 0, 200, 255};

    void    setup(Vector2 pos, int tierLevel);
    void    update(float dt);
    void    render() const;
    bool    isActive() const { return state == PortalState::Active; }
    bool    isDead()   const { return state == PortalState::Closed; }
    void    takeDamage(float dmg);
    void    startClosing();
    Vector2 getSpawnPosition() const;

private:
    void renderOpeningAnim()  const;
    void renderActivePortal() const;
    void renderClosingAnim()  const;
};

struct StormSystem {
    bool  active        = false;
    bool  atmospheric   = false;  // rain/wind environment for dark zones (in the anomaly wave)
    float intensity     = 0.0f;
    float maxIntensity  = 1.0f;   // intensity ceiling (atmospheric uses less)
    float lightningTimer = 0.0f;
    float lightningDur  = 0.0f;
    float ambientTimer  = 0.0f;
    float windPhase     = 0.0f;   // variable wind gusts

    struct RainDrop {
        Vector2 pos;
        float   speed;
        float   alpha;
    };
    std::vector<RainDrop> drops;

    void start();              // strong storm (anomaly wave)
    void startAtmospheric();   // rain/wind environment for dark zones
    void stop();
    void update(float dt);
    void render(int screenW, int screenH) const;
};

struct AnomalySystem {
    std::vector<AnomalyPortal> portals;
    StormSystem storm;
    bool  waveActive   = false;
    int   waveNumber   = 0;
    float waveTimer    = 0.0f;   // cooldown between waves
    float hudPulse     = 0.0f;

    void spawnWave(int zoneW, int zoneH, Vector2 playerPos,
                   Vector2 diskCenter = {0, 0}, float diskRadius = 0.0f);
    void update(float dt, Vector2 playerPos);
    void renderWorld() const;                            // world-space portals (inside BeginMode2D)
    void renderStorm(int screenW, int screenH) const;   // screen-space storm overlay
    void renderHUD(int screenW, int screenH) const;      // screen-space portal counter

    bool checkProjectileHit(Vector2 projPos, float projRadius, float damage);
    bool pollSpawn(int& outEnemyTypeInt, Vector2& outPos,
                   const std::function<bool(Vector2)>& isFree = nullptr);

    int  countOpen() const;
    bool hasActiveWave() const { return waveActive && countOpen() > 0; }
};
