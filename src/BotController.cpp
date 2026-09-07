#include "BotController.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <queue>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ─── Helpers ─────────────────────────────────────────────────────────────────

static Vector2 safeNormalize(Vector2 v) {
    float len = Vector2Length(v);
    if (len < 0.001f) return {1.0f, 0.0f};
    return {v.x / len, v.y / len};
}

// 8-direction table: N, NE, E, SE, S, SW, W, NW (angles in radians)
static const float k8DirAngles[8] = {
    0.0f,                               // E
    (float)(M_PI * 0.25),               // NE
    (float)(M_PI * 0.5),                // N
    (float)(M_PI * 0.75),               // NW
    (float)(M_PI),                      // W
    (float)(M_PI * 1.25),               // SW
    (float)(M_PI * 1.5),                // S
    (float)(M_PI * 1.75),               // SE
};

void BotController::addLog(const std::string& msg) {
    log.push_back(msg);
    if ((int)log.size() > 20) log.erase(log.begin());
}

// Reset COMPLETO between partidas: telemetria, maquina of estados, timers, route
// cacheada and sensores. Preserva active/autoTest/testDuration for the autotest
// continue rodando in the new match. (toggle() only fazia um reset partial, and the
// timers/contadores antigos vazavam of uma match for the other.)
void BotController::reset() {
    bool  keepActive   = active;
    bool  keepAutoTest = autoTest;
    float keepDuration = testDuration;
    *this = BotController();
    active       = keepActive;
    autoTest     = keepAutoTest;
    testDuration = keepDuration;
}

int BotController::countEnemiesInRadius(const std::vector<Vector2>& positions,
                                         Vector2 center, float radius) const {
    int count = 0;
    for (const auto& p : positions)
        if (Vector2Distance(center, p) <= radius) count++;
    return count;
}

// ─── Anti-wall (improved 8-direction sweep) ──────────────────────────────────
//
// Strategy:
//   - Track distance to current goal each frame.
//   - If the player hasn't moved > 3px in the last frame, increment stuckTimer.
//   - At 2s stuck: pick the new escape direction from the 8-direction table, cycling
//     through them only we try all compass headings before repeating.
//   - Track longStuckTimer separately for bug reporting (> 10s).
//   - When not stuck, decay the angle offset back toward 0 only we return to the
//     correct heading once clear of the obstacle.
//
// Diferenca angular normalizada to [-PI, PI]
static float angleDiff(float the, float b) {
    float d = the - b;
    while (d >  (float)M_PI) d -= 2.0f * (float)M_PI;
    while (d < -(float)M_PI) d += 2.0f * (float)M_PI;
    return d;
}

// Rastreio of "preso" — usado both pelo pathfinding global the pelo desvio
// reativo of fallback, and alimenta the telemetria of stuck events of the report.
void BotController::updateStuckTracking(Vector2 currentPos, float dt) {
    // BUG QUE ISTO CORRIGE: the versao previous comparava the deslocamento of UM
    // FRAME with 3 px. A 60 fps and speed 155 u/s the character anda 2,58 px by
    // frame — i.and., andando normally ele era classificado as STUCK the
    // time all. O bot vivia in "escape", never engajava (0 kills) and the
    // report acusava travamento without haver travamento none.
    // Agora the medida and by WINDOW DE TEMPO: distance accumulated over 0.5 s.
    stuckAccum += Vector2Distance(currentPos, lastPos);
    lastPos     = currentPos;
    stuckSample += dt;
    if (stuckSample < 0.5f) return;

    // 0,5 s of caminhada normal percorre ~75 px; 15 px and floor of sobra to
    // distinguir "empurrando wall" of "andando devagar".
    bool blockedNow = (stuckAccum < 15.0f);
    stuckAccum  = 0.0f;
    stuckSample = 0.0f;

    if (blockedNow) {
        stuckTimer     += 0.5f;
        longStuckTimer += 0.5f;
        isStuck = true;
        if (stuckTimer >= 2.0f && !stuckCounted) { stuckEvents++; stuckCounted = true; }
    } else {
        stuckTimer     = 0.0f;
        longStuckTimer = 0.0f;
        isStuck        = false;
        stuckCounted   = false;
    }

    if (longStuckTimer >= 10.0f) {
        longStuckTimer = 0.0f;
        longStuckEvents++;
        issueLog.push_back(TextFormat("STUCK for >10s at (%.0f,%.0f) — possible pathfinding bug",
                                      currentPos.x, currentPos.y));
        addLog("ALERT: stuck >10s — reported");
    }
}

