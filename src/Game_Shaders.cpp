// Game_Shaders.cpp — shader de mundo (luz/fog), pos-processamento (bloom/tonemap),
// apresentacao do frame e virtualizacao do mouse. Extraido de Game.cpp. Mesma classe Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <string>
#include <thread>

// ── RenderTexture helpers ─────────────────────────────────────────────────────

// --- Pos-processamento ------------------------------------------------------
// Sem shader o jogo era 100% funcao fixa: cada pixel saia exatamente com a cor
// desenhada, sem faixa dinamica. Dai a sensacao de "chapado e escuro" - luz forte
// nao estourava e sombra nao tinha pe. Este passe da halo nas fontes de luz
// (bloom) e curva filmica na imagem inteira (tonemap).
void Game::initWorldShader() {
    m_worldLit = false;
    if (!FileExists("resources/shaders/world.fs")) return;
    m_shWorld = LoadShader("resources/shaders/world.vs", "resources/shaders/world.fs");
    if (!IsShaderValid(m_shWorld)) {
        TraceLog(LOG_WARNING, "WORLDLIT: shader nao compilou - 3D segue sem luz");
        return;
    }
    m_locLightDir  = GetShaderLocation(m_shWorld, "lightDir");
    m_locLightCol  = GetShaderLocation(m_shWorld, "lightColor");
    m_locAmbCol    = GetShaderLocation(m_shWorld, "ambientColor");
    m_locCamPos    = GetShaderLocation(m_shWorld, "camPos");
    m_locFogCol    = GetShaderLocation(m_shWorld, "fogColor");
    m_locFogStart  = GetShaderLocation(m_shWorld, "fogStart");
    m_locFogEnd    = GetShaderLocation(m_shWorld, "fogEnd");
    m_locRim       = GetShaderLocation(m_shWorld, "rimStrength");
    m_worldLit = true;
    TraceLog(LOG_INFO, "WORLDLIT: iluminacao 3D ATIVA");
}

void Game::applyWorldShader(Model& m) const {
    if (!m_worldLit || m.materialCount <= 0) return;
    for (int i = 0; i < m.materialCount; ++i) m.materials[i].shader = m_shWorld;
}

void Game::updateWorldShaderUniforms() {
    if (!m_worldLit) return;
    // O sol gira com o ciclo do dia: sombra e luz mudam de lado ao longo da partida.
    float ang = worldClock * 6.2831853f;
    Vector3 ld = { -0.42f * cosf(ang) - 0.30f, -1.0f, -0.30f * sinf(ang) - 0.22f };
    float lm = sqrtf(ld.x*ld.x + ld.y*ld.y + ld.z*ld.z);
    ld = { ld.x/lm, ld.y/lm, ld.z/lm };

    Color a = lightSystem.ambientColor;
    float amb = 0.34f + (1.0f - lightSystem.ambientDark) * 0.30f;
    Vector3 ambV = { a.r/255.0f*amb, a.g/255.0f*amb, a.b/255.0f*amb };
    // luz direta puxa o matiz do ambiente, mas mais quente e mais forte de dia
    float sunK = 0.42f + worldSun * 0.40f;
    Vector3 lcV = { (a.r/255.0f*0.55f + 0.45f) * sunK,
                    (a.g/255.0f*0.62f + 0.38f) * sunK,
                    (a.b/255.0f*0.70f + 0.30f) * sunK };
    Vector3 cam = camera3D.position;
    // Fog morre na cor do CEU da fase (skyColorFor, mesma conta do ClearBackground):
    // geometria distante some exatamente no horizonte — e cada fase ganha um
    // horizonte com matiz proprio (vermelho no inferno, azul-noite na fantasma).
    Color skc = skyColorFor(currentZone);
    float sdk = 0.30f + (1.0f - lightSystem.ambientDark) * 0.50f;
    Vector3 fog = { skc.r/255.0f * sdk, skc.g/255.0f * sdk, skc.b/255.0f * sdk };
    float fs = 900.0f, fe = 2600.0f, rim = 0.30f;
    SetShaderValue(m_shWorld, m_locLightDir, &ld,   SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld, m_locLightCol, &lcV,  SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld, m_locAmbCol,   &ambV, SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld, m_locCamPos,   &cam,  SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld, m_locFogCol,   &fog,  SHADER_UNIFORM_VEC3);
    SetShaderValue(m_shWorld, m_locFogStart, &fs,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shWorld, m_locFogEnd,   &fe,   SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shWorld, m_locRim,      &rim,  SHADER_UNIFORM_FLOAT);
}

