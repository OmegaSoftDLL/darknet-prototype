#include "LightSystem.h"
#include <cmath>
#include <algorithm>
#include <raymath.h>

// ─── Init / Shutdown ─────────────────────────────────────────────────────────

void LightSystem::init(int w, int h) {
    maskW = w; maskH = h;
    lightMask = LoadRenderTexture(w, h);
    SetTextureFilter(lightMask.texture, TEXTURE_FILTER_BILINEAR);
}

void LightSystem::shutdown() {
    if (maskW > 0) UnloadRenderTexture(lightMask);
}

void LightSystem::clear() {
    lights.clear();
}

// ─── Light adders ────────────────────────────────────────────────────────────

void LightSystem::addLight(Vector2 pos, float radius, float intensity, Color color, bool flicker) {
    LightSource l;
    l.position  = pos;
    l.radius    = radius;
    l.intensity = intensity;
    l.color     = color;
    l.flicker   = flicker;
    lights.push_back(l);
}

void LightSystem::addPlayerLight(Vector2 pos) {
    // Always slot 0 — large warm bubble so the player always sees nearby
    // scenery (houses, trees, etc.) even in the darkest zones.
    LightSource l;
    l.position  = pos;
    l.radius    = 430.0f;
    l.intensity = 0.72f;   // larga e fraca: clareia o entorno sem virar holofote
    l.color     = {255, 226, 180, 255};   // tom quente (clima Diablo)
    l.flicker   = false;
    lights.insert(lights.begin(), l); // always index 0
}

void LightSystem::addTorchLight(Vector2 pos) {
    LightSource l;
    l.position    = pos;
    l.radius      = 190.0f;
    l.intensity   = 0.82f;
    l.color       = {255, 140, 40, 255};
    l.flicker     = true;
    l.flickerTimer = (float)(GetRandomValue(0, 314)) * 0.01f;
    l.flickerSpeed = 3.0f + (float)GetRandomValue(-8, 8) * 0.1f;
    l.flickerAmt  = 0.28f;
    lights.push_back(l);
}

void LightSystem::addPortalLight(Vector2 pos, Color color) {
    LightSource l;
    l.position    = pos;
    l.radius      = 230.0f;
    l.intensity   = 0.90f;
    l.color       = color;
    l.flicker     = true;
    l.flickerTimer = (float)(GetRandomValue(0, 200)) * 0.01f;
    l.flickerSpeed = 5.0f;
    l.flickerAmt  = 0.20f;
    lights.push_back(l);
}

void LightSystem::addBuildingLight(Vector2 pos) {
    addLight(pos, 95.0f, 0.50f, {255, 220, 120, 255}, true);
    lights.back().flickerSpeed = 1.5f;
    lights.back().flickerAmt   = 0.10f;
}

// ─── Update ──────────────────────────────────────────────────────────────────

void LightSystem::updateFlicker(float dt) {
    for (auto& l : lights) {
        if (!l.flicker) continue;
        l.flickerTimer += dt * l.flickerSpeed;
        float flicker = std::sin(l.flickerTimer)         * 0.50f
                      + std::sin(l.flickerTimer * 2.73f) * 0.30f
                      + std::sin(l.flickerTimer * 0.47f) * 0.20f;
        l.intensity = 0.75f + flicker * l.flickerAmt;
        l.intensity = std::max(0.35f, std::min(1.0f, l.intensity));
    }
}

void LightSystem::updatePlayerPos(Vector2 pos) {
    if (!lights.empty()) lights[0].position = pos;
}

// ─── Render ──────────────────────────────────────────────────────────────────

