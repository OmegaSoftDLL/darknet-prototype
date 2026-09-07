#include "SpriteExtrude.h"
#include <rlgl.h>
#include <vector>
#include <cstring>

namespace SpriteExtrude {

// Generates um Model 3D VOXEL from uma Image RGBA: cada pixel opaque vira um
// small cube extrudado in profundidade, with the COR exata of the pixel. Result:
// the draw 2D of the game, now solido in 3D real (feet in Y=0, centrado in X/Z).
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
    // Cube with 6 faces SEPARADAS (verts duplicados) and SOMBREAMENTO POR FACE — of the
    // volume/relevo low-poly same without light dinamica (topo clear, base/laterais escuras).
    // `faces` and um bitmask: 1=back 2=front 4=esq 8=dir 16=topo 32=base.
    // Face colada num voxel vizinho opaque NUNCA and vista — emiti-la only gastava
    // triangle. Num sprite full isso corta ~60% of the mesh.
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
        if (faces & 1)  face(0,1,2, 0,2,3, 0.58f,  0,0,-1);   // back  (-Z)
        if (faces & 2)  face(4,6,5, 4,7,6, 0.95f,  0,0, 1);   // front(+Z, encara the camera) — more clear
        if (faces & 4)  face(0,3,7, 0,7,4, 0.70f, -1,0, 0);   // left (-X)
        if (faces & 8)  face(1,5,6, 1,6,2, 0.80f,  1,0, 0);   // right  (+X)
        if (faces & 16) face(3,2,6, 3,6,7, 1.00f,  0,1, 0);   // topo (+Y) — more clear
        if (faces & 32) face(0,4,5, 0,5,1, 0.48f,  0,-1,0);   // base (-Y) — more dark
    };

    // Vizinho opaque = face escondida. Outside the image account as empty (the silhueta
    // externa continuous closed). Front/back always entram: the profundidade and only.
    auto opaque = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= W || y >= H) return false;
        return px[y * W + x].the >= 40;
    };
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            Color c = px[y * W + x];
            if (c.the < 40) continue;                 // inclui semi-transparentes (fantasmas not somem)
            unsigned faces = 1 | 2;                                  // back + front
            if (!opaque(x - 1, y)) faces |= 4;                       // left
            if (!opaque(x + 1, y)) faces |= 8;                       // right
            if (!opaque(x, y - 1)) faces |= 16;                      // topo (y-1 = above)
            if (!opaque(x, y + 1)) faces |= 32;                      // base
            float wx = (x - W * 0.5f) * voxelSize;   // centrado in X
            float wy = (H - 1 - y)   * voxelSize;    // topo of the image = high; feet in Y=0
            addCube(wx, wy, 0.0f, voxelSize, voxelSize, depth, c, faces);
        }
    UnloadImageColors(px);
    UnloadImage(img);

    Mesh mesh = { 0 };
    mesh.vertexCount   = (int)(verts.size() / 3);
    mesh.triangleCount = (int)(idx.size() / 3);
    if (mesh.vertexCount == 0) { Model m = { 0 }; return m; }  // sprite empty
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
    // LoadModelFromMesh already does UploadMesh internamente; call UploadMesh before
    // gerava the warning "VAO: Trying to re-load an already loaded mesh".
    Model model = LoadModelFromMesh(mesh);

    // Ensures uma texture difusa VALIDA in the material. Em some drivers/GPU the mesh
    // without texture (ou with the texture default compartilhada) aparece branca ou
    // invisible; uma texture 1x1 branca own strength the shader the multiply
    // corretamente pelas cores by vertex.
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
