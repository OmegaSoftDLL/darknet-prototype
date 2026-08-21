#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// EnemyDirector — a IA que EVOLUI durante a partida.
//
// Nao e dificuldade por relogio nem inimigo com mais HP: o diretor OBSERVA como
// o jogador luta (distancia em que engaja, se fica correndo/kitando, quao rapido
// mata, quanto apanha) e responde em duas frentes:
//
//   1) COMPOSICAO — muda o peso de cada arquetipo no spawn. Quem kita de longe
//      passa a receber corpo-a-corpo rapido; quem briga colado passa a receber
//      atiradores.
//   2) TATICA — distribui papeis entre os inimigos proximos (frontal, flanco,
//      cerco) e devolve um PONTO DE APROXIMACAO por inimigo. O Game passa esse
//      ponto no lugar da posicao do jogador enquanto o inimigo esta longe, entao
//      o grupo cerca em vez de virar uma bola atras do heroi.
//
// O nivel de adaptacao sobe quando o jogador domina e desce quando ele apanha —
// a IA "aprende" no ritmo da partida, sem punir quem esta perdendo.
// ─────────────────────────────────────────────────────────────────────────────
#include <raylib.h>
#include <vector>

enum class SquadRole { Frontal, FlankLeft, FlankRight, Encircle };

struct DirectorProfile {
    float engageDist   = 250.0f;  // distancia media em que o jogador luta
    float kiteScore    = 0.0f;    // 0 = planta os pes, 1 = vive recuando
    float killSpeed    = 0.0f;    // abates por minuto (media movel)
    float damageTaken  = 0.0f;    // dano sofrido por minuto (media movel)
    int   adaptLevel   = 0;       // 0..5 — quanto a IA ja se ajustou ao jogador
};

class EnemyDirector {
public:
    // Chamado 1x por frame com o estado do jogador e dos inimigos vivos.
    void observe(float dt, Vector2 playerPos, float playerHP, float playerMaxHP,
                 int killsTotal, int enemyCount);

    // Peso relativo de um arquetipo no sorteio de spawn (1.0 = neutro).
    // `fast`/`ranged`/`tanky` descrevem o arquetipo, nao o tipo concreto: assim
    // o diretor funciona para os 51 inimigos sem conhecer nenhum deles.
    float spawnWeight(bool fast, bool ranged, bool tanky) const;

    // Papel tatico por indice de inimigo (estavel enquanto ele viver).
    SquadRole roleFor(int enemyIndex) const;

    // Ponto de aproximacao para um inimigo, dado seu papel e a posicao do alvo.
    // Perto do alvo (< closeRange) devolve o proprio alvo: mira e ataque tem que
    // continuar corretos, o desvio e so na aproximacao.
    Vector2 approachPoint(int enemyIndex, Vector2 enemyPos, Vector2 targetPos,
                          float closeRange = 240.0f) const;

    const DirectorProfile& profile() const { return prof; }

private:
    DirectorProfile prof;
    float   sampleTimer   = 0.0f;
    Vector2 lastPlayerPos = { 0.0f, 0.0f };
    float   moveAccum     = 0.0f;   // distancia percorrida na janela
    int     lastKills     = 0;
    float   lastHP        = -1.0f;
    float   windowTimer   = 0.0f;
};
