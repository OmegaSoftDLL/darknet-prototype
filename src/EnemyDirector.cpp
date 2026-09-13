#include "EnemyDirector.h"
#include <cmath>

// Janela de observacao: 6s. Curta demais e a IA oscila a cada tiro; longa demais
// e ela so reage depois que a luta ja acabou.
static const float WINDOW = 6.0f;

void EnemyDirector::observe(float dt, Vector2 playerPos, float playerHP,
                            float playerMaxHP, int killsTotal, int enemyCount) {
    if (lastHP < 0.0f) { lastHP = playerHP; lastPlayerPos = playerPos; lastKills = killsTotal; }

    // distancia percorrida na janela -> quanto o jogador "kita"
    float dx = playerPos.x - lastPlayerPos.x, dy = playerPos.y - lastPlayerPos.y;
    moveAccum    += std::sqrt(dx*dx + dy*dy);
    lastPlayerPos = playerPos;

    // dano sofrido (so quedas de HP contam; cura nao vira dano negativo)
    if (playerHP < lastHP) prof.damageTaken += (lastHP - playerHP);
    lastHP = playerHP;

    windowTimer += dt;
    if (windowTimer < WINDOW) return;

    int   kills = killsTotal - lastKills;
    float kpm   = (float)kills * (60.0f / WINDOW);
    float dpm   = prof.damageTaken * (60.0f / WINDOW);

    // medias moveis: a IA muda de opiniao devagar, senao vira ioio
    prof.killSpeed   = prof.killSpeed   * 0.6f + kpm * 0.4f;
    prof.damageTaken = prof.damageTaken * 0.6f + dpm * 0.4f;
    prof.kiteScore   = prof.kiteScore   * 0.6f
                     + std::fmin(1.0f, moveAccum / (WINDOW * 220.0f)) * 0.4f;

    // Nivel de adaptacao: sobe quando o jogador domina (mata rapido e quase nao
    // apanha), desce quando ele esta apanhando. Nunca pune quem ja esta perdendo.
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
    float a = (float)prof.adaptLevel / 5.0f;      // 0..1

    // Jogador que vive recuando e atirando: mande quem fecha distancia.
    if (prof.kiteScore > 0.55f) {
        if (fast)   w *= 1.0f + 1.10f * a;
        if (ranged) w *= 1.0f - 0.35f * a;
    }
    // Jogador que briga colado: mande quem castiga de longe.
    else if (prof.kiteScore < 0.30f) {
        if (ranged) w *= 1.0f + 0.95f * a;
        if (fast)   w *= 1.0f - 0.25f * a;
    }
    // Jogador que limpa tudo muito rapido: mande quem aguenta o tranco.
    if (prof.killSpeed > 25.0f && tanky) w *= 1.0f + 0.80f * a;

    return (w < 0.15f) ? 0.15f : w;   // nenhum arquetipo some por completo
}

SquadRole EnemyDirector::roleFor(int enemyIndex) const {
    // Distribuicao ciclica e estavel por indice: sem sorteio por frame (o inimigo
    // ficaria trocando de ideia) e sem estado extra dentro do Enemy.
    // Quanto maior a adaptacao, mais gente vai para flanco/cerco em vez de vir
    // de frente igual boi.
    int   mod  = enemyIndex % 4;
    float a    = (float)prof.adaptLevel / 5.0f;
    int   flankers = (a > 0.6f) ? 3 : (a > 0.25f) ? 2 : 1;
    if (mod == 0) return SquadRole::Frontal;
    if (mod <= flankers)
        return (mod % 2 == 1) ? SquadRole::FlankLeft : SquadRole::FlankRight;
    return (a > 0.75f) ? SquadRole::Encircle : SquadRole::Frontal;
}

Vector2 EnemyDirector::approachPoint(int enemyIndex, Vector2 enemyPos,
                                     Vector2 targetPos, float closeRange) const {
    float dx = targetPos.x - enemyPos.x, dy = targetPos.y - enemyPos.y;
    float d  = std::sqrt(dx*dx + dy*dy);
    // Colado no alvo: devolve o alvo REAL. Mira, ataque e telegrafo dependem
    // disso; o desvio tatico e so no caminho ate la.
    if (d < closeRange || d < 1.0f) return targetPos;

    float ux = dx / d, uy = dy / d;
    float px = -uy, py = ux;                    // perpendicular

    // O desvio TEM que morrer conforme o inimigo se aproxima. Na primeira versao
    // ele era proporcional a distancia e sobrava sempre ~250u de deslocamento: os
    // flanqueadores orbitavam o jogador a ~450u e NUNCA encostavam - combate
    // zerado. Agora o lateral e limitado pelo quanto ainda falta para encostar,
    // entao a rota vira um arco que termina em cima do alvo.
    float over = d - closeRange;                       // > 0 aqui
    float lat  = over * 0.85f; if (lat  > 340.0f) lat  = 340.0f;
    float back = over * 0.30f; if (back > 160.0f) back = 160.0f;

    switch (roleFor(enemyIndex)) {
        case SquadRole::FlankLeft:
            return { targetPos.x - ux * back + px * lat,
                     targetPos.y - uy * back + py * lat };
        case SquadRole::FlankRight:
            return { targetPos.x - ux * back - px * lat,
                     targetPos.y - uy * back - py * lat };
        case SquadRole::Encircle:                // fecha pela retaguarda do alvo
            return { targetPos.x + ux * back * 1.4f + px * lat * 0.5f,
                     targetPos.y + uy * back * 1.4f + py * lat * 0.5f };
        case SquadRole::Frontal:
        default:
            return targetPos;
    }
}
