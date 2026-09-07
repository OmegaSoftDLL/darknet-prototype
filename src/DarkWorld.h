#pragma once
#include <raylib.h>
#include <vector>
#include <string>

struct SceneryObject {
    Vector2   position;
    int       type;     // 0=house 1=barn 2=deadtree 3=gravestone 4=fence 5=streetlight 6=car 7=building 8=silo 9=arch 10=statue 11=grass 12=pedras
    float     rotation;
    float     scale;
    Color     tint;
    long long chunk = -1;  // -1 = scenario fixed of the region; >=0 = chunk procedural infinito
};

struct DarkWorld {
    std::string             name;
    std::string             subtitle;
    Color                   fogColor    = {200, 220, 200, 255};
    float                   fogDensity  = 0.5f;
    Color                   skyTop      = {8,  8,  18, 255};
    Color                   skyBottom   = {20, 18, 28, 255};
    std::vector<SceneryObject> scenery;

    // Generate scenery for the zone using the seed
    void generate(int zoneId, unsigned int seed);

    // Draw sky, moon, stars
    void renderSky(float ambientTime, int screenW, int screenH) const;

    // Draw ground objects (call before entities)
    void renderScenery(Vector2 cameraOffset, float time) const;

    // Draw fog overlay (call after entities)
    void renderFog(float ambientTime, int screenW, int screenH) const;

    // Individual structure draw functions (world-space, in-camera)
    static void drawHouse(Vector2 pos, float scale, bool lights, float time);
    static void drawBarn(Vector2 pos, float scale);
    static void drawDeadTree(Vector2 pos, float scale);
    static void drawGravestone(Vector2 pos, float rot);
    static void drawFence(Vector2 pos, float rot, float scale);
    static void drawStreetLight(Vector2 pos, bool on, float time);
    static void drawAbandonedCar(Vector2 pos, float rot);
    static void drawCityBuilding(Vector2 pos, float w, float h, float time);
    static void drawSilo(Vector2 pos, float scale);
    static void drawStatue(Vector2 pos, float scale);
    static void drawCatacombArch(Vector2 pos);
};

struct DarkWorldSystem {
    DarkWorld currentZone;
    bool      active       = false;
    float     ambientTime  = 0.0f;

    void load(int zoneId, unsigned int seed);
    void applyWorldOffset(Vector2 offset);
    void update(float dt);
    void renderBackground(int screenW, int screenH) const;
    void renderScenery(Vector2 cameraTarget, float zoom) const;
    void renderFog(int screenW, int screenH) const;
};