// ─── Pathfinding global (BFS in the grade of tiles transitaveis) ─────────────────
//
// Substitui the desvio reativo of sensores locais (that oscilava in cantos concavos)
// by uma busca in width completa numa window of tiles around of the bot. A BFS
// explora all the region acessivel and generates uma route of waypoints until the alvo. If the
// alvo for inalcancavel, the route goes until the celula acessivel more next of the alvo —
// the that permite contornar bolsoes in "U" retreatsndo of verdade pela output real.
Vector2 BotController::computePathDir(Vector2 from, Vector2 to) {
    const float TS = 64.0f;   // tileSize (Tilemap::tileSize)
    const int   R  = 34;      // radius of the window of busca in tiles (~2176px)
    const int   W  = 2 * R + 1;

    bool need = cachedPath.empty()
             || Vector2Distance(to, lastPathTarget) > 96.0f
             || repathTimer <= 0.0f;

    if (need && wallQuery) {
        repathTimer    = 0.25f;
        lastPathTarget = to;
        cachedPath.clear();

        int pgx = (int)(from.x / TS), pgy = (int)(from.y / TS);
        int tgx = (int)(to.x   / TS), tgy = (int)(to.y   / TS);
        int ox  = pgx - R, oy = pgy - R;             // tile of world in the celula (0,0)

        // alvo clampeado to inside the window
        tgx = std::max(ox, std::min(ox + W - 1, tgx));
        tgy = std::max(oy, std::min(oy + W - 1, tgy));

        // grade of transitabilidade (center of cada tile)
        std::vector<char> walk(W * W);
        for (int gy = 0; gy < W; ++gy)
            for (int gx = 0; gx < W; ++gx) {
                float cx = (ox + gx) * TS + TS * 0.5f;
                float cy = (oy + gy) * TS + TS * 0.5f;
                walk[gy * W + gx] = wallQuery({cx, cy}) ? 0 : 1;
            }

        int startI = (pgy - oy) * W + (pgx - ox);
        int goalI  = (tgy - oy) * W + (tgx - ox);
        walk[startI] = 1;   // ensures that the celula of the bot and transitavel

        std::vector<int> parent(W * W, -2);          // -2 = not visitado
        std::queue<int>  q;
        q.push(startI);
        parent[startI] = -1;

        int   bestI = startI;
        float bestD = std::hypot((float)(tgx - pgx), (float)(tgy - pgy));
        int   farI  = startI;      // celula alcancavel more LONGE of the bot
        float farD  = 0.0f;

        static const int dxs[8] = { 1,-1, 0, 0, 1, 1,-1,-1 };
        static const int dys[8] = { 0, 0, 1,-1, 1,-1, 1,-1 };

        while (!q.empty()) {
            int cur = q.front(); q.pop();
            int cgx = cur % W, cgy = cur / W;
            float dd = std::hypot((float)((ox + cgx) - tgx), (float)((oy + cgy) - tgy));
            if (dd < bestD) { bestD = dd; bestI = cur; }
            float fd = std::hypot((float)((ox + cgx) - pgx), (float)((oy + cgy) - pgy));
            if (fd > farD) { farD = fd; farI = cur; }
            if (cur == goalI) break;

            for (int k = 0; k < 8; ++k) {
                int nx = cgx + dxs[k], ny = cgy + dys[k];
                if (nx < 0 || ny < 0 || nx >= W || ny >= W) continue;
                int ni = ny * W + nx;
                if (parent[ni] != -2) continue;
                if (!walk[ni]) continue;
                if (k >= 4) {  // diagonal: impede cortar quina between duas walls
                    if (!walk[cgy * W + nx] || !walk[ny * W + cgx]) continue;
                }
                parent[ni] = cur;
                q.push(ni);
            }
        }

        // destino of emergencia: the point acessivel more distante that the BFS found
        hasFarReach = (farD > 4.0f);
        farthestReachable = { (ox + farI % W) * TS + TS * 0.5f,
                              (oy + farI / W) * TS + TS * 0.5f };

        // reconstroi the partir of the celula acessivel more next of the alvo
        std::vector<Vector2> rev;
        for (int node = bestI; node != -1; node = parent[node]) {
            int gx = node % W, gy = node / W;
            rev.push_back({ (ox + gx) * TS + TS * 0.5f, (oy + gy) * TS + TS * 0.5f });
        }
        for (int i = (int)rev.size() - 1; i >= 0; --i) cachedPath.push_back(rev[i]);
        if (!cachedPath.empty()) cachedPath.erase(cachedPath.begin()); // descarta the tile current
    }

    // consome waypoints already alcancados
    while (!cachedPath.empty() && Vector2Distance(from, cachedPath.front()) < 36.0f)
        cachedPath.erase(cachedPath.begin());

    if (cachedPath.empty())
        return safeNormalize({ to.x - from.x, to.y - from.y });

    Vector2 wp = cachedPath.front();
    return safeNormalize({ wp.x - from.x, wp.y - from.y });
}

Vector2 BotController::computeAntiWall(Vector2 desired, Vector2 currentPos, float dt) {
    // ── Rastreio of "preso" (to report and to escapar of bolsoes) ────────
    updateStuckTracking(currentPos, dt);

    Vector2 dn          = safeNormalize(desired);
    float   desiredAng  = std::atan2(dn.y, dn.x);

    // Quantas directions are livres?
    int openCount = 0;
    for (int d = 0; d < 8; d++) if (!blockedDir[d]) openCount++;

    // Cercado by all the lados — empurra in the direction desejada (last resource)
    if (openCount == 0) return dn;

    // If the direction desejada is essencialmente livre and not estamos presos,
    // segue reto for the alvo (without zigue-zague unnecessary).
    {
        // checks the setor of 8-dir more alinhado to the desejo
        int   nearestIdx = 0; float nearestDelta = 1e9f;
        for (int d = 0; d < 8; d++) {
            float delta = std::fabs(angleDiff(k8DirAngles[d], desiredAng));
            if (delta < nearestDelta) { nearestDelta = delta; nearestIdx = d; }
        }
        if (!blockedDir[nearestIdx] && !isStuck) {
            return dn;
        }
    }

    // Caso contrario: escolhe the direction ABERTA more next of the alvo.
    // Se estamos presos ha um time, gira the preferencia to leave of bolsoes
    // concavos (cantos) instead of insistir in the same direction.
    float stuckBias = 0.0f;
    if (stuckTimer > 0.5f) {
        // (the contagem of stuckEvents mora in updateStuckTracking — here era
        // codigo dead that never incrementava)
        // gira the preferencia ~90 graus conforme the time preso aumenta
        stuckBias = (stuckEscapeDir > 0 ? 1.0f : -1.0f) * (float)(M_PI * 0.5);
        // alterna the lado of fuga the cada ~1.5s preso
        if (stuckTimer > 1.5f) { stuckEscapeDir = -stuckEscapeDir; stuckTimer = 0.6f; }
    }
    float targetAng = desiredAng + stuckBias;

    int   bestDir = -1; float bestDelta = 1e9f;
    for (int d = 0; d < 8; d++) {
        if (blockedDir[d]) continue;
        float delta = std::fabs(angleDiff(k8DirAngles[d], targetAng));
        if (delta < bestDelta) { bestDelta = delta; bestDir = d; }
    }
    if (bestDir < 0) return dn;

    float the = k8DirAngles[bestDir];
    return {std::cos(the), std::sin(the)};
}

