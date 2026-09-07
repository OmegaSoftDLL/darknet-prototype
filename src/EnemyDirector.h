#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// EnemyDirector — the IA that EVOLUI during the match.
//
// Not and difficulty by relogio nem enemy with more HP: the director OBSERVES how
// the player luta (distance in that engaja, if stays correndo/kitando, quao fast
// mata, the apanha) and responde in duas frentes:
//
//   1) COMPOSICAO — muda the peso of cada arquetipo in the spawn. Quem kita of far
//      passes the receive body-the-body fast; quem briga colado passes the receive
//      atiradores.
//   2) TACTIC — distribui papeis among the enemies proximos (frontal, flank,
//      cerco) and devolve um PONTO DE APROXIMACAO by enemy. O Game passes esse
//      point in the lugar of the position of the player while the enemy is far, entao
//      the group fence instead of virar uma bola behind of the hero.
//
// O level of adaptacao goes up when the player domina and goes down when ele apanha —
// the IA "aprende" in the ritmo of the match, without punir quem is perdendo.
// ─────────────────────────────────────────────────────────────────────────────
#include <raylib.h>
#include <vector>

enum class SquadRole { Frontal, FlankLeft, FlankRight, Encircle };

struct DirectorProfile {
    float engageDist   = 250.0f;  // distance media in that the player luta
    float kiteScore    = 0.0f;    // 0 = plant the feet, 1 = vive retreatsndo
    float killSpeed    = 0.0f;    // kills by minute (media movel)
    float damageTaken  = 0.0f;    // damage sofrido by minute (media movel)
    int   adaptLevel   = 0;       // 0..5 — the the IA already if ajustou to the player
};

class EnemyDirector {
public:
    // Chamado 1x by frame with the state of the player and of the enemies vivos.
    void observe(float dt, Vector2 playerPos, float playerHP, float playerMaxHP,
                 int killsTotal, int enemyCount);

    // Peso relativo of um arquetipo in the sorteio of spawn (1.0 = neutral).
    // `fast`/`ranged`/`tanky` descrevem the arquetipo, not the type concreto: so
    // the diretor funciona to the 51 enemies without conhecer none deles.
    float spawnWeight(bool fast, bool ranged, bool tanky) const;

    // Papel tatico by indice of enemy (stable enquanto ele viver).
    SquadRole roleFor(int enemyIndex) const;

    // Point of aproximacao to um enemy, dado your papel and the position of the alvo.
    // Near the alvo (< closeRange) devolve the own alvo: mira and attack has that
    // continue corretos, the desvio and only in the aproximacao.
    Vector2 approachPoint(int enemyIndex, Vector2 enemyPos, Vector2 targetPos,
                          float closeRange = 240.0f) const;

    const DirectorProfile& profile() const { return prof; }

private:
    DirectorProfile prof;
    float   sampleTimer   = 0.0f;
    Vector2 lastPlayerPos = { 0.0f, 0.0f };
    float   moveAccum     = 0.0f;   // distance percorrida in the window
    int     lastKills     = 0;
    float   lastHP        = -1.0f;
    float   windowTimer   = 0.0f;
};
