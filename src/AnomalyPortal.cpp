#include "AnomalyPortal.h"
#include "Enemy.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>

// ─── AnomalyPortal ───────────────────────────────────────────────────────────

void AnomalyPortal::setup(Vector2 pos, int tierLevel) {
    position  = pos;
    tier      = tierLevel;
    state     = PortalState::Opening;
    stateTimer = 0.0f;
    spawnTimer = 0.0f;
    totalSpawned = 0;
    pulseTimer = 0.0f;
    arcTimer   = 0.0f;
    particleAngle = 0.0f;

    switch (tier) {
        case 1:
            health = maxHealth = 350.0f;
            spawnRate  = 5.0f;
            baseRadius = 45.0f;
            portalColor = {120, 0, 200, 255};
            break;
        case 2:
            health = maxHealth = 600.0f;
            spawnRate  = 3.5f;
            baseRadius = 65.0f;
            portalColor = {0, 50, 220, 255};
            break;
        case 3:
        default:
            health = maxHealth = 1000.0f;
            spawnRate  = 2.0f;
            baseRadius = 90.0f;
            portalColor = {200, 0, 80, 255};
            break;
    }
}

void AnomalyPortal::update(float dt) {
    pulseTimer    += dt;
    arcTimer      += dt * 2.5f;
    particleAngle += dt * 1.8f;

    switch (state) {
        case PortalState::Opening:
            stateTimer += dt;
            if (stateTimer >= 1.5f) { state = PortalState::Active; stateTimer = 0.0f; }
            break;
        case PortalState::Active:
            spawnTimer += dt;
            break;
        case PortalState::Closing:
            stateTimer += dt;
            if (stateTimer >= 2.0f) { state = PortalState::Closed; }
            break;
        case PortalState::Closed:
            break;
    }
}

void AnomalyPortal::takeDamage(float dmg) {
    if (state != PortalState::Active) return;
    health -= dmg;
    if (health <= 0.0f) { health = 0.0f; startClosing(); }
}

void AnomalyPortal::startClosing() {
    state      = PortalState::Closing;
    stateTimer = 0.0f;
}

Vector2 AnomalyPortal::getSpawnPosition() const {
    float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
    return {
        position.x + cosf(angle) * baseRadius * 0.5f,
        position.y + sinf(angle) * baseRadius * 0.9f
    };
}

void AnomalyPortal::renderOpeningAnim() const {
    float progress = stateTimer / 1.5f;
    float px = position.x;
    float py = position.y;
    float h  = baseRadius * 1.7f * progress;
    float w  = baseRadius * 0.45f * progress;

    // Tear in space — vertical rect growing
    DrawRectangle((int)(px - w * 0.5f), (int)(py - h * 0.5f),
                  (int)w, (int)h, {5, 0, 15, 255});

    // Glowing border
    Color edgeCol = ColorAlpha(portalColor, progress);
    DrawRectangleLinesEx({px - w * 0.5f - 2, py - h * 0.5f - 2, w + 4, h + 4},
                         2.0f, edgeCol);

    // Electric sparks radiating outward
    for (int i = 0; i < 6; i++) {
        float a = (i / 6.0f) * PI * 2.0f + arcTimer;
        float len = baseRadius * 0.5f * progress;
        Vector2 from = {px + cosf(a) * w * 0.4f, py + sinf(a) * h * 0.4f};
        float na = a + sinf(arcTimer + i) * 0.6f;
        Vector2 to   = {from.x + cosf(na) * len, from.y + sinf(na) * len};
        DrawLineEx(from, to, 1.2f, ColorAlpha(portalColor, 0.8f * progress));
    }

    // Center flash
    float flashR = 6.0f * progress;
    DrawCircle((int)px, (int)py, flashR + 2.0f, ColorAlpha(WHITE, 0.4f * progress));
    DrawCircle((int)px, (int)py, flashR, ColorAlpha(portalColor, 0.9f * progress));
}

