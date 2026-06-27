#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// SpriteExtrude — converte a arte 2D EXISTENTE (pixel-art procedural) em um MODELO
// 3D REAL (malha voxel), 100% fiel ao desenho 2D, só que com volume/profundidade.
//
// Fluxo: renderiza o render() 2D da entidade para uma RenderTexture (fundo
// transparente, centrado), lê os pixels e gera uma malha 3D onde cada pixel opaco
// vira uma coluna (voxel) extrudada em profundidade, com a COR exata do pixel.
// O resultado é o personagem do jogo, fiel, agora em 3D de verdade.
//
// Os modelos são gerados UMA vez por tipo e cacheados (caro só no 1º uso).
// ─────────────────────────────────────────────────────────────────────────────
#include <raylib.h>
#include <functional>

namespace SpriteExtrude {
    // Gera um Model 3D voxel a partir de uma Image RGBA (pixels opacos = sólidos).
    // voxelSize: tamanho de cada voxel no mundo. depth: espessura (eixo Z/profund.).
    // O modelo fica centrado em X/Z e assenta a base em Y=0.
    Model BuildVoxelModel(Image img, float voxelSize, float depth);

    // Captura o desenho 2D (drawFn) CENTRADO em worldTarget para uma Image RGBA
    // cap×cap (fundo transparente). drawFn desenha em coords de mundo normais.
    Image CaptureToImage(int cap, Vector2 worldTarget, std::function<void()> drawFn);
}
