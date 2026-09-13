#include "InfernoZone.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static float frand(unsigned int& seed) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return (float)(seed & 0x7FFF) / (float)0x7FFF;
}

// ─── generate ────────────────────────────────────────────────────────────────

void InfernoZoneSystem::generate(int mapW, int mapH, unsigned int seed) {
    reset();
    mapWidth  = mapW;
    mapHeight = mapH;
    active    = true;
    rngSeed   = seed;

    // Scatter lava pools across the map, avoiding the centre spawn area
    int poolCount = 8 + (int)(frand(rngSeed) * 5);  // 8-12 pools
    for (int i = 0; i < poolCount; ++i) {
        LavaPool p;
        p.position.x = 200.0f + frand(rngSeed) * (mapW - 400);
        p.position.y = 200.0f + frand(rngSeed) * (mapH - 400);
        p.radius    = 55.0f + frand(rngSeed) * 90.0f;  // 55-145px
        p.glowPulse = frand(rngSeed) * 6.28f;
        // Alternate between orange and red-orange lava
        if (i % 3 == 0)
            p.color = {255, 60, 0, 220};
        else if (i % 3 == 1)
            p.color = {255, 140, 0, 220};
        else
            p.color = {220, 30, 0, 210};
        pools.push_back(p);
    }

    // Geysers — spread around map
    int geyserCount = 5 + (int)(frand(rngSeed) * 4);  // 5-8 geysers
    for (int i = 0; i < geyserCount; ++i) {
        LavaGeyser g;
        g.position.x = 150.0f + frand(rngSeed) * (mapW - 300);
        g.position.y = 150.0f + frand(rngSeed) * (mapH - 300);
        g.radius    = 22.0f;
        g.cooldown  = 2.0f + frand(rngSeed) * 8.0f;  // first eruption 2-10s
        g.erupting  = 0.0f;
        g.phase     = frand(rngSeed) * 6.28f;
        geysers.push_back(g);
    }

    // Seed some initial ash particles
    for (int i = 0; i < 60; ++i) {
        AshParticle a;
        a.position.x = frand(rngSeed) * (float)mapW;
        a.position.y = frand(rngSeed) * (float)mapH;
        a.velocity.x = (frand(rngSeed) - 0.5f) * 18.0f;
        a.velocity.y = -(4.0f + frand(rngSeed) * 14.0f);
        a.maxLife    = 4.0f + frand(rngSeed) * 6.0f;
        a.life       = frand(rngSeed) * a.maxLife;  // staggered start
        a.size       = 1.5f + frand(rngSeed) * 3.0f;
        ash.push_back(a);
    }
}

// ─── update ──────────────────────────────────────────────────────────────────

void InfernoZoneSystem::update(float dt, Vector2 playerPos,
                               float& playerHP, float playerMaxHP,
                               bool playerShielded) {
    if (!active) return;

    // Update geysers
    for (auto& g : geysers) {
        if (g.erupting > 0.0f) {
            g.erupting -= dt;
        } else {
            g.cooldown -= dt;
            if (g.cooldown <= 0.0f) {
                g.erupting = 1.2f;
                g.cooldown = 4.0f + frand(rngSeed) * 7.0f;  // 4-11s between
                // Damage player if near erupting geyser
                if (!playerShielded && Vector2Distance(playerPos, g.position) < 80.0f) {
                    playerHP -= 25.0f;
                    if (playerHP < 0.0f) playerHP = 0.0f;
                }
            }
        }
    }

    // Lava pool damage
    lavaDmgTimer += dt;
    if (lavaDmgTimer >= 0.5f) {
        lavaDmgTimer = 0.0f;
        if (!playerShielded && isInLava(playerPos)) {
            playerHP -= 8.0f;  // 16 dmg/sec (ticked every 0.5s)
            if (playerHP < 0.0f) playerHP = 0.0f;
        }
    }

    // Update ash particles
    for (auto& a : ash) {
        a.position.x += a.velocity.x * dt;
        a.position.y += a.velocity.y * dt;
        a.life       -= dt;
    }

    // Remove dead ash
    ash.erase(std::remove_if(ash.begin(), ash.end(),
        [](const AshParticle& a) { return a.life <= 0.0f; }), ash.end());

    // Spawn new ash near pools
    ashSpawnTimer += dt;
    if (ashSpawnTimer >= 0.15f) {
        ashSpawnTimer = 0.0f;
        if (!pools.empty()) {
            spawnAsh(pools[(int)(frand(rngSeed) * pools.size())].position);
        }
    }
}

void InfernoZoneSystem::spawnAsh(Vector2 near) {
    AshParticle a;
    a.position.x = near.x + (frand(rngSeed) * 200.0f - 100.0f);
    a.position.y = near.y + frand(rngSeed) * 60.0f;
    a.velocity.x = frand(rngSeed) * 30.0f - 15.0f;
    a.velocity.y = -(5.0f + frand(rngSeed) * 20.0f);
    a.maxLife    = 3.0f + frand(rngSeed) * 5.0f;
    a.life       = a.maxLife;
    a.size       = 1.5f + frand(rngSeed) * 3.0f;
    ash.push_back(a);
}

// ─── isInLava / nearGeyser ───────────────────────────────────────────────────

bool InfernoZoneSystem::isInLava(Vector2 pos) const {
    for (const auto& p : pools) {
        if (Vector2Distance(pos, p.position) < p.radius - 8.0f)
            return true;
    }
    return false;
}

bool InfernoZoneSystem::nearGeyser(Vector2 pos, float range) const {
    for (const auto& g : geysers) {
        if (g.erupting > 0.0f && Vector2Distance(pos, g.position) < range)
            return true;
    }
    return false;
}

