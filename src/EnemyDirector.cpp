#include "EnemyDirector.h"
#include <cmath>

// Window of observacao: 6s. Curta demais and the IA oscila the cada shot; longa demais
// and ela only reage after that the luta already acabou.
static const float WINDOW = 6.0f;

void EnemyDirector::observe(float dt, Vector2 playerPos, float playerHP,
                            float playerMaxHP, int killsTotal, int enemyCount) {
    if (lastHP < 0.0f) { lastHP = playerHP; lastPlayerPos = playerPos; lastKills = killsTotal; }

    // distance percorrida in the window -> the the player "kita"
    float dx = playerPos.x - lastPlayerPos.x, dy = playerPos.y - lastPlayerPos.y;
    moveAccum    += std::sqrt(dx*dx + dy*dy);
    lastPlayerPos = playerPos;

    // damage sofrido (only quedas of HP contam; healing not vira damage negative)
    if (playerHP < lastHP) prof.damageTaken += (lastHP - playerHP);
    lastHP = playerHP;

    windowTimer += dt;
    if (windowTimer < WINDOW) return;

    int   kills = killsTotal - lastKills;
    float kpm   = (float)kills * (60.0f / WINDOW);
    float dpm   = prof.damageTaken * (60.0f / WINDOW);

    // medias moveis: the IA muda of opiniao devagar, otherwise vira ioio
    prof.killSpeed   = prof.killSpeed   * 0.6f + kpm * 0.4f;
    prof.damageTaken = prof.damageTaken * 0.6f + dpm * 0.4f;
    prof.kiteScore   = prof.kiteScore   * 0.6f
                     + std::fmin(1.0f, moveAccum / (WINDOW * 220.0f)) * 0.4f;

    // Level of adaptacao: goes up when the player domina (mata fast and almost not
    // apanha), goes down when ele is apanhando. Nunca pune quem already is perdendo.
    float pressure = (playerMaxHP > 0.0f) ? (dpm / playerMaxHP) : 0.0f;
    if (kills > 0 && pressure < 0.25f && enemyCount > 0) {
        if (prof.adaptLevel < 5) prof.adaptLevel++;
    } else if (pressure > 0.75f) {
        if (prof.adaptLevel > 0) prof.adaptLevel--;
    }

    prof.damageTaken = 0.0f;
    moveAccum        = 0.0f;
    lastKills        = killsTotal;
    windowTimer      = 0.0f;
}

float EnemyDirector::spawnWeight(bool fast, bool ranged, bool tanky) const {
    float w = 1.0f;
    float the = (float)prof.adaptLevel / 5.0f;      // 0..1

    // Player that vive retreatsndo and atirando: mande quem closes distance.
    if (prof.kiteScore > 0.55f) {
        if (fast)   w *= 1.0f + 1.10f * the;
        if (ranged) w *= 1.0f - 0.35f * the;
    }
    // Player that briga colado: mande quem castiga of far.
    else if (prof.kiteScore < 0.30f) {
        if (ranged) w *= 1.0f + 0.95f * the;
        if (fast)   w *= 1.0f - 0.25f * the;
    }
    // Player that limpa tudo very fast: mande quem aguenta the tranco.
    if (prof.killSpeed > 25.0f && tanky) w *= 1.0f + 0.80f * the;

    return (w < 0.15f) ? 0.15f : w;   // none arquetipo some by complete
}

SquadRole EnemyDirector::roleFor(int enemyIndex) const {
    // Distribuicao ciclica and stable by indice: without sorteio by frame (the enemy
    // ficaria trocando of ideia) and without state extra inside the Enemy.
    // As maior the adaptacao, more gente goes to flank/cerco instead of come
    // of front igual boi.
    int   mod  = enemyIndex % 4;
    float the    = (float)prof.adaptLevel / 5.0f;
    int   flankers = (the > 0.6f) ? 3 : (the > 0.25f) ? 2 : 1;
    if (mod == 0) return SquadRole::Frontal;
    if (mod <= flankers)
        return (mod % 2 == 1) ? SquadRole::FlankLeft : SquadRole::FlankRight;
    return (the > 0.75f) ? SquadRole::Encircle : SquadRole::Frontal;
}

Vector2 EnemyDirector::approachPoint(int enemyIndex, Vector2 enemyPos,
                                     Vector2 targetPos, float closeRange) const {
    float dx = targetPos.x - enemyPos.x, dy = targetPos.y - enemyPos.y;
    float d  = std::sqrt(dx*dx + dy*dy);
    // Colado in the alvo: devolve the alvo REAL. Mira, attack and telegrafo dependem
    // disso; the desvio tatico and only in the path until la.
    if (d < closeRange || d < 1.0f) return targetPos;

    float ux = dx / d, uy = dy / d;
    float px = -uy, py = ux;                    // perpendicular

    // O desvio TEM that die conforme the enemy if aproxima. Na first versao
    // ele era proporcional the distance and sobrava always ~250u of deslocamento: the
    // flanqueadores orbitavam the player the ~450u and NUNCA encostavam - combat
    // zerado. Agora the side and limitado pelo the still falta to encostar,
    // entao the route vira um arco that ends in up of the alvo.
    float over = d - closeRange;                       // > 0 here
    float lat  = over * 0.85f; if (lat  > 340.0f) lat  = 340.0f;
    float back = over * 0.30f; if (back > 160.0f) back = 160.0f;

    switch (roleFor(enemyIndex)) {
        case SquadRole::FlankLeft:
            return { targetPos.x - ux * back + px * lat,
                     targetPos.y - uy * back + py * lat };
        case SquadRole::FlankRight:
            return { targetPos.x - ux * back - px * lat,
                     targetPos.y - uy * back - py * lat };
        case SquadRole::Encircle:                // closes pela rear of the alvo
            return { targetPos.x + ux * back * 1.4f + px * lat * 0.5f,
                     targetPos.y + uy * back * 1.4f + py * lat * 0.5f };
        case SquadRole::Frontal:
        default:
            return targetPos;
    }
}
