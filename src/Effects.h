#pragma once
#include <raylib.h>
#include <cmath>

// Draws circle with glow neon (fake bloom)
inline void DrawGlowCircle(Vector2 pos, float radius, Color col, float glowScale = 3.0f) {
    DrawCircleV(pos, radius * glowScale,        ColorAlpha(col, 0.04f));
    DrawCircleV(pos, radius * glowScale * 0.7f, ColorAlpha(col, 0.08f));
    DrawCircleV(pos, radius * glowScale * 0.5f, ColorAlpha(col, 0.18f));
    DrawCircleV(pos, radius,                    col);
    DrawCircleV(pos, radius * 0.45f,            ColorAlpha(WHITE, 0.85f));
}

// Line with glow neon
inline void DrawGlowLine(Vector2 the, Vector2 b, float thick, Color col) {
    DrawLineEx(the, b, thick * 5.0f, ColorAlpha(col, 0.06f));
    DrawLineEx(the, b, thick * 3.0f, ColorAlpha(col, 0.15f));
    DrawLineEx(the, b, thick * 1.5f, ColorAlpha(col, 0.4f));
    DrawLineEx(the, b, thick,        col);
}

// Rectangle with edge neon
inline void DrawNeonRect(Rectangle rect, Color fill, Color glow, float borderThick = 2.0f) {
    DrawRectangleRec(rect, fill);
    // Glow external
    Rectangle outer = {rect.x - 2, rect.y - 2, rect.width + 4, rect.height + 4};
    DrawRectangleLinesEx(outer, borderThick + 2, ColorAlpha(glow, 0.15f));
    DrawRectangleLinesEx(rect, borderThick, glow);
}

// Overlay of scanlines CRT (call apos EndMode2D)
// Cached 1xH texture stretched to the screen: the single draw call.
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
        // Fallback if the texture falhe.
        DrawRectangle(0, 0, screenW, screenH, ColorAlpha(BLACK, 0.06f));
    }
}

// Vignette (bordas escuras) — sutil, without faixas grossas cobrindo the visao of the player
inline void DrawVignette(int screenW, int screenH) {
    // Enquadra without apagar: and the TERCEIRO multiplicador dark about the scene
    // (after the floor fog and of the mascara of light). Faixas more largas and MUITO
    // more suaves leem as enquadramento; the antigas leem as perda of image.
    int vTop = 96;
    int vSide = 150;
    DrawRectangleGradientV(0, 0, screenW, vTop, ColorAlpha(BLACK, 0.20f), BLANK);
    DrawRectangleGradientV(0, screenH - vTop, screenW, vTop, BLANK, ColorAlpha(BLACK, 0.20f));
    DrawRectangleGradientH(0, 0, vSide, screenH, ColorAlpha(BLACK, 0.18f), BLANK);
    DrawRectangleGradientH(screenW - vSide, 0, vSide, screenH, BLANK, ColorAlpha(BLACK, 0.18f));
}

// Barra of health estilizada
inline void DrawHealthBar(Vector2 pos, float pct, float w, float h, Color col) {
    Rectangle bg = {pos.x - w/2, pos.y, w, h};
    Rectangle fg = {pos.x - w/2, pos.y, w * pct, h};
    DrawRectangleRec(bg, ColorAlpha(BLACK, 0.7f));
    DrawRectangleRec(fg, col);
    DrawRectangleLinesEx(bg, 1.0f, ColorAlpha(col, 0.5f));
}
