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
    m_shWorld = GfxShader(LoadShader("resources/shaders/world.vs", "resources/shaders/world.fs"));
    if (!m_shWorld.valid()) {
        TraceLog(LOG_WARNING, "WORLDLIT: shader nao compilou - 3D segue sem luz");
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
    // especular sutil: so o suficiente para metal/lataria nunca ficar emba?ado
    float sk = 0.28f;
    // period = tamanho caracteristico do mundo (open-world: 950 do grid urbano;
    // mapa fixo: os tiles nao tem grid, usa 480). A fbm gera manchas coerentes
    // e SEM repeticao visivel a cada chunk.
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
        TraceLog(LOG_WARNING, "POSTFX: resources/shaders ausente - seguindo sem bloom");
        return;
    }
    m_shBright = GfxShader(LoadShader(0, "resources/shaders/bloom_bright.fs"));
    m_shBlur   = GfxShader(LoadShader(0, "resources/shaders/blur.fs"));
    m_shGrade  = GfxShader(LoadShader(0, "resources/shaders/grade.fs"));
    if (!m_shBright.valid() || !m_shBlur.valid() || !m_shGrade.valid()) {
        TraceLog(LOG_WARNING, "POSTFX: shader nao compilou - seguindo sem bloom");
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

    // 1/4 de resolucao: o borrao e largo de proposito, resolucao cheia so custaria
    // fillrate. BILINEAR e o que faz o halo subir de escala liso, sem serrilha.
    m_bloomA = GfxRenderTexture(LoadRenderTexture(screenWidth / 4, screenHeight / 4));
    m_bloomB = GfxRenderTexture(LoadRenderTexture(screenWidth / 4, screenHeight / 4));
    SetTextureFilter(m_bloomA.get().texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(m_bloomB.get().texture, TEXTURE_FILTER_BILINEAR);

    float thr = 0.62f, knee = 0.30f;
    SetShaderValue(m_shBright.get(), m_locThreshold, &thr,  SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shBright.get(), m_locKnee,      &knee, SHADER_UNIFORM_FLOAT);
    // Com tonemap no fim da cadeia a cena NAO precisa mais ser desenhada clara:
    // exposicao perto de 1.0 + contraste alto = pretos com pe e ilhas de luz
    // (o visual do genero), em vez do cinza chapado de antes.
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
        // 1) BRILHO: extrai so os pixels acima do threshold, ja em 1/4 de res.
        Rectangle bDst = { 0.f, 0.f, (float)m_bloomA.get().texture.width,
                                     (float)m_bloomA.get().texture.height };
        BeginTextureMode(m_bloomA.get());
            ClearBackground(BLACK);
            BeginShaderMode(m_shBright.get());
                DrawTexturePro(gameTarget.get().texture, srcFull, bDst, {0,0}, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 2) BORRAO em duas passadas (separavel): horizontal A->B, vertical B->A.
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

        // 3) COMPOSICAO: cena + halo, tonemap filmico, contraste e saturacao.
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

