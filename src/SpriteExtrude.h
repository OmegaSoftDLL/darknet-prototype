#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// SpriteExtrude — converts the EXISTING 2D art (procedural pixel-art) into the MODEL
// 3D REAL (mesh voxel), 100% fiel to the draw 2D, only that with volume/profundidade.
//
// Fluxo: renders the render() 2D of the entidade to uma RenderTexture (fundo
// transparent, centrado), reads the pixels and generates uma mesh 3D where cada pixel opaque
// vira uma column (voxel) extrudada in profundidade, with the COR exata of the pixel.
// O result is the character of the game, fiel, now in 3D of verdade.
//
// Os modelos are gerados UMA vez by type and cacheados (caro only in the 1o uso).
// ─────────────────────────────────────────────────────────────────────────────
#include <raylib.h>

namespace SpriteExtrude {
    // Generates um Model 3D voxel from uma Image RGBA (pixels opacos = solidos).
    // voxelSize: size of cada voxel in the world. depth: espessura (eixo Z/profund.).
    // O model stays centrado in X/Z and assenta the base in Y=0.
    Model BuildVoxelModel(Image img, float voxelSize, float depth);

    // Captura the draw 2D (drawFn) CENTRADO in worldTarget to uma Image RGBA
    // cap×cap (fundo transparent). drawFn draws in coords of world normais.
    // Template: evita std::function in the hot path (uma alocacao by entidade by frame).
    template<typename Fn>
    Image CaptureToImage(int cap, Vector2 worldTarget, Fn&& drawFn);
}

#include "rlgl.h"

template<typename Fn>
Image SpriteExtrude::CaptureToImage(int cap, Vector2 worldTarget, Fn&& drawFn) {
    RenderTexture2D rt = LoadRenderTexture(cap, cap);
    BeginTextureMode(rt);
    ClearBackground(BLANK);
    Camera2D cam = { 0 };
    cam.target   = worldTarget;
    cam.offset   = { cap * 0.5f, cap * 0.5f };
    cam.rotation = 0.0f;
    cam.zoom     = 1.0f;
    BeginMode2D(cam);
    drawFn();
    EndMode2D();
    EndTextureMode();
    Image img = LoadImageFromTexture(rt.texture);
    ImageFlipVertical(&img);              // RenderTexture comes of head to down
    UnloadRenderTexture(rt);
    return img;
}
