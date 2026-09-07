#pragma once
#include <raylib.h>
#include <cmath>

// Desenha circulo com glow neon (fake bloom)
inline void DrawGlowCircle(Vector2 pos, float radius, Color col, float glowScale = 3.0f) {
    DrawCircleV(pos, radius * glowScale,        ColorAlpha(col, 0.04f));
    DrawCircleV(pos, radius * glowScale * 0.7f, ColorAlpha(col, 0.08f));
    DrawCircleV(pos, radius * glowScale * 0.5f, ColorAlpha(col, 0.18f));
    DrawCircleV(pos, radius,                    col);
    DrawCircleV(pos, radius * 0.45f,            ColorAlpha(WHITE, 0.85f));
}

// Linha com glow neon
inline void DrawGlowLine(Vector2 a, Vector2 b, float thick, Color col) {
    DrawLineEx(a, b, thick * 5.0f, ColorAlpha(col, 0.06f));
    DrawLineEx(a, b, thick * 3.0f, ColorAlpha(col, 0.15f));
    DrawLineEx(a, b, thick * 1.5f, ColorAlpha(col, 0.4f));
    DrawLineEx(a, b, thick,        col);
}

// Retangulo com borda neon
inline void DrawNeonRect(Rectangle rect, Color fill, Color glow, float borderThick = 2.0f) {
    DrawRectangleRec(rect, fill);
    // Glow externo
    Rectangle outer = {rect.x - 2, rect.y - 2, rect.width + 4, rect.height + 4};
    DrawRectangleLinesEx(outer, borderThick + 2, ColorAlpha(glow, 0.15f));
    DrawRectangleLinesEx(rect, borderThick, glow);
}

// Overlay de scanlines CRT (chamar apos EndMode2D)
// Cached 1xH texture stretched to the screen: a single draw call.
inline void DrawScanlines(int screenW, int screenH) {
    static Texture2D scanTex = {0};
    static int cachedH = 0;
    if (scanTex.id == 0 || cachedH != screenH) {
        if (scanTex.id != 0) UnloadTexture(scanTex);
        Image img = GenImageColor(1, screenH, ColorAlpha(BLACK, 0.06f));
        Color* pixels = LoadImageColors(img);
        if (pixels) {
            for (int y = 0; y < screenH; y += 6) {
                pixels[y] = ColorAlpha(BLACK, 0.14f);
            }
            UnloadImage(img);
            img = Image{pixels, 1, screenH, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
            scanTex = LoadTextureFromImage(img);
            UnloadImageColors(pixels);
        } else {
            UnloadImage(img);
        }
        cachedH = screenH;
    }
    if (scanTex.id != 0) {
        DrawTexturePro(scanTex,
                       Rectangle{0.0f, 0.0f, 1.0f, static_cast<float>(screenH)},
                       Rectangle{0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH)},
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    } else {
        // Fallback caso a textura falhe.
        DrawRectangle(0, 0, screenW, screenH, ColorAlpha(BLACK, 0.06f));
    }
}

// Vignette (bordas escuras) — sutil, sem faixas grossas cobrindo a visao do jogador
inline void DrawVignette(int screenW, int screenH) {
    // Enquadra sem apagar: e o TERCEIRO multiplicador escuro sobre a cena
    // (depois do fog do chao e da mascara de luz). Faixas mais largas e MUITO
    // mais suaves leem como enquadramento; as antigas leem como perda de imagem.
    int vTop = 96;
    int vSide = 150;
    DrawRectangleGradientV(0, 0, screenW, vTop, ColorAlpha(BLACK, 0.20f), BLANK);
    DrawRectangleGradientV(0, screenH - vTop, screenW, vTop, BLANK, ColorAlpha(BLACK, 0.20f));
    DrawRectangleGradientH(0, 0, vSide, screenH, ColorAlpha(BLACK, 0.18f), BLANK);
    DrawRectangleGradientH(screenW - vSide, 0, vSide, screenH, BLANK, ColorAlpha(BLACK, 0.18f));
}

// Barra de vida estilizada
inline void DrawHealthBar(Vector2 pos, float pct, float w, float h, Color col) {
    Rectangle bg = {pos.x - w/2, pos.y, w, h};
    Rectangle fg = {pos.x - w/2, pos.y, w * pct, h};
    DrawRectangleRec(bg, ColorAlpha(BLACK, 0.7f));
    DrawRectangleRec(fg, col);
    DrawRectangleLinesEx(bg, 1.0f, ColorAlpha(col, 0.5f));
}
