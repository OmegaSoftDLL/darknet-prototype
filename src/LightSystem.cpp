#include "LightSystem.h"
#include <cmath>
#include <algorithm>
#include <raymath.h>

// ─── Init / Shutdown ─────────────────────────────────────────────────────────

void LightSystem::init(int w, int h) {
    maskW = w; maskH = h;
    // Mascara in MEIA resolution: light smooth borrada by natureza, entao the upscale
    // bilinear and imperceptivel — and the cost of fill of the mascara falls ~4x.
    lightMask = GfxRenderTexture(LoadRenderTexture(w / 2, h / 2));
    SetTextureFilter(lightMask.get().texture, TEXTURE_FILTER_BILINEAR);

    // GLOW radial pre-generated: substitui the 31 aneis concentricos by light by UMA
    // quad texturizada. O profile of alpha reproduz EXATAMENTE the soma of the aneis
    // antigos: in the blend aditivo (src*srcA) cada anel contribuia color*bright^2,
    // entao P(f) = soma of the (0.30*t^3)^2 of the aneis that cobrem the fracao f of the radius.
    {
        const int GS = 256;
        const int steps = 30;
        float lut[GS + 1] = {};
        for (int s = 0; s <= steps; ++s) {
            float t = (float)s / (float)steps;
            float k = (float)(steps - s + 1) / (float)(steps + 1); // fracao of the radius of the anel
            float b = 0.30f * t * t * t;                           // bright of the anel (intensity=1)
            int lim = (int)(k * GS + 0.5f);
            if (lim > GS) lim = GS;
            for (int i = 0; i <= lim; ++i) lut[i] += b * b;
        }
        glowPeak = lut[0];
        Image img = GenImageColor(GS, GS, BLANK);
        Color* px = (Color*)img.data;   // GenImageColor leaves in R8G8B8A8
        for (int y = 0; y < GS; ++y) {
            for (int x = 0; x < GS; ++x) {
                float fx = ((float)x + 0.5f - GS * 0.5f) / (GS * 0.5f);
                float fy = ((float)y + 0.5f - GS * 0.5f) / (GS * 0.5f);
                float f  = sqrtf(fx * fx + fy * fy);
                float the  = 0.0f;
                if (f < 1.0f) the = lut[(int)(f * GS)] / glowPeak;
                px[y * GS + x] = { 255, 255, 255, (unsigned char)(the * 255.0f + 0.5f) };
            }
        }
        glowTex = GfxTexture(LoadTextureFromImage(img));
        SetTextureFilter(glowTex.get(), TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(glowTex.get(), TEXTURE_WRAP_CLAMP);
        UnloadImage(img);
    }
}

void LightSystem::shutdown() {
    lightMask.reset();
    glowTex.reset();
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
    // Always slot 0 — large warm bubble only the player always sees nearby
    // scenery (houses, trees, etc.) even in the darkest zones.
    // MINIMUM guaranteed reach/intensity: pitch-black night never hides the
    // hero himself or enemies around him (audit 2, readability).
    LightSource l;
    l.position  = pos;
    l.radius    = 480.0f;
    l.intensity = 0.82f;   // larga and fraca: clareia the entorno without virar holofote
    l.color     = {255, 226, 180, 255};   // tom hot (clima Diablo)
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

    BeginTextureMode(lightMask.get());
    // Fill with ambient darkness
    float amb = 1.0f - ambientDark;
    // AMBIENT floor: the mask is MULTIPLICATIVE — below ~0.33 luminance,
    // scenery and actors turn pure black (audit 2). The dark mood stays in HUE
    // (nighttime ambientColor), not in turning off the scene.
    if (amb < 0.64f) amb = 0.64f;
    ClearBackground({ (unsigned char)(amb*ambientColor.r), (unsigned char)(amb*ambientColor.g), (unsigned char)(amb*ambientColor.b), 255 });

    // Mascara in meia resolution: scale the camera for the alvo menor.
    const float ms = (float)lightMask.get().texture.width / (float)maskW;
    camera.offset.x *= ms; camera.offset.y *= ms; camera.zoom *= ms;

    BeginMode2D(camera);
    BeginBlendMode(BLEND_ADDITIVE);

    for (const auto& l : lights) {
        if (!l.active) continue;

        // UMA quad with the glow radial pre-generated (same profile of the 31 circles).
        unsigned char ta = (unsigned char)fminf(255.0f, l.intensity * l.intensity * glowPeak * 255.0f);
        Color tint = { l.color.r, l.color.g, l.color.b, ta };
        float r = l.radius;
        DrawTexturePro(glowTex.get(),
                       { 0.0f, 0.0f, (float)glowTex.get().width, (float)glowTex.get().height },
                       { l.position.x - r, l.position.y - r, r * 2.0f, r * 2.0f },
                       { 0.0f, 0.0f }, 0.0f, tint);
    }

    EndBlendMode();
    EndMode2D();
    EndTextureMode();
}

void LightSystem::prepareMask3D(const Camera3D& camera3D, int screenW, int screenH) {
    if (!enabled) return;

    BeginTextureMode(lightMask.get());
    // Fill with ambient darkness
    float amb = 1.0f - ambientDark;
    // PISO of environment (same of the prepareMask 2D): night NUNCA apaga the scene —
    // ~0.33 of luminosidade minima to silhueta of scenario and atores lerem.
    if (amb < 0.64f) amb = 0.64f;
    ClearBackground({ (unsigned char)(amb*ambientColor.r), (unsigned char)(amb*ambientColor.g), (unsigned char)(amb*ambientColor.b), 255 });

    BeginBlendMode(BLEND_ADDITIVE);

    // Mascara in meia resolution: projeta in coordenadas of screen cheia and scale.
    const float ms = (float)lightMask.get().texture.width / (float)screenW;

    for (const auto& l : lights) {
        if (!l.active) continue;

        // A light falls in the CHAO, entao the poca and uma ELIPSE in perspectiva - not um
        // circle. Projetar only the radius in X and draw circle era the that fazia cada
        // lamp (and the own hero) virar um "sol" chapado colado in the screen.
        // Projeto duas bordas: +X of the the semieixo horizontal, +Z the vertical.
        Vector3 pos3D   = { l.position.x, 0.0f, l.position.y };
        Vector2 centerS = GetWorldToScreenEx(pos3D, camera3D, screenW, screenH);
        Vector2 edgeX   = GetWorldToScreenEx({ l.position.x + l.radius, 0.0f, l.position.y },
                                             camera3D, screenW, screenH);
        Vector2 edgeZ   = GetWorldToScreenEx({ l.position.x, 0.0f, l.position.y + l.radius },
                                             camera3D, screenW, screenH);
        float cx = centerS.x * ms, cy = centerS.y * ms;
        float rx = Vector2Distance(centerS, edgeX) * ms;
        float ry = Vector2Distance(centerS, edgeZ) * ms;   // achatado pela inclinacao of the camera
        if (rx < 1.0f || ry < 1.0f) continue;

        // UMA quad with the glow radial pre-generated in the lugar of 31 elipses: same
        // profile of glow acumulado (see init), ~31x less draw calls by light.
        // A quad esticada in rx/ry different vira the elipse in perspectiva.
        unsigned char ta = (unsigned char)fminf(255.0f, l.intensity * l.intensity * glowPeak * 255.0f);
        Color tint = { l.color.r, l.color.g, l.color.b, ta };
        DrawTexturePro(glowTex.get(),
                       { 0.0f, 0.0f, (float)glowTex.get().width, (float)glowTex.get().height },
                       { cx - rx, cy - ry, rx * 2.0f, ry * 2.0f },
                       { 0.0f, 0.0f }, 0.0f, tint);
    }

    EndBlendMode();
    EndTextureMode();
}

void LightSystem::applyMask() const {
    if (!enabled) return;

    // Draw the lightMask over the current render target using MULTIPLY
    // Where mask is black → darkens (shadows); where mask is white → unchanged
    // A mascara and rendered in meia resolution and esticada here (bilinear).
    BeginBlendMode(BLEND_MULTIPLIED);
    DrawTexturePro(
        lightMask.get().texture,
        { 0.0f, 0.0f, (float)lightMask.get().texture.width, -(float)lightMask.get().texture.height }, // flip Y
        { 0.0f, 0.0f, (float)maskW,                       (float)maskH },
        { 0.0f, 0.0f }, 0.0f, WHITE
    );
    EndBlendMode();
}