void AnomalyPortal::renderActivePortal() const {
    float px = position.x;
    float py = position.y;
    float r  = baseRadius;
    float pulse = sinf(pulseTimer * 3.0f);

    // 1. Dark void center
    DrawEllipse((int)px, (int)py, r * 0.45f, r * 0.85f, {5, 0, 15, 255});

    // 2. Layered glowing border
    for (int layer = 3; layer >= 0; layer--) {
        float lr    = r * (1.0f + layer * 0.12f);
        float alpha = (180.0f - layer * 38.0f) / 255.0f;
        alpha *= (0.7f + pulse * 0.3f);
        DrawEllipseLines((int)px, (int)py, lr * 0.45f, lr * 0.85f,
                         ColorAlpha(portalColor, alpha));
    }

    // 3. Electric arcs (8 arcs)
    for (int arc = 0; arc < 8; arc++) {
        float angle   = (arc / 8.0f) * PI * 2.0f + arcTimer;
        float arcLen  = r * 0.4f;
        Vector2 aFrom = {px + cosf(angle) * r * 0.45f,
                         py + sinf(angle) * r * 0.85f};
        float noiseA  = angle + sinf(arcTimer + arc) * 0.8f;
        Vector2 aTo   = {aFrom.x + cosf(noiseA) * arcLen,
                         aFrom.y + sinf(noiseA) * arcLen};
        DrawLineEx(aFrom, aTo, 1.5f, ColorAlpha(portalColor, 0.85f));
    }

    // 4. Orbiting particles (12)
    for (int p = 0; p < 12; p++) {
        float pAngle = particleAngle + (p / 12.0f) * PI * 2.0f;
        float pDist  = r * (0.9f + sinf(pulseTimer * 2.0f + p) * 0.15f);
        Vector2 pPos = {px + cosf(pAngle) * pDist * 0.45f,
                        py + sinf(pAngle) * pDist * 0.85f};
        float pSize  = 2.0f + sinf(pulseTimer * 3.0f + p) * 1.0f;
        if (pSize < 1.0f) pSize = 1.0f;
        DrawCircle((int)pPos.x, (int)pPos.y, pSize, ColorAlpha(portalColor, 0.8f));
    }

    // 5. Suction lines pointing inward
    for (int s = 0; s < 16; s++) {
        float sAngle = (s / 16.0f) * PI * 2.0f;
        float sFrom  = r * (1.5f + sinf(pulseTimer * 2.0f + s) * 0.2f);
        float sTo    = r * 0.5f;
        Vector2 sStart = {px + cosf(sAngle) * sFrom * 0.6f,
                          py + sinf(sAngle) * sFrom};
        Vector2 sEnd   = {px + cosf(sAngle) * sTo * 0.45f,
                          py + sinf(sAngle) * sTo * 0.85f};
        DrawLineEx(sStart, sEnd, 0.7f, ColorAlpha(portalColor, 0.22f));
    }

    // 6. HP bar (when player in range)
    if (playerInRange) {
        float hpPct = (maxHealth > 0.0f) ? (health / maxHealth) : 0.0f;
        int   bw    = (int)(r * 2.0f);
        int   bx    = (int)px - bw / 2;
        int   by    = (int)(py - r * 0.9f) - 20;
        DrawRectangle(bx, by, bw, 8, {50, 0, 0, 200});
        DrawRectangle(bx, by, (int)(bw * hpPct), 8, portalColor);
        DrawRectangleLinesEx({(float)bx, (float)by, (float)bw, 8.0f}, 1.0f, {200, 200, 200, 140});
        const char* lbl = "ANOMALIA";
        DrawText(lbl, bx + bw / 2 - MeasureText(lbl, 10) / 2, by - 14, 10,
                 ColorAlpha(portalColor, 0.9f));
    }

    // 7. Warning pulse before spawn
    if (spawnTimer > spawnRate * 0.75f) {
        float wAlpha = sinf(pulseTimer * 8.0f) * 0.5f + 0.5f;
        DrawText("!", (int)px - 4, (int)py - 10, 22, ColorAlpha({255, 50, 50, 255}, wAlpha));
    }
}

