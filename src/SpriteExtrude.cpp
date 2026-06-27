#include "SpriteExtrude.h"
#include <rlgl.h>
#include <vector>
#include <cstring>

namespace SpriteExtrude {

// Captura o desenho 2D (drawFn) centrado em worldTarget para uma Image RGBA cap×cap.
Image CaptureToImage(int cap, Vector2 worldTarget, std::function<void()> drawFn) {
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
    ImageFlipVertical(&img);              // RenderTexture vem de cabeça pra baixo
    UnloadRenderTexture(rt);
    return img;
}

// Gera um Model 3D VOXEL a partir de uma Image RGBA: cada pixel opaco vira um
// pequeno cubo extrudado em profundidade, com a COR exata do pixel. Resultado:
// o desenho 2D do jogo, agora sólido em 3D real (pés em Y=0, centrado em X/Z).
Model BuildVoxelModel(Image src, float voxelSize, float depth) {
    Image img = ImageCopy(src);
    const int MAX = 40;                   // limita contagem de voxels
    if (img.width > MAX || img.height > MAX) ImageResizeNN(&img, MAX, MAX);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const int W = img.width, H = img.height;
    Color* px = LoadImageColors(img);

    std::vector<float>          verts, norms;
    std::vector<unsigned char>  cols;
    std::vector<unsigned short> idx;
    verts.reserve(4096); idx.reserve(8192);

    auto addCube = [&](float cx, float cy, float cz,
                       float sx, float sy, float sz, Color c) {
        unsigned short b = (unsigned short)(verts.size() / 3);
        float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;
        const float v[8][3] = {
            {cx-hx,cy-hy,cz-hz},{cx+hx,cy-hy,cz-hz},{cx+hx,cy+hy,cz-hz},{cx-hx,cy+hy,cz-hz},
            {cx-hx,cy-hy,cz+hz},{cx+hx,cy-hy,cz+hz},{cx+hx,cy+hy,cz+hz},{cx-hx,cy+hy,cz+hz}
        };
        // sombreamento leve por face (fake light) p/ leitura de volume mesmo sem luz
        for (int i = 0; i < 8; ++i) {
            verts.push_back(v[i][0]); verts.push_back(v[i][1]); verts.push_back(v[i][2]);
            norms.push_back(0.0f); norms.push_back(0.0f); norms.push_back(1.0f);
            cols.push_back(c.r); cols.push_back(c.g); cols.push_back(c.b); cols.push_back(255);
        }
        static const unsigned short f[36] = {
            0,1,2, 0,2,3,   4,6,5, 4,7,6,   0,3,7, 0,7,4,
            1,5,6, 1,6,2,   3,2,6, 3,6,7,   0,4,5, 0,5,1
        };
        for (int i = 0; i < 36; ++i) idx.push_back((unsigned short)(b + f[i]));
    };

    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            Color c = px[y * W + x];
            if (c.a < 90) continue;                 // só pixels opacos
            float wx = (x - W * 0.5f) * voxelSize;   // centrado em X
            float wy = (H - 1 - y)   * voxelSize;    // topo da imagem = alto; pés em Y=0
            addCube(wx, wy, 0.0f, voxelSize, voxelSize, depth, c);
        }
    UnloadImageColors(px);
    UnloadImage(img);

    Mesh mesh = { 0 };
    mesh.vertexCount   = (int)(verts.size() / 3);
    mesh.triangleCount = (int)(idx.size() / 3);
    if (mesh.vertexCount == 0) { Model m = { 0 }; return m; }  // sprite vazio
    mesh.vertices = (float*)RL_MALLOC(verts.size() * sizeof(float));
    memcpy(mesh.vertices, verts.data(), verts.size() * sizeof(float));
    mesh.normals  = (float*)RL_MALLOC(norms.size() * sizeof(float));
    memcpy(mesh.normals, norms.data(), norms.size() * sizeof(float));
    mesh.colors   = (unsigned char*)RL_MALLOC(cols.size());
    memcpy(mesh.colors, cols.data(), cols.size());
    mesh.indices  = (unsigned short*)RL_MALLOC(idx.size() * sizeof(unsigned short));
    memcpy(mesh.indices, idx.data(), idx.size() * sizeof(unsigned short));
    UploadMesh(&mesh, false);
    return LoadModelFromMesh(mesh);
}

} // namespace SpriteExtrude
