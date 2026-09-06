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
    const int MAX = 34;                   // limita voxels (36 verts/voxel < 65535 idx)
    if (img.width > MAX || img.height > MAX) ImageResizeNN(&img, MAX, MAX);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const int W = img.width, H = img.height;
    Color* px = LoadImageColors(img);

    std::vector<float>          verts, norms, uvs;
    std::vector<unsigned char>  cols;
    std::vector<unsigned short> idx;
    verts.reserve(4096); idx.reserve(8192);

    auto shadeC = [](Color c, float k) -> Color {
        auto ch = [&](unsigned char v){ float r = v * k; return (unsigned char)(r > 255.0f ? 255.0f : r); };
        return Color{ ch(c.r), ch(c.g), ch(c.b), 255 };
    };
    // Cubo com 6 faces SEPARADAS (verts duplicados) e SOMBREAMENTO POR FACE — dá
    // volume/relevo low-poly mesmo sem luz dinâmica (topo claro, base/laterais escuras).
    // `faces` e um bitmask: 1=tras 2=frente 4=esq 8=dir 16=topo 32=base.
    // Face colada num voxel vizinho opaco NUNCA e vista — emiti-la so gastava
    // triangulo. Num sprite cheio isso corta ~60% da malha.
    auto addCube = [&](float cx, float cy, float cz,
                       float sx, float sy, float sz, Color c, unsigned faces) {
        float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;
        const float p[8][3] = {
            {cx-hx,cy-hy,cz-hz},{cx+hx,cy-hy,cz-hz},{cx+hx,cy+hy,cz-hz},{cx-hx,cy+hy,cz-hz},
            {cx-hx,cy-hy,cz+hz},{cx+hx,cy-hy,cz+hz},{cx+hx,cy+hy,cz+hz},{cx-hx,cy+hy,cz+hz}
        };
        auto face = [&](int i0,int i1,int i2,int i3,int i4,int i5,
                        float k, float nx,float ny,float nz) {
            int id[6] = { i0,i1,i2,i3,i4,i5 };
            unsigned short b = (unsigned short)(verts.size() / 3);
            Color sc = shadeC(c, k);
            for (int j = 0; j < 6; ++j) {
                const float* vv = p[id[j]];
                verts.push_back(vv[0]); verts.push_back(vv[1]); verts.push_back(vv[2]);
                norms.push_back(nx); norms.push_back(ny); norms.push_back(nz);
                uvs.push_back(0.0f); uvs.push_back(0.0f);
                cols.push_back(sc.r); cols.push_back(sc.g); cols.push_back(sc.b); cols.push_back(255);
                idx.push_back((unsigned short)(b + j));
            }
        };
        if (faces & 1)  face(0,1,2, 0,2,3, 0.58f,  0,0,-1);   // trás  (-Z)
        if (faces & 2)  face(4,6,5, 4,7,6, 0.95f,  0,0, 1);   // frente(+Z, encara a câmera) — mais clara
        if (faces & 4)  face(0,3,7, 0,7,4, 0.70f, -1,0, 0);   // esquerda (-X)
        if (faces & 8)  face(1,5,6, 1,6,2, 0.80f,  1,0, 0);   // direita  (+X)
        if (faces & 16) face(3,2,6, 3,6,7, 1.00f,  0,1, 0);   // topo (+Y) — mais claro
        if (faces & 32) face(0,4,5, 0,5,1, 0.48f,  0,-1,0);   // base (-Y) — mais escuro
    };

    // Vizinho opaco = face escondida. Fora da imagem conta como vazio (a silhueta
    // externa continua fechada). Frente/trás sempre entram: a profundidade e unica.
    auto opaque = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= W || y >= H) return false;
        return px[y * W + x].a >= 40;
    };
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            Color c = px[y * W + x];
            if (c.a < 40) continue;                 // inclui semi-transparentes (fantasmas não somem)
            unsigned faces = 1 | 2;                                  // trás + frente
            if (!opaque(x - 1, y)) faces |= 4;                       // esquerda
            if (!opaque(x + 1, y)) faces |= 8;                       // direita
            if (!opaque(x, y - 1)) faces |= 16;                      // topo (y-1 = acima)
            if (!opaque(x, y + 1)) faces |= 32;                      // base
            float wx = (x - W * 0.5f) * voxelSize;   // centrado em X
            float wy = (H - 1 - y)   * voxelSize;    // topo da imagem = alto; pés em Y=0
            addCube(wx, wy, 0.0f, voxelSize, voxelSize, depth, c, faces);
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
    mesh.texcoords = (float*)RL_MALLOC(uvs.size() * sizeof(float));
    memcpy(mesh.texcoords, uvs.data(), uvs.size() * sizeof(float));
    mesh.colors   = (unsigned char*)RL_MALLOC(cols.size());
    memcpy(mesh.colors, cols.data(), cols.size());
    mesh.indices  = (unsigned short*)RL_MALLOC(idx.size() * sizeof(unsigned short));
    memcpy(mesh.indices, idx.data(), idx.size() * sizeof(unsigned short));
    // LoadModelFromMesh já faz UploadMesh internamente; chamar UploadMesh antes
    // gerava o warning "VAO: Trying to re-load an already loaded mesh".
    Model model = LoadModelFromMesh(mesh);

    // Garante uma textura difusa VALIDA no material. Em alguns drivers/GPU a malha
    // sem textura (ou com a textura default compartilhada) aparece branca ou
    // invisivel; uma textura 1x1 branca propria forca o shader a multiplicar
    // corretamente pelas cores por vertice.
    static Texture2D whiteTex = [](){
        Image img = GenImageColor(1, 1, WHITE);
        Texture2D t = LoadTextureFromImage(img);
        UnloadImage(img);
        return t;
    }();
    if (model.materialCount > 0) {
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    }
    return model;
}

} // namespace SpriteExtrude
