// Game_Shaders.cpp — shader of world (light/fog), pos-processamento (bloom/tonemap),
// apresentacao of the frame and virtualizacao of the mouse. Extraido of Game.cpp. Same class Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <string>
#include <thread>

// ── RenderTexture helpers ─────────────────────────────────────────────────────

// --- Pos-processamento ------------------------------------------------------
// Sem shader the game era 100% function fixa: cada pixel saia exatamente with the color
// drawn, without range dinamica. Dai the sensation of "chapado and dark" - light strong
// not estourava and shadow not had foot. Este passe of the halo in the fontes of light
// (bloom) and curva filmica in the image whole (tonemap).
void Game::initWorldShader() {
    m_worldLit = false;
    if (!FileExists("resources/shaders/world.fs")) return;
    m_shWorld = GfxShader(LoadShader("resources/shaders/world.vs", "resources/shaders/world.fs"));
    if (!m_shWorld.valid()) {
        TraceLog(LOG_WARNING, "WORLDLIT: shader not compilou - 3D segue without light");
        return;
    }
    m_locLightDir  = GetShaderLocation(m_shWorld.get(), "lightDir");
    m_locLightCol  = GetShaderLocation(m_shWorld.get(), "lightColor");
    m_locAmbCol    = GetShaderLocation(m_shWorld.get(), "ambientColor");
    m_locCamPos    = GetShaderLocation(m_shWorld.get(), "camPos");
    m_locFogCol    = GetShaderLocation(m_shWorld.get(), "fogColor");
    m_locFogStart  = GetShaderLocation(m_shWorld.get(), "fogStart");
    m_locFogEnd    = GetShaderLocation(m_shWorld.get(), "fogEnd");
    m_locRim       = GetShaderLocation(m_shWorld.get(), "rimStrength");
    m_locSpecK     = GetShaderLocation(m_shWorld.get(), "specularK");
    m_locWorldPer  = GetShaderLocation(m_shWorld.get(), "worldPeriod");
    m_worldLit = true;
    TraceLog(LOG_INFO, "WORLDLIT: iluminacao 3D ATIVA");
}

void Game::applyWorldShader(Model& m) const {
    if (!m_worldLit || m.materialCount <= 0) return;
    for (int i = 0; i < m.materialCount; ++i) m.materials[i].shader = m_shWorld.get();
}

void Game::applyWorldShader(GfxModel& m) const {
    if (!m.valid()) return;
    applyWorldShader(m.get());
}

void Game::updateWorldShaderUniforms() {
    if (!m_worldLit) return;
    // O sol gira with the ciclo of the day: shadow and light mudam of lado along of the match.
    float ang = worldClock * 6.2831853f;
    Vector3 ld = { -0.42f * cosf(ang) - 0.30f, -1.0f, -0.30f * sinf(ang) - 0.22f };
    float lm = sqrtf(ld.x*ld.x + ld.y*ld.y + ld.z*ld.z);
    ld = { ld.x/lm, ld.y/lm, ld.z/lm };

    Color the = lightSystem.ambientColor;
    float amb = 0.34f + (1.0f - lightSystem.ambientDark) * 0.30f;
    Vector3 ambV = { the.r/255.0f*amb, the.g/255.0f*amb, the.b/255.0f*amb };
    // light direta puxa the matiz of the environment, mas more hot and more strong of day
    float sunK = 0.42f + worldSun * 0.40f;
    Vector3 lcV = { (the.r/255.0f*0.55f + 0.45f) * sunK,
                    (the.g/255.0f*0.62f + 0.38f) * sunK,
                    (the.b/255.0f*0.70f + 0.30f) * sunK };
    Vector3 cam = camera3D.position;
    // Fog morre in the color of the CEU of the phase (skyColorFor, same account of the ClearBackground):
    // geometria distante some exatamente in the horizonte — and cada phase ganha um
    // horizonte with matiz own (red in the inferno, blue-night in the ghost).
    Color skc = skyColorFor(currentZone);
    float sdk = 0.30f + (1.0f - lightSystem.ambientDark) * 0.50f;
    Vector3 fog = { skc.r/255.0f * sdk, skc.g/255.0f * sdk, skc.b/255.0f * sdk };
    float fs = 900.0f, fe = 2600.0f, rim = 0.30f;
    // especular sutil: only the suficiente to metal/lataria never stay emba?ado
    float sk = 0.28f;
    // period = size caracteristico of the world (open-world: 950 of the grid urbano;
    // map fixed: the tiles not has grid, usa 480). A fbm generates manchas coerentes
    // and SEM repeticao visible the cada chunk.
    float per = openWorldMode && currentZone == ZoneID::LARuins ? 950.0f : 480.0f;
    SetShaderValue(m_shWorld.get(), m_locLightDir, &ld,   SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld.get(), m_locLightCol, &lcV,  SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld.get(), m_locAmbCol,   &ambV, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld.get(), m_locCamPos,   &cam,  SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld.get(), m_locFogCol,   &fog,  SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld.get(), m_locFogStart, &fs,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shWorld.get(), m_locFogEnd,   &fe,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shWorld.get(), m_locRim,      &rim,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shWorld.get(), m_locSpecK,    &sk,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shWorld.get(), m_locWorldPer, &per,  SHADER_UNIFORM_FLOAT);
}

