#include "Particle.h"
#include "Effects.h"
#include <cmath>

Particle::Particle(Vector2 pos, Vector2 vel, Color col, float rad, float life,
                   ParticleShape s, bool glowing)
    : position(pos), velocity(vel), color(col), radius(rad),
      lifetime(life), maxLifetime(life), shape(s), glow(glowing) {
    rotSpeed = (float)GetRandomValue(-300, 300);
}

void Particle::update(float dt) {
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    velocity.x *= drag;
    velocity.y *= drag;
    velocity.y += gravity * dt;
    rotation   += rotSpeed * dt;
    lifetime   -= dt;
    if (lifetime <= 0.0f) active = false;
}

void Particle::render() const {
    float t     = lifetime / maxLifetime;          // 1 -> 0 ao longo da vida
    float alpha = t * t;                            // fade quadratico (suave)

    // Tamanho: scaleEnd==1 mantem o "encolher" classico (radius*t); caso
    // contrario interpola de 1.0 (nascimento) ate scaleEnd (morte) — assim
    // fumaca expande (scaleEnd>1) e aneis colapsam (scaleEnd<1) DE VERDADE.
    float sizeMul;
    if (std::fabs(scaleEnd - 1.0f) < 0.01f) sizeMul = t;
    else                                    sizeMul = 1.0f + (scaleEnd - 1.0f) * (1.0f - t);
    float r = radius * sizeMul;
    if (r < 0.0f) r = 0.0f;

    Color c = ColorAlpha(color, alpha);

    switch (shape) {
        case ParticleShape::Circle:
            if (glow) {
                // Glow aditivo para faiscas/energia brilharem de verdade
                BeginBlendMode(BLEND_ADDITIVE);
                DrawCircleV(position, r * 3.2f, ColorAlpha(color, alpha * 0.05f));
                DrawCircleV(position, r * 1.9f, ColorAlpha(color, alpha * 0.14f));
                EndBlendMode();
            }
            DrawCircleV(position, r, c);
            DrawCircleV(position, r * 0.4f, ColorAlpha(WHITE, alpha * 0.7f));
            break;

        case ParticleShape::Spark: {
            // Faisca = risco fino e nitido na direcao do movimento + nucleo branco
            float speed = std::sqrt(velocity.x*velocity.x + velocity.y*velocity.y);
            float trail = std::min(0.05f, 14.0f / (speed + 1.0f));
            Vector2 tail = {position.x - velocity.x * trail,
                            position.y - velocity.y * trail};
            float thick = std::max(1.0f, r * 0.5f);
            if (glow) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawLineEx(tail, position, thick * 2.6f, ColorAlpha(color, alpha * 0.18f));
                EndBlendMode();
            }
            DrawLineEx(tail, position, thick, c);
            DrawCircleV(position, std::max(1.0f, r * 0.55f), ColorAlpha(WHITE, alpha));
            break;
        }

        case ParticleShape::Square: {
            float s2 = r * 1.4f;
            if (glow) {
                BeginBlendMode(BLEND_ADDITIVE);
                DrawCircleV(position, s2 * 1.2f, ColorAlpha(color, alpha * 0.12f));
                EndBlendMode();
            }
            DrawRectanglePro({position.x, position.y, s2, s2},
                             {s2 * 0.5f, s2 * 0.5f}, rotation, c);
            break;
        }
    }
}

// ─── ParticleSystem ───────────────────────────────────────────────────────────

