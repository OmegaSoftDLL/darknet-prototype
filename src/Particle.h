#pragma once
#include <raylib.h>
#include <vector>

enum class ParticleShape { Circle, Square, Spark };

enum class ParticleType {
    Default,
    BloodSplatter, FireSpark, IceShards, PoisonDroplet,
    LightningArc,  VoidRipple, SoulFragment, PlasmaTrail,
    ExplosionDebris, SmokeCloud, SparkFlash, NeonGlow,
    LavaDroplet, AshDrift, EnergyOrb, DeathBurst,
    LevelUpStar, HealOrb, ShieldBreak,
    PortalSuck, BossAura, MeleeSlash, RangedTrail,
    ChainEffect, FreezeRing, BurnRing, PoisonPool
};

class Particle {
public:
    Vector2       position;
    Vector2       velocity;
    Color         color;
    float         radius;
    float         lifetime;
    float         maxLifetime;
    ParticleShape shape    = ParticleShape::Circle;
    ParticleType  type     = ParticleType::Default;
    float         rotation = 0.0f;
    float         rotSpeed = 0.0f;
    bool          glow     = false;
    bool          active   = true;
    float         gravity  = 60.0f;
    float         drag     = 0.93f;
    float         scaleEnd = 1.0f;  // scale multiplier at end of life (for expanding rings)

    Particle(Vector2 pos, Vector2 vel, Color col, float rad, float life,
             ParticleShape s = ParticleShape::Circle, bool glowing = false);

    void update(float dt);
    void render() const;
};

class ParticleSystem {
public:
    std::vector<Particle> particles;

    // ── Original ──────────────────────────────────────────────────────────────
    void spawnExplosion(Vector2 pos, Color color, int count = 16);
    void spawnHit(Vector2 pos, Color color, int count = 8);
    void spawnLevelUp(Vector2 pos, int count = 40);
    void spawnElectric(Vector2 pos, Color color, int count = 12);
    void spawnBloodSparks(Vector2 pos, Color color, int count = 20);
    void spawnShockwave(Vector2 pos, Color color);

    // ── New expanded spawners ─────────────────────────────────────────────────
    void spawnDeathBurst(Vector2 pos, Color col, int count = 20);
    void spawnBloodHit(Vector2 pos, Color col);
    void spawnLevelUpEffect(Vector2 pos);
    void spawnExplosionLarge(Vector2 pos, float radius, Color col);
    void spawnFireTrail(Vector2 pos);
    void spawnIceBreak(Vector2 pos);
    void spawnHealEffect(Vector2 pos);
    void spawnPortalEffect(Vector2 center, float radius);
    void spawnVoidRipple(Vector2 pos);
    void spawnPoisonCloud(Vector2 pos);
    void spawnLavaDroplets(Vector2 pos);
    void spawnBossAura(Vector2 center, float radius, Color col);
    void spawnChainLightning(Vector2 from, Vector2 to);
    void spawnMeleeSlash(Vector2 pos, float angle);
    void spawnFreezeRing(Vector2 pos);
    void spawnBurnRing(Vector2 pos);
    void spawnNeonGlow(Vector2 pos, Color col);
    void spawnSoulFragment(Vector2 pos);
    void spawnShieldBreak(Vector2 pos);
    void spawnEnergyOrb(Vector2 pos, Color col);
    void spawnSmokeCloud(Vector2 pos);
    void spawnAshDrift(Vector2 pos);

    void update(float dt);
    void render() const;
};
