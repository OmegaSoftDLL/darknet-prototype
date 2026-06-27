#pragma once
#include <raylib.h>
#include <vector>
#include <cmath>
#include "Zone.h"

// Objeto de cenario pre-gerado (carros, escombros, postes, fogueiras)
struct EnvObject {
    enum class Type { Car, Rubble, Lamppost, FirePit, Crate, Terminal };
    Type    type;
    Vector2 pos;
    float   rotation  = 0.0f;
    Color   tint      = WHITE;
    float   scale     = 1.0f;
    bool    hasFire   = false;
    float   animTimer = 0.0f;
};

struct ParallaxStar {
    Vector2 pos;        // screen-space at reference scroll=0
    float   brightness; // 0-1
    float   layer;      // 0=far (slow), 1=near (faster)
    float   twinkle;    // phase offset for twinkle
};

struct DataStreamDrop {
    float x, y;
    float speed;
    int   len;
    float alpha;
};

class Background {
public:
    std::vector<EnvObject>     envObjects;
    std::vector<Vector2>       ashParticles;
    std::vector<Vector2>       ashVelocities;
    std::vector<ParallaxStar>  stars;
    std::vector<DataStreamDrop> dataDrops;

    float time    = 0.0f;
    ZoneID lastZone = ZoneID::LARuins;

    void generate(ZoneID zone, int mapW, int mapH, int tileSize);
    void update(float dt, Camera2D cam);

    // Desenha o ceu (antes do BeginMode2D) — agora inclui parallax stars
    void drawSky(int screenW, int screenH, ZoneID zone) const;

    // Desenha silhueta de predio ao fundo
    void drawSkyline(Camera2D cam, int screenW, int screenH, ZoneID zone) const;

    // Parallax stars (screen-space, call after drawSky)
    void drawParallaxStars(Camera2D cam, int screenW, int screenH) const;

    // Data stream / Matrix rain (screen-space, for tech zones)
    void drawDataStream(int screenW, int screenH, ZoneID zone) const;

    // Desenha objetos de cenario (dentro do BeginMode2D)
    void drawEnvObjects() const;

    // Particulas de cinza
    void drawAsh() const;

private:
    void placeObjects(ZoneID zone, int mapW, int mapH, int tileSize);
    void generateStars(int screenW, int screenH);
    void drawCar(Vector2 pos, float rot, Color tint) const;
    void drawRubble(Vector2 pos, float scale, Color tint) const;
    void drawLamppost(Vector2 pos, bool lit) const;
    void drawFirePit(Vector2 pos, float t) const;
    void drawCrate(Vector2 pos, Color tint) const;
    void drawTerminal(Vector2 pos, float t) const;
};