void LightSystem::prepareMask(Camera2D camera) {
    if (!enabled) return;

    BeginTextureMode(lightMask);
    // Fill with ambient darkness
    float amb = 1.0f - ambientDark;
    ClearBackground({ (unsigned char)(amb*ambientColor.r), (unsigned char)(amb*ambientColor.g), (unsigned char)(amb*ambientColor.b), 255 });

    BeginMode2D(camera);
    BeginBlendMode(BLEND_ADDITIVE);

    for (const auto& l : lights) {
        if (!l.active) continue;

        // Soft gradient: 14 concentric circles from outer to inner
        // Inner circles are brighter; outer circles fade to 0.
        const int steps = 30;
        for (int s = steps; s >= 0; s--) {
            float t      = (float)s / (float)steps;     // 1.0 = inner, 0.0 = outer
            float r      = l.radius * (float)(steps - s + 1) / (float)(steps + 1);
            float bright = l.intensity * t * t * t * 0.30f;     // queda cubica, pico baixo
            Color c = {
                (unsigned char)((float)l.color.r * bright),
                (unsigned char)((float)l.color.g * bright),
                (unsigned char)((float)l.color.b * bright),
                (unsigned char)(bright * 255.0f)
            };
            DrawCircleV(l.position, r, c);
        }
    }

    EndBlendMode();
    EndMode2D();
    EndTextureMode();
}

void LightSystem::prepareMask3D(const Camera3D& camera3D, int screenW, int screenH) {
    if (!enabled) return;

    BeginTextureMode(lightMask);
    // Fill with ambient darkness
    float amb = 1.0f - ambientDark;
    ClearBackground({ (unsigned char)(amb*ambientColor.r), (unsigned char)(amb*ambientColor.g), (unsigned char)(amb*ambientColor.b), 255 });

    BeginBlendMode(BLEND_ADDITIVE);

    for (const auto& l : lights) {
        if (!l.active) continue;

        // A luz cai no CHAO, entao a poca e uma ELIPSE em perspectiva - nao um
        // circulo. Projetar so o raio em X e desenhar circulo era o que fazia cada
        // lampada (e o proprio heroi) virar um "sol" chapado colado na tela.
        // Projeto duas bordas: +X da o semieixo horizontal, +Z o vertical.
        Vector3 pos3D   = { l.position.x, 0.0f, l.position.y };
        Vector2 centerS = GetWorldToScreenEx(pos3D, camera3D, screenW, screenH);
        Vector2 edgeX   = GetWorldToScreenEx({ l.position.x + l.radius, 0.0f, l.position.y },
                                             camera3D, screenW, screenH);
        Vector2 edgeZ   = GetWorldToScreenEx({ l.position.x, 0.0f, l.position.y + l.radius },
                                             camera3D, screenW, screenH);
        float rx = Vector2Distance(centerS, edgeX);
        float ry = Vector2Distance(centerS, edgeZ);   // achatado pela inclinacao da camera
        if (rx < 1.0f || ry < 1.0f) continue;

        // 30 aneis com pico BAIXO: o degrade fica continuo (14 aneis fortes
        // desenhavam faixas visiveis) e a borda morre em zero, sem circulo duro.
        const int steps = 30;
        for (int s = steps; s >= 0; s--) {
            float t  = (float)s / (float)steps;        // 1.0 = centro, 0.0 = borda
            float k  = (float)(steps - s + 1) / (float)(steps + 1);
            float ff = t * t * t;                      // queda cubica: centro concentrado
            float bright = l.intensity * ff * 0.30f;
            Color c = {
                (unsigned char)((float)l.color.r * bright),
                (unsigned char)((float)l.color.g * bright),
                (unsigned char)((float)l.color.b * bright),
                (unsigned char)(bright * 255.0f)
            };
            DrawEllipse((int)centerS.x, (int)centerS.y, rx * k, ry * k, c);
        }
    }

    EndBlendMode();
    EndTextureMode();
}

void LightSystem::applyMask() const {
    if (!enabled) return;

    // Draw the lightMask over the current render target using MULTIPLY
    // Where mask is black → darkens (shadows); where mask is white → unchanged
    BeginBlendMode(BLEND_MULTIPLIED);
    DrawTexturePro(
        lightMask.texture,
        { 0.0f, 0.0f, (float)maskW, -(float)maskH },   // flip Y (RenderTexture is upside-down)
        { 0.0f, 0.0f, (float)maskW,  (float)maskH },
        { 0.0f, 0.0f }, 0.0f, WHITE
    );
    EndBlendMode();
}
