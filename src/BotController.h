#pragma once
#include <raylib.h>
#include <vector>
#include <string>
#include <fstream>
#include <functional>

enum class BotState {
    FleeFromDanger,  // priority 1: HP < 25%
    CollectItem,     // priority 2: item < 150px and not in heavy combat
    AttackEnemy,     // priority 3: enemy in range, kite/orbit
    ClosePortal,     // priority 4: anomaly portal nearby
    AdvancePhase,    // priority 5: zone clear, go to exit portal
    Explore,         // priority 6: spiral exploration
};

class BotController {
public:
    bool  active    = false;
    bool  autoTest  = false;
    float testDuration  = 120.0f;
    float testTimer     = 0.0f;
    std::vector<std::string> log;

    struct BotDecision {
        bool    shouldMove        = false;
        Vector2 moveTarget        = {0, 0};
        bool    shouldMeleeAttack = false;
        bool    shouldUseSkill1   = false;
        bool    shouldUseSkill2   = false;
        bool    shouldUseSkill3   = false;
        bool    shouldUseSkill4   = false;
        bool    shouldUseSkill5   = false;
        bool    shouldUseSkill6   = false;
        int     nearestEnemyIdx   = -1;
        Vector2 nearestEnemyPos   = {0, 0};  // for skill aiming
        bool    shouldUsePortal   = false;   // bot wants acionar the portal of phase (equivale the key E)
        bool    shouldQuit        = false;
        BotState currentState     = BotState::Explore;
    };

    // Full update — accepts all 6 skill readiness flags + portal positions
    BotDecision update(float dt,
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
                       int     openAnomalyPortals);

    void toggle() {
        active = !active;
        addLog(active ? "BOT ATIVADO [F12 p/ desligar]" : "BOT DESLIGADO");
        // Reset stuck state on toggle
        stuckTimer       = 0.0f;
        stuckAngleOffset = 0.0f;
        stuckEscapeDir   = 1;
        longStuckTimer   = 0.0f;
        clearTimer       = 0.0f;
        exploreStep      = 0;
        totalDistance    = 0.0f;
    }

    void addLog(const std::string& msg);
    void writeReport(const std::string& path) const;

    // Reset COMPLETO between partidas (telemetria, state, timers, route cacheada).
    // Preserva active/autoTest/testDuration — called by Game::restartRun.
    void reset();

    // PORTAO DE VALIDATION: transforma the report in pass/fail. Sem isto qualquer
    // change (minha ou of other agente) podia quebrar the game without ninguem notar
    // until open and play. `reasons` receives the motivo of cada reprovacao.
    bool passed(std::vector<std::string>* reasons = nullptr) const;

    // Sensores of wall — Game preenche before update() the partir of the tilemap.
    // Indices: 0=E 1=NE 2=N 3=NW 4=W 5=SW 6=S 7=SE (igual k8DirAngles)
    bool blockedDir[8] = {false,false,false,false,false,false,false,false};

    // Consulta of collision global of the map (preenchida pelo Game the partir of the tilemap).
    // Returns true if the world position is a wall/obstacle.
    // Quando definida, the bot usa pathfinding global (BFS) instead of the desvio reativo.
    std::function<bool(Vector2)> wallQuery;

    // Center of the map (preenchido pelo Game) — destino of escape when the bot stays
    // preso numa edge/barrier of the world open. {0,0} = not defined.
    Vector2 worldCenter = {0, 0};
    float   worldRadius = 0.0f;   // radius playable of the phase (0 = desconhecido)

    // Zone segura (refuge) — preenchida pelo Game. Enemies only spawnam FORA
    // dela, entao the bot precisa leave dai to find combat (0 = desconhecida).
    Vector2 safeZoneCenter = {0, 0};
    float   safeZoneRadius = 0.0f;

    // Open-world PHASE portal — filled by Game every frame
    // (owPortalPos/owPortalOpen). Sem isto the bot not sabia to where go when the
    // portal abria: recebia only tilemap.portals (system old, empty in the OW).
    Vector2 owPortalPos  = {0, 0};
    bool    owPortalOpen = false;
    // Celula alcancavel more LONGE of the bot in the last BFS. E the only destino that
    // if can prometer that produz deslocamento when ele is encurralado.
    // window of medicao of the 'preso' (see updateStuckTracking)
    float   stuckAccum  = 0.0f;
    float   stuckSample = 0.0f;
    bool    stuckCounted = false;
    Vector2 farthestReachable = {0, 0};
    bool    hasFarReach       = false;