void Game::initPostFX() {
    m_postFX = false;
    if (!FileExists("resources/shaders/grade.fs")) {
        TraceLog(LOG_WARNING, "POSTFX: resources/shaders ausente - seguindo without bloom");
        return;
    }
    m_shBright = GfxShader(LoadShader(0, "resources/shaders/bloom_bright.fs"));
    m_shBlur   = GfxShader(LoadShader(0, "resources/shaders/blur.fs"));
    m_shGrade  = GfxShader(LoadShader(0, "resources/shaders/grade.fs"));
    if (!m_shBright.valid() || !m_shBlur.valid() || !m_shGrade.valid()) {
        TraceLog(LOG_WARNING, "POSTFX: shader not compilou - seguindo without bloom");
        unloadPostFX();
        return;
    }
    m_locThreshold  = GetShaderLocation(m_shBright.get(), "threshold");
    m_locKnee       = GetShaderLocation(m_shBright.get(), "knee");
    m_locBlurDir    = GetShaderLocation(m_shBlur.get(),   "direction");
    m_locBloomTex   = GetShaderLocation(m_shGrade.get(),  "texture1");
    m_locBloomStr   = GetShaderLocation(m_shGrade.get(),  "bloomStrength");
    m_locExposure   = GetShaderLocation(m_shGrade.get(),  "exposure");
    m_locSaturation = GetShaderLocation(m_shGrade.get(),  "saturation");
    m_locContrast   = GetShaderLocation(m_shGrade.get(),  "contrast");

    // 1/4 of resolution: the borrao and wide of purpose, resolution cheia only custaria
    // fillrate. BILINEAR and the that does the halo go up of scale smooth, without serrilha.
    m_bloomA = GfxRenderTexture(LoadRenderTexture(screenWidth / 4, screenHeight / 4));
    m_bloomB = GfxRenderTexture(LoadRenderTexture(screenWidth / 4, screenHeight / 4));
    SetTextureFilter(m_bloomA.get().texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_bloomB.get().texture, TEXTURE_FILTER_BILINEAR);

    float thr = 0.62f, knee = 0.30f;
    SetShaderValue(m_shBright.get(), m_locThreshold, &thr,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shBright.get(), m_locKnee,      &knee, SHADER_UNIFORM_FLOAT);
    // Com tonemap in the end of the cadeia the scene NOT precisa more be drawn clear:
    // exposure near of 1.0 + contrast high = pretos with foot and ilhas of light
    // (the visual of the genero), instead of the gray chapado of before.
    float bs = 1.15f, ex = 1.06f, sat = 1.28f, con = 1.16f;
    SetShaderValue(m_shGrade.get(), m_locBloomStr,   &bs,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shGrade.get(), m_locExposure,   &ex,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shGrade.get(), m_locSaturation, &sat, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shGrade.get(), m_locContrast,   &con, SHADER_UNIFORM_FLOAT);
    m_postFX = true;
    TraceLog(LOG_INFO, "POSTFX: bloom + tonemap ATIVO");
}

void Game::unloadPostFX() {
    m_shBright.reset();
    m_shBlur.reset();
    m_shGrade.reset();
    m_bloomA.reset();
    m_bloomB.reset();
    m_postFX = false;
}