// ─── Report ──────────────────────────────────────────────────────────────────

bool BotController::passed(std::vector<std::string>* reasons) const {
    auto fail = [&](const std::string& why) { if (reasons) reasons->push_back(why); };
    bool ok = true;
    float avg = (fpsSamples > 0) ? (fpsAccum / fpsSamples) : 0.0f;
    // Limiares deliberadamente FROUXOS: the portao gets quebra grave (crash,
    // travamento, game that not roda), not briga by 2 fps.
    if (avg < 45.0f)            { ok = false; fail(TextFormat("Average FPS %.0f < 45", avg)); }
    if (longStuckEvents > 0)    { ok = false; fail(TextFormat("%d stuck event(s) > 10s", longStuckEvents)); }
    if (deathCount > 3)         { ok = false; fail(TextFormat("%d deaths in the row", deathCount)); }
    if (killCount == 0)         { ok = false; fail("none enemy defeated (combat quebrado?)"); }
    if (totalDistance < 500.0f) { ok = false; fail("bot praticamente not andou (movement travado?)"); }
    // Blinda the achievement of the ciclo (avanco of phase pelo portal) contra regressao
    // silenciosa: run longo without NENHUMA zone avancada = mecanica central quebrada.
    if (testDuration >= 180.0f && zonesVisited == 0)
        { ok = false; fail("nenhuma zone avancada in run >= 180s (portal/phase regrediu?)"); }
    return ok;
}

