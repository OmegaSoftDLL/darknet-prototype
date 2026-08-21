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
//   - At 2s stuck: pick a new escape direction from the 8-direction table, cycling
//     through them so we try all compass headings before repeating.
//   - Track longStuckTimer separately for bug reporting (> 10s).
//   - When not stuck, decay the angle offset back toward 0 so we return to the
//     correct heading once clear of the obstacle.
//
// Diferenca angular normalizada para [-PI, PI]
static float angleDiff(float a, float b) {
    float d = a - b;
    while (d >  (float)M_PI) d -= 2.0f * (float)M_PI;
    while (d < -(float)M_PI) d += 2.0f * (float)M_PI;
    return d;
}

// Rastreio de "preso" — usado tanto pelo pathfinding global quanto pelo desvio
// reativo de fallback, e alimenta a telemetria de stuck events do relatorio.
void BotController::updateStuckTracking(Vector2 currentPos, float dt) {
    // BUG QUE ISTO CORRIGE: a versao anterior comparava o deslocamento de UM
    // FRAME com 3 px. A 60 fps e velocidade 155 u/s o personagem anda 2,58 px por
    // frame — ou seja, andando normalmente ele era classificado como PRESO o
    // tempo todo. O bot vivia em "escape", nunca engajava (0 abates) e o
    // relatorio acusava travamento sem haver travamento nenhum.
    // Agora a medida e por JANELA DE TEMPO: distancia acumulada em 0,5 s.
    stuckAccum += Vector2Distance(currentPos, lastPos);
    lastPos     = currentPos;
    stuckSample += dt;
    if (stuckSample < 0.5f) return;

    // 0,5 s de caminhada normal percorre ~75 px; 15 px e chao de sobra para
    // distinguir "empurrando parede" de "andando devagar".
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
        issueLog.push_back(TextFormat("PRESO por >10s em (%.0f,%.0f) — possivel bug de pathfinding",
                                      currentPos.x, currentPos.y));
        addLog("ALERTA: preso >10s — reportado");
    }
}

// ─── Pathfinding global (BFS na grade de tiles transitaveis) ─────────────────
//
// Substitui o desvio reativo de sensores locais (que oscilava em cantos concavos)
// por uma busca em largura completa numa janela de tiles ao redor do bot. A BFS
// explora toda a regiao acessivel e gera uma rota de waypoints ate o alvo. Se o
// alvo for inalcancavel, a rota vai ate a celula acessivel mais proxima do alvo —
// o que permite contornar bolsoes em "U" recuando de verdade pela saida real.
Vector2 BotController::computePathDir(Vector2 from, Vector2 to) {
    const float TS = 64.0f;   // tileSize (Tilemap::tileSize)
    const int   R  = 34;      // raio da janela de busca em tiles (~2176px)
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
        int ox  = pgx - R, oy = pgy - R;             // tile de mundo na celula (0,0)

        // alvo clampeado para dentro da janela
        tgx = std::max(ox, std::min(ox + W - 1, tgx));
        tgy = std::max(oy, std::min(oy + W - 1, tgy));

        // grade de transitabilidade (centro de cada tile)
        std::vector<char> walk(W * W);
        for (int gy = 0; gy < W; ++gy)
            for (int gx = 0; gx < W; ++gx) {
                float cx = (ox + gx) * TS + TS * 0.5f;
                float cy = (oy + gy) * TS + TS * 0.5f;
                walk[gy * W + gx] = wallQuery({cx, cy}) ? 0 : 1;
            }

        int startI = (pgy - oy) * W + (pgx - ox);
        int goalI  = (tgy - oy) * W + (tgx - ox);
        walk[startI] = 1;   // garante que a celula do bot e transitavel

        std::vector<int> parent(W * W, -2);          // -2 = nao visitado
        std::queue<int>  q;
        q.push(startI);
        parent[startI] = -1;

        int   bestI = startI;
        float bestD = std::hypot((float)(tgx - pgx), (float)(tgy - pgy));
        int   farI  = startI;      // celula alcancavel mais LONGE do bot
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
                if (k >= 4) {  // diagonal: impede cortar quina entre duas paredes
                    if (!walk[cgy * W + nx] || !walk[ny * W + cgx]) continue;
                }
                parent[ni] = cur;
                q.push(ni);
            }
        }

        // destino de emergencia: o ponto acessivel mais distante que a BFS achou
        hasFarReach = (farD > 4.0f);
        farthestReachable = { (ox + farI % W) * TS + TS * 0.5f,
                              (oy + farI / W) * TS + TS * 0.5f };

        // reconstroi a partir da celula acessivel mais proxima do alvo
        std::vector<Vector2> rev;
        for (int node = bestI; node != -1; node = parent[node]) {
            int gx = node % W, gy = node / W;
            rev.push_back({ (ox + gx) * TS + TS * 0.5f, (oy + gy) * TS + TS * 0.5f });
        }
        for (int i = (int)rev.size() - 1; i >= 0; --i) cachedPath.push_back(rev[i]);
        if (!cachedPath.empty()) cachedPath.erase(cachedPath.begin()); // descarta o tile atual
    }

    // consome waypoints ja alcancados
    while (!cachedPath.empty() && Vector2Distance(from, cachedPath.front()) < 36.0f)
        cachedPath.erase(cachedPath.begin());

    if (cachedPath.empty())
        return safeNormalize({ to.x - from.x, to.y - from.y });

    Vector2 wp = cachedPath.front();
    return safeNormalize({ wp.x - from.x, wp.y - from.y });
}