void ParticleSystem::spawnExplosion(Vector2 pos, Color color, int count) {
    // Flash central brilhante (o "estouro")
    Particle flash(pos, {0,0}, WHITE, 14.0f, 0.14f, ParticleShape::Circle, true);
    flash.gravity = 0.0f; flash.drag = 1.0f; flash.scaleEnd = 0.1f;
    particles.push_back(flash);
    // Anel de choque que EXPANDE (colapso de alpha) — agora visivel de verdade
    Particle ring(pos, {0,0}, color, 8.0f, 0.30f, ParticleShape::Circle, true);
    ring.gravity = 0.0f; ring.drag = 1.0f; ring.scaleEnd = 5.0f;
    particles.push_back(ring);

    // Shockwave ring (particulas)
    spawnShockwave(pos, color);

    for (int i = 0; i < count; ++i) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(100, 350);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        float life  = GetRandomValue(4, 10) / 10.0f;
        bool  glowing = (i % 3 == 0);

        // Mix sparks and circles
        ParticleShape s = (i % 2 == 0) ? ParticleShape::Spark : ParticleShape::Circle;
        particles.emplace_back(pos, vel, color, (float)GetRandomValue(3, 9), life, s, glowing);
    }
    // White flash particles
    for (int i = 0; i < 5; ++i) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(50, 180);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        particles.emplace_back(pos, vel, WHITE, (float)GetRandomValue(4, 10), 0.2f,
                               ParticleShape::Circle, true);
    }
}

void ParticleSystem::spawnHit(Vector2 pos, Color color, int count) {
    // Flash de impacto curtissimo (nucleo brilhante) — da "soco" ao hit
    Particle flash(pos, {0,0}, WHITE, 7.0f, 0.10f, ParticleShape::Circle, true);
    flash.gravity = 0.0f; flash.drag = 1.0f; flash.scaleEnd = 0.2f;
    particles.push_back(flash);

    for (int i = 0; i < count; ++i) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(60, 220);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        ParticleShape s = (i % 3 == 0) ? ParticleShape::Spark : ParticleShape::Circle;
        Particle p(pos, vel, color, (float)GetRandomValue(2, 6), 0.35f, s, true);
        p.gravity = 40.0f; p.drag = 0.90f;
        particles.push_back(p);
    }
    // Tiny white sparks (velozes, finos)
    for (int i = 0; i < 5; ++i) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(160, 320);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        Particle p(pos, vel, WHITE, 2.0f, 0.16f, ParticleShape::Spark, true);
        p.drag = 0.86f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnLevelUp(Vector2 pos, int count) {
    spawnShockwave(pos, GOLD);
    for (int i = 0; i < count; ++i) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(100, 400);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed - 50.0f};
        Color col = (i % 3 == 0) ? WHITE :
                    (i % 3 == 1) ? GOLD : Color{255, 200, 50, 255};
        ParticleShape s = (i % 2 == 0) ? ParticleShape::Spark : ParticleShape::Square;
        particles.emplace_back(pos, vel, col, (float)GetRandomValue(3, 10), 1.2f, s, true);
    }
}

void ParticleSystem::spawnElectric(Vector2 pos, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float speed = GetRandomValue(80, 280);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        particles.emplace_back(pos, vel, color, (float)GetRandomValue(2, 5), 0.5f,
                               ParticleShape::Spark, true);
    }
}

void ParticleSystem::spawnBloodSparks(Vector2 pos, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = GetRandomValue(-30, 210) * DEG2RAD; // upward arc
        float speed = GetRandomValue(80, 300);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed - 80.0f};
        particles.emplace_back(pos, vel, color, (float)GetRandomValue(2, 6), 0.5f,
                               ParticleShape::Spark, false);
    }
}

void ParticleSystem::spawnShockwave(Vector2 pos, Color color) {
    // Fake shockwave: many particles in ring at high speed
    for (int i = 0; i < 24; ++i) {
        float angle = (float)i / 24.0f * 360.0f * DEG2RAD;
        float speed = GetRandomValue(200, 400);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        particles.emplace_back(pos, vel, color, (float)GetRandomValue(3, 6), 0.25f,
                               ParticleShape::Circle, true);
    }
}

// ─── New expanded spawn functions ─────────────────────────────────────────────

