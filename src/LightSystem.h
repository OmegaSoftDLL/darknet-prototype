#pragma once
#include <raylib.h>
#include <vector>

struct LightSource {
    Vector2 position;
    float   radius       = 150.0f;
    float   intensity    = 1.0f;
    Color   color        = {255, 200, 120, 255};
    bool    flicker      = false;
    float   flickerTimer = 0.0f;
    float   flickerSpeed = 3.0f;
    float   flickerAmt   = 0.25f;
    bool    active       = true;
};

struct LightSystem {
    std::vector<LightSource> lights;
    RenderTexture2D lightMask  = {};
    bool  enabled              = false;
    float ambientDark          = 0.55f; // fraction of screen in darkness (0=bright, 1=black)
    int   maskW                = 0;
    int   maskH                = 0;

    void init(int w, int h);
    void shutdown();
    void clear();  // remove all lights

    // Convenience adders
    void addLight(Vector2 pos, float radius, float intensity, Color color, bool flicker = false);
    void addPlayerLight(Vector2 pos);
    void addTorchLight(Vector2 pos);
    void addPortalLight(Vector2 pos, Color color);
    void addBuildingLight(Vector2 pos);

    // Update flickering each frame
    void updateFlicker(float dt);

    // Set player light position (index 0 is always player)
    void updatePlayerPos(Vector2 pos);

    // Two-step render:
    // 1. Call prepareMask() BEFORE BeginTextureMode(gameTarget) — renders lights to lightMask
    void prepareMask(Camera2D camera);
    // 2. Call applyMask() INSIDE BeginTextureMode(gameTarget), after EndMode2D()
    void applyMask() const;

    void setEnabled(bool b) { enabled = b; }

    bool isDarkZone(int zoneId) const {
        return zoneId >= 4; // Cemetery(4)..AbandonedManor(9)
    }
};