void Game::initPostFX() {
    m_postFX = false;
    if (!FileExists("resources/shaders/grade.fs")) {
        TraceLog(LOG_WARNING, "POSTFX: resources/shaders ausente - seguindo sem bloom");
        return;
    }
    m_shBright = LoadShader(0, "resources/shaders/bloom_bright.fs");
    m_shBlur   = LoadShader(0, "resources/shaders/blur.fs");
    m_shGrade  = LoadShader(0, "resources/shaders/grade.fs");
    if (!IsShaderValid(m_shBright) || !IsShaderValid(m_shBlur) || !IsShaderValid(m_shGrade)) {
        TraceLog(LOG_WARNING, "POSTFX: shader nao compilou - seguindo sem bloom");
        unloadPostFX();
        return;
    }
    m_locThreshold  = GetShaderLocation(m_shBright, "threshold");
    m_locKnee       = GetShaderLocation(m_shBright, "knee");
    m_locBlurDir    = GetShaderLocation(m_shBlur,   "direction");
    m_locBloomTex   = GetShaderLocation(m_shGrade,  "texture1");
    m_locBloomStr   = GetShaderLocation(m_shGrade,  "bloomStrength");
    m_locExposure   = GetShaderLocation(m_shGrade,  "exposure");
    m_locSaturation = GetShaderLocation(m_shGrade,  "saturation");
    m_locContrast   = GetShaderLocation(m_shGrade,  "contrast");

    // 1/4 de resolucao: o borrao e largo de proposito, resolucao cheia so custaria
    // fillrate. BILINEAR e o que faz o halo subir de escala liso, sem serrilha.
    m_bloomA = LoadRenderTexture(screenWidth / 4, screenHeight / 4);
    m_bloomB = LoadRenderTexture(screenWidth / 4, screenHeight / 4);
    SetTextureFilter(m_bloomA.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_bloomB.texture, TEXTURE_FILTER_BILINEAR);

    float thr = 0.62f, knee = 0.30f;
    SetShaderValue(m_shBright, m_locThreshold, &thr,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shBright, m_locKnee,      &knee, SHADER_UNIFORM_FLOAT);
    // Com tonemap no fim da cadeia a cena NAO precisa mais ser desenhada clara:
    // exposicao perto de 1.0 + contraste alto = pretos com pe e ilhas de luz
    // (o visual do genero), em vez do cinza chapado de antes.
    float bs = 0.95f, ex = 1.06f, sat = 1.22f, con = 1.16f;
    SetShaderValue(m_shGrade, m_locBloomStr,   &bs,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shGrade, m_locExposure,   &ex,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shGrade, m_locSaturation, &sat, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shGrade, m_locContrast,   &con, SHADER_UNIFORM_FLOAT);
    m_postFX = true;
    TraceLog(LOG_INFO, "POSTFX: bloom + tonemap ATIVO");
}

void Game::unloadPostFX() {
    if (IsShaderValid(m_shBright)) UnloadShader(m_shBright);
    if (IsShaderValid(m_shBlur))   UnloadShader(m_shBlur);
    if (IsShaderValid(m_shGrade))  UnloadShader(m_shGrade);
    m_shBright = {}; m_shBlur = {}; m_shGrade = {};
    if (m_bloomA.id > 0) UnloadRenderTexture(m_bloomA);
    if (m_bloomB.id > 0) UnloadRenderTexture(m_bloomB);
    m_bloomA = {}; m_bloomB = {};
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
        // 1) BRILHO: extrai so os pixels acima do threshold, ja em 1/4 de res.
        Rectangle bDst = { 0.f, 0.f, (float)m_bloomA.texture.width,
                                     (float)m_bloomA.texture.height };
        BeginTextureMode(m_bloomA);
            ClearBackground(BLACK);
            BeginShaderMode(m_shBright);
                DrawTexturePro(gameTarget.texture, srcFull, bDst, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 2) BORRAO em duas passadas (separavel): horizontal A->B, vertical B->A.
        Rectangle bSrc = { 0.f, 0.f, (float)m_bloomA.texture.width,
                                    -(float)m_bloomA.texture.height };
        Vector2 dirH = { 1.0f / (float)m_bloomA.texture.width, 0.0f };
        Vector2 dirV = { 0.0f, 1.0f / (float)m_bloomA.texture.height };
        BeginTextureMode(m_bloomB);
            ClearBackground(BLACK);
            SetShaderValue(m_shBlur, m_locBlurDir, &dirH, SHADER_UNIFORM_VEC2);
            BeginShaderMode(m_shBlur);
                DrawTexturePro(m_bloomA.texture, bSrc, bDst, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();
        BeginTextureMode(m_bloomA);
            ClearBackground(BLACK);
            SetShaderValue(m_shBlur, m_locBlurDir, &dirV, SHADER_UNIFORM_VEC2);
            BeginShaderMode(m_shBlur);
                DrawTexturePro(m_bloomB.texture, bSrc, bDst, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 3) COMPOSICAO: cena + halo, tonemap filmico, contraste e saturacao.
        BeginDrawing();
            ClearBackground(BLACK);
            SetShaderValueTexture(m_shGrade, m_locBloomTex, m_bloomA.texture);
            BeginShaderMode(m_shGrade);
                DrawTexturePro(gameTarget.texture, srcFull, dstFull, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndDrawing();
    } else {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(gameTarget.texture, srcFull, dstFull, {0, 0}, 0.0f, WHITE);
        EndDrawing();
    }
    // TEMP-SHOT: no autotest, salva um frame a cada 30s p/ inspecao visual.
    // 10 shots x 30s cobrem ~300s de run = fases 1-4 (antes 10s = so a fase 1).
    // TEM que ser DEPOIS de EndDrawing — antes, o framebuffer ainda esta preto.
    if (botController.autoTest) {
        // lastShot/shotN sao MEMBROS (eram static de funcao — nao resetavam
        // entre partidas e as screenshots paravam de sair na segunda run).
        double now = GetTime();
        if (now - lastShot > 30.0 && now > 8.0 && shotN < 10) {
            lastShot = now;
            // TakeScreenshot SINCRONO derrubava 1 frame p/ ~6 FPS a cada 10s:
            // o readback da tela e barato, mas o encode PNG (stb) de um frame
            // inteiro custa 110-180ms na main thread. Agora so o readback fica
            // aqui (contexto GL); o encode+gravacao vao pra uma thread auxiliar.
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