    // ── Telemetry ─────────────────────────────────────────────────────────────
    int   frameCount        = 0;
    int   meleeHits         = 0;
    int   skillsFired       = 0;
    int   itemsCollected    = 0;   // items really collected (incrementado pelo Game)
    int   itemsChased       = 0;   // items moved toward
    int   killCount         = 0;
    int   damageEvents      = 0;
    float totalDmgTaken     = 0.0f;
    int   deathCount        = 0;
    float minFPS            = 9999.0f;
    float maxFPS            = 0.0f;
    float fpsAccum          = 0.0f;
    int   fpsSamples        = 0;
    float lowestHP          = 999.0f;
    int   dangersZones      = 0;
    int   areasExplored     = 0;
    int   zonesVisited      = 0;    // how many times AdvancePhase portal was reached
    float totalDistance     = 0.0f; // total pixels traveled
    int   skillUsageCounts[6] = {0,0,0,0,0,0}; // per-skill fire counts
    int   stuckEvents       = 0;    // times stuck for > 2s
    int   longStuckEvents   = 0;    // times stuck for > 10s (bug report)
    std::vector<std::string> issueLog;

    // Picos of contagem of entidades (diagnostic of FPS) — preenchidos pelo Game.
    int   peakEnemies     = 0;
    int   peakItems       = 0;
    int   peakOrbs        = 0;
    int   peakProjectiles = 0;
    int   peakEnemyProj   = 0;
    int   peakParticles   = 0;
    int   peakUnits       = 0;
    int   fpsLowEnemies   = 0;   // contagens in the instante of the FPS more down
    int   fpsLowProj      = 0;
    int   fpsLowParticles = 0;
    float fpsLowValue     = 9999.0f;
    float peakUpdateMs    = 0.0f; // pior time of update()
    float peakRenderMs    = 0.0f; // pior time of render()
    // Window of quarantine of the FPS: apos um frame of CARGA (dt > 0,25s — worldgen
    // of match/phase), the GetFPS() of the raylib stays envenenado by ~0,5s (media
    // movel of 30 amostras) and reporta FPS ~6 with the game rodando the 60. During the
    // quarantine the amostras of FPS sao ignoradas: the "FPS minimum" mede GAMEPLAY,
    // not screen of loading.
    float fpsQuarantine   = 0.0f;

private:
    // State machine
    BotState  botState      = BotState::Explore;
    Vector2   botTarget     = {0, 0};
    float     logicTimer    = 0.0f;
    float     reportTimer   = 0.0f;
    float     attackTimer   = 0.0f;
    float     skillTimer    = 0.0f;    // general skill cooldown gate
    int       lastLoggedMode = -1;

    // Anti-wall / stuck detection (improved 8-direction)
    Vector2   lastPos            = {0, 0};
    float     stuckTimer         = 0.0f;   // resets when moving
    float     longStuckTimer     = 0.0f;   // accumulates total stuck time
    float     stuckAngleOffset   = 0.0f;   // rotated escape angle when stuck
    int       stuckEscapeDir     = 1;      // alternates +/-
    int       stuckDirIdx        = 0;      // index into 8-direction table
    bool      isStuck            = false;

    // Orbit / kite
    float     orbitAngle    = 0.0f;
    float     orbitRadius   = 220.0f;

    // Phase advance
    float     clearTimer    = 0.0f;    // time with in the enemies + in the items
    bool      wasLowHP      = false;
    Vector2   fleeTarget    = {0, 0};
    Vector2   lastAdvancePortalPos = {-99999.0f, -99999.0f}; // evita contar the same portal multiplas vezes

    // Exploration spiral
    float     exploreTimer  = 0.0f;
    int       exploreStep   = 0;
    float     exploreSafeZoneTimer = 0.0f; // time preso DENTRO of the zone segura

    // Engage melee (~1.5s fechando distance direct, without recuo nem orbita)
    float     engageTimer   = 0.0f;

    // Area tracking
    int       lastQuadrant  = -1;
    float     quadrantTimer = 0.0f;

    // Distance tracking
    Vector2   prevPlayerPos = {-9999, -9999};

    // Stagnation / death detection
    bool      wasDeadLastFrame = false;
    int       lastKillCheck    = 0;
    int       lastZoneCheck    = 0;
    float     stagnationTimer  = 0.0f;

    // ── Pathfinding global (BFS in the grade of tiles) ────────────────────────────
    std::vector<Vector2> cachedPath;                 // waypoints in coordenadas of world
    float                repathTimer    = 0.0f;      // recalcula when <= 0
    Vector2              lastPathTarget = {-9999, -9999};

    // Escape of bordas/bolsoes of the map
    float                escapeTimer    = 0.0f;
    Vector2              escapeTarget   = {0, 0};

    // Helpers
    void    updateStuckTracking(Vector2 currentPos, float dt);
    Vector2 computePathDir(Vector2 from, Vector2 to);
    Vector2 computeAntiWall(Vector2 desired, Vector2 currentPos, float dt);
    int     countEnemiesInRadius(const std::vector<Vector2>& positions,
                                  Vector2 center, float radius) const;
};