void AnomalyPortal::renderClosingAnim() const {
    float progress = stateTimer / 2.0f;  // 0 → 1
    float px = position.x;
    float py = position.y;
    float r  = baseRadius * (1.0f - progress);

    if (r < 0.5f) {
        // Final flash
        float flashR = 60.0f * (progress - 0.85f) / 0.15f;
        if (flashR > 0) DrawCircle((int)px, (int)py, flashR, ColorAlpha(WHITE, 0.8f * (1.0f - progress)));
        return;
    }

    // Imploding portal
    DrawEllipse((int)px, (int)py, r * 0.45f, r * 0.85f, {5, 0, 15, 255});
    DrawEllipseLines((int)px, (int)py, r * 0.45f, r * 0.85f,
                     ColorAlpha(portalColor, 1.0f - progress));

    // Shockwave ring expanding outward
    float shockR = baseRadius * (1.0f + progress * 1.5f);
    DrawCircleLines((int)px, (int)py, shockR, ColorAlpha(portalColor, (1.0f - progress) * 0.6f));

    // Particles exploding outward
    for (int p = 0; p < 10; p++) {
        float a   = (p / 10.0f) * PI * 2.0f + progress * 5.0f;
        float dist = baseRadius * 1.2f * progress;
        Vector2 pp = {px + cosf(a) * dist, py + sinf(a) * dist};
        DrawCircle((int)pp.x, (int)pp.y, 3.0f * (1.0f - progress),
                   ColorAlpha(portalColor, 1.0f - progress));
    }

    if (progress > 0.85f) {
        float flashA = (progress - 0.85f) / 0.15f;
        DrawCircle((int)px, (int)py, 40.0f * flashA, ColorAlpha(WHITE, 0.6f * flashA));
    }
}

void AnomalyPortal::render() const {
    if (state == PortalState::Closed) return;
    switch (state) {
        case PortalState::Opening: renderOpeningAnim(); break;
        case PortalState::Active:  renderActivePortal(); break;
        case PortalState::Closing: renderClosingAnim(); break;
        default: break;
    }
}

// ─── StormSystem ─────────────────────────────────────────────────────────────

void StormSystem::start() {
    active       = true;
    atmospheric  = false;
    maxIntensity = 1.0f;
    intensity = 0.0f;
    lightningTimer = 0.0f;
    lightningDur   = 0.0f;
    ambientTimer   = 0.0f;
    drops.clear();
    // Spawn 320 rain drops (tempestade forte)
    drops.resize(320);
    for (auto& d : drops) {
        d.pos   = {(float)GetRandomValue(0, 1280), (float)GetRandomValue(0, 720)};
        d.speed = (float)GetRandomValue(400, 700);
        d.alpha = (float)GetRandomValue(30, 80) / 100.0f;
    }
}

void StormSystem::startAtmospheric() {
    // Chuva e vento ambiente para zonas sombrias — mais suave que a tempestade
    // de anomalia, mas sempre visivel.
    if (active && atmospheric) return; // ja ativa
    active       = true;
    atmospheric  = true;
    maxIntensity = 0.75f;
    intensity = 0.0f;
    lightningTimer = 0.0f;
    lightningDur   = 0.0f;
    ambientTimer   = 0.0f;
    drops.clear();
    drops.resize(240);
    for (auto& d : drops) {
        d.pos   = {(float)GetRandomValue(0, 1280), (float)GetRandomValue(0, 720)};
        d.speed = (float)GetRandomValue(350, 620);
        d.alpha = (float)GetRandomValue(25, 70) / 100.0f;
    }
}

void StormSystem::stop() {
    active      = false;
    atmospheric = false;
    intensity   = 0.0f;
    drops.clear();
}

