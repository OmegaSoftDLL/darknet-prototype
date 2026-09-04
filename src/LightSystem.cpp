#include "LightSystem.h"
#include <cmath>
#include <algorithm>
#include <raymath.h>

// ─── Init / Shutdown ─────────────────────────────────────────────────────────

void LightSystem::init(int w, int h) {
    maskW = w; maskH = h;
    // Mascara em MEIA resolucao: luz suave borrada por natureza, entao o upscale
    // bilinear e imperceptivel — e o custo de fill da mascara cai ~4x.
    lightMask = LoadRenderTexture(w / 2, h / 2);
    SetTextureFilter(lightMask.texture, TEXTURE_FILTER_BILINEAR);

    // GLOW radial pre-gerado: substitui os 31 aneis concentricos por luz por UMA
    // quad texturizada. O perfil de alpha reproduz EXATAMENTE a soma dos aneis
    // antigos: no blend aditivo (src*srcA) cada anel contribuia cor*bright^2,
    // entao P(f) = soma dos (0.30*t^3)^2 dos aneis que cobrem a fracao f do raio.
    {
        const int GS = 256;
        const int steps = 30;
        float lut[GS + 1] = {};
        for (int s = 0; s <= steps; ++s) {
            float t = (float)s / (float)steps;
            float k = (float)(steps - s + 1) / (float)(steps + 1); // fracao do raio do anel
            float b = 0.30f * t * t * t;                           // bright do anel (intensity=1)
            int lim = (int)(k * GS + 0.5f);
            if (lim > GS) lim = GS;
            for (int i = 0; i <= lim; ++i) lut[i] += b * b;
        }
        glowPeak = lut[0];
        Image img = GenImageColor(GS, GS, BLANK);
        Color* px = (Color*)img.data;   // GenImageColor sai em R8G8B8A8
        for (int y = 0; y < GS; ++y) {
            for (int x = 0; x < GS; ++x) {
                float fx = ((float)x + 0.5f - GS * 0.5f) / (GS * 0.5f);
                float fy = ((float)y + 0.5f - GS * 0.5f) / (GS * 0.5f);
                float f  = sqrtf(fx * fx + fy * fy);
                float a  = 0.0f;
                if (f < 1.0f) a = lut[(int)(f * GS)] / glowPeak;
                px[y * GS + x] = { 255, 255, 255, (unsigned char)(a * 255.0f + 0.5f) };
            }
        }
        glowTex = LoadTextureFromImage(img);
        SetTextureFilter(glowTex, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(glowTex, TEXTURE_WRAP_CLAMP);
        UnloadImage(img);
    }
}

void LightSystem::shutdown() {
    if (maskW > 0) UnloadRenderTexture(lightMask);
    if (glowTex.id > 0) UnloadTexture(glowTex);
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
    // Alcance/intensidade MINIMOS garantidos: a noite fechada nunca esconde o
    // proprio heroi nem os inimigos em volta dele (auditoria 2, legibilidade).
    LightSource l;
    l.position  = pos;
    l.radius    = 480.0f;
    l.intensity = 0.82f;   // larga e fraca: clareia o entorno sem virar holofote
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
    // PISO de ambiente: a mascara e MULTIPLICATIVA — abaixo de ~0.33 de
    // luminosidade cenario e atores viram preto puro (auditoria 2). O clima
    // sombrio fica no MATIZ (ambientColor noturno), nao em apagar a cena.
    if (amb < 0.64f) amb = 0.64f;
    ClearBackground({ (unsigned char)(amb*ambientColor.r), (unsigned char)(amb*ambientColor.g), (unsigned char)(amb*ambientColor.b), 255 });

    // Mascara em meia resolucao: escala a camera para o alvo menor.
    const float ms = (float)lightMask.texture.width / (float)maskW;
    camera.offset.x *= ms; camera.offset.y *= ms; camera.zoom *= ms;

    BeginMode2D(camera);
    BeginBlendMode(BLEND_ADDITIVE);

    for (const auto& l : lights) {
        if (!l.active) continue;

        // UMA quad com o glow radial pre-gerado (mesmo perfil dos 31 circulos).
        unsigned char ta = (unsigned char)fminf(255.0f, l.intensity * l.intensity * glowPeak * 255.0f);
        Color tint = { l.color.r, l.color.g, l.color.b, ta };
        float r = l.radius;
        DrawTexturePro(glowTex,
                       { 0.0f, 0.0f, (float)glowTex.width, (float)glowTex.height },
                       { l.position.x - r, l.position.y - r, r * 2.0f, r * 2.0f },
                       { 0.0f, 0.0f }, 0.0f, tint);
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
    // PISO de ambiente (mesmo do prepareMask 2D): noite NUNCA apaga a cena —
    // ~0.33 de luminosidade minima pra silhueta de cenario e atores lerem.
    if (amb < 0.64f) amb = 0.64f;
    ClearBackground({ (unsigned char)(amb*ambientColor.r), (unsigned char)(amb*ambientColor.g), (unsigned char)(amb*ambientColor.b), 255 });

    BeginBlendMode(BLEND_ADDITIVE);

    // Mascara em meia resolucao: projeta em coordenadas de tela cheia e escala.
    const float ms = (float)lightMask.texture.width / (float)screenW;

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
        float cx = centerS.x * ms, cy = centerS.y * ms;
        float rx = Vector2Distance(centerS, edgeX) * ms;
        float ry = Vector2Distance(centerS, edgeZ) * ms;   // achatado pela inclinacao da camera
        if (rx < 1.0f || ry < 1.0f) continue;

        // UMA quad com o glow radial pre-gerado no lugar de 31 elipses: mesmo
        // perfil de brilho acumulado (ver init), ~31x menos draw calls por luz.
        // A quad esticada em rx/ry diferentes vira a elipse em perspectiva.
        unsigned char ta = (unsigned char)fminf(255.0f, l.intensity * l.intensity * glowPeak * 255.0f);
        Color tint = { l.color.r, l.color.g, l.color.b, ta };
        DrawTexturePro(glowTex,
                       { 0.0f, 0.0f, (float)glowTex.width, (float)glowTex.height },
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
    // A mascara e renderizada em meia resolucao e esticada aqui (bilinear).
    BeginBlendMode(BLEND_MULTIPLIED);
    DrawTexturePro(
        lightMask.texture,
        { 0.0f, 0.0f, (float)lightMask.texture.width, -(float)lightMask.texture.height }, // flip Y
        { 0.0f, 0.0f, (float)maskW,                       (float)maskH },
        { 0.0f, 0.0f }, 0.0f, WHITE
    );
    EndBlendMode();
}
