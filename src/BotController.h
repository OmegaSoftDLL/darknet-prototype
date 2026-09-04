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
        bool    shouldUsePortal   = false;   // bot quer acionar o portal de fase (equivale a tecla E)
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

    // Reset COMPLETO entre partidas (telemetria, estado, timers, rota cacheada).
    // Preserva active/autoTest/testDuration — chamado por Game::restartRun.
    void reset();

    // PORTAO DE VALIDACAO: transforma o relatorio em pass/fail. Sem isto qualquer
    // mudanca (minha ou de outro agente) podia quebrar o jogo sem ninguem notar
    // ate abrir e jogar. `reasons` recebe o motivo de cada reprovacao.
    bool passed(std::vector<std::string>* reasons = nullptr) const;

    // Sensores de parede — Game preenche antes de update() a partir do tilemap.
    // Indices: 0=E 1=NE 2=N 3=NW 4=W 5=SW 6=S 7=SE (igual k8DirAngles)
    bool blockedDir[8] = {false,false,false,false,false,false,false,false};

    // Consulta de colisao global do mapa (preenchida pelo Game a partir do tilemap).
    // Retorna true se a posicao de mundo dada e uma parede/obstaculo.
    // Quando definida, o bot usa pathfinding global (BFS) em vez do desvio reativo.
    std::function<bool(Vector2)> wallQuery;

    // Centro do mapa (preenchido pelo Game) — destino de escape quando o bot fica
    // preso numa borda/barreira do mundo aberto. {0,0} = nao definido.
    Vector2 worldCenter = {0, 0};
    float   worldRadius = 0.0f;   // raio jogavel da fase (0 = desconhecido)

    // Zona segura (refugio) — preenchida pelo Game. Inimigos so spawnam FORA
    // dela, entao o bot precisa sair dai para encontrar combate (0 = desconhecida).
    Vector2 safeZoneCenter = {0, 0};
    float   safeZoneRadius = 0.0f;

    // Portal de FASE do mundo aberto — preenchido pelo Game a cada frame
    // (owPortalPos/owPortalOpen). Sem isto o bot nao sabia para onde ir quando o
    // portal abria: recebia so tilemap.portals (sistema antigo, vazio no OW).
    Vector2 owPortalPos  = {0, 0};
    bool    owPortalOpen = false;
    // Celula alcancavel mais LONGE do bot na ultima BFS. E o unico destino que
    // se pode prometer que produz deslocamento quando ele esta encurralado.
    // janela de medicao do 'preso' (ver updateStuckTracking)
    float   stuckAccum  = 0.0f;
    float   stuckSample = 0.0f;
    bool    stuckCounted = false;
    Vector2 farthestReachable = {0, 0};
    bool    hasFarReach       = false;

    // ── Telemetry ─────────────────────────────────────────────────────────────
    int   frameCount        = 0;
    int   meleeHits         = 0;
    int   skillsFired       = 0;
    int   itemsCollected    = 0;   // itens realmente coletados (incrementado pelo Game)
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

    // Picos de contagem de entidades (diagnostico de FPS) — preenchidos pelo Game.
    int   peakEnemies     = 0;
    int   peakItems       = 0;
    int   peakOrbs        = 0;
    int   peakProjectiles = 0;
    int   peakEnemyProj   = 0;
    int   peakParticles   = 0;
    int   peakUnits       = 0;
    int   fpsLowEnemies   = 0;   // contagens no instante do FPS mais baixo
    int   fpsLowProj      = 0;
    int   fpsLowParticles = 0;
    float fpsLowValue     = 9999.0f;
    float peakUpdateMs    = 0.0f; // pior tempo de update()
    float peakRenderMs    = 0.0f; // pior tempo de render()
    // Janela de quarentena do FPS: apos um frame de CARGA (dt > 0,25s — worldgen
    // de partida/fase), o GetFPS() do raylib fica envenenado por ~0,5s (media
    // movel de 30 amostras) e reporta FPS ~6 com o jogo rodando a 60. Durante a
    // quarentena as amostras de FPS sao ignoradas: o "FPS minimo" mede GAMEPLAY,
    // nao tela de loading.
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
    float     clearTimer    = 0.0f;    // time with no enemies + no items
    bool      wasLowHP      = false;
    Vector2   fleeTarget    = {0, 0};

    // Exploration spiral
    float     exploreTimer  = 0.0f;
    int       exploreStep   = 0;
    float     exploreSafeZoneTimer = 0.0f; // tempo preso DENTRO da zona segura

    // Engage melee (~1.5s fechando distancia direto, sem recuo nem orbita)
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

    // ── Pathfinding global (BFS na grade de tiles) ────────────────────────────
    std::vector<Vector2> cachedPath;                 // waypoints em coordenadas de mundo
    float                repathTimer    = 0.0f;      // recalcula quando <= 0
    Vector2              lastPathTarget = {-9999, -9999};

    // Escape de bordas/bolsoes do mapa
    float                escapeTimer    = 0.0f;
    Vector2              escapeTarget   = {0, 0};

    // Helpers
    void    updateStuckTracking(Vector2 currentPos, float dt);
    Vector2 computePathDir(Vector2 from, Vector2 to);
    Vector2 computeAntiWall(Vector2 desired, Vector2 currentPos, float dt);
    int     countEnemiesInRadius(const std::vector<Vector2>& positions,
                                  Vector2 center, float radius) const;
};