void StormSystem::update(float dt) {
    if (!active) return;

    ambientTimer += dt;
    windPhase    += dt;

    // Ramp intensity up to maxIntensity over ~3s
    if (intensity < maxIntensity) intensity = std::min(maxIntensity, intensity + dt * 0.35f);

    // Rajadas de vento variaveis — o vento muda de força/direção ao longo do tempo
    float windGust = sinf(windPhase * 0.5f) * 0.4f + sinf(windPhase * 1.7f) * 0.15f;
    float windX    = (0.25f + windGust) * 1.0f;  // fator horizontal do vento

    // Move raindrops com vento
    for (auto& d : drops) {
        d.pos.x += d.speed * windX * dt;
        d.pos.y += d.speed * dt;
        if (d.pos.y > 720.0f) { d.pos.y -= 730.0f; d.pos.x = (float)GetRandomValue(-40, 1280); }
        if (d.pos.x > 1320.0f) { d.pos.x -= 1360.0f; }
        if (d.pos.x < -60.0f)  { d.pos.x += 1360.0f; }
    }

    // Lightning timer — atmosferica tem relampagos mais raros
    lightningTimer += dt;
    float baseStrike = atmospheric ? 7.0f : 4.0f;
    float nextStrike = baseStrike + sinf(ambientTimer * 0.3f) * 2.0f;
    if (lightningTimer >= nextStrike) {
        lightningTimer = 0.0f;
        lightningDur   = 0.12f;
    }
    if (lightningDur > 0.0f) lightningDur -= dt;
    if (lightningDur < 0.0f) lightningDur = 0.0f;
}

void StormSystem::render(int screenW, int screenH) const {
    if (!active) return;

    // Dark vignette overlay
    unsigned char darkAlpha = (unsigned char)(int)(70 * intensity);
    DrawRectangle(0, 0, screenW, screenH, {0, 0, 20, darkAlpha});

    // Vignette edges
    int vSize = 80;
    for (int i = 0; i < vSize; i++) {
        unsigned char va = (unsigned char)((vSize - i) * 2 / vSize * (int)(intensity * 60));
        DrawRectangle(0, 0, i, screenH, {0, 0, 0, va});
        DrawRectangle(screenW - i, 0, i, screenH, {0, 0, 0, va});
        DrawRectangle(0, screenH - i, screenW, i, {0, 0, 0, va});
    }

    // Rain — inclinacao acompanha o vento (rajadas)
    float windGust = sinf(windPhase * 0.5f) * 0.4f + sinf(windPhase * 1.7f) * 0.15f;
    float slantX   = (0.25f + windGust) * 22.0f;
    for (const auto& d : drops) {
        Vector2 from = d.pos;
        Vector2 to   = {d.pos.x + slantX, d.pos.y + 20.0f};
        DrawLineEx(from, to, 1.1f, ColorAlpha({190, 210, 255, 255}, d.alpha * intensity));
    }

    // Folhas/poeira levadas pelo vento (reforça a sensação de vento)
    for (int i = 0; i < 18; i++) {
        float t  = fmodf(windPhase * 0.4f + i * 0.37f, 1.0f);
        float fx = fmodf(i * 137.0f + windPhase * 80.0f * (0.5f + slantX * 0.02f), (float)screenW);
        float fy = fmodf(i * 91.0f + windPhase * 30.0f, (float)screenH);
        float a  = (0.3f + 0.2f * sinf(windPhase * 2.0f + i)) * intensity;
        DrawCircle((int)fx, (int)fy, 1.5f, ColorAlpha({160, 150, 130, 255}, a));
    }

    // Lightning flash
    if (lightningDur > 0.0f) {
        float flashA = lightningDur / 0.12f;
        DrawRectangle(0, 0, screenW, screenH, ColorAlpha(WHITE, flashA * 0.28f));
        // Zigzag lightning bolt from top
        int lx = GetRandomValue(200, screenW - 200);
        int segH = screenH / 8;
        Vector2 prev = {(float)lx, 0};
        for (int seg = 0; seg < 8; seg++) {
            Vector2 next = {(float)(lx + GetRandomValue(-60, 60)),
                            (float)(segH * (seg + 1))};
            DrawLineEx(prev, next, 1.8f, ColorAlpha({220, 240, 255, 255}, flashA * 0.9f));
            prev = next;
        }
    }
}