Vector2 BotController::computeAntiWall(Vector2 desired, Vector2 currentPos, float dt) {
    // ── Rastreio de "preso" (para relatorio e para escapar de bolsoes) ────────
    updateStuckTracking(currentPos, dt);

    Vector2 dn          = safeNormalize(desired);
    float   desiredAng  = std::atan2(dn.y, dn.x);

    // Quantas direcoes estao livres?
    int openCount = 0;
    for (int d = 0; d < 8; d++) if (!blockedDir[d]) openCount++;

    // Cercado por todos os lados — empurra na direcao desejada (ultimo recurso)
    if (openCount == 0) return dn;

    // Se a direcao desejada esta essencialmente livre e nao estamos presos,
    // segue reto para o alvo (sem zigue-zague desnecessario).
    {
        // checa o setor de 8-dir mais alinhado ao desejo
        int   nearestIdx = 0; float nearestDelta = 1e9f;
        for (int d = 0; d < 8; d++) {
            float delta = std::fabs(angleDiff(k8DirAngles[d], desiredAng));
            if (delta < nearestDelta) { nearestDelta = delta; nearestIdx = d; }
        }
        if (!blockedDir[nearestIdx] && !isStuck) {
            return dn;
        }
    }

    // Caso contrario: escolhe a direcao ABERTA mais proxima do alvo.
    // Se estamos presos ha um tempo, gira a preferencia para sair de bolsoes
    // concavos (cantos) em vez de insistir na mesma direcao.
    float stuckBias = 0.0f;
    if (stuckTimer > 0.5f) {
        stuckEvents += (stuckTimer < 0.5f + dt) ? 1 : 0;
        // gira a preferencia ~90 graus conforme o tempo preso aumenta
        stuckBias = (stuckEscapeDir > 0 ? 1.0f : -1.0f) * (float)(M_PI * 0.5);
        // alterna o lado de fuga a cada ~1.5s preso
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

    float a = k8DirAngles[bestDir];
    return {std::cos(a), std::sin(a)};
}

// ─── Report ──────────────────────────────────────────────────────────────────

bool BotController::passed(std::vector<std::string>* reasons) const {
    auto fail = [&](const std::string& why) { if (reasons) reasons->push_back(why); };
    bool ok = true;
    float avg = (fpsSamples > 0) ? (fpsAccum / fpsSamples) : 0.0f;
    // Limiares deliberadamente FROUXOS: o portao pega quebra grave (crash,
    // travamento, jogo que nao roda), nao briga por 2 fps.
    if (avg < 45.0f)            { ok = false; fail(TextFormat("FPS medio %.0f < 45", avg)); }
    if (longStuckEvents > 0)    { ok = false; fail(TextFormat("%d travamento(s) > 10s", longStuckEvents)); }
    if (deathCount > 3)         { ok = false; fail(TextFormat("%d mortes seguidas", deathCount)); }
    if (killCount == 0)         { ok = false; fail("nenhum inimigo abatido (combate quebrado?)"); }
    if (totalDistance < 500.0f) { ok = false; fail("bot praticamente nao andou (movimento travado?)"); }
    return ok;
}

void BotController::writeReport(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return;

    float avgFPS = (fpsSamples > 0) ? (fpsAccum / fpsSamples) : 0.0f;

    f << "========================================================\n";
    f << "  DARKNET BOT - RELATORIO DE SESSAO\n";
    f << "========================================================\n\n";

    f << "[DESEMPENHO TECNICO]\n";
    f << "  FPS medio   : " << (int)avgFPS << "\n";
    f << "  FPS minimo  : " << (int)minFPS << "\n";
    f << "  FPS maximo  : " << (int)maxFPS << "\n";
    if (minFPS < 40.0f)
        f << "  PROBLEMA: FPS caiu abaixo de 40 - otimizacao necessaria\n";

    f << "\n[COMBATE]\n";
    f << "  Inimigos abatidos : " << killCount << "\n";
    f << "  Ataques melee     : " << meleeHits << "\n";
    f << "  Skills disparadas : " << skillsFired << "\n";
    f << "    Skill 1 (Laser)    : " << skillUsageCounts[0] << "x\n";
    f << "    Skill 2 (EMP)      : " << skillUsageCounts[1] << "x\n";
    f << "    Skill 3 (Granada)  : " << skillUsageCounts[2] << "x\n";
    f << "    Skill 4 (Sobrecarga): " << skillUsageCounts[3] << "x\n";
    f << "    Skill 5 (Barreira) : " << skillUsageCounts[4] << "x\n";
    f << "    Skill 6 (Rajada)   : " << skillUsageCounts[5] << "x\n";
    if (killCount == 0)
        f << "  PROBLEMA: Nenhum inimigo morto - combate nao funciona\n";
    if (meleeHits == 0)
        f << "  PROBLEMA: Melee nao acionou - bug no sistema de ataque\n";

    f << "\n[COLETA DE ITENS]\n";
    f << "  Itens perseguidos : " << itemsChased << "\n";
    f << "  Itens coletados   : " << itemsCollected << "\n";
    if (itemsChased > 0 && itemsCollected == 0)
        f << "  PROBLEMA: Bot perseguiu itens mas nao coletou nenhum (tecla E?)\n";

    f << "\n[EXPLORACAO E PROGRESSO]\n";
    f << "  Areas exploradas  : " << areasExplored << "/4 quadrantes\n";
    f << "  Zonas avancadas   : " << zonesVisited << "\n";
    f << "  Distancia total   : " << (int)totalDistance << " px\n";
    if (areasExplored < 2)
        f << "  PROBLEMA: Jogador preso num canto - mapa tem areas bloqueadas?\n";
    if (zonesVisited == 0 && testTimer > 60.0f)
        f << "  PROBLEMA: Bot nao avancou de zona em 60s\n";

    f << "\n[SOBREVIVENCIA]\n";
    f << "  Mortes            : " << deathCount << "\n";
    f << "  HP mais baixo     : " << (int)lowestHP << "\n";
    f << "  Vezes em perigo   : " << dangersZones << " (HP < 25%)\n";
    f << "  Dano total tomado : " << (int)totalDmgTaken << "\n";
    f << "  Eventos de dano   : " << damageEvents << "\n";
    if (deathCount > 3)
        f << "  PROBLEMA: Muitas mortes - dificuldade muito alta ou HP muito baixo\n";
    if (dangersZones > 5)
        f << "  SUGESTAO: Adicionar mais cura / kits no mapa\n";

    f << "\n[PATHFINDING / STUCK]\n";
    f << "  Eventos de bloqueio (>2s): " << stuckEvents << "\n";
    f << "  Bloqueios criticos (>10s): " << longStuckEvents << "\n";
    if (longStuckEvents > 0)
        f << "  BUG: Bot ficou preso por mais de 10s em " << longStuckEvents
          << " ocasiao(oes) - revisar colisao/mapa\n";

    f << "\n[DIAGNOSTICO DE ENTIDADES (picos)]\n";
    f << "  Inimigos (pico)        : " << peakEnemies << "\n";
    f << "  Projeteis player (pico): " << peakProjectiles << "\n";
    f << "  Projeteis inimigo(pico): " << peakEnemyProj << "\n";
    f << "  Itens no chao (pico)   : " << peakItems << "\n";
    f << "  XP orbs (pico)         : " << peakOrbs << "\n";
    f << "  Unidades aliadas (pico): " << peakUnits << "\n";
    f << "  No FPS mais baixo (" << (int)fpsLowValue << "): inimigos=" << fpsLowEnemies
      << " projeteis=" << fpsLowProj << "\n";
    f << "  Pior tempo update()    : " << peakUpdateMs << " ms\n";
    f << "  Pior tempo render()    : " << peakRenderMs << " ms\n";

    f << "\n[PROBLEMAS DETECTADOS]\n";
    if (issueLog.empty()) {
        f << "  Nenhum problema critico detectado\n";
    } else {
        for (const auto& issue : issueLog)
            f << "  - " << issue << "\n";
    }

    f << "\n[LOG DE ATIVIDADE (ultimas acoes)]\n";
    for (const auto& entry : log)
        f << "  " << entry << "\n";

    f << "\n[PRIORIDADES DE MELHORIA SUGERIDAS]\n";
    int pri = 1;
    if (minFPS < 40.0f)
        f << "  " << pri++ << ". Otimizar rendering - FPS baixo detectado\n";
    if (killCount < 5)
        f << "  " << pri++ << ". Corrigir sistema de combate - poucos kills\n";
    if (deathCount > 3)
        f << "  " << pri++ << ". Balancear dificuldade - muitas mortes\n";
    if (dangersZones > 5)
        f << "  " << pri++ << ". Adicionar mais HealthPacks no mapa\n";
    if (areasExplored < 2)
        f << "  " << pri++ << ". Corrigir geracao de mapa - areas inacessiveis\n";
    if (itemsChased > 0 && itemsCollected == 0)
        f << "  " << pri++ << ". Verificar tecla de coleta de item (E)\n";
    if (itemsCollected < 3)
        f << "  " << pri++ << ". Verificar spawn de itens - poucos itens encontrados\n";
    if (skillsFired < 5)
        f << "  " << pri++ << ". Checar skills - cooldowns muito longos?\n";
    if (longStuckEvents > 0)
        f << "  " << pri++ << ". Investigar pathfinding - bot ficou preso >10s\n";

    f << "\n========================================================\n";
    f << "  Duracao da sessao: " << (int)testTimer << "s\n";
    f << "  Frames processados: " << frameCount << "\n";
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
            addLog(TextFormat("TESTE CONCLUIDO (%.0fs)", testTimer));
            writeReport("bot_report.txt");
            return dec;
        }
    }

    // ── FPS tracking ──────────────────────────────────────────────────────────
    if (currentFPS > 0.0f) {
        fpsAccum += currentFPS; fpsSamples++;
        if (currentFPS < minFPS) minFPS = currentFPS;
        if (currentFPS > maxFPS) maxFPS = currentFPS;
        if (currentFPS < 30.0f && logicTimer <= 0.0f)
            issueLog.push_back(TextFormat("FPS critico: %.0f", currentFPS));
    }

    // ── HP tracking ───────────────────────────────────────────────────────────
    if (playerMaxHP > 0.0f) {
        float hpPct = playerHP / playerMaxHP;
        if (playerHP < lowestHP) lowestHP = playerHP;
        if (hpPct < 0.25f && !wasLowHP) {
            wasLowHP = true;
            dangersZones++;
            addLog(TextFormat("PERIGO! HP=%.0f/%.0f", playerHP, playerMaxHP));
            issueLog.push_back(TextFormat("HP critico em %.0f%%", hpPct * 100));
        }
        if (hpPct > 0.4f) wasLowHP = false;

        // Death detection — count only on transition, restart state in autoTest
        if (playerHP <= 0.0f && !wasDeadLastFrame) {
            deathCount++;
            wasDeadLastFrame = true;
            issueLog.push_back(TextFormat("MORTE #%d em %.0fs pos=(%.0f,%.0f)",
                                          deathCount, testTimer, playerPos.x, playerPos.y));
            addLog(TextFormat("[MORTE #%d] em %.0fs", deathCount, testTimer));
        } else if (playerHP > 10.0f && wasDeadLastFrame) {
            wasDeadLastFrame = false;
            if (autoTest) {
                // Respawn — reset state machine to continue test
                botState   = BotState::Explore;
                stuckTimer = 0.0f;
                addLog("Respawn detectado — bot retomando");
            }
        }
        // Return neutral while dead so bot doesn't move into walls
        if (playerHP <= 0.0f && autoTest) return dec;
    }

    // ── Stagnation detection (no kills + no zone advance for 2 min) ──────────
    stagnationTimer += dt;
    if (stagnationTimer >= 120.0f) {
        stagnationTimer = 0.0f;
        if (killCount == lastKillCheck && zonesVisited == lastZoneCheck) {
            issueLog.push_back(TextFormat(
                "AVISO: Sem progresso em 2min (kills=%d zonas=%d pos=%.0f,%.0f)",
                killCount, zonesVisited, playerPos.x, playerPos.y));
            addLog("AVISO: sem progresso em 2min");
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
        // Quadrante RELATIVO ao centro do mundo. O limiar fixo de 1280 vinha de um
        // mapa de 2560; com a fase centrada em 4096 o bot ficava eternamente no
        // mesmo quadrante e o relatorio acusava "preso num canto" sem estar.
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
        addLog(TextFormat("t=%.0fs kills=%d hp=%.0f lvl=%d $%d itens=%d zonas=%d",
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
    int   nearbyEnemyCount200 = countEnemiesInRadius(enemyPositions, playerPos, 200.0f);
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
    // Priority 2: Collect item within 150px if not in heavy combat
    else if (hasItem && nearbyEnemyCount300 < 3) {
        newState = BotState::CollectItem;
    }
    // Priority 3: Attack nearest enemy
    else if (hasEnemy) {
        newState = BotState::AttackEnemy;
    }
    // Priority 4: Anomaly portal open nearby
    else if (openAnomalyPortals > 0) {
        newState = BotState::ClosePortal;
    }
    // Priority 5: Advance phase after zone clear (5s with no enemies/items)
    else if (clearTimer > 5.0f && hasPortal) {
        newState = BotState::AdvancePhase;
    }
    // Priority 6: Explore
    else {
        newState = BotState::Explore;
    }

    if (newState != botState) {
        botState = newState;
        const char* labels[] = {"FUGINDO","COLETANDO","ATACANDO","FECHA PORTAL","AVANCANDO FASE","EXPLORANDO"};
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

        // Barreira (skill 5): use when HP < 25% — primary defensive skill
        if (skillsReady[4] && skillTimer <= 0.0f) {
            dec.shouldUseSkill5 = true;
            skillsFired++;
            skillUsageCounts[4]++;
            skillTimer = 0.5f;
            addLog("Barreira ativada (HP critico)");
        }
        // Sobrecarga (skill 4): use when HP < 50% to gain speed/power
        if (skillsReady[3] && hpPct < 0.50f && skillTimer <= 0.0f) {
            dec.shouldUseSkill4 = true;
            skillsFired++;
            skillUsageCounts[3]++;
            skillTimer = 0.5f;
        }
        break;
    }

    // ── Collect item ─────────────────────────────────────────────────────────
    case BotState::CollectItem: {
        botTarget = itemPositions[nearestItemIdx];
        itemsChased++;

        // When very close (20px), trigger E key to pick up
        if (nearestItemDist < 20.0f) {
            dec.shouldPickupItem = true;
            itemsCollected++;
            addLog(TextFormat("Item coletado em (%.0f,%.0f)",
                              itemPositions[nearestItemIdx].x,
                              itemPositions[nearestItemIdx].y));
        }
        break;
    }

    // ── Attack enemy ─────────────────────────────────────────────────────────
    case BotState::AttackEnemy: {
        Vector2 ePos = enemyPositions[nearestEnemyIdx];

        if (nearestEnemyDist <= playerRange * 0.9f) {
            if (attackTimer <= 0.0f) {
                dec.shouldMeleeAttack = true;
                attackTimer = 0.38f;
                meleeHits++;
            }
            botTarget = ePos;
        } else if (nearestEnemyDist < 150.0f) {
            // Too close for shooter build — back off
            Vector2 away = safeNormalize({playerPos.x - ePos.x, playerPos.y - ePos.y});
            botTarget = {playerPos.x + away.x * 200.0f, playerPos.y + away.y * 200.0f};
        } else {
            // Orbit at ~220px shooting distance
            botTarget = {
                ePos.x + std::cos(orbitAngle) * orbitRadius,
                ePos.y + std::sin(orbitAngle) * orbitRadius
            };
        }

        // ── Strategic skill usage ─────────────────────────────────────────────
        if (skillTimer <= 0.0f) {
            // Skill 2 (EMP): use when >3 enemies within 200px
            if (skillsReady[1] && nearbyEnemyCount200 > 3) {
                dec.shouldUseSkill2 = true;
                skillsFired++;
                skillUsageCounts[1]++;
                skillTimer = 0.5f;
                addLog(TextFormat("EMP disparado (%d inimigos em 200px)", nearbyEnemyCount200));
            }
            // Skill 3 (Granada): use when >2 enemies within 150px
            else if (skillsReady[2] && countEnemiesInRadius(enemyPositions, playerPos, 150.0f) > 2) {
                dec.shouldUseSkill3 = true;
                skillsFired++;
                skillUsageCounts[2]++;
                skillTimer = 0.5f;
                addLog("Granada lancada (cluster de inimigos)");
            }
            // Skill 6 (Rajada): use when enemy within 100px
            else if (skillsReady[5] && nearestEnemyDist < 100.0f) {
                dec.shouldUseSkill6 = true;
                skillsFired++;
                skillUsageCounts[5]++;
                skillTimer = 0.5f;
            }
            // Skill 1 (Laser): single enemy in range as fallback
            else if (skillsReady[0] && nearestEnemyDist < 500.0f) {
                dec.shouldUseSkill1 = true;
                skillsFired++;
                skillUsageCounts[0]++;
                skillTimer = 0.5f;
            }
        }

        // Skill 4 (Sobrecarga): use when HP < 50% (damage boost + survivability)
        if (skillsReady[3] && hpPct < 0.50f) {
            dec.shouldUseSkill4 = true;
            skillsFired++;
            skillUsageCounts[3]++;
            addLog("Sobrecarga ativada (HP<50%)");
        }

        // Skill 5 (Barreira): use when HP < 25% even during combat
        if (skillsReady[4] && hpPct < 0.25f && skillTimer <= 0.0f) {
            dec.shouldUseSkill5 = true;
            skillsFired++;
            skillUsageCounts[4]++;
            skillTimer = 0.5f;
            addLog("Barreira ativada (HP<25% em combate)");
        }

        break;
    }

    // ── Close anomaly portal ─────────────────────────────────────────────────
    case BotState::ClosePortal: {
        if (hasItem) botTarget = itemPositions[nearestItemIdx];
        else         botTarget = {playerPos.x, playerPos.y};
        if (skillsReady[0] && skillTimer <= 0.0f) {
            dec.shouldUseSkill1 = true;
            skillsFired++;
            skillUsageCounts[0]++;
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
        botTarget  = nearestPortalPos;
        clearTimer = 0.0f; // reset so we don't loop

        // Track when we actually reach the portal
        if (nearestPortalDist < 40.0f) {
            zonesVisited++;
            addLog(TextFormat("ZONA AVANCADA! Total=%d", zonesVisited));
        }

        if (logicTimer <= 0.0f) {
            addLog(TextFormat("Indo para portal dist=%.0f", nearestPortalDist));
            logicTimer = 3.0f;
        }
        break;
    }

    // ── Explore (spiral) ─────────────────────────────────────────────────────
    case BotState::Explore: {
        exploreTimer += dt;
        if (exploreTimer > 3.0f || Vector2Distance(playerPos, botTarget) < 40.0f) {
            exploreTimer = 0.0f;
            float angle  = exploreStep * 0.7f;
            // Raio ate 1800: com 900 o bot nunca saia da ZONA SEGURA (raio 1050),
            // onde inimigo e empurrado pra fora. Resultado: 0 tiros, 0 abates - o
            // portao de validacao pegou isso como "combate quebrado".
            float radius = 300.0f + exploreStep * 90.0f;
            if (radius > 1800.0f) { radius = 300.0f; exploreStep = 0; }
            auto pick = [&](float a) {
                return Vector2{ playerPos.x + std::cos(a) * radius,
                                playerPos.y + std::sin(a) * radius };
            };
            botTarget = pick(angle);
            // Alvo dentro de parede/barreira = anda ate encostar e trava. Gira o
            // angulo procurando um ponto livre (puxar pro centro so prendia o bot
            // em volta do refugio).
            for (int tryI = 1; tryI < 8 && wallQuery && wallQuery(botTarget); ++tryI)
                botTarget = pick(angle + tryI * 0.785f);
            exploreStep++;
        }
        // Skill 1 if any enemy spotted during exploration
        if (skillsReady[0] && hasEnemy && nearestEnemyDist < 400.0f && skillTimer <= 0.0f) {
            dec.shouldUseSkill1 = true;
            skillsFired++;
            skillUsageCounts[0]++;
            skillTimer = 0.5f;
        }
        break;
    }
    } // end switch

    // ── Escape de bordas/barreiras do mundo aberto ────────────────────────────
    // Se o alvo de exploracao cai dentro de uma barreira solida (ex.: borda do
    // mapa), o BFS leva ate a beirada e o bot encosta sem progredir. Quando o
    // stuck se prolonga, ruma temporariamente para o centro do mapa — direcao
    // garantidamente transitavel — para sair do bolsao.
    if (wallQuery) updateStuckTracking(playerPos, dt);
    escapeTimer -= dt;
    if (stuckTimer > 2.0f && escapeTimer <= 0.0f) {
        // O escape TEM que produzir viagem. Mandar para "o centro" era um laco
        // infinito depois que o centro virou o proprio refugio onde o bot estava:
        // ele chegava, parava, era considerado preso de novo e reescapava - 0
        // abates em 100s com 30 inimigos vivos na tela.
        // Agora: um ponto a meia distancia da borda da fase, num rumo LIVRE e
        // longe de onde ele ja esta.
        Vector2 c   = (worldCenter.x != 0.0f || worldCenter.y != 0.0f) ? worldCenter : playerPos;
        float   ring = (worldRadius > 400.0f) ? worldRadius * 0.60f : 1400.0f;
        Vector2 best = playerPos; float bestD = -1.0f;
        for (int i = 0; i < 8; ++i) {
            float a = (float)i * 0.785f + (float)GetRandomValue(0, 62) * 0.01f;
            Vector2 cand = { c.x + std::cos(a) * ring, c.y + std::sin(a) * ring };
            if (wallQuery && wallQuery(cand)) continue;
            float dx = cand.x - playerPos.x, dy = cand.y - playerPos.y;
            float d  = dx*dx + dy*dy;
            if (d > bestD) { bestD = d; best = cand; }
        }
        // Encurralado de verdade (>4s): o ponto sorteado pode ser inalcancavel e o
        // escape vira outro laco. A celula que a BFS alcancou e uma promessa.
        if (stuckTimer > 4.0f && hasFarReach &&
            Vector2Distance(farthestReachable, playerPos) > 120.0f) {
            best = farthestReachable;
            addLog("Escape -> celula alcancavel mais distante (BFS)");
        }
        escapeTarget = best;
        escapeTimer  = 5.0f;
        cachedPath.clear();   // recalcula rota imediatamente para o escape
        addLog(TextFormat("Escape -> (%.0f,%.0f)", best.x, best.y));
    }
    if (escapeTimer > 0.0f) botTarget = escapeTarget;

    // ── Movimento: pathfinding global (BFS) com fallback de desvio reativo ─────
    Vector2 dir;
    if (wallQuery) {
        // Rota global na grade de tiles — escapa de cantos concavos de verdade.
        dir = computePathDir(playerPos, botTarget);
    } else {
        // Sem consulta de mapa: mantem o desvio reativo de 8 direcoes (legado).
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