void Game::presentFrame() const {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float scaleX = (float)sw / screenWidth;
    float scaleY = (float)sh / screenHeight;
    float scale  = scaleX < scaleY ? scaleX : scaleY;
    float drawW  = screenWidth  * scale;
    float drawH  = screenHeight * scale;
    float drawX  = (sw - drawW) * 0.5f;
    float drawY  = (sh - drawH) * 0.5f;
    Rectangle srcFull = { 0.f, 0.f, (float)screenWidth, -(float)screenHeight };
    Rectangle dstFull = { drawX, drawY, drawW, drawH };

    if (m_postFX) {
        // 1) BRILHO: extrai only the pixels above the threshold, already in 1/4 of res.
        Rectangle bDst = { 0.f, 0.f, (float)m_bloomA.get().texture.width,
                                     (float)m_bloomA.get().texture.height };
        BeginTextureMode(m_bloomA.get());
            ClearBackground(BLACK);
            BeginShaderMode(m_shBright.get());
                DrawTexturePro(gameTarget.get().texture, srcFull, bDst, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 2) BORRAO in duas passadas (separavel): horizontal A->B, vertical B->A.
        Rectangle bSrc = { 0.f, 0.f, (float)m_bloomA.get().texture.width,
                                    -(float)m_bloomA.get().texture.height };
        Vector2 dirH = { 1.0f / (float)m_bloomA.get().texture.width, 0.0f };
        Vector2 dirV = { 0.0f, 1.0f / (float)m_bloomA.get().texture.height };
        BeginTextureMode(m_bloomB.get());
            ClearBackground(BLACK);
            SetShaderValue(m_shBlur.get(), m_locBlurDir, &dirH, SHADER_UNIFORM_VEC2);
            BeginShaderMode(m_shBlur.get());
                DrawTexturePro(m_bloomA.get().texture, bSrc, bDst, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();
        BeginTextureMode(m_bloomA.get());
            ClearBackground(BLACK);
            SetShaderValue(m_shBlur.get(), m_locBlurDir, &dirV, SHADER_UNIFORM_VEC2);
            BeginShaderMode(m_shBlur.get());
                DrawTexturePro(m_bloomB.get().texture, bSrc, bDst, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 3) COMPOSICAO: scene + halo, tonemap filmico, contrast and saturation.
        BeginDrawing();
            ClearBackground(BLACK);
            SetShaderValueTexture(m_shGrade.get(), m_locBloomTex, m_bloomA.get().texture);
            BeginShaderMode(m_shGrade.get());
                DrawTexturePro(gameTarget.get().texture, srcFull, dstFull, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndDrawing();
    } else {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(gameTarget.get().texture, srcFull, dstFull, {0, 0}, 0.0f, WHITE);
        EndDrawing();
    }
    // TEMP-SHOT: in the autotest, saves um frame the cada 30s p/ inspecao visual.
    // 10 shots x 30s cobrem ~300s of run = phases 1-4 (before 10s = only the phase 1).
    // TEM that be DEPOIS of EndDrawing — before, the framebuffer still is black.
    if (botController.autoTest) {
        // lastShot/shotN sao MEMBROS (eram static of function — not resetavam
        // between partidas and the screenshots paravam of leave in the second run).
        double now = GetTime();
        if (now - lastShot > 30.0 && now > 8.0 && shotN < 10) {
            lastShot = now;
            // TakeScreenshot SINCRONO derrubava 1 frame p/ ~6 FPS the cada 10s:
            // the readback of the screen and barato, mas the encode PNG (stb) of um frame
            // integer custa 110-180ms in the main thread. Agora only the readback stays
            // here (contexto GL); the encode+gravacao vao to uma thread auxiliary.
            std::string fn = TextFormat("shot_%02d.png", shotN++);
            Image img = LoadImageFromScreen();
            std::thread([img, fn]() mutable {
                ExportImage(img, fn.c_str());
                UnloadImage(img);
            }).detach();
        }
    }
}

Vector2 Game::virtualizeMousePos(Vector2 m) const {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float scaleX = (float)sw / screenWidth;
    float scaleY = (float)sh / screenHeight;
    float scale  = scaleX < scaleY ? scaleX : scaleY;
    float drawX  = (sw - screenWidth  * scale) * 0.5f;
    float drawY  = (sh - screenHeight * scale) * 0.5f;
    return {(m.x - drawX) / scale, (m.y - drawY) / scale};
}