// ─── AnomalySystem ───────────────────────────────────────────────────────────

void AnomalySystem::spawnWave(int zoneW, int zoneH, Vector2 playerPos) {
    portals.clear();
    waveActive = true;
    waveNumber++;
    hudPulse = 0.0f;

    // Candidate positions spread around the zone
    const float CX = (float)zoneW * 0.5f;
    const float CY = (float)zoneH * 0.5f;
    float margin = 320.0f;
    Vector2 candidates[] = {
        {margin,           margin},
        {CX,               margin},
        {(float)zoneW - margin, margin},
        {margin,           CY},
        {(float)zoneW - margin, CY},
        {margin,           (float)zoneH - margin},
        {CX,               (float)zoneH - margin},
        {(float)zoneW - margin, (float)zoneH - margin},
        {CX * 0.5f,        CY},
        {CX * 1.5f,        CY},
    };
    int numCandidates = 10;

    // How many portals and tiers by wave
    struct PortalSpec { int tier; };
    std::vector<PortalSpec> specs;
    if (waveNumber == 1) {
        specs = {{1},{1},{1}};
    } else if (waveNumber == 2) {
        specs = {{1},{1},{2},{2}};
    } else {
        specs = {{1},{2},{2},{3},{3}};
    }

    // Shuffle candidate indices (Fisher-Yates lite)
    int order[10] = {0,1,2,3,4,5,6,7,8,9};
    for (int i = numCandidates - 1; i > 0; i--) {
        int j = GetRandomValue(0, i);
        int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
    }

    int placed = 0;
    for (int i = 0; i < numCandidates && placed < (int)specs.size(); i++) {
        Vector2 cpos = candidates[order[i]];
        if (Vector2Distance(cpos, playerPos) < 400.0f) continue;
        AnomalyPortal p;
        p.setup(cpos, specs[placed].tier);
        portals.push_back(p);
        placed++;
    }

    storm.start();
}

void AnomalySystem::update(float dt, Vector2 playerPos) {
    // A tempestade SEMPRE anima quando ativa (inclusive chuva atmosferica sem onda)
    if (storm.active) storm.update(dt);

    if (!waveActive) {
        waveTimer += dt;
        return;
    }

    hudPulse += dt;

    bool anyActive = false;
    for (auto& p : portals) {
        p.playerInRange = (Vector2Distance(p.position, playerPos) < p.baseRadius * 2.0f + 40.0f);
        p.update(dt);
        if (!p.isDead()) anyActive = true;
    }

    if (!anyActive) {
        waveActive = false;
        // So para a tempestade se NAO for atmosferica (zona sombria mantem a chuva)
        if (!storm.atmospheric) storm.stop();
    }
}

void AnomalySystem::renderWorld() const {
    for (const auto& p : portals) p.render();
}

void AnomalySystem::renderStorm(int screenW, int screenH) const {
    storm.render(screenW, screenH);
}