// ─── renderGround (world-space, BEFORE entities) ─────────────────────────────

void InfernoZoneSystem::renderGround(Vector2 camTarget) {
    if (!active) return;
    float t = (float)GetTime();

    for (const auto& p : pools) {
        float pulse = 0.85f + 0.15f * std::sin(t * 1.8f + p.glowPulse);
        float r     = p.radius * pulse;

        // Outer dark crust ring
        DrawCircleV(p.position, r + 14.0f, ColorAlpha({60, 10, 0, 255}, 0.85f));
        // Dark orange lava body
        DrawCircleV(p.position, r,        ColorAlpha(p.color, 0.90f));
        // Bright inner hot core
        DrawCircleV(p.position, r * 0.52f, ColorAlpha({255, 220, 50, 255}, 0.80f));
        // Glow halo
        DrawCircleV(p.position, r + 28.0f, ColorAlpha({255, 80, 0, 255}, 0.12f));
        DrawCircleV(p.position, r + 18.0f, ColorAlpha({255, 120, 0, 255}, 0.18f));

        // Crackling surface lines
        for (int c = 0; c < 4; ++c) {
            float ca = t * 0.4f + p.glowPulse + c * 1.57f;
            float cx1 = p.position.x + std::cos(ca) * r * 0.2f;
            float cy1 = p.position.y + std::sin(ca) * r * 0.2f;
            float cx2 = p.position.x + std::cos(ca + 0.6f) * r * 0.55f;
            float cy2 = p.position.y + std::sin(ca + 0.6f) * r * 0.55f;
            DrawLineEx({cx1,cy1},{cx2,cy2}, 1.8f, ColorAlpha({255,200,0,255}, 0.55f));
        }
    }
}

// ─── renderEffects (world-space, AFTER entities) ─────────────────────────────

void InfernoZoneSystem::renderEffects(Vector2 camTarget) {
    if (!active) return;
    float t = (float)GetTime();

    // Ash particles in world-space
    for (const auto& a : ash) {
        float alpha = std::min(a.life / a.maxLife, 1.0f) * 0.65f;
        Color c = ColorAlpha({180, 90, 40, 255}, alpha);
        DrawCircleV(a.position, a.size, c);
    }

    // Geysers
    for (const auto& g : geysers) {
        if (g.erupting > 0.0f) {
            float progress = 1.0f - (g.erupting / 1.2f);  // 0->1 as eruption goes
            float height   = 120.0f + 80.0f * progress;
            float spread   = 18.0f + 22.0f * progress;

            // Flame column — drawn as stacked circles getting smaller
            for (int layer = 0; layer < 8; ++layer) {
                float layerPct = (float)layer / 8.0f;
                float y        = g.position.y - layerPct * height;
                float layerR   = spread * (1.0f - layerPct * 0.7f);
                float alpha    = (1.0f - layerPct) * 0.85f;
                Color fc;
                if (layerPct < 0.3f)
                    fc = ColorAlpha({255, 200, 0, 255}, alpha);   // yellow base
                else if (layerPct < 0.6f)
                    fc = ColorAlpha({255, 100, 0, 255}, alpha);   // orange mid
                else
                    fc = ColorAlpha({200, 20, 0, 255}, alpha);    // red tip
                DrawCircleV({g.position.x, y}, layerR, fc);
            }
            // Bright geyser base
            DrawCircleV(g.position, g.radius + 8.0f,
                        ColorAlpha({255, 220, 80, 255}, 0.9f));
        } else {
            // Idle geyser — small bubbling vent
            float pulse = 0.5f + 0.5f * std::sin(t * 2.5f + g.phase);
            DrawCircleV(g.position, g.radius,
                        ColorAlpha({180, 50, 0, 255}, 0.75f));
            DrawCircleV(g.position, g.radius * 0.45f * pulse,
                        ColorAlpha({255, 150, 0, 255}, 0.65f));
        }
    }
}

// ─── renderAtmosphere (screen-space overlay) ─────────────────────────────────

void InfernoZoneSystem::renderAtmosphere(int screenW, int screenH) {
    if (!active) return;
    float t = (float)GetTime();

    // Red-orange vignette / heat haze around edges
    float pulse = 0.03f + 0.02f * std::sin(t * 0.8f);
    DrawRectangle(0, 0, screenW, screenH, ColorAlpha({180, 30, 0, 255}, pulse));

    // Corner glow — gives lava-lit feel
    int corner = 120;
    DrawRectangleGradientV(0, 0, screenW, corner,
                           ColorAlpha({200, 40, 0, 255}, 0.28f),
                           ColorAlpha({0, 0, 0, 0}, 0.0f));
    DrawRectangleGradientV(0, screenH - corner, screenW, corner,
                           ColorAlpha({0, 0, 0, 0}, 0.0f),
                           ColorAlpha({200, 40, 0, 255}, 0.32f));

    // Inferno zone label
    float labelAlpha = 0.55f + 0.35f * std::sin(t * 1.2f);
    const char* label = "[ ZONA INFERNO ]";
    int lw = MeasureText(label, 14);
    DrawText(label, screenW/2 - lw/2, screenH - 36, 14,
             ColorAlpha({255, 120, 0, 255}, labelAlpha));

    // Lava damage warning when player is in lava (flashing red)
    // (handled in update — just draw a red flash frame here if needed)
}

// ─── reset ───────────────────────────────────────────────────────────────────

void InfernoZoneSystem::reset() {
    active = false;
    pools.clear();
    geysers.clear();
    ash.clear();
    ashSpawnTimer = 0.0f;
    lavaDmgTimer  = 0.0f;
}