void ParticleSystem::spawnDeathBurst(Vector2 pos, Color col, int count) {
    spawnShockwave(pos, col);
    for (int i = 0; i < count; ++i) {
        float angle = (float)i / count * 360.0f * DEG2RAD;
        float speed = (float)GetRandomValue(120, 380);
        Vector2 vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        ParticleShape s = (i % 3 == 0) ? ParticleShape::Spark : ParticleShape::Circle;
        Particle p(pos, vel, col, (float)GetRandomValue(3, 9),
                   GetRandomValue(5, 12) / 10.0f, s, true);
        p.type = ParticleType::DeathBurst;
        particles.push_back(p);
    }
    // white flash
    for (int i = 0; i < 6; ++i) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        Vector2 vel = {std::cos(angle)*200.0f, std::sin(angle)*200.0f};
        particles.emplace_back(pos, vel, WHITE, 5.0f, 0.18f, ParticleShape::Circle, true);
    }
}

void ParticleSystem::spawnBloodHit(Vector2 pos, Color col) {
    for (int i = 0; i < 12; ++i) {
        float angle = GetRandomValue(-40, 220) * DEG2RAD;
        float speed = (float)GetRandomValue(60, 220);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed - 60.0f};
        Particle p(pos, vel, col, (float)GetRandomValue(2,5), 0.45f, ParticleShape::Spark, false);
        p.type    = ParticleType::BloodSplatter;
        p.gravity = 180.0f;
        p.drag    = 0.88f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnLevelUpEffect(Vector2 pos) {
    spawnShockwave(pos, GOLD);
    spawnShockwave(pos, {255,255,150,255});
    for (int i = 0; i < 50; ++i) {
        float angle = (float)i / 50.0f * 360.0f * DEG2RAD + GetRandomValue(-20,20)*DEG2RAD;
        float speed = (float)GetRandomValue(80, 320);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed - 100.0f};
        Color col   = (i%3==0)?WHITE:(i%3==1)?GOLD:Color{255,220,50,255};
        ParticleShape s = (i%2==0)?ParticleShape::Spark:ParticleShape::Square;
        Particle p(pos, vel, col, (float)GetRandomValue(3,10), 1.4f, s, true);
        p.type    = ParticleType::LevelUpStar;
        p.gravity = -30.0f;
        p.drag    = 0.96f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnExplosionLarge(Vector2 pos, float radius, Color col) {
    spawnShockwave(pos, col);
    spawnShockwave(pos, WHITE);
    for (int i = 0; i < 40; ++i) {
        float angle = GetRandomValue(0,360)*DEG2RAD;
        float speed = GetRandomValue(80,400) * (radius / 40.0f);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        ParticleShape s = (i%3==0)?ParticleShape::Spark:ParticleShape::Circle;
        Particle p(pos, vel, col, GetRandomValue(4,14)*radius/40.0f,
                   GetRandomValue(6,14)/10.0f, s, true);
        p.type = ParticleType::ExplosionDebris;
        particles.push_back(p);
    }
    // Smoke clouds
    for (int i = 0; i < 8; ++i) {
        float angle = GetRandomValue(0,360)*DEG2RAD;
        float speed = (float)GetRandomValue(20,80);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed - 40.0f};
        Particle p(pos, vel, {80,80,80,255}, radius*0.4f, 1.0f, ParticleShape::Circle, false);
        p.type    = ParticleType::SmokeCloud;
        p.gravity = -15.0f;
        p.drag    = 0.98f;
        p.scaleEnd= 2.5f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnFireTrail(Vector2 pos) {
    for (int i = 0; i < 6; ++i) {
        Vector2 offset = {(float)GetRandomValue(-10,10), (float)GetRandomValue(-10,10)};
        Vector2 p2 = {pos.x+offset.x, pos.y+offset.y};
        Vector2 vel = {(float)GetRandomValue(-20,20), (float)GetRandomValue(-80,-30)};
        Color col = (i%2==0)?Color{255,120,20,255}:Color{255,60,0,255};
        Particle p(p2, vel, col, (float)GetRandomValue(3,7), 0.4f, ParticleShape::Circle, true);
        p.type    = ParticleType::FireSpark;
        p.gravity = -40.0f;
        p.drag    = 0.97f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnIceBreak(Vector2 pos) {
    for (int i = 0; i < 16; ++i) {
        float angle = GetRandomValue(0,360)*DEG2RAD;
        float speed = (float)GetRandomValue(80,280);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        Color col = (i%3==0)?Color{180,230,255,255}:(i%3==1)?Color{100,180,255,255}:WHITE;
        Particle p(pos, vel, col, (float)GetRandomValue(3,8), 0.6f, ParticleShape::Square, true);
        p.type    = ParticleType::IceShards;
        p.gravity = 120.0f;
        p.drag    = 0.92f;
        particles.push_back(p);
    }
    // Ring ripple
    spawnShockwave(pos, {140,200,255,255});
}

void ParticleSystem::spawnHealEffect(Vector2 pos) {
    for (int i = 0; i < 14; ++i) {
        float angle = GetRandomValue(0,360)*DEG2RAD;
        float speed = (float)GetRandomValue(30,120);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed - 60.0f};
        Color col = (i%2==0)?Color{50,255,100,255}:Color{150,255,180,255};
        Particle p(pos, vel, col, (float)GetRandomValue(3,7), 0.9f, ParticleShape::Circle, true);
        p.type    = ParticleType::HealOrb;
        p.gravity = -50.0f;
        p.drag    = 0.97f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnPortalEffect(Vector2 center, float radius) {
    for (int i = 0; i < 20; ++i) {
        float angle = GetRandomValue(0,360)*DEG2RAD;
        float dist  = radius * (0.7f + GetRandomValue(0,30)/100.0f);
        Vector2 startPos = {center.x + std::cos(angle)*dist,
                            center.y + std::sin(angle)*dist};
        // pull toward center
        float speed = (float)GetRandomValue(40,120);
        Vector2 vel = {(center.x - startPos.x)*speed/dist,
                       (center.y - startPos.y)*speed/dist};
        Color col = (i%3==0)?Color{160,0,255,255}:(i%3==1)?Color{80,0,200,255}:Color{200,100,255,255};
        Particle p(startPos, vel, col, (float)GetRandomValue(2,6), 0.6f, ParticleShape::Circle, true);
        p.type    = ParticleType::PortalSuck;
        p.gravity = 0.0f;
        p.drag    = 1.0f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnVoidRipple(Vector2 pos) {
    for (int i = 0; i < 32; ++i) {
        float angle = (float)i/32.0f * 360.0f * DEG2RAD;
        float speed = (float)GetRandomValue(150,250);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        Particle p(pos, vel, {80,0,200,255}, 5.0f, 0.5f, ParticleShape::Circle, true);
        p.type    = ParticleType::VoidRipple;
        p.gravity = 0.0f;
        p.drag    = 0.94f;
        p.scaleEnd= 0.0f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnPoisonCloud(Vector2 pos) {
    for (int i = 0; i < 10; ++i) {
        Vector2 offset = {(float)GetRandomValue(-20,20),(float)GetRandomValue(-20,20)};
        Vector2 p2 = {pos.x+offset.x, pos.y+offset.y};
        Vector2 vel = {(float)GetRandomValue(-15,15), (float)GetRandomValue(-30,-10)};
        Color col = (i%2==0)?Color{60,180,40,255}:Color{40,140,20,255};
        Particle p(p2, vel, col, (float)GetRandomValue(8,18), 1.2f, ParticleShape::Circle, false);
        p.type    = ParticleType::PoisonDroplet;
        p.gravity = -10.0f;
        p.drag    = 0.99f;
        p.scaleEnd= 1.8f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnLavaDroplets(Vector2 pos) {
    for (int i = 0; i < 12; ++i) {
        float angle = GetRandomValue(-160,-20)*DEG2RAD;
        float speed = (float)GetRandomValue(80,300);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        Color col = (i%3==0)?Color{255,100,0,255}:(i%3==1)?Color{255,60,0,255}:Color{200,40,0,255};
        Particle p(pos, vel, col, (float)GetRandomValue(3,9), 0.8f, ParticleShape::Circle, true);
        p.type    = ParticleType::LavaDroplet;
        p.gravity = 220.0f;
        p.drag    = 0.96f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnBossAura(Vector2 center, float radius, Color col) {
    for (int i = 0; i < 10; ++i) {
        float angle = GetRandomValue(0,360)*DEG2RAD;
        float r     = radius * (0.8f + GetRandomValue(0,20)/100.0f);
        Vector2 startPos = {center.x + std::cos(angle)*r, center.y + std::sin(angle)*r};
        // Tangential velocity (orbit)
        Vector2 vel = {-std::sin(angle)*30.0f, std::cos(angle)*30.0f};
        Particle p(startPos, vel, col, (float)GetRandomValue(4,10), 0.8f, ParticleShape::Circle, true);
        p.type    = ParticleType::BossAura;
        p.gravity = 0.0f;
        p.drag    = 1.0f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnChainLightning(Vector2 from, Vector2 to) {
    int steps = 8;
    Vector2 prev = from;
    for (int i = 1; i <= steps; ++i) {
        float t = (float)i / steps;
        Vector2 lerped = {from.x + (to.x-from.x)*t + GetRandomValue(-15,15),
                          from.y + (to.y-from.y)*t + GetRandomValue(-15,15)};
        // Spark at each node
        Vector2 vel = {(float)GetRandomValue(-50,50),(float)GetRandomValue(-50,50)};
        Particle p(lerped, vel, {180,220,255,255}, 3.0f, 0.2f, ParticleShape::Spark, true);
        p.type    = ParticleType::LightningArc;
        p.gravity = 0.0f;
        particles.push_back(p);
        prev = lerped;
    }
}

void ParticleSystem::spawnMeleeSlash(Vector2 pos, float angle) {
    for (int i = 0; i < 8; ++i) {
        float spread = (angle + GetRandomValue(-25,25)) * DEG2RAD;
        float speed  = (float)GetRandomValue(150,300);
        Vector2 vel  = {std::cos(spread)*speed, std::sin(spread)*speed};
        Color col = (i%2==0)?Color{255,220,100,255}:WHITE;
        Particle p(pos, vel, col, (float)GetRandomValue(2,5), 0.2f, ParticleShape::Spark, true);
        p.type    = ParticleType::MeleeSlash;
        p.gravity = 0.0f;
        p.drag    = 0.90f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnFreezeRing(Vector2 pos) {
    spawnShockwave(pos, {140,200,255,255});
    for (int i = 0; i < 20; ++i) {
        float angle = (float)i/20.0f*360.0f*DEG2RAD;
        float speed = (float)GetRandomValue(60,160);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        Particle p(pos, vel, {180,230,255,255}, (float)GetRandomValue(3,7), 0.6f, ParticleShape::Square, true);
        p.type    = ParticleType::FreezeRing;
        p.gravity = 0.0f;
        p.drag    = 0.93f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnBurnRing(Vector2 pos) {
    spawnShockwave(pos, {255,80,0,255});
    for (int i = 0; i < 20; ++i) {
        float angle = (float)i/20.0f*360.0f*DEG2RAD;
        float speed = (float)GetRandomValue(60,180);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        Color col = (i%2==0)?Color{255,80,0,255}:Color{255,200,50,255};
        Particle p(pos, vel, col, (float)GetRandomValue(3,8), 0.5f, ParticleShape::Circle, true);
        p.type    = ParticleType::BurnRing;
        p.gravity = -20.0f;
        p.drag    = 0.94f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnNeonGlow(Vector2 pos, Color col) {
    for (int i = 0; i < 12; ++i) {
        float angle = GetRandomValue(0,360)*DEG2RAD;
        float speed = (float)GetRandomValue(40,180);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        Particle p(pos, vel, col, (float)GetRandomValue(4,10), 0.8f, ParticleShape::Circle, true);
        p.type    = ParticleType::NeonGlow;
        p.gravity = 0.0f;
        p.drag    = 0.97f;
        p.scaleEnd= 0.2f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnSoulFragment(Vector2 pos) {
    for (int i = 0; i < 8; ++i) {
        float angle = GetRandomValue(0,360)*DEG2RAD;
        float speed = (float)GetRandomValue(40,140);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed - 80.0f};
        Color col = (i%2==0)?Color{200,200,255,255}:Color{150,100,255,255};
        Particle p(pos, vel, col, (float)GetRandomValue(4,8), 1.0f, ParticleShape::Circle, true);
        p.type    = ParticleType::SoulFragment;
        p.gravity = -60.0f;
        p.drag    = 0.98f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnShieldBreak(Vector2 pos) {
    for (int i = 0; i < 24; ++i) {
        float angle = (float)i/24.0f*360.0f*DEG2RAD;
        float speed = (float)GetRandomValue(100,300);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        Color col = (i%3==0)?Color{0,180,255,255}:(i%3==1)?Color{100,220,255,255}:WHITE;
        Particle p(pos, vel, col, (float)GetRandomValue(3,8), 0.45f, ParticleShape::Square, true);
        p.type    = ParticleType::ShieldBreak;
        p.gravity = 30.0f;
        p.drag    = 0.90f;
        particles.push_back(p);
    }
    spawnShockwave(pos, {0,200,255,255});
}

void ParticleSystem::spawnEnergyOrb(Vector2 pos, Color col) {
    for (int i = 0; i < 10; ++i) {
        float angle = (float)i/10.0f*360.0f*DEG2RAD;
        float speed = (float)GetRandomValue(20,80);
        Vector2 vel = {std::cos(angle)*speed, std::sin(angle)*speed};
        Particle p(pos, vel, col, (float)GetRandomValue(5,12), 0.9f, ParticleShape::Circle, true);
        p.type    = ParticleType::EnergyOrb;
        p.gravity = 0.0f;
        p.drag    = 0.99f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnSmokeCloud(Vector2 pos) {
    for (int i = 0; i < 6; ++i) {
        Vector2 offset = {(float)GetRandomValue(-15,15),(float)GetRandomValue(-15,15)};
        Vector2 p2 = {pos.x+offset.x, pos.y+offset.y};
        Vector2 vel = {(float)GetRandomValue(-10,10),(float)GetRandomValue(-25,-5)};
        Color col = {(unsigned char)GetRandomValue(60,100),(unsigned char)GetRandomValue(60,100),(unsigned char)GetRandomValue(60,100),255};
        Particle p(p2, vel, col, (float)GetRandomValue(10,20), 1.0f, ParticleShape::Circle, false);
        p.type    = ParticleType::SmokeCloud;
        p.gravity = -8.0f;
        p.drag    = 0.99f;
        p.scaleEnd= 2.0f;
        particles.push_back(p);
    }
}

void ParticleSystem::spawnAshDrift(Vector2 pos) {
    for (int i = 0; i < 8; ++i) {
        Vector2 vel = {(float)GetRandomValue(-30,30),(float)GetRandomValue(-40,-10)};
        Color col = {(unsigned char)GetRandomValue(180,220),(unsigned char)GetRandomValue(160,200),(unsigned char)GetRandomValue(140,180),255};
        Particle p(pos, vel, col, (float)GetRandomValue(1,3), GetRandomValue(8,20)/10.0f, ParticleShape::Circle, false);
        p.type    = ParticleType::AshDrift;
        p.gravity = 5.0f;
        p.drag    = 0.99f;
        particles.push_back(p);
    }
}

// ─── Update / Render ─────────────────────────────────────────────────────────

void ParticleSystem::update(float dt) {
    for (auto it = particles.begin(); it != particles.end();) {
        it->update(dt);
        if (!it->active) it = particles.erase(it);
        else ++it;
    }
    // Teto de seguranca: nunca mais que ~1400 particulas vivas (descarta as
    // mais antigas) — protege o FPS em explosoes/ondas grandes.
    const size_t kMaxParticles = 1400;
    if (particles.size() > kMaxParticles)
        particles.erase(particles.begin(),
                        particles.begin() + (particles.size() - kMaxParticles));
}

void ParticleSystem::render() const {
    for (const auto& p : particles) p.render();
}