void AnomalySystem::renderHUD(int screenW, int screenH) const {
    if (!waveActive) return;
    int open = countOpen();
    int total = (int)portals.size();

    // Panel top-center
    int panelW = 260;
    int panelX = screenW / 2 - panelW / 2;
    int panelY = 8;
    DrawRectangle(panelX, panelY, panelW, 42, ColorAlpha(BLACK, 0.78f));
    DrawRectangleLinesEx({(float)panelX, (float)panelY, (float)panelW, 42.0f},
                         1.5f, ColorAlpha({200, 0, 255, 255}, 0.8f));

    // Title
    const char* title = TextFormat("ANOMALIAS: %d / %d ABERTAS", open, total);
    int tw = MeasureText(title, 13);
    Color titleCol = (open > 0) ? Color{255, 60, 60, 255} : Color{0, 220, 80, 255};
    DrawText(title, panelX + panelW / 2 - tw / 2, panelY + 5, 13, titleCol);

    // Portal status icons
    int iconSpacing = panelW / (total + 1);
    for (int i = 0; i < total; i++) {
        int ix = panelX + iconSpacing * (i + 1);
        int iy = panelY + 24;
        bool closed = portals[i].isDead() || portals[i].state == PortalState::Closing;
        Color ic = closed ? Color{80, 80, 80, 200}
                          : ColorAlpha(portals[i].portalColor,
                                       0.65f + sinf(hudPulse * 3.0f) * 0.35f);
        DrawCircle(ix, iy, 7.0f, ic);
        DrawCircleLines(ix, iy, 7.0f, ColorAlpha(WHITE, 0.6f));
    }

    // All closed reward message
    if (open == 0) {
        const char* msg = "TODAS ANOMALIAS FECHADAS! +2500 XP";
        int mw = MeasureText(msg, 15);
        DrawRectangle(screenW / 2 - mw / 2 - 12, screenH / 2 - 28, mw + 24, 32,
                      ColorAlpha(BLACK, 0.85f));
        DrawText(msg, screenW / 2 - mw / 2, screenH / 2 - 22, 15, {0, 255, 100, 255});
    }
}

bool AnomalySystem::checkProjectileHit(Vector2 projPos, float projRadius, float damage) {
    for (auto& p : portals) {
        if (!p.isActive()) continue;
        // Check collision with elliptical portal (approximate with circle using avg radius)
        float avgR = (p.baseRadius * 0.45f + p.baseRadius * 0.85f) * 0.5f;
        if (Vector2Distance(projPos, p.position) <= avgR + projRadius) {
            p.takeDamage(damage);
            return true;
        }
    }
    return false;
}

bool AnomalySystem::pollSpawn(int& outEnemyTypeInt, Vector2& outPos) {
    for (auto& p : portals) {
        if (!p.isActive()) continue;
        if (p.spawnTimer >= p.spawnRate) {
            p.spawnTimer = 0.0f;
            p.totalSpawned++;
            outPos = p.getSpawnPosition();

            int r = GetRandomValue(0, 99);
            int type;
            switch (p.tier) {
                case 1:
                    // Scout(40%) Ghost(30%) Zombie(20%) Kamikaze(10%)
                    if      (r < 40) type = (int)EnemyType::Scout;
                    else if (r < 70) type = (int)EnemyType::Ghost;
                    else if (r < 90) type = (int)EnemyType::Zombie;
                    else             type = (int)EnemyType::Kamikaze;
                    break;
                case 2:
                    // ZombieRager(25%) Hydra(25%) GhostElite(20%) HunterDrone(30%)
                    if      (r < 25) type = (int)EnemyType::ZombieRager;
                    else if (r < 50) type = (int)EnemyType::Hydra;
                    else if (r < 70) type = (int)EnemyType::GhostElite;
                    else             type = (int)EnemyType::HunterDrone;
                    break;
                case 3:
                default:
                    // MorphX(20%) Broodmother(15%) ZombieRager(25%) OrcCibernetico(40%)
                    if      (r < 20) type = (int)EnemyType::MorphX;
                    else if (r < 35) type = (int)EnemyType::Broodmother;
                    else if (r < 60) type = (int)EnemyType::ZombieRager;
                    else             type = (int)EnemyType::OrcCibernetico;
                    break;
            }
            outEnemyTypeInt = type;
            return true;
        }
    }
    return false;
}

int AnomalySystem::countOpen() const {
    int count = 0;
    for (const auto& p : portals)
        if (!p.isDead()) count++;
    return count;
}