void BotController::writeReport(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return;

    float avgFPS = (fpsSamples > 0) ? (fpsAccum / fpsSamples) : 0.0f;

    f << "========================================================\n";
    f << "  DARKNET BOT - SESSION REPORT\n";
    f << "========================================================\n\n";

    f << "[TECHNICAL PERFORMANCE]\n";
    f << "  Average FPS : " << (int)avgFPS << "\n";
    f << "  Minimum FPS  : " << (int)minFPS << "\n";
    f << "  Maximum FPS  : " << (int)maxFPS << "\n";
    if (minFPS < 40.0f)
        f << "  PROBLEM: FPS fell below 40 - optimization needed\n";

    f << "\n[COMBAT]\n";
    f << "  Enemies killed : " << killCount << "\n";
    f << "  Melee attacks   : " << meleeHits << "\n";
    f << "  Skills fired    : " << skillsFired << "\n";
    f << "    Skill 1 (Laser)    : " << skillUsageCounts[0] << "x\n";
    f << "    Skill 2 (EMP)      : " << skillUsageCounts[1] << "x\n";
    f << "    Skill 3 (Grenade)  : " << skillUsageCounts[2] << "x\n";
    f << "    Skill 4 (Overload): " << skillUsageCounts[3] << "x\n";
    f << "    Skill 5 (Barrier) : " << skillUsageCounts[4] << "x\n";
    f << "    Skill 6 (Burst)   : " << skillUsageCounts[5] << "x\n";
    if (killCount == 0)
        f << "  PROBLEM: No enemies killed - combat is not working\n";
    if (meleeHits == 0)
        f << "  PROBLEM: Melee did not trigger - attack system bug\n";

    f << "\n[ITEM COLLECTION]\n";
    f << "  Items chased   : " << itemsChased << "\n";
    f << "  Items collected: " << itemsCollected << "\n";
    if (itemsChased > 0 && itemsCollected == 0)
        f << "  PROBLEM: Bot chased items but collected none (key E?)\n";

    f << "\n[EXPLORATION AND PROGRESS]\n";
    f << "  Areas explored  : " << areasExplored << "/4 quadrants\n";
    f << "  Advanced zones  : " << zonesVisited << "\n";
    f << "  Total distance  : " << (int)totalDistance << " px\n";
    if (areasExplored < 2)
        f << "  PROBLEM: Player stuck in the corner - map has blocked areas?\n";
    if (zonesVisited == 0 && testTimer > 60.0f)
        f << "  PROBLEM: Bot did not advance zones in 60s\n";

    f << "\n[SURVIVAL]\n";
    f << "  Deaths            : " << deathCount << "\n";
    f << "  Lowest HP         : " << (int)lowestHP << "\n";
    f << "  Danger moments    : " << dangersZones << " (HP < 25%)\n";
    f << "  Total damage taken: " << (int)totalDmgTaken << "\n";
    f << "  Damage events     : " << damageEvents << "\n";
    if (deathCount > 3)
        f << "  PROBLEM: Many deaths - difficulty too high or HP too low\n";
    if (dangersZones > 5)
        f << "  SUGGESTION: Add more healing / kits to the map\n";

    f << "\n[PATHFINDING / STUCK]\n";
    f << "  Stuck events (>2s)   : " << stuckEvents << "\n";
    f << "  Critical stuck (>10s): " << longStuckEvents << "\n";
    if (longStuckEvents > 0)
        f << "  BUG: Bot stayed stuck for more than 10s in " << longStuckEvents
          << " occurrence(s) - review collision/map\n";

    f << "\n[ENTITY DIAGNOSTIC (peaks)]\n";
    f << "  Enemies (peak)            : " << peakEnemies << "\n";
    f << "  Player projectiles (peak) : " << peakProjectiles << "\n";
    f << "  Enemy projectiles (peak)  : " << peakEnemyProj << "\n";
    f << "  Items on floor (peak)     : " << peakItems << "\n";
    f << "  XP orbs (peak)            : " << peakOrbs << "\n";
    f << "  Allied units (peak)       : " << peakUnits << "\n";
    f << "  At lowest FPS (" << (int)fpsLowValue << "): enemies=" << fpsLowEnemies
      << " projectiles=" << fpsLowProj << "\n";
    f << "  Worst update() time       : " << peakUpdateMs << " ms\n";
    f << "  Worst render() time       : " << peakRenderMs << " ms\n";

    f << "\n[DETECTED PROBLEMS]\n";
    if (issueLog.empty()) {
        f << "  No critical problem detected\n";
    } else {
        for (const auto& issue : issueLog)
            f << "  - " << issue << "\n";
    }

    f << "\n[ACTIVITY LOG (latest actions)]\n";
    for (const auto& entry : log)
        f << "  " << entry << "\n";

    f << "\n[SUGGESTED IMPROVEMENT PRIORITIES]\n";
    int pri = 1;
    if (minFPS < 40.0f)
        f << "  " << pri++ << ". Optimize rendering - low FPS detected\n";
    if (killCount < 5)
        f << "  " << pri++ << ". Fix combat system - few kills\n";
    if (deathCount > 3)
        f << "  " << pri++ << ". Balance difficulty - many deaths\n";
    if (dangersZones > 5)
        f << "  " << pri++ << ". Add more HealthPacks to the map\n";
    if (areasExplored < 2)
        f << "  " << pri++ << ". Fix map generation - inaccessible areas\n";
    if (itemsChased > 0 && itemsCollected == 0)
        f << "  " << pri++ << ". Check item pickup key (E)\n";
    if (itemsCollected < 3)
        f << "  " << pri++ << ". Check item spawn - few items found\n";
    if (skillsFired < 5)
        f << "  " << pri++ << ". Check skills - cooldowns too long?\n";
    if (longStuckEvents > 0)
        f << "  " << pri++ << ". Investigate pathfinding - bot stayed stuck >10s\n";

    f << "\n========================================================\n";
    f << "  Session duration: " << (int)testTimer << "s\n";
    f << "  Frames processed: " << frameCount << "\n";
    f << "========================================================\n";
    f.close();
}

// ─── Main update — state machine ─────────────────────────────────────────────

