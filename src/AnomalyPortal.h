#pragma once
#include <raylib.h>
#include <vector>
#include <cmath>

enum class PortalState {
    Opening,  // animacao de abertura 1.5s
    Active,   // aberto — spawna inimigos, pode ser atacado
    Closing,  // player fechou — animacao de colapso 2.0s
    Closed    // morto
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
    bool  atmospheric   = false;  // chuva ambiente de zona sombria (sem onda de anomalia)
    float intensity     = 0.0f;
    float maxIntensity  = 1.0f;   // teto da intensidade (atmosferica usa menos)
    float lightningTimer = 0.0f;
    float lightningDur  = 0.0f;
    float ambientTimer  = 0.0f;
    float windPhase     = 0.0f;   // rajadas de vento variaveis

    struct RainDrop {
        Vector2 pos;
        float   speed;
        float   alpha;
    };
    std::vector<RainDrop> drops;

    void start();              // tempestade forte (onda de anomalia)
    void startAtmospheric();   // chuva/vento ambiente de zona sombria
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

    void spawnWave(int zoneW, int zoneH, Vector2 playerPos);
    void update(float dt, Vector2 playerPos);
    void renderWorld() const;                            // world-space portals (inside BeginMode2D)
    void renderStorm(int screenW, int screenH) const;   // screen-space storm overlay
    void renderHUD(int screenW, int screenH) const;      // screen-space portal counter

    bool checkProjectileHit(Vector2 projPos, float projRadius, float damage);
    bool pollSpawn(int& outEnemyTypeInt, Vector2& outPos);

    int  countOpen() const;
    bool hasActiveWave() const { return waveActive && countOpen() > 0; }
};