BotController::BotDecision BotController::update(
        float dt,
        Vector2 playerPos,
        float   playerRange,
        float   playerHP,
        float   playerMaxHP,
        int     playerLevel,
        int     playerCredits,
        float   currentFPS,
        const std::vector<Vector2>& enemyPositions,
        const std::vector<Vector2>& itemPositions,
        const bool skillsReady[6],
        const std::vector<Vector2>& portalPositions,
        int     openAnomalyPortals)
{
    BotDecision dec;
    if (!active) return dec;

    frameCount++;
    attackTimer -= dt;
    logicTimer  -= dt;
    reportTimer -= dt;
    skillTimer  -= dt;
    repathTimer -= dt;
    orbitAngle  += dt * 1.2f;

    // ── Auto-test timer ───────────────────────────────────────────────────────
    if (autoTest) {
        testTimer += dt;
        if (testTimer >= testDuration) {
            dec.shouldQuit = true;
            addLog(TextFormat("TEST COMPLETED (%.0fs)", testTimer));
            writeReport("bot_report.txt");
            return dec;
        }
    }

    // ── FPS tracking ──────────────────────────────────────────────────────────
    // Um frame of CARGA (worldgen of match/phase, dt > 0,25s) envenena the media
    // movel of the GetFPS() by ~0,5s: the report acusava "FPS minimum 6" with the game
    // the 60 — era the frame of loading entering in the window. Quarantine of 1s apos
    // qualquer frame desses; only if mede FPS of gameplay.
    if (dt > 0.25f) fpsQuarantine = 1.0f;
    else if (fpsQuarantine > 0.0f) fpsQuarantine -= dt;
    if (currentFPS > 0.0f && fpsQuarantine <= 0.0f) {
        fpsAccum += currentFPS; fpsSamples++;
        if (currentFPS < minFPS) minFPS = currentFPS;
        if (currentFPS > maxFPS) maxFPS = currentFPS;
        if (currentFPS < 30.0f && logicTimer <= 0.0f)
            issueLog.push_back(TextFormat("FPS critical: %.0f", currentFPS));
    }

    // ── HP tracking ───────────────────────────────────────────────────────────
    if (playerMaxHP > 0.0f) {
        float hpPct = playerHP / playerMaxHP;
        if (playerHP < lowestHP) lowestHP = playerHP;
        if (hpPct < 0.25f && !wasLowHP) {
            wasLowHP = true;
            dangersZones++;
            addLog(TextFormat("DANGER! HP=%.0f/%.0f", playerHP, playerMaxHP));
            issueLog.push_back(TextFormat("HP critical at %.0f%%", hpPct * 100));
        }
        if (hpPct > 0.4f) wasLowHP = false;

        // Death detection — count only on transition, restart state in autoTest
        if (playerHP <= 0.0f && !wasDeadLastFrame) {
            deathCount++;
            wasDeadLastFrame = true;
            issueLog.push_back(TextFormat("DEATH #%d at %.0fs pos=(%.0f,%.0f)",
                                          deathCount, testTimer, playerPos.x, playerPos.y));
            addLog(TextFormat("[DEATH #%d] at %.0fs", deathCount, testTimer));
        } else if (playerHP > 10.0f && wasDeadLastFrame) {
            wasDeadLastFrame = false;
            if (autoTest) {
                // Respawn — reset state machine to continue test
                botState   = BotState::Explore;
                stuckTimer = 0.0f;
                addLog("Respawn detected — bot resuming");
            }
        }
        // Return neutral while dead only bot doesn't move into walls
        if (playerHP <= 0.0f && autoTest) return dec;
    }

    // ── Stagnation detection (in the kills + in the zone advance for 2 min) ──────────
    stagnationTimer += dt;
    if (stagnationTimer >= 120.0f) {
        stagnationTimer = 0.0f;
        if (killCount == lastKillCheck && zonesVisited == lastZoneCheck) {
            issueLog.push_back(TextFormat(
                "WARNING: No progress in 2min (kills=%d zones=%d pos=%.0f,%.0f)",
                killCount, zonesVisited, playerPos.x, playerPos.y));
            addLog("WARNING: in the progress in 2min");
            // Force exploration state to break stagnation
            botState     = BotState::Explore;
            exploreStep  = (exploreStep + 1) % 8;
        }
        lastKillCheck = killCount;
        lastZoneCheck = zonesVisited;
    }

    // ── Distance tracking ─────────────────────────────────────────────────────
    if (prevPlayerPos.x > -9000.0f) {
        totalDistance += Vector2Distance(playerPos, prevPlayerPos);
    }
    prevPlayerPos = playerPos;

    // ── Area tracking ─────────────────────────────────────────────────────────
    quadrantTimer += dt;
    if (quadrantTimer > 3.0f) {
        quadrantTimer = 0.0f;
        // Quadrante RELATIVO to the center of the world. O threshold fixed of 1280 vinha of um
        // map of 2560; with the phase centrada in 4096 the bot ficava eternamente in the
        // same quadrante and the report acusava "preso num canto" without estar.
        float ccx = (worldCenter.x != 0.0f) ? worldCenter.x : 1280.0f;
        float ccy = (worldCenter.y != 0.0f) ? worldCenter.y : 1280.0f;
        int qx = (playerPos.x > ccx) ? 1 : 0;
        int qy = (playerPos.y > ccy) ? 1 : 0;
        int quad = qy * 2 + qx;
        if (quad != lastQuadrant) {
            lastQuadrant = quad;
            areasExplored = std::min(areasExplored + 1, 4);
        }
    }

    // ── Periodic log ─────────────────────────────────────────────────────────
    if (reportTimer <= 0.0f) {
        reportTimer = 15.0f;
        addLog(TextFormat("t=%.0fs kills=%d hp=%.0f lvl=%d $%d items=%d zones=%d",
               testTimer, killCount, playerHP, playerLevel, playerCredits,
               itemsCollected, zonesVisited));
    }

    // ── Find nearest enemy ───────────────────────────────────────────────────
    float nearestEnemyDist = 1e9f;
    int   nearestEnemyIdx  = -1;
    for (int i = 0; i < (int)enemyPositions.size(); ++i) {
        float d = Vector2Distance(playerPos, enemyPositions[i]);
        if (d < nearestEnemyDist) { nearestEnemyDist = d; nearestEnemyIdx = i; }
    }
    dec.nearestEnemyIdx = nearestEnemyIdx;
    if (nearestEnemyIdx >= 0)
        dec.nearestEnemyPos = enemyPositions[nearestEnemyIdx];

    // ── Find nearest item within 150px (improved: only chase nearby items) ───
    float nearestItemDist = 1e9f;
    int   nearestItemIdx  = -1;
    for (int i = 0; i < (int)itemPositions.size(); ++i) {
        float d = Vector2Distance(playerPos, itemPositions[i]);
        if (d < nearestItemDist) { nearestItemDist = d; nearestItemIdx = i; }
    }

    // ── Find nearest zone exit portal ────────────────────────────────────────
    float nearestPortalDist = 1e9f;
    Vector2 nearestPortalPos = {0, 0};
    for (const auto& pp : portalPositions) {
        float d = Vector2Distance(playerPos, pp);
        if (d < nearestPortalDist) { nearestPortalDist = d; nearestPortalPos = pp; }
    }

    float hpPct = (playerMaxHP > 0.0f) ? (playerHP / playerMaxHP) : 1.0f;
    bool  hasEnemy   = nearestEnemyIdx >= 0;
    // Item collection: pick up items within 150px (reduced from 300px to focus on nearby loot)
    bool  hasItem    = nearestItemIdx >= 0 && nearestItemDist < 150.0f;
    bool  hasPortal  = !portalPositions.empty();
    int   nearbyEnemyCount300 = countEnemiesInRadius(enemyPositions, playerPos, 300.0f);

    // ── Clear timer (for phase advance) ──────────────────────────────────────
    if (enemyPositions.empty() && itemPositions.empty()) {
        clearTimer += dt;
    } else {
        clearTimer = 0.0f;
    }

    // =========================================================================
    // STATE MACHINE — priority order
    // =========================================================================

    BotState newState = BotState::Explore;

    // Priority 1: Flee when HP < 25%
    if (hpPct < 0.25f && hasEnemy) {
        newState = BotState::FleeFromDanger;
    }
    // Priority 2 (world open): portal of PHASE open — advance of world only
    // perde for the fuga of death iminente. Enemies continuam spawnando, entao
    // esperar "zone limpa" (clearTimer) significava NUNCA go to the portal.
    else if (owPortalOpen) {
        newState = BotState::AdvancePhase;
    }
    // Priority 3: Collect item within 150px if not in heavy combat
    else if (hasItem && nearbyEnemyCount300 < 3) {
        newState = BotState::CollectItem;
    }
    // Priority 4: Attack nearest enemy
    else if (hasEnemy) {
        newState = BotState::AttackEnemy;
    }
    // Priority 5: Anomaly portal open nearby
    else if (openAnomalyPortals > 0) {
        newState = BotState::ClosePortal;
    }
    // Priority 6: Advance phase after zone clear (5s with in the enemies/items)
    else if (clearTimer > 5.0f && hasPortal) {
        newState = BotState::AdvancePhase;
    }
    // Priority 7: Explore
    else {
        newState = BotState::Explore;
    }

    if (newState != botState) {
        botState = newState;
        // itemsChased account TRANSICOES to coleta, not frames (before inflava
        // the taxa of "perseguidos" and escondia the failure real of coleta).
        if (botState == BotState::CollectItem) itemsChased++;
        const char* labels[] = {"FLEEING","COLLECTING","ATTACKING","CLOSING PORTAL","ADVANCING PHASE","EXPLORING"};
        addLog(TextFormat(">> %s", labels[(int)botState]));
        lastLoggedMode = (int)botState;
    }
    dec.currentState = botState;

    // =========================================================================
    // ACTIONS per state
    // =========================================================================

    switch (botState) {

    // ── Flee ─────────────────────────────────────────────────────────────────
    case BotState::FleeFromDanger: {
        Vector2 toEnemy = {
            enemyPositions[nearestEnemyIdx].x - playerPos.x,
            enemyPositions[nearestEnemyIdx].y - playerPos.y
        };
        Vector2 fleeDir = safeNormalize({-toEnemy.x, -toEnemy.y});
        botTarget = {playerPos.x + fleeDir.x * 400.0f,
                     playerPos.y + fleeDir.y * 400.0f};

        // Barrier (skill 5): use when HP < 25% — primary defensive skill
        // (contagem of shot stays in the Game, after the confirmacao isReady())
        if (skillsReady[4] && skillTimer <= 0.0f) {
            dec.shouldUseSkill5 = true;
            skillTimer = 0.5f;
            addLog("Barrier activated (HP critical)");
        }
        // Overload (skill 4): use when HP < 50% to gain speed/power
        if (skillsReady[3] && hpPct < 0.50f && skillTimer <= 0.0f) {
            dec.shouldUseSkill4 = true;
            skillTimer = 0.5f;
        }
        break;
    }

    // ── Collect item ─────────────────────────────────────────────────────────
    case BotState::CollectItem: {
        // A coleta real and AUTOMATICA in the Game (radius ~player.radius+52, with
        // magnetismo the 230px) — the contador itemsCollected and incrementado la,
        // when the item leaves of the vector. Contar here by proximidade (<20px)
        // never disparava: the item sumia before.
        botTarget = itemPositions[nearestItemIdx];
        break;
    }

    // ── Attack enemy ─────────────────────────────────────────────────────────
    case BotState::AttackEnemy: {
        Vector2 ePos = enemyPositions[nearestEnemyIdx];
        engageTimer -= dt;

        if (nearestEnemyDist <= playerRange * 0.9f) {
            if (attackTimer <= 0.0f) {
                dec.shouldMeleeAttack = true;
                attackTimer = 0.38f;
                meleeHits++;
            }
            botTarget  = ePos;
            engageTimer = 0.0f;   // connected — encerra the engage
        } else if (nearestEnemyDist < 150.0f || engageTimer > 0.0f) {
            // ENGAGE (~1.5s): closes distance DIRETO in the enemy, without recuo nem
            // orbita. Antes the bot RECUAVA 200px when dist<150 and orbitava the
            // 220px in the resto — with the melee the 90px of range, the distance never
            // fechava ("Ataques melee: 0" in the report).
            if (engageTimer <= 0.0f) engageTimer = 1.5f;
            botTarget = ePos;
        } else {
            // Orbita COLADA (~80px) for the range of the melee close — the orbita the
            // 220px mantinha the bot far demais to attack.
            botTarget = {
                ePos.x + std::cos(orbitAngle) * 80.0f,
                ePos.y + std::sin(orbitAngle) * 80.0f
            };
        }

        // ── Strategic skill usage ─────────────────────────────────────────────
        // (contagem skillsFired/skillUsageCounts stays in the Game, apos isReady())
        if (skillTimer <= 0.0f) {
            // Skill 2 (EMP): use when >=2 enemies within 250px
            if (skillsReady[1] && countEnemiesInRadius(enemyPositions, playerPos, 250.0f) >= 2) {
                dec.shouldUseSkill2 = true;
                skillTimer = 0.5f;
                addLog(TextFormat("EMP fired (%d enemies within 250px)",
                                  countEnemiesInRadius(enemyPositions, playerPos, 250.0f)));
            }
            // Skill 3 (Grenade): use when >=2 enemies within 200px
            else if (skillsReady[2] && countEnemiesInRadius(enemyPositions, playerPos, 200.0f) >= 2) {
                dec.shouldUseSkill3 = true;
                skillTimer = 0.5f;
                addLog("Grenade thrown (enemy cluster)");
            }
            // Skill 6 (Burst): use when enemy within 100px
            else if (skillsReady[5] && nearestEnemyDist < 100.0f) {
                dec.shouldUseSkill6 = true;
                skillTimer = 0.5f;
            }
            // Skill 1 (Laser): single enemy in range the fallback
            else if (skillsReady[0] && nearestEnemyDist < 500.0f) {
                dec.shouldUseSkill1 = true;
                skillTimer = 0.5f;
            }
        }

        // Skill 4 (Overload): use when HP < 50% (damage boost + survivability)
        if (skillsReady[3] && hpPct < 0.50f) {
            dec.shouldUseSkill4 = true;
            addLog("Overload activated (HP<50%)");
        }

        // Skill 5 (Barrier): use when HP < 25% even during combat
        if (skillsReady[4] && hpPct < 0.25f && skillTimer <= 0.0f) {
            dec.shouldUseSkill5 = true;
            skillTimer = 0.5f;
            addLog("Barrier activated (HP<25% in combat)");
        }

        break;
    }

    // ── Close anomaly portal ─────────────────────────────────────────────────
    case BotState::ClosePortal: {
        if (hasItem) botTarget = itemPositions[nearestItemIdx];
        else         botTarget = {playerPos.x, playerPos.y};
        if (skillsReady[0] && skillTimer <= 0.0f) {
            dec.shouldUseSkill1 = true;
            skillTimer = 0.5f;
        }
        if (attackTimer <= 0.0f) {
            dec.shouldMeleeAttack = true;
            attackTimer = 0.38f;
        }
        break;
    }

    // ── Advance to next zone ─────────────────────────────────────────────────
    case BotState::AdvancePhase: {
        // Open world: the target is the PHASE portal (owPortalPos). The portals of
        // zone of the tilemap sao the system old — empty in the world open. A less
        // of 100u the bot pede the avanco: the Game trata shouldUsePortal as key E.
        if (owPortalOpen) {
            botTarget = owPortalPos;
            float owDist = Vector2Distance(playerPos, owPortalPos);
            if (owDist < 100.0f) dec.shouldUsePortal = true;
            if (logicTimer <= 0.0f) {
                addLog(TextFormat("Going to phase portal dist=%.0f", owDist));
                logicTimer = 3.0f;
            }
            break;
        }
        botTarget  = nearestPortalPos;
        clearTimer = 0.0f; // reset only we don't loop

        // Track when we actually reach the portal (uma vez by position of portal)
        if (nearestPortalDist < 40.0f && Vector2Distance(nearestPortalPos, lastAdvancePortalPos) > 80.0f) {
            zonesVisited++;
            lastAdvancePortalPos = nearestPortalPos;
            addLog(TextFormat("ZONE ADVANCED! Total=%d", zonesVisited));
        }

        if (logicTimer <= 0.0f) {
            addLog(TextFormat("Going to portal dist=%.0f", nearestPortalDist));
            logicTimer = 3.0f;
        }
        break;
    }

    // ── Explore (spiral) ─────────────────────────────────────────────────────
    case BotState::Explore: {
        exploreTimer += dt;

        // SAFE ZONE: enemies only spawn OUTSIDE it (Game pushes any
        // to outside the radius). O passeio random centrado in the player mantinha the
        // bot eternamente in the refuge — 0 enemies vistos, 0 kills. Se stay
        // >3s inside the zone, the next alvo and FORCADO to outside dela.
        bool insideSafe = false;
        if (safeZoneRadius > 0.0f) {
            insideSafe = Vector2Distance(playerPos, safeZoneCenter) < safeZoneRadius;
            if (insideSafe) exploreSafeZoneTimer += dt;
            else            exploreSafeZoneTimer = 0.0f;
        }

        if (exploreTimer > 3.0f || Vector2Distance(playerPos, botTarget) < 40.0f) {
            exploreTimer = 0.0f;

            if (exploreSafeZoneTimer > 3.0f && safeZoneRadius > 0.0f) {
                // Outside the zone segura: angle random, radius alem of the refuge,
                // medido the partir of the CENTRO of the zone (not of the player).
                exploreSafeZoneTimer = 0.0f;
                float the = (float)GetRandomValue(0, 359) * DEG2RAD;
                float r = safeZoneRadius + 300.0f + (float)GetRandomValue(0, 900);
                botTarget = { safeZoneCenter.x + std::cos(the) * r,
                              safeZoneCenter.y + std::sin(the) * r };
                addLog("Leaving safe zone to hunt");
            } else {
                float angle  = exploreStep * 0.7f;
                // Radius up to 1800: with 900 the bot would never leave the SAFE ZONE (radius 1050),
                // where enemy and empurrado to outside. Result: 0 shots, 0 kills - the
                // portao of validation got isso as "combat quebrado".
                float radius = 300.0f + exploreStep * 90.0f;
                if (radius > 1800.0f) { radius = 300.0f; exploreStep = 0; }
                auto pick = [&](float the) {
                    return Vector2{ playerPos.x + std::cos(the) * radius,
                                    playerPos.y + std::sin(the) * radius };
                };
                botTarget = pick(angle);
                // Alvo inside of wall/barrier = anda until encostar and trava. Gira the
                // angle procurando um point livre (puxar to the center only prendia the bot
                // in returns of the refuge).
                for (int tryI = 1; tryI < 8 && wallQuery && wallQuery(botTarget); ++tryI)
                    botTarget = pick(angle + tryI * 0.785f);
                exploreStep++;
            }
        }
        // Skill 1 if any enemy spotted during exploration
        if (skillsReady[0] && hasEnemy && nearestEnemyDist < 400.0f && skillTimer <= 0.0f) {
            dec.shouldUseSkill1 = true;
            skillTimer = 0.5f;
        }
        break;
    }
    } // end switch

    // ── Escape of bordas/barreiras of the world open ────────────────────────────
    // If the alvo of exploracao falls inside of uma barrier solida (ex.: edge of the
    // map), the BFS leva until the beirada and the bot encosta without progredir. When the
    // stuck if prolonga, ruma temporariamente for the center of the map — direction
    // garantidamente transitavel — to leave of the bolsao.
    if (wallQuery) updateStuckTracking(playerPos, dt);
    escapeTimer -= dt;
    if (stuckTimer > 2.0f && escapeTimer <= 0.0f) {
        // O escape TEM that produzir viagem. Send to "the center" era um laco
        // infinito after that the center virou the own refuge where the bot was:
        // ele chegava, parava, era considered preso of new and reescapava - 0
        // kills in 100s with 30 enemies vivos in the screen.
        // Agora: um point the meia distance of the edge of the phase, num rumo LIVRE and
        // far of where ele already is.
        Vector2 c   = (worldCenter.x != 0.0f || worldCenter.y != 0.0f) ? worldCenter : playerPos;
        float   ring = (worldRadius > 400.0f) ? worldRadius * 0.60f : 1400.0f;
        // Sem enemies ha very time = the bot precisa of COMBAT. Nesse if the
        // escape NOT can puxar of returns for the refuge: enemies only existem
        // outside the zone segura, entao candidatos inside dela sao rejeitados.
        bool needCombat = enemyPositions.empty();
        Vector2 best = playerPos; float bestD = -1.0f;
        for (int i = 0; i < 8; ++i) {
            float the = (float)i * 0.785f + (float)GetRandomValue(0, 62) * 0.01f;
            Vector2 cand = { c.x + std::cos(the) * ring, c.y + std::sin(the) * ring };
            if (wallQuery && wallQuery(cand)) continue;
            if (needCombat && safeZoneRadius > 0.0f &&
                Vector2Distance(cand, safeZoneCenter) < safeZoneRadius + 150.0f) continue;
            float dx = cand.x - playerPos.x, dy = cand.y - playerPos.y;
            float d  = dx*dx + dy*dy;
            if (d > bestD) { bestD = d; best = cand; }
        }
        // Encurralado of verdade (>4s): the point sorteado can be inalcancavel and the
        // escape vira other laco. A celula that the BFS alcancou and uma promessa.
        if (stuckTimer > 4.0f && hasFarReach &&
            Vector2Distance(farthestReachable, playerPos) > 120.0f) {
            best = farthestReachable;
            addLog("Escape -> farthest reachable cell (BFS)");
        }
        escapeTarget = best;
        escapeTimer  = 5.0f;
        cachedPath.clear();   // recalcula route imediatamente for the escape
        addLog(TextFormat("Escape -> (%.0f,%.0f)", best.x, best.y));
    }
    if (escapeTimer > 0.0f) botTarget = escapeTarget;

    // ── Movement: pathfinding global (BFS) with fallback of desvio reativo ─────
    Vector2 dir;
    if (wallQuery) {
        // Route global in the grade of tiles — escapa of cantos concavos of verdade.
        dir = computePathDir(playerPos, botTarget);
    } else {
        // Sem consulta of map: mantem the desvio reativo of 8 directions (legacy).
        Vector2 rawDir = {botTarget.x - playerPos.x, botTarget.y - playerPos.y};
        dir = computeAntiWall(rawDir, playerPos, dt);
    }
    float targetDist = std::max(80.0f, Vector2Distance(playerPos, botTarget));
    Vector2 safeTarget = {playerPos.x + dir.x * targetDist,
                          playerPos.y + dir.y * targetDist};

    dec.shouldMove = true;
    dec.moveTarget = safeTarget;

    return dec;
}
