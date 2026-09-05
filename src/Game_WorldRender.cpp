// Game_WorldRender.cpp â€” renderizacao 3D do mundo (extraido de Game.cpp).
// Mesma classe Game: funcoes movidas por copia literal (ensureVoxel, drawVoxel,
// drawGenericStructure, drawArkStructure, sphereInCameraFrustum, renderWorld3D).
// Statics duplicadas localmente (mesmo padrao de Game_WorldGen.cpp).
#include "Game.h"
#include "SpriteExtrude.h"
#include "SpriteGen.h"
#include <raylib.h>
#include <raymath.h>
#include "rlgl.h"
#include <cmath>
#include <algorithm>
#include <functional>
#include <string>

// Global definida em Game.cpp (sinal de captura de sprite p/ voxelizacao).
extern bool g_voxelCapture;

static Color structureTintFor(ZoneID z) {
    switch (z) {
        case ZoneID::Cemetery:       return { 150, 158, 172, 255 };
        case ZoneID::DarkForest:     return { 148, 156, 132, 255 };
        case ZoneID::CursedFarm:     return { 198, 176, 132, 255 };
        case ZoneID::Bunker:         return { 138, 150, 140, 255 };
        case ZoneID::Catacombs:      return { 152, 140, 126, 255 };
        case ZoneID::AbandonedManor: return { 158, 140, 162, 255 };
        case ZoneID::KronosForge:    return { 186, 142, 110, 255 };
        case ZoneID::InfernoZone:    return { 150,  96,  80, 255 };
        case ZoneID::KronosNexus:    return { 130, 168, 196, 255 };
        case ZoneID::GhostCity:      return { 160, 168, 180, 255 };
        case ZoneID::LARuins:
        default:                     return { 255, 255, 255, 255 };
    }
}

// Zonas onde os modelos MEDIEVAIS (castle.obj / house.obj) sao coerentes: areas
// rurais/goticas. Nas zonas urbanas e sci-fi (LA, cidade fantasma, bunker, forja,
// nexus...) o castelo de torres e a casa de telha quebram a direcao de arte —
// la o BuildingSystem desenha estruturas modernas em primitivas (auditoria P1).
static bool isMedievalZone(ZoneID z) {
    return z == ZoneID::CursedFarm || z == ZoneID::DarkForest ||
           z == ZoneID::Cemetery  || z == ZoneID::AbandonedManor;
}

// ── ESCALA DO MUNDO ──────────────────────────────────────────────────────────
// Tudo ancorado no heroi: ~28 unidades de altura = 1,75 m, entao 1 metro ~ 16u.
// Os valores antigos (casa 110u = 7 m na MAIOR dimensao) deixavam predio menor
// que gente: a cidade lia como maquete e o personagem como um poste ao lado dela.
static constexpr float FIT_HOUSE    = 175.0f;   // casa de 2 andares ~11 m
static constexpr float FIT_BARRACKS = 190.0f;   // celeiro/galpao ~12 m
static constexpr float FIT_CASTLE   = 340.0f;   // predio/castelo ~21 m
static constexpr float FIT_TURRET   =  95.0f;
static constexpr float FIT_MARKET   = 200.0f;
static constexpr float FIT_WELL     = 130.0f;   // silo alto
static constexpr float FIT_CAR      =  68.0f;   // carro ~4,2 m de comprimento

static void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    rlSetTexture(texture.id);

    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);

        // Front Face
        rlNormal3f(0.0f, 0.0f, 1.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);

        // Back Face
        rlNormal3f(0.0f, 0.0f, -1.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);

        // Top Face
        rlNormal3f(0.0f, 1.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);

        // Bottom Face
        rlNormal3f(0.0f, -1.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);

        // Right face
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);

        // Left Face
        rlNormal3f(-1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);
    rlEnd();

    rlSetTexture(rlGetTextureIdDefault());   // P0: religa branca p/ não vazar textura nas primitivas
}
void Game::ensureVoxel(int key, Vector2 capPos, std::function<void()> drawFn) {
    // hasVoxel() ja filtrou no call site — aqui e so cinto de seguranca.
    if (m_voxModels.count(key)) return;
    if (m_voxGenBudget <= 0) return;   // amortiza: poucas geracoes por frame (anti-engasgo)
    m_voxGenBudget--;
    g_voxelCapture = true;
    Image img = SpriteExtrude::CaptureToImage(96, capPos, drawFn);
    g_voxelCapture = false;
    // ESCALA — o unico lugar que define o tamanho de TODO personagem em 3D.
    // A captura tem 96px e a malha e reamostrada pra 34 celulas, entao
    // voxelSize = 2.82 reproduz EXATAMENTE o tamanho do sprite 2D em unidades de
    // mundo (1px 2D = 1 unidade). 3.4 deixava o personagem 20% MAIOR que a arte
    // 2D — perto da casa (110u) e do carro (40u) ele lia como gigante.
    // 1.95 = ~70% da arte 2D: heroi com ~28u (0,45 tile), ~1/4 da casa.
    const float VOX = 1.95f;    // tamanho do voxel (altura do personagem)
    const float VOX_DEPTH = 11.0f;  // espessura: com 7.5 o corpo lia como tabua/poste
    m_voxModels[key] = SpriteExtrude::BuildVoxelModel(img, VOX, VOX_DEPTH);
    applyWorldShader(m_voxModels[key]);
    {   // MEDIDA (nao chute): tamanho real do personagem em unidades de mundo,
        // pra comparar com casa/carro. tile = 64u.
        BoundingBox bb = GetModelBoundingBox(m_voxModels[key]);
        TraceLog(LOG_INFO, "VOXSIZE key=%d  L=%.1f  A=%.1f  P=%.1f", key,
                 bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.max.z - bb.min.z);
    }
    UnloadImage(img);
}

// Construcao generica do jogador (tipos sem modelo .obj proprio). Era um CUBO
// cinza com arestas - lia como placeholder de engine largado no cenario.
void Game::drawGenericStructure(Vector2 pos, float sc) const {
    float x = pos.x, z = pos.y;
    const Color CONCRETE = {  96,  98, 104, 255 };
    const Color METAL    = { 118, 122, 132, 255 };
    const Color TRIM     = {  60, 132, 150, 255 };
    DrawCylinderEx({ x, 0.08f, z }, { x, 0.09f, z }, 34.0f*sc, 34.0f*sc, 14,
                   ColorAlpha(BLACK, 0.32f));                                    // contato
    DrawCubeV({ x, 4.0f*sc,  z }, { 62.0f*sc,  8.0f*sc, 62.0f*sc }, CONCRETE);   // base
    DrawCubeV({ x, 26.0f*sc, z }, { 50.0f*sc, 36.0f*sc, 50.0f*sc }, METAL);      // corpo
    DrawCubeV({ x, 45.0f*sc, z }, { 56.0f*sc,  5.0f*sc, 56.0f*sc }, CONCRETE);   // beiral
    DrawCubeV({ x, 26.0f*sc, z - 25.0f*sc }, { 22.0f*sc, 20.0f*sc, 2.0f*sc }, TRIM);
    DrawCylinderEx({ x + 18.0f*sc, 47.0f*sc, z + 18.0f*sc },
                   { x + 18.0f*sc, 76.0f*sc, z + 18.0f*sc }, 1.8f*sc, 1.0f*sc, 5, METAL);
    DrawSphereEx({ x + 18.0f*sc, 78.0f*sc, z + 18.0f*sc }, 3.0f*sc, 5, 5,
                 Color{ 255, 120, 60, 255 });
}

// ARCA em zona urbana/sci-fi: base de respawn como FORTIFICACAO moderna — bunker
// de concreto com antena, holofotes e faixas de luz cyan. O castle.obj (torres
// vermelhas medievais) no meio do asfalto era o objeto mais olhado do jogo
// traindo a direcao de arte (auditoria P1). Mesma escala do castelo (~340u).
void Game::drawArkStructure(Vector2 pos) const {
    float x = pos.x, z = pos.y;
    const Color CONCRETE = { 104, 108, 114, 255 };
    const Color DARK     = {  66,  70,  76, 255 };
    const Color METAL    = { 128, 132, 142, 255 };
    const Color CYAN     = {  60, 220, 255, 255 };
    float t  = (float)GetTime();
    float pl = 0.55f + 0.45f * sinf(t * 2.2f);                 // pulso dos farois

    DrawCylinderEx({ x, 0.10f, z }, { x, 0.11f, z }, 165.0f, 165.0f, 18,
                   ColorAlpha(BLACK, 0.34f));                              // sombra de contato
    DrawCubeV({ x, 7.0f,   z }, { 300.0f, 14.0f, 300.0f }, DARK);          // plataforma
    DrawCubeV({ x, 42.0f,  z }, { 220.0f, 70.0f, 190.0f }, CONCRETE);      // corpo principal
    DrawCubeV({ x, 90.0f,  z }, { 150.0f, 28.0f, 130.0f }, METAL);         // convés superior
    DrawCubeV({ x, 106.0f, z }, { 90.0f,  8.0f,  74.0f }, DARK);           // casulo do topo
    // porta frontal com faixa luminosa (le como ENTRADA da base)
    DrawCubeV({ x, 26.0f, z - 96.0f }, { 54.0f, 52.0f, 6.0f }, DARK);
    DrawCubeV({ x, 56.0f, z - 97.0f }, { 66.0f, 5.0f, 4.0f }, ColorAlpha(CYAN, 0.65f + 0.30f * pl));
    // faixas de luz cyan nas laterais — marca "base da resistencia", nao ruina
    for (int sI = 0; sI < 2; ++sI) {
        float sx = sI ? 111.0f : -111.0f;
        DrawCubeV({ x + sx, 52.0f, z }, { 3.0f, 8.0f, 150.0f }, ColorAlpha(CYAN, 0.45f + 0.25f * pl));
    }
    // mastro de antena + farol pulsante (o marco que o jogador ve de longe)
    DrawCylinderEx({ x + 52.0f, 110.0f, z + 40.0f }, { x + 52.0f, 200.0f, z + 40.0f },
                   3.2f, 1.4f, 6, METAL);
    DrawSphereEx({ x + 52.0f, 204.0f, z + 40.0f }, 7.0f, 7, 7, ColorAlpha(CYAN, 0.55f + 0.45f * pl));
    DrawCylinderEx({ x - 58.0f, 104.0f, z - 34.0f }, { x - 58.0f, 158.0f, z - 34.0f },
                   2.6f, 1.2f, 6, METAL);
    DrawSphereEx({ x - 58.0f, 161.0f, z - 34.0f }, 5.0f, 6, 6,
                 ColorAlpha(Color{ 255, 130, 60, 255 }, 0.50f + 0.40f * (1.0f - pl)));
    // barricadas nos cantos da plataforma (leitura de fortificacao)
    for (int cI = 0; cI < 4; ++cI) {
        float bx = (cI & 1) ? 128.0f : -128.0f, bz = (cI & 2) ? 128.0f : -128.0f;
        DrawCubeV({ x + bx, 22.0f, z + bz }, { 34.0f, 30.0f, 34.0f }, DARK);
    }
}

void Game::drawVoxel(int base, Vector2 pos, float rotDeg, float walkPhase, bool moving) {
    // Quadro do passo pela fase da caminhada da PROPRIA entidade (nao pelo relogio):
    // parado = pose 0, andando = ciclo de VOX_POSES quadros.
    int pose = 0;
    if (moving) {
        float w = walkPhase * (float)VOX_POSES / (2.0f * PI);
        pose = ((int)floorf(w) % VOX_POSES + VOX_POSES) % VOX_POSES;
    }
    auto it = m_voxModels.find(voxKey(base, pose));
    if (it == m_voxModels.end()) it = m_voxModels.find(voxKey(base, 0));   // pose ainda nao gerada
    if (it == m_voxModels.end() || it->second.meshCount == 0) return;
    Model& mdl = it->second;

    // LOD de custo: alem de 600u o contorno escuro e a sombra de contato nao se
    // distinguem mais — economiza 2 draw calls por entidade distante (restam a
    // silhueta projetada, que le o relevo da sombra, e o modelo).
    float vdx = pos.x - camera3D.target.x, vdz = pos.y - camera3D.target.z;
    bool  vNear = (vdx*vdx + vdz*vdz) < 600.0f*600.0f;

    // SOMBRA REAL: projeta a SILHUETA do modelo no chão, deslocada pela DIREÇÃO da
    // luz (matriz de projeção em y=0). Não é disco — tem o formato do personagem.
    const Vector3 L = { -0.42f, -1.0f, -0.30f };       // direção da luz (de cima/frente)
    Matrix saved = mdl.transform;
    Matrix sm = MatrixIdentity();
    sm.m4 = -L.x / L.y;   // x deslocado pela altura (alonga na direção oposta à luz)
    sm.m5 = 0.0f;         // achata a altura (projeta no chão)
    sm.m6 = -L.z / L.y;   // z deslocado pela altura
    mdl.transform = sm;
    rlDisableDepthMask();                               // evita z-fight da silhueta
    DrawModel(mdl, { pos.x, 0.07f, pos.y }, 1.0f, ColorAlpha(BLACK, 0.42f));
    rlEnableDepthMask();
    mdl.transform = saved;

    // Sombra de CONTATO: mancha curta EXATAMENTE sob os pes. A silhueta projetada
    // acima da a direcao da luz; esta aqui e a que prega o personagem no chao.
    if (vNear)
        DrawCylinderEx({ pos.x, 2.30f, pos.y }, { pos.x, 2.34f, pos.y },
                       9.0f, 9.0f, 12, ColorAlpha(BLACK, 0.34f));

    // "Respiro" em ESCALA, nunca em translacao: o bob antigo levantava o modelo
    // inteiro (ate 1.2u) enquanto a sombra ficava parada no chao - era isso que
    // fazia TODO personagem/NPC parecer flutuar. Agora os pes ficam colados e so
    // o corpo estica ~1,5%. SINK afunda um tico pra nao sobrar fresta sob os pes.
    float breath = 1.0f + sinf((float)GetTime() * 2.4f + pos.x * 0.05f) * 0.015f;
    // Balanco do passo: o corpo sobe no meio da passada e desce no apoio. Some
    // quando parado, entao nao volta a parecer que flutua.
    float step = moving ? fabsf(sinf(walkPhase)) : 0.0f;
    breath += step * 0.040f;   // pisada visivel sem levantar o modelo (pes colados)
    rotDeg  += moving ? sinf(walkPhase) * 3.0f : 0.0f;   // leve gingado
    const float SINK = 0.6f;
    Vector3 at = { pos.x, -SINK, pos.y };
    // CONTORNO: mesma malha 6% maior, escura, desenhada ANTES. O modelo real
    // cobre o miolo e sobra so uma borda - separa o personagem do cenario, que
    // e o que faltava pra ele nao sumir no verde da floresta.
    // RIM LIGHT noturno: de noite o contorno escuro apaga JUNTO com o chao.
    // Conforme o ambiente fecha, a borda vira um fio de luar frio — a silhueta
    // do heroi e dos inimigos continua lendo no escuro (auditoria 2, P2).
    float rimK = (lightSystem.ambientDark - 0.30f) / 0.22f;  // 0 de dia, ~1 na noite fechada
    rimK = fminf(1.0f, fmaxf(0.0f, rimK));
    Color outlineC = { (unsigned char)(10.0f + rimK * 62.0f),
                       (unsigned char)(12.0f + rimK * 84.0f),
                       (unsigned char)(18.0f + rimK * 128.0f), 255 };
    if (vNear)
        DrawModelEx(mdl, at, { 0.0f, 1.0f, 0.0f }, rotDeg,
                    { 1.06f, 1.05f * breath, 1.06f }, outlineC);
    DrawModelEx(mdl, at, { 0.0f, 1.0f, 0.0f }, rotDeg, { 1.0f, breath, 1.0f }, WHITE);
}

// Teste esfera × frustum da camera 3D, em espaco de VIEW (raylib: frente = -Z).
// Barato: 1 transform + 3 comparacoes. O corte antigo de cenario era so uma CAIXA
// de distancia (±1400u): tudo ATRAS da camera e fora do cone estreito de 30°
// (a maior parte da caixa) ainda era desenhado primitiva por primitiva.
static bool sphereInCameraFrustum(const Camera3D& cam, const Matrix& view, float aspect,
                                  Vector3 p, float radius) {
    float vz = view.m2*p.x + view.m6*p.y + view.m10*p.z + view.m14;
    float dist = -vz;                                       // >0 = na frente
    if (dist < -radius) return false;                       // totalmente atras
    if (dist < 20.0f) return true;                          // colado na camera: aprova
    float vx = view.m0*p.x + view.m4*p.y + view.m8*p.z  + view.m12;
    float vy = view.m1*p.x + view.m5*p.y + view.m9*p.z  + view.m13;
    float tanH = tanf(cam.fovy * 0.5f * (float)DEG2RAD);
    float limY = dist * tanH + radius;
    if (vy < -limY || vy > limY) return false;
    float limX = dist * tanH * aspect + radius;
    return vx >= -limX && vx <= limX;
}

void Game::renderWorld3D() {
    // PRE-PASS (sem FBO ativo): captura/voxeliza o sprite 2D em MODELO 3D real, por tipo.
    // Amortizado: 1 geracao por frame. Cada geracao faz um readback GPU->CPU sincrono
    // (LoadImageFromTexture) que drena o pipeline; com 3/frame a soma derrubava um
    // frame pra ~22 FPS ao aparecer tipo novo (medido: 3 caps = 32ms num frame).
    // Com 1, o pior frame medido cai pra dentro do orcamento de 60 FPS; o custo e
    // as 4 poses de um tipo novo levarem 4 frames pra aparecer (pop-in imperceptivel).
    m_voxGenBudget = 1;
    // Gera os QUADROS DO PASSO de cada tipo: o render 2D e chamado com a fase da
    // caminhada forcada, entao cada pose sai com as pernas noutra posicao. Sem isto
    // o 3D tinha um unico modelo estatico por tipo e todo mundo deslizava.
    auto poseAngle = [](int pose) { return (float)pose * (PI * 0.5f); };
    // chave do player = classe + APARENCIA: trocar de arma/armadura gera modelo novo
    const int playerVoxBase = 1000 + (int)player.charClass * 1000 + player.visualSignature();
    if (!hasVoxelPoses(playerVoxBase)) {
        float sv = player.walkAnimTimer; bool mv = player.isMoving;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(playerVoxBase, po);
            if (hasVoxel(k)) continue;
            player.walkAnimTimer = poseAngle(po); player.isMoving = true;
            ensureVoxel(k, player.position, [this](){ player.render(); });
        }
        player.walkAnimTimer = sv; player.isMoving = mv;
    }
    for (auto& e : enemies) {
        if (hasVoxelPoses(100 + (int)e.type)) continue;
        float sv = e.walkAnimTimer;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(100 + (int)e.type, po);
            if (hasVoxel(k)) continue;
            e.walkAnimTimer = poseAngle(po);
            ensureVoxel(k, e.position, [&e](){ e.render(); });
        }
        e.walkAnimTimer = sv;
    }
    for (auto& n : npcs) {
        if (hasVoxelPoses(300 + (int)n.role)) continue;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(300 + (int)n.role, po);
            if (hasVoxel(k)) continue;
            ensureVoxel(k, n.position, [&n, po, &poseAngle](){
                NPC t; t.role = n.role; t.color = n.color; t.position = n.position;
                t.walkPhase = poseAngle(po); t.walking = true; t.render();
            });
        }
    }
    for (auto& f : cityFolk) {
        if (hasVoxelPoses(300 + f.role)) continue;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(300 + f.role, po);
            if (hasVoxel(k)) continue;
            ensureVoxel(k, f.position, [&f, po, &poseAngle](){
                NPC t; t.role = (NPCRole)f.role; t.position = f.position;
                t.walkPhase = poseAngle(po); t.walking = true; t.render();
            });
        }
    }
    for (auto& c : companions) {
        if (!c.active || hasVoxelPoses(500 + (int)c.type)) continue;
        float sv = c.walkTimer;
        for (int po = 0; po < VOX_POSES; ++po) {
            int k = voxKey(500 + (int)c.type, po);
            if (hasVoxel(k)) continue;
            c.walkTimer = poseAngle(po);
            ensureVoxel(k, c.position, [&c](){ c.render(); });
        }
        c.walkTimer = sv;
    }

    // Prepare light mask before drawing (uses screen-space projection of 3D lights)
    lightSystem.prepareMask3D(camera3D, screenWidth, screenHeight);

    BeginTextureMode(gameTarget);
    {   // Ceu/horizonte com MATIZ PROPRIO por fase (skyColorFor): o ceu vermelho
        // do inferno e o azul-noite da cidade fantasma sao metade da leitura do
        // bioma. A MESMA cor alimenta o fog do shader (updateWorldShaderUniforms),
        // entao a geometria distante morre exatamente na cor do ceu.
        Color sk = skyColorFor(currentZone);
        float dk = 0.30f + (1.0f - lightSystem.ambientDark) * 0.50f;  // noite escurece o ceu
        ClearBackground(Color{ (unsigned char)(sk.r * dk), (unsigned char)(sk.g * dk),
                               (unsigned char)(sk.b * dk), 255 });
    }
    {   // Ceu PROCEDURAL por cima da cor base: estrelas discretas + bruma no
        // horizonte (parallax pelo alvo da camera). So visual/deterministico —
        // nao toca gameplay nem a validacao de FPS. A noite realca as estrelas.
        Color sk = skyColorFor(currentZone);
        Color halo = { (unsigned char)std::min(255, (int)sk.r + 40),
                       (unsigned char)std::min(255, (int)sk.g + 40),
                       (unsigned char)std::min(255, (int)sk.b + 40), 255 };
        float px = (float)std::fmod(camera.target.x, 950.0f) / 950.0f;
        float py = (float)std::fmod(camera.target.y, 950.0f) / 950.0f;
        float starA = 0.34f * (1.0f - lightSystem.ambientDark);   // noite = mais estrelas
        if (starA > 0.04f) {
            unsigned int hse = 0x9e3779b9u ^ ((unsigned int)(screenWidth * screenHeight) & 0xFFFF);
            auto hrnd = [&]() { hse = hse * 1664525u + 1013904223u; return (float)((hse >> 8) & 0xFFFF) / 65535.0f; };
            for (int i = 0; i < 140; ++i) {
                float sx = (hrnd() + px * 0.22f) * (float)screenWidth;
                float sy = hrnd() * (float)screenHeight * 0.40f;
                float r  = 0.8f + hrnd() * 1.1f;
                Color c = (hrnd() < 0.18f) ? Color{255,230,170,255}            // estrela quente rara
                                           : Color{220,230,255,255};
                DrawCircle((int)sx, (int)sy, r,
                           ColorAlpha(c, starA * (0.5f + 0.5f * hrnd())));
            }
        }
        for (int i = 0; i < 8; ++i) {   // faixas de bruma (horizonte), cor do ceu clareada
            float yy = (float)i / 8.0f * (float)screenHeight * 0.34f + py * 14.0f;
            float wd = (0.20f + 0.20f * (float)((i * 2654435761u) & 0xFFFF) / 65535.0f)
                       * (float)screenWidth;
            float xx = (float)((i * 40503u) & 0xFFFF) / 65535.0f * (float)screenWidth + px * 30.0f;
            float t  = 0.9f + 0.1f * std::sin((float)GetTime() * 0.5f + i * 1.7f);
            DrawCircleGradient((int)(xx + wd * 0.5f), (int)yy, wd * 0.34f * t,
                               ColorAlpha(halo, 0.10f * (1.0f - lightSystem.ambientDark * 0.4f)),
                               ColorAlpha(halo, 0.0f));
        }
    }
    updateWorldShaderUniforms();

    // ── 1. Modo 3D: Chão, Paredes, Sombras e Entidades (Billboards) ───────────
    rlSetClipPlanes(10.0, 4000.0);
    BeginMode3D(camera3D);
        // Render do mapa 3D
        tilemap.render3D(camera.target, camera3D, (float)screenWidth / (float)screenHeight);

        // Decalques de chão em 3D (sangue/queimado)
        for (const auto& d : decals) {
            if (std::fabs(d.pos.x - camera.target.x) > 1000 || std::fabs(d.pos.y - camera.target.y) > 650) continue;
            float a = (d.life / d.maxLife);
            if (d.type == 0) { // sangue
                DrawPlane({ d.pos.x, 0.12f, d.pos.y }, { d.size * 2.0f, d.size * 1.2f }, ColorAlpha(d.color, 0.45f * a));
                DrawPlane({ d.pos.x - d.size * 0.8f, 0.12f, d.pos.y + 4.0f }, { d.size * 0.7f, d.size * 0.7f }, ColorAlpha(d.color, 0.40f * a));
                DrawPlane({ d.pos.x + d.size * 1.0f, 0.12f, d.pos.y - 2.0f }, { d.size * 0.6f, d.size * 0.6f }, ColorAlpha(d.color, 0.35f * a));
            } else { // queimado
                DrawPlane({ d.pos.x, 0.12f, d.pos.y }, { d.size * 1.4f, d.size * 1.4f }, ColorAlpha(Color{20,18,16,255}, 0.5f * a));
                DrawPlane({ d.pos.x, 0.13f, d.pos.y }, { d.size * 1.5f, d.size * 1.5f }, ColorAlpha(Color{255,120,30,255}, 0.2f * a));
            }
        }

        // Sombras e luzes de poste do cenário (owDecor) + billboards 3D reais
        if (openWorldMode && owDecorBuilt) {
            SpriteBank& sb = SpriteBank::get();
            float time = (float)GetTime();
            // Frustum da camera (matriz de view + cone de 30°): calculado 1x por frame.
            const Matrix camView = MatrixLookAt(camera3D.position, camera3D.target, camera3D.up);
            const float camAspect = (float)screenWidth / (float)screenHeight;
            for (const auto& obj : owDecor.scenery) {
                float dx = obj.position.x - camera.target.x;
                float dy = obj.position.y - camera.target.y;
                if (dx < -1400 || dx > 1400 || dy < -1400 || dy > 1400) continue;
                // FRUSTUM CULLING real: mais da metade da caixa ±1400u fica atras
                // da camera ou fora do cone de 30°. Raio generoso (400u) cobre ate
                // os predios maiores (FIT_CASTLE=340 · scale) sem risco de pop-in.
                if (!sphereInCameraFrustum(camera3D, camView, camAspect,
                                           { obj.position.x, 100.0f, obj.position.y },
                                           400.0f)) continue;
                // LOD dos props pequenos. Grama tem 27 mil instancias no mundo; a
                // ~1200 delas caiam dentro do corte antigo e cada uma custava 10
                // primitivas = 12 mil draws por frame. Era isso que derrubava o FPS
                // pra ~20 (render 51ms). Props pequenos morrem cedo e simplificam.
                float d2cam = dx*dx + dy*dy;
                bool  smallProp = (obj.type == 11 || obj.type == 12 || obj.type == 13 ||
                                   obj.type == 21);
                if (smallProp && d2cam > 950.0f*950.0f) continue;
                bool  lodFar = d2cam > 560.0f*560.0f;

                float w = 64.0f, h = 64.0f;
                bool hasSprite = (sb.ready && obj.type >= 0 && obj.type < SpriteBank::NUM_SCENERY);

                // Estruturas grandes = MODELOS 3D REAIS (não billboard 2.5D).
                Model* mdl = nullptr; float mscale = 40.0f;
                switch (obj.type) {
                    case 0: mdl = &m_houseModel;    mscale = m_houseScale; break; // casa
                    case 1: mdl = &m_barracksModel; mscale = m_barracksScale; break; // celeiro
                    case 7: mdl = &m_castleModel;   mscale = m_castleScale; break; // predio
                    case 8: mdl = &m_wellModel;     mscale = m_wellScale; break; // silo
                    default: break;
                }
                if (mdl && m_modelsLoaded && mdl->meshCount > 0) {
                    float s = mscale * (obj.scale > 0.01f ? obj.scale : 1.0f);
                    // SOMBRA PROJETADA do predio: mesma malha achatada em y=0 e
                    // cisalhada pela direcao da luz (o truque usado nos personagens).
                    // E o que da profundidade a uma cidade - sem ela os predios
                    // parecem adesivos colados num chao chapado.
                    // 3 elipses concentricas deslocadas na direcao da luz. A versao
                    // anterior projetava a MALHA achatada e, num modelo caixote, saia
                    // uma LAJE PRETA retangular no chao - pior que nao ter sombra.
                    {
                        float fr = 60.0f * obj.scale;
                        switch (obj.type) {
                            case 0: fr = FIT_HOUSE    * obj.scale * 0.46f; break;
                            case 1: fr = FIT_BARRACKS * obj.scale * 0.46f; break;
                            case 7: fr = FIT_CASTLE   * obj.scale * 0.40f; break;
                            case 8: fr = FIT_WELL     * obj.scale * 0.42f; break;
                            default: break;
                        }
                        float offX = fr * 0.30f, offZ = fr * 0.22f;
                        const float rk[3] = { 1.06f, 0.78f, 0.50f };
                        const float ak[3] = { 0.10f, 0.12f, 0.14f };
                        rlDisableDepthMask();
                        for (int sI = 0; sI < 3; ++sI)
                            DrawCylinderEx({ obj.position.x + offX, 2.10f + sI*0.06f, obj.position.y + offZ },
                                           { obj.position.x + offX, 2.14f + sI*0.06f, obj.position.y + offZ },
                                           fr * rk[sI], fr * rk[sI], 18, ColorAlpha(BLACK, ak[sI]));
                        rlEnableDepthMask();
                    }
                    // OCLUSAO: a camera olha de +Z, entao construcao com y de mundo
                    // MAIOR que a do player fica na frente dele. Sumir atras de um
                    // predio e perder o proprio personagem de vista - ARPG isometrico
                    // resolve isso deixando o oclusor translucido.
                    // TINTA por bioma: o mesmo modelo lido como pedra clara em LA e
                    // como pedra queimada no inferno ja muda a cidade inteira.
                    Color zt  = structureTintFor(tilemap.biomeAtWorld(obj.position.x, obj.position.y));
                    float odx = fabsf(obj.position.x - player.position.x);
                    float odz = obj.position.y - player.position.y;
                    bool  occludes = (odz > 0.0f && odz < 620.0f && odx < 230.0f);
                    DrawModelEx(*mdl, { obj.position.x, 0.0f, obj.position.y }, { 0.0f, 1.0f, 0.0f },
                                obj.rotation * RAD2DEG, { s, s, s },
                                occludes ? ColorAlpha(zt, 0.30f) : zt);
                    w = s; h = s;
                } else {
                    // Fogueira acende luz de verdade (o bloom faz o resto)
                    if (obj.type == 22)
                        lightSystem.addTorchLight(obj.position);
                    // Props do cenario em PRIMITIVAS 3D (sem billboard "tabua de pe").
                    float sc = (obj.scale > 0.01f ? obj.scale : 1.0f);
                    float x = obj.position.x, zz = obj.position.y;
                    float H = 78.0f * sc, rr = 17.0f * sc;
                    w = rr * 2.0f; h = H;
                    switch (obj.type) {
                        case 2: { // arvore: tronco + galhos + copa em massa de volumes
                            // Antes eram 3 esferas concentricas = "bola verde no palito".
                            // Agora: tronco que afina, 3 galhos saindo dele e uma copa
                            // de 7 volumes irregulares com gradiente (claro em cima,
                            // escuro embaixo) — le como massa de folhagem, nao como bola.
                            // PALETA POR BIOMA: cemiterio/inferno/cidade fantasma tem
                            // ARVORE MORTA (sem copa, so galhos nus) — a grama verde e
                            // a arvore frondosa no inferno eram o maior delator de
                            // "mesmo mundo com outra tinta".
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            unsigned hsh = (unsigned)(x * 0.7f) * 73856093u ^ (unsigned)(zz * 0.7f) * 19349663u;
                            auto jit = [&](int k, float amp) {   // deslocamento estavel por arvore
                                return (float)(((hsh >> (k * 3)) & 15) - 7) / 7.0f * amp;
                            };
                            // altura do tronco varia: floresta com so uma altura le como
                            // stamp repetido. 0.44..0.72 do H do objeto.
                            float TH = H * (0.44f + (float)((hsh >> 5) & 15) / 15.0f * 0.28f);
                            bool dead = (cp.canopyBulk <= 0.01f);
                            Color trunkC = dead ? Color{66,58,50,255} : Color{84,58,34,255};
                            Color barkC  = dead ? Color{52,46,40,255} : Color{68,46,27,255};
                            DrawCylinderEx({x,0,zz}, {x, TH, zz}, rr*0.34f, rr*0.19f, 8, trunkC);
                            DrawCylinderEx({x,0,zz}, {x, TH*0.30f, zz}, rr*0.42f, rr*0.34f, 8, barkC); // raiz/base
                            for (int gI = 0; gI < 3; ++gI) {            // galhos
                                float ga = gI * 2.094f + jit(gI, 1.0f);
                                DrawCylinderEx({x, TH*0.72f, zz},
                                               {x + cosf(ga)*rr*0.85f, TH*1.02f, zz + sinf(ga)*rr*0.85f},
                                               rr*0.11f, rr*0.06f, 5, dead ? Color{58,50,44,255} : Color{74,52,30,255});
                            }
                            if (dead) {
                                // galhos NUS apontando pra cima: a silhueta seca e o
                                // que le "lugar morto" (cemiterio/inferno/fantasma).
                                for (int gI = 0; gI < 3; ++gI) {
                                    float ga = gI * 2.094f + jit(gI, 1.0f) + 0.7f;
                                    DrawCylinderEx({x, TH*0.88f, zz},
                                                   {x + cosf(ga)*rr*0.70f, TH*1.24f, zz + sinf(ga)*rr*0.70f},
                                                   rr*0.06f, rr*0.015f, 4, {58,50,44,255});
                                }
                                h = TH * 1.28f;
                                break;
                            }
                            struct Puff { float ox, oy, oz, r; float k; };
                            const Puff pf[7] = {
                                { 0.00f, 1.00f,  0.00f, 1.00f, 1.00f },   // topo (mais claro)
                                {-0.62f, 0.86f, -0.18f, 0.74f, 0.88f },
                                { 0.60f, 0.88f,  0.16f, 0.78f, 0.94f },
                                { 0.12f, 0.84f,  0.62f, 0.72f, 0.82f },
                                {-0.20f, 0.82f, -0.60f, 0.70f, 0.76f },
                                {-0.40f, 0.66f,  0.34f, 0.62f, 0.66f },   // base (mais escuro)
                                { 0.42f, 0.64f, -0.30f, 0.60f, 0.62f },
                            };
                            // ESPECIE por hash: 3 paletas de folhagem POR BIOMA. Um verde
                            // unico pra floresta inteira le como textura repetida, nao mata.
                            const Color CANOPY = cp.canopy[(hsh >> 17) % 3];
                            // porte e densidade tambem variam por arvore (e por bioma)
                            float bulk = (0.86f + (float)((hsh >> 11) & 15) / 15.0f * 0.34f) * cp.canopyBulk;
                            float tone = 0.84f + (float)((hsh >> 21) & 15) / 15.0f * 0.30f;
                            for (int pI = 0; pI < 7; ++pI) {
                                const Puff& q = pf[pI];
                                float k = q.k * tone;
                                Color cc = { (unsigned char)fminf(255.0f, CANOPY.r * k),
                                             (unsigned char)fminf(255.0f, CANOPY.g * k),
                                             (unsigned char)fminf(255.0f, CANOPY.b * k), 255 };
                                DrawSphereEx({ x  + (q.ox + jit(pI, 0.16f)) * rr * bulk,
                                               TH + q.oy * rr * 0.92f,
                                               zz + (q.oz + jit(pI + 4, 0.16f)) * rr * bulk },
                                             rr * q.r * bulk, 7, 7, cc);   // 7 segmentos = facetado low-poly
                            }
                            h = TH + rr * 1.9f * bulk;
                        } break;
                        case 3: { // lapide: laje + topo curvo + base
                            DrawCubeV({x, H*0.32f, zz}, {rr*1.1f, H*0.55f, rr*0.35f}, {120,122,130,255});
                            DrawSphereEx({x, H*0.58f, zz}, rr*0.55f, 8, 8, {120,122,130,255});
                            DrawCubeV({x, H*0.06f, zz}, {rr*1.4f, H*0.12f, rr*0.6f}, {92,92,98,255});
                            h = H*0.64f;
                        } break;
                        case 4: { // cerca: 2 postes + travessa (orientada por rot)
                            float c=cosf(obj.rotation), s2=sinf(obj.rotation), L=rr*1.6f;
                            DrawCylinderEx({x-c*L,0,zz-s2*L},{x-c*L,H*0.5f,zz-s2*L}, rr*0.12f, rr*0.12f, 6, {70,52,34,255});
                            DrawCylinderEx({x+c*L,0,zz+s2*L},{x+c*L,H*0.5f,zz+s2*L}, rr*0.12f, rr*0.12f, 6, {70,52,34,255});
                            DrawCubeV({x, H*0.40f, zz}, {L*2.0f, rr*0.18f, rr*0.14f}, {84,62,40,255});
                            h = H*0.5f;
                        } break;
                        case 5: { // poste de luz: haste + luminaria emissiva
                            DrawCylinderEx({x,0,zz},{x,H,zz}, rr*0.12f, rr*0.09f, 6, {46,46,54,255});
                            DrawSphereEx({x, H*0.97f, zz}, rr*0.28f, 8, 8, {255,224,150,255});
                        } break;
                        case 9: { // arco/catacumba: 2 pilares + lintel
                            float c=cosf(obj.rotation), s2=sinf(obj.rotation), L=rr*1.2f;
                            DrawCubeV({x-c*L,H*0.45f,zz-s2*L},{rr*0.5f,H*0.9f,rr*0.5f},{96,90,80,255});
                            DrawCubeV({x+c*L,H*0.45f,zz+s2*L},{rr*0.5f,H*0.9f,rr*0.5f},{96,90,80,255});
                            DrawCubeV({x,H*0.92f,zz},{L*2.4f,rr*0.5f,rr*0.6f},{104,98,86,255});
                        } break;
                        case 10: { // estatua: pedestal + figura low-poly de pedra
                            DrawCubeV({x, H*0.10f, zz}, {rr*1.3f, H*0.2f, rr*1.3f}, {108,108,116,255});
                            DrawCapsule({x, H*0.25f, zz}, {x, H*0.74f, zz}, rr*0.45f, 8, 8, {150,150,158,255});
                            DrawSphereEx({x, H*0.84f, zz}, rr*0.42f, 8, 8, {150,150,158,255});
                        } break;
                        case 14: { // CRIPTA / MAUSOLEU (cemiterio, catacumbas)
                            float W = 68.0f * sc, D = 54.0f * sc, Hh = 66.0f * sc;
                            Color stone = { 138, 140, 148, 255 };
                            Color dark2 = {  92,  94, 102, 255 };
                            rlPushMatrix(); rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0, 1, 0);
                            DrawCube({0, 5.0f*sc, 0}, W*1.18f, 10.0f*sc, D*1.18f, dark2);
                            DrawCube({0, Hh*0.5f, 0}, W, Hh, D, stone);
                            DrawCylinderEx({-W*0.5f, Hh, 0}, {W*0.5f, Hh, 0}, D*0.62f, D*0.62f, 3, dark2);
                            DrawCube({0, Hh*0.34f, -D*0.52f}, W*0.34f, Hh*0.62f, 4.0f*sc, {52,54,60,255});
                            for (int cI = 0; cI < 2; ++cI)
                                DrawCylinderEx({ (cI?1:-1)*W*0.40f, 0.0f, -D*0.5f },
                                               { (cI?1:-1)*W*0.40f, Hh*0.92f, -D*0.5f },
                                               6.0f*sc, 5.0f*sc, 7, stone);
                            rlPopMatrix();
                            w = W * 1.3f; h = Hh + D*0.6f;
                        } break;
                        case 15: { // BUNKER de concreto (bunker, forja, cidade fantasma)
                            float W = 96.0f * sc, D = 78.0f * sc, Hh = 42.0f * sc;
                            Color conc  = { 120, 124, 118, 255 };
                            Color dark2 = {  78,  82,  78, 255 };
                            Color slit  = {  30,  32,  30, 255 };
                            // OCLUSAO: mesma regra do predio moderno (janela pela
                            // pegada real) — bunker e baixo, mas largo o bastante
                            // pra esconder o heroi agachado atras dele.
                            float odxB = fabsf(x - player.position.x);
                            float odzB = zz - player.position.y;
                            bool  occludes = (odzB > -D * 0.5f &&
                                              odzB <  Hh * 1.25f + D * 0.5f &&
                                              odxB <  W * 0.55f + 40.0f);
                            if (occludes) {
                                conc  = ColorAlpha(conc,  0.30f);
                                dark2 = ColorAlpha(dark2, 0.30f);
                                slit  = ColorAlpha(slit,  0.30f);
                                rlDisableDepthMask();   // translucido nao grava profundidade
                            }
                            rlPushMatrix(); rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0, 1, 0);
                            DrawCube({0, Hh*0.5f, 0}, W, Hh, D, conc);
                            DrawCube({0, Hh + 6.0f*sc, 0}, W*0.82f, 12.0f*sc, D*0.82f, dark2);
                            DrawCube({0, Hh*0.55f, -D*0.52f}, W*0.52f, 9.0f*sc, 5.0f*sc, slit);
                            DrawCylinderEx({W*0.28f, Hh+12.0f*sc, D*0.22f},
                                           {W*0.28f, Hh+52.0f*sc, D*0.22f}, 2.4f*sc, 1.2f*sc, 5, dark2);
                            for (int sI = 0; sI < 3; ++sI)
                                DrawCube({ (sI-1)*W*0.38f, 9.0f*sc, D*0.72f },
                                         W*0.22f, 18.0f*sc, 12.0f*sc, dark2);
                            rlPopMatrix();
                            if (occludes) rlEnableDepthMask();
                            w = W * 1.2f; h = Hh + 60.0f*sc;
                        } break;
                        case 16: { // ESPIRA INFERNAL (inferno, forja)
                            float R = 26.0f * sc, Hh = 190.0f * sc;
                            Color rock = { 62, 44, 42, 255 };
                            Color glow = { 226, 96, 40, 255 };
                            float t2 = (float)GetTime();
                            DrawCylinderEx({x, 0.0f, zz}, {x, Hh*0.45f, zz}, R, R*0.62f, 7, rock);
                            DrawCylinderEx({x, Hh*0.45f, zz}, {x, Hh, zz}, R*0.60f, R*0.10f, 7, rock);
                            for (int sI = 0; sI < 3; ++sI) {
                                float a = sI * 2.094f + obj.rotation;
                                DrawCylinderEx({x + cosf(a)*R*1.3f, 0.0f, zz + sinf(a)*R*1.3f},
                                               {x + cosf(a)*R*0.9f, Hh*0.34f, zz + sinf(a)*R*0.9f},
                                               R*0.34f, R*0.06f, 5, rock);
                            }
                            float pulse = 0.55f + 0.45f * sinf(t2 * 2.0f + x * 0.01f);
                            DrawSphereEx({x, Hh*0.98f, zz}, R*0.38f, 7, 7, ColorAlpha(glow, pulse));
                            DrawCylinderEx({x, Hh*0.5f, zz}, {x, Hh*0.92f, zz}, R*0.20f, R*0.06f, 6,
                                           ColorAlpha(glow, 0.30f * pulse));
                            w = R * 2.6f; h = Hh;
                        } break;
                        case 17: { // MONOLITO ALIENIGENA (nexus)
                            float W = 34.0f * sc, Hh = 170.0f * sc;
                            Color body2 = { 42, 58, 74, 255 };
                            Color neon  = { 90, 220, 255, 255 };
                            float t2 = (float)GetTime();
                            rlPushMatrix(); rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0, 1, 0);
                            DrawCube({0, Hh*0.5f, 0}, W, Hh, W*0.55f, body2);
                            DrawCube({0, 6.0f*sc, 0}, W*1.6f, 12.0f*sc, W*1.2f, {32,42,54,255});
                            for (int lI = 0; lI < 4; ++lI) {
                                float k = 0.25f + lI * 0.20f;
                                float pulse = 0.35f + 0.45f * sinf(t2 * 1.6f + lI * 1.3f);
                                DrawCube({0, Hh*k, -W*0.30f}, W*0.70f, 5.0f*sc, 2.0f*sc,
                                         ColorAlpha(neon, pulse));
                            }
                            rlPopMatrix();
                            w = W * 2.0f; h = Hh;
                        } break;
                        case 18: { // CABANA DE MADEIRA (floresta, fazenda)
                            float W = 74.0f * sc, D = 62.0f * sc, Hh = 46.0f * sc;
                            Color wood  = { 104, 74, 46, 255 };
                            Color roof2 = {  74, 58, 40, 255 };
                            rlPushMatrix(); rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0, 1, 0);
                            for (int lg = 0; lg < 4; ++lg)
                                DrawCylinderEx({-W*0.5f, 9.0f*sc + lg*11.0f*sc, -D*0.5f},
                                               { W*0.5f, 9.0f*sc + lg*11.0f*sc, -D*0.5f},
                                               5.5f*sc, 5.5f*sc, 6, wood);
                            DrawCube({0, Hh*0.5f, 0}, W, Hh, D, wood);
                            DrawCylinderEx({-W*0.5f, Hh, 0}, {W*0.5f, Hh, 0}, D*0.60f, D*0.60f, 3, roof2);
                            DrawCube({0, Hh*0.34f, -D*0.52f}, W*0.28f, Hh*0.60f, 3.0f*sc, {44,32,22,255});
                            DrawCylinderEx({W*0.32f, Hh, D*0.20f}, {W*0.32f, Hh+34.0f*sc, D*0.20f},
                                           6.0f*sc, 5.0f*sc, 6, {86,84,80,255});
                            rlPopMatrix();
                            w = W * 1.3f; h = Hh + D*0.6f;
                        } break;
                        case 19: { // TORRE DE VIGIA (universal: muda a silhueta da cidade)
                            float R = 15.0f * sc, Hh = 132.0f * sc;
                            Color post = { 96, 78, 56, 255 };
                            Color top2 = { 74, 60, 44, 255 };
                            for (int lI = 0; lI < 4; ++lI) {
                                float a = lI * 1.5708f + obj.rotation;
                                DrawCylinderEx({x + cosf(a)*R*1.5f, 0.0f, zz + sinf(a)*R*1.5f},
                                               {x + cosf(a)*R*0.55f, Hh*0.78f, zz + sinf(a)*R*0.55f},
                                               4.0f*sc, 3.0f*sc, 5, post);
                            }
                            DrawCube({x, Hh*0.82f, zz}, R*3.0f, 8.0f*sc, R*3.0f, top2);
                            DrawCube({x, Hh*0.95f, zz}, R*2.6f, 20.0f*sc, R*2.6f, ColorAlpha(post, 0.85f));
                            DrawCylinderEx({x, Hh, zz}, {x, Hh + 16.0f*sc, zz}, R*2.0f, 0.5f*sc, 4, top2);
                            w = R * 3.4f; h = Hh + 20.0f*sc;
                        } break;
                        case 22: { // FOGUEIRA / barril em chamas: luz, cor e vida
                            // Blizzard amarra COR a evento e usa luz para guiar o olho.
                            // Cinza uniforme nao guia nada: a fogueira e ancora visual,
                            // ponto de referencia e o unico calor da rua.
                            unsigned hsh = (unsigned)(x * 0.8f) * 2246822519u ^ (unsigned)(zz * 0.8f) * 374761393u;
                            float t2 = (float)GetTime() + (float)(hsh & 255) * 0.01f;
                            float R  = 13.0f * sc;
                            Color drum = { 96, 72, 52, 255 };
                            // barril
                            DrawCylinderEx({ x, 0.0f, zz }, { x, R * 1.5f, zz }, R, R * 0.96f, 10, drum);
                            DrawCylinderEx({ x, R * 1.5f, zz }, { x, R * 1.56f, zz }, R * 1.06f, R * 1.06f, 10,
                                           Color{ 68, 52, 38, 255 });
                            // chama: 3 lambidas pulsando em alturas diferentes
                            for (int fI = 0; fI < 3; ++fI) {
                                float ph = t2 * (2.6f + fI * 0.7f) + fI * 2.1f;
                                float hgt = R * (1.5f + 0.9f + 0.35f * sinf(ph));
                                float wob = sinf(ph * 1.7f) * R * 0.18f;
                                Color c1 = (fI == 0) ? Color{ 255, 210, 120, 235 }
                                         : (fI == 1) ? Color{ 250, 140,  50, 205 }
                                                     : Color{ 200,  70,  30, 170 };
                                DrawCylinderEx({ x + wob * 0.3f, R * 1.5f, zz + wob * 0.2f },
                                               { x + wob,        hgt,      zz + wob * 0.6f },
                                               R * (0.62f - fI * 0.14f), R * 0.05f, 6, c1);
                            }
                            // brasa no chao + fumaca
                            DrawCylinderEx({ x, 2.0f, zz }, { x, 2.2f, zz }, R * 1.5f, R * 1.5f, 12,
                                           ColorAlpha(Color{ 255, 120, 40, 255 }, 0.14f));
                            DrawSphereEx({ x + sinf(t2) * 4.0f, R * 3.6f, zz + cosf(t2 * 0.7f) * 3.0f },
                                         R * 0.5f, 5, 5, ColorAlpha(Color{ 60, 58, 56, 255 }, 0.22f));
                            w = R * 2.4f; h = R * 3.0f;
                        } break;
                        case 21: { // ENTULHO: laje partida, viga exposta, tijolo
                            // Diablo enche o chao de destroco com volume. Chao limpo
                            // entre predios e o que fazia a cidade parecer maquete.
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            unsigned hsh = (unsigned)(x * 1.1f) * 2654435761u ^ (unsigned)(zz * 1.1f) * 668265263u;
                            float R = 26.0f * sc;
                            Color slab  = cp.slabA;
                            Color slab2 = cp.slabB;
                            Color rebar = { 122,  78,  48, 255 };
                            // monte de lajes inclinadas
                            for (int i = 0; i < (lodFar ? 2 : 5); ++i) {
                                float a  = i * 1.257f + (float)((hsh >> (i * 3)) & 7) * 0.22f;
                                float rd = R * (0.20f + (float)((hsh >> (i * 2)) & 7) / 7.0f * 0.62f);
                                float sx = R * (0.42f + (float)((hsh >> i) & 3) * 0.12f);
                                float sy = R * (0.16f + (float)((hsh >> (i + 4)) & 3) * 0.10f);
                                rlPushMatrix();
                                rlTranslatef(x + cosf(a) * rd, sy * 0.55f, zz + sinf(a) * rd);
                                rlRotatef(a * RAD2DEG, 0.0f, 1.0f, 0.0f);
                                rlRotatef(12.0f + (float)((hsh >> i) & 15), 0.0f, 0.0f, 1.0f);
                                DrawCube({ 0.0f, 0.0f, 0.0f }, sx, sy, sx * 0.72f,
                                         (i & 1) ? slab : slab2);
                                rlPopMatrix();
                            }
                            // vergalhoes tortos saindo do monte
                            for (int i = 0; i < 3; ++i) {
                                float a = i * 2.0f + (float)((hsh >> (i * 5)) & 7) * 0.3f;
                                DrawCylinderEx({ x + cosf(a) * R * 0.3f, 0.0f, zz + sinf(a) * R * 0.3f },
                                               { x + cosf(a) * R * 0.7f, R * 0.85f, zz + sinf(a) * R * 0.5f },
                                               1.2f * sc, 0.7f * sc, 4, rebar);
                            }
                            // cascalho miudo em volta
                            for (int i = 0; i < (lodFar ? 0 : 6); ++i) {
                                float a  = i * 1.047f + (float)((hsh >> (i + 2)) & 7) * 0.25f;
                                float rd = R * (0.75f + (float)((hsh >> i) & 3) * 0.16f);
                                DrawSphereEx({ x + cosf(a) * rd, 2.4f * sc, zz + sinf(a) * rd },
                                             (2.0f + (float)((hsh >> i) & 3)) * sc, 5, 5, slab2);
                            }
                            w = R * 2.2f; h = R;
                        } break;
                        case 20: { // PREDIO MODERNO (cidade: LA, fantasma, forja, nexus)
                            // Um castelo de torres nao tem o que fazer numa rua com
                            // asfalto e carro. Predio de concreto com fileiras de
                            // janela e caixa d'agua no topo: e isso que faz o lugar
                            // ler como cidade destruida, e nao como cenario medieval.
                            unsigned hsh = (unsigned)(x * 0.6f) * 374761393u ^ (unsigned)(zz * 0.6f) * 668265263u;
                            int   floors = 3 + (int)(hsh % 4);            // 3..6 andares
                            float FH     = 78.0f * sc;                    // pe-direito estilizado
                            float W      = (250.0f + (float)((hsh >> 5) & 15) * 5.0f) * sc;   // ~3,8x o heroi
                            float D      = (220.0f + (float)((hsh >> 9) & 15) * 4.0f) * sc;
                            float Hh     = FH * floors;
                            // OCLUSAO: mesma regra do branch de modelos .obj, mas com a
                            // janela baseada na PEGADA REAL do predio (a fixa 620/230
                            // nao escala com W/D/Hh). Player atras da massa (entre o
                            // predio e a camera, que olha de +Z) => predio translucido;
                            // senao o heroi some 100% atras dele no meio do combate
                            // (auditoria 2, P1 — oclusor dominante da fase urbana).
                            float odxB = fabsf(x - player.position.x);
                            float odzB = zz - player.position.y;
                            bool  occludes = (odzB > -D * 0.5f &&
                                              odzB <  Hh * 1.25f + D * 0.5f &&
                                              odxB <  W * 0.55f + 60.0f);
                            // PALETA por bioma (nao cinza universal): concreto quente
                            // e ocre em LA, concreto frio na cidade fantasma, metal
                            // queimado na forja. Cor amarrada ao lugar, como no D3.
                            // MATERIAL por predio, nao um bege universal com +-15 de
                            // variacao (era isso que fazia a cidade inteira ter a mesma
                            // cor). Cada predio sorteia um material real da rua.
                            ZoneID pz = tilemap.biomeAtWorld(x, zz);
                            const Color MAT_CITY[6] = {
                                { 148, 132, 104, 255 },   // reboco ocre
                                { 122, 118, 112, 255 },   // concreto cinza
                                { 132,  80,  62, 255 },   // tijolo vermelho
                                {  86, 104, 118, 255 },   // torre de vidro azulada
                                { 108,  96,  84, 255 },   // concreto sujo
                                {  74,  78,  84, 255 },   // aco escuro
                            };
                            const Color MAT_GHOST[6] = {
                                {  96, 104, 116, 255 }, {  78,  88, 100, 255 },
                                { 110, 112, 118, 255 }, {  70,  84,  96, 255 },
                                {  92,  90,  96, 255 }, {  62,  70,  80, 255 },
                            };
                            const Color MAT_FORGE[6] = {
                                { 128,  96,  74, 255 }, { 146, 110,  70, 255 },
                                { 104,  84,  70, 255 }, { 118,  78,  56, 255 },
                                {  96,  86,  78, 255 }, { 140, 120,  86, 255 },
                            };
                            const Color* MAT = (pz == ZoneID::GhostCity)   ? MAT_GHOST
                                             : (pz == ZoneID::KronosForge) ? MAT_FORGE
                                             : MAT_CITY;
                            Color base2 = MAT[(hsh >> 17) % 6];
                            float wear  = 0.86f + (float)((hsh >> 21) & 15) / 15.0f * 0.28f;
                            Color conc  = { (unsigned char)fminf(255.0f, base2.r * wear),
                                            (unsigned char)fminf(255.0f, base2.g * wear),
                                            (unsigned char)fminf(255.0f, base2.b * wear), 255 };
                            Color dark2  = { (unsigned char)(conc.r * 0.72f),
                                             (unsigned char)(conc.g * 0.72f),
                                             (unsigned char)(conc.b * 0.72f), 255 };
                            Color win    = { 44, 54, 64, 255 };
                            bool  lit    = (obj.tint.r > 128);            // predio com luz acesa
                            if (occludes) {   // alpha 0.30: o heroi le atraves da massa
                                conc  = ColorAlpha(conc,  0.30f);
                                dark2 = ColorAlpha(dark2, 0.30f);
                                win   = ColorAlpha(win,   0.30f);
                            }

                            if (occludes) rlDisableDepthMask();   // translucido NAO grava profundidade: o heroi atras continua desenhando
                            rlPushMatrix();
                            rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0.0f, 1.0f, 0.0f);
                            DrawCube({0.0f, 3.0f * sc, 0.0f}, W * 1.14f, 6.0f * sc, D * 1.14f, dark2); // calcada/base
                            DrawCube({0.0f, Hh * 0.5f, 0.0f}, W, Hh, D, conc);                          // massa
                            // fileiras de janela nas 4 faces (faixa por andar)
                            for (int f = 0; f < floors; ++f) {
                                float wy = FH * (f + 0.62f);
                                Color wc = win;
                                if (lit && ((hsh >> f) & 3) == 0) wc = Color{ 226, 198, 128, conc.a };
                                DrawCube({0.0f, wy, -D * 0.51f}, W * 0.76f, FH * 0.34f, 1.5f * sc, wc);
                                DrawCube({0.0f, wy,  D * 0.51f}, W * 0.76f, FH * 0.34f, 1.5f * sc, wc);
                                DrawCube({-W * 0.51f, wy, 0.0f}, 1.5f * sc, FH * 0.34f, D * 0.76f, wc);
                                DrawCube({ W * 0.51f, wy, 0.0f}, 1.5f * sc, FH * 0.34f, D * 0.76f, wc);
                            }
                            // RUINA: 40% dos predios perdem o topo. Silhueta quebrada
                            // e o que separa "cidade destruida" de "conjunto habitacional".
                            bool ruined = ((hsh >> 12) % 100) < 40;
                            if (ruined) {
                                // laje partida: 3 pedacos irregulares no lugar do topo
                                for (int rI = 0; rI < 3; ++rI) {
                                    float rw2 = W * (0.24f + (float)((hsh >> (rI * 3)) & 7) / 7.0f * 0.30f);
                                    float rh2 = FH * (0.30f + (float)((hsh >> rI) & 3) * 0.22f);
                                    float rx2 = ((float)((hsh >> (rI * 4)) & 15) / 15.0f - 0.5f) * W * 0.6f;
                                    float rz2 = ((float)((hsh >> (rI * 2)) & 15) / 15.0f - 0.5f) * D * 0.6f;
                                    DrawCube({ rx2, Hh + rh2 * 0.5f, rz2 }, rw2, rh2, rw2 * 0.8f, conc);
                                }
                                // buraco na laje: da pra ver o andar de baixo
                                DrawCube({ W * 0.10f, Hh - FH * 0.42f, -D * 0.10f },
                                         W * 0.44f, FH * 0.10f, D * 0.44f,
                                         Color{ (unsigned char)(conc.r * 0.42f),
                                                (unsigned char)(conc.g * 0.42f),
                                                (unsigned char)(conc.b * 0.46f), conc.a });
                                for (int pI = 0; pI < 3; ++pI)   // laje partida na borda do buraco
                                    DrawCube({ W * (0.10f + (pI - 1) * 0.20f), Hh + 2.0f * sc,
                                               -D * (0.10f + (pI - 1) * 0.16f) },
                                             W * 0.16f, 5.0f * sc, D * 0.16f, dark2);
                                // vigas expostas
                                for (int rI = 0; rI < 2; ++rI)
                                    DrawCylinderEx({ W * (rI ? 0.3f : -0.3f), Hh, D * 0.2f },
                                                   { W * (rI ? 0.42f : -0.42f), Hh + FH * 0.7f, D * 0.1f },
                                                   1.6f * sc, 1.0f * sc, 4, Color{ 122, 78, 48, conc.a });
                            } else {
                                // ── LAJE COMPLETA ────────────────────────────────
                                Color roofC = { (unsigned char)(conc.r * 0.80f),
                                                (unsigned char)(conc.g * 0.82f),
                                                (unsigned char)(conc.b * 0.86f), conc.a };
                                DrawCube({0.0f, Hh + 3.0f * sc, 0.0f}, W * 1.02f, 6.0f * sc, D * 1.02f, roofC);
                                // platibanda (mureta) nas 4 bordas: da espessura ao topo
                                float pb = 9.0f * sc;
                                DrawCube({0.0f, Hh + pb * 0.5f + 6.0f * sc, -D * 0.51f}, W * 1.06f, pb, 6.0f * sc, dark2);
                                DrawCube({0.0f, Hh + pb * 0.5f + 6.0f * sc,  D * 0.51f}, W * 1.06f, pb, 6.0f * sc, dark2);
                                DrawCube({-W * 0.51f, Hh + pb * 0.5f + 6.0f * sc, 0.0f}, 6.0f * sc, pb, D * 1.06f, dark2);
                                DrawCube({ W * 0.51f, Hh + pb * 0.5f + 6.0f * sc, 0.0f}, 6.0f * sc, pb, D * 1.06f, dark2);
                                float ry2 = Hh + 8.0f * sc;
                                // caixa d'agua sobre pes
                                float tkx = W * 0.24f, tkz = -D * 0.20f, tkr = 17.0f * sc;
                                for (int lI = 0; lI < 4; ++lI)
                                    DrawCylinderEx({ tkx + ((lI & 1) ? tkr*0.6f : -tkr*0.6f), ry2,
                                                     tkz + ((lI & 2) ? tkr*0.6f : -tkr*0.6f) },
                                                   { tkx + ((lI & 1) ? tkr*0.6f : -tkr*0.6f), ry2 + 14.0f*sc,
                                                     tkz + ((lI & 2) ? tkr*0.6f : -tkr*0.6f) },
                                                   2.0f*sc, 2.0f*sc, 4, dark2);
                                DrawCylinderEx({ tkx, ry2 + 14.0f*sc, tkz }, { tkx, ry2 + 40.0f*sc, tkz },
                                               tkr, tkr * 0.96f, 10, Color{ 128, 118, 104, conc.a });
                                DrawCylinderEx({ tkx, ry2 + 40.0f*sc, tkz }, { tkx, ry2 + 44.0f*sc, tkz },
                                               tkr * 1.08f, tkr * 0.5f, 10, dark2);
                                // casa de maquinas / saida de escada
                                DrawCube({ -W * 0.26f, ry2 + 15.0f * sc, D * 0.22f },
                                         W * 0.26f, 30.0f * sc, D * 0.24f, conc);
                                DrawCube({ -W * 0.26f, ry2 + 31.0f * sc, D * 0.22f },
                                         W * 0.28f, 4.0f * sc, D * 0.26f, dark2);
                                // condensadoras (caixas de ar-condicionado)
                                for (int aI = 0; aI < 3; ++aI) {
                                    float ax = (-0.30f + aI * 0.28f) * W;
                                    DrawCube({ ax, ry2 + 6.0f * sc, -D * 0.30f },
                                             20.0f * sc, 12.0f * sc, 16.0f * sc, Color{ 116, 116, 112, conc.a });
                                    DrawCylinderEx({ ax, ry2 + 12.0f * sc, -D * 0.30f },
                                                   { ax, ry2 + 14.0f * sc, -D * 0.30f },
                                                   6.0f * sc, 6.0f * sc, 8, dark2);
                                }
                                // dutos correndo pela laje
                                DrawCylinderEx({ -W * 0.34f, ry2 + 4.0f * sc, -D * 0.06f },
                                               {  W * 0.30f, ry2 + 4.0f * sc, -D * 0.06f },
                                               3.4f * sc, 3.4f * sc, 6, Color{ 104, 100, 94, conc.a });
                                // entulho e manchas na laje
                                for (int dI = 0; dI < 4; ++dI) {
                                    float dx2 = ((float)((hsh >> (dI * 3)) & 15) / 15.0f - 0.5f) * W * 0.8f;
                                    float dz2 = ((float)((hsh >> (dI * 2)) & 15) / 15.0f - 0.5f) * D * 0.8f;
                                    DrawCube({ dx2, ry2 + 2.0f * sc, dz2 },
                                             (8.0f + (float)((hsh >> dI) & 7)) * sc, 4.0f * sc,
                                             (7.0f + (float)((hsh >> dI) & 5)) * sc, dark2);
                                }
                            }
                            DrawCylinderEx({ W * 0.22f, Hh + 8.0f * sc,  D * 0.20f },
                                           { W * 0.22f, Hh + 30.0f * sc, D * 0.20f },
                                           9.0f * sc, 9.0f * sc, 8, dark2);
                            DrawCylinderEx({ -W * 0.26f, Hh + 8.0f * sc, -D * 0.24f },
                                           { -W * 0.26f, Hh + 46.0f * sc, -D * 0.24f },
                                           1.8f * sc, 1.0f * sc, 5, dark2);
                            rlPopMatrix();
                            if (occludes) rlEnableDepthMask();
                            w = W * 1.2f; h = Hh + 46.0f * sc;
                        } break;
                        case 6: { // VEICULO de GUERRA — sedã carbonizado / technical armado / caminhao militar
                            // Le por SILHUETA (cintura continua, capo baixo, para-brisa
                            // inclinado, pneus rentes) + cenário de GUERRA: lataria queimada,
                            // capo aberto com motor exposto, rodas estouradas/pisadas,
                            // pneus ausentes, esta de combate com arma montada na carroceria.
                            unsigned hsh = (unsigned)(x * 0.5f) * 2246822519u ^ (unsigned)(zz * 0.5f) * 3266489917u;
                            int   kind = (int)(hsh % 3);                 // 0 sedã 1 technical 2 caminhao
                            float L    = (kind == 2 ? 164.0f : kind == 1 ? 122.0f : 100.0f) * sc;
                            float BW   = (kind == 2 ?  60.0f : kind == 1 ?  54.0f :  50.0f) * sc;
                            float wr   = (kind == 2 ?  12.0f : kind == 1 ?  11.0f :  10.5f) * sc;  // pneu
                            float ww   = 7.0f * sc;                      // largura do pneu
                            float track= BW * 0.47f;                      // eixo quase rente a lataria

                            // ── paleta POR TIPO de guerra ──
                            Color body, dark, glass, tire, chrome, soot;
                            tire  = { 18, 18, 20, 255 };
                            chrome= { 128, 132, 140, 255 };
                            soot  = { 9, 9, 11, 255 };
                            if (kind == 2 || kind == 1) {                // militares
                                float mt = (float)((hsh >> 11) & 3);
                                if (mt == 0)      body = { 96, 102, 64, 255 };   // verde-oliva
                                else if (mt == 1) body = { 138, 124, 78, 255 };  // areia
                                else if (mt == 2) body = { 94, 98, 106, 255 };  // cinza de combate
                                else              body = { 84, 88, 98, 255 };
                                glass = { 48, 62, 74, 255 };
                            } else {                                       // civil carbonizado
                                static const Color CPAL[6] = { {178, 78, 60,255}, { 96,120,160,255},
                                                               {150,150,158,255}, {118,132, 92,255},
                                                               {186,166,108,255}, {112,114,120,255} };
                                Color bc = CPAL[(hsh >> 7) % 6];
                                float burn = 0.52f + (float)((hsh >> 13) & 7) / 7.0f * 0.14f;
                                body = { (unsigned char)(bc.r * burn), (unsigned char)(bc.g * burn),
                                         (unsigned char)(bc.b * burn), 255 };
                                glass = { 26, 28, 32, 255 };
                            }
                            dark = { (unsigned char)(body.r * 0.60f), (unsigned char)(body.g * 0.60f),
                                     (unsigned char)(body.b * 0.60f), 255 };

                            rlPushMatrix();
                            rlTranslatef(x, 0.0f, zz);
                            rlRotatef(obj.rotation * RAD2DEG, 0.0f, 1.0f, 0.0f);

                            float floorY = 6.0f * sc;                    // soleira / linha do assoalho
                            float bodyH  = (kind == 2 ? 16.0f : 13.0f) * sc;
                            float beltY  = floorY + bodyH;               // cintura (base dos vidros)
                            float roofH  = (kind == 2 ? 10.0f : kind == 1 ? 15.0f : 11.0f) * sc;
                            float sideW  = 6.5f * sc;                    // sainha lateral

                            // ── assoalho + sainhas (ancora no chao, mata o "flutuando") ──
                            float uL = L * 0.62f;
                            DrawCube({ -L * 0.05f, floorY * 0.55f, 0.0f }, uL, 4.6f * sc, BW * 0.52f,
                                     { 26, 27, 30, 255 });
                            DrawCube({ -L * 0.05f, floorY * 0.50f,  BW * 0.5f - sideW * 0.5f }, uL, 4.2f * sc, sideW, dark);
                            DrawCube({ -L * 0.05f, floorY * 0.50f, -BW * 0.5f + sideW * 0.5f }, uL, 4.2f * sc, sideW, dark);

                            // ── lataria em 3 secoes com a MESMA cintura (sem escadinha) ──
                            DrawCube({ -L * 0.05f, beltY - bodyH * 0.5f, 0.0f }, L * 0.46f, bodyH, BW, body);
                            DrawCube({  L * 0.30f, beltY - bodyH * 0.56f, 0.0f }, L * 0.30f, bodyH * 0.86f, BW * 0.90f, body);
                            DrawCube({ -L * 0.30f, beltY - bodyH * 0.53f, 0.0f }, L * 0.26f, bodyH * 0.92f, BW * 0.92f, body);
                            DrawCube({ 0.0f, beltY - 1.6f * sc, 0.0f }, L * 0.96f, 2.2f * sc, BW * 0.96f, chrome);

                            // ── cabine / carroceria ──
                            if (kind == 2) {   // caminhao militar: cabine na frente, bau com lona atras
                                float boxTB = beltY + 12.0f * sc;
                                float boxH  = 14.0f * sc;
                                DrawCube({ -L * 0.29f, boxTB + boxH * 0.5f, 0.0f }, L * 0.36f, boxH, BW * 0.88f, body);
                                // lonas/arcos do bau (camuflagem quebrada pelos frisos)
                                for (int rI = 0; rI < 3; ++rI) {
                                    float rx = -L * 0.44f + rI * 0.15f * L;
                                    DrawCube({ rx, boxTB + boxH + 4.0f * sc, 0.0f }, 2.5f * sc, 8.0f * sc, BW * 0.90f, dark);
                                }
                                DrawCube({ -L * 0.295f, boxTB + boxH -  4.5f * sc, 0.0f }, L * 0.37f, 2.0f * sc, BW * 0.94f, dark);
                                DrawCube({ -L * 0.295f, boxTB + boxH - 10.0f * sc, 0.0f }, L * 0.37f, 2.0f * sc, BW * 0.94f, dark);
                                // roda sobressalente apoiada na lateral do bau
                                DrawCylinderEx({ -L * 0.28f, boxTB + boxH * 0.55f, BW * 0.54f },
                                               { -L * 0.28f, boxTB + boxH * 0.55f + wr * 1.3f, BW * 0.54f },
                                               wr * 0.80f, wr * 0.80f, 12, tire);
                                // cabine frentista + para-brisa + teto
                                DrawCube({  L * 0.25f, beltY + 6.5f * sc, 0.0f }, L * 0.26f, 13.0f * sc, BW * 0.84f, body);
                                rlPushMatrix();
                                rlTranslatef(L * 0.32f, beltY + 13.0f * sc, 0.0f);
                                rlRotatef(-42.0f, 0.0f, 0.0f, 1.0f);
                                DrawCube({ 0.0f, 0.0f, 0.0f }, L * 0.07f, 12.0f * sc, BW * 0.76f, glass);
                                rlPopMatrix();
                                DrawCube({  L * 0.245f, beltY + 12.5f * sc, 0.0f }, L * 0.26f, 1.8f * sc, BW * 0.78f, dark);
                                // tambores de combustivel ao lado da cabine
                                for (int dI = 0; dI < 2; ++dI)
                                    DrawCylinderEx({ L * 0.10f + dI * 9.0f * sc, beltY + 4.0f * sc, BW * 0.50f + 6.0f * sc },
                                                   { L * 0.10f + dI * 9.0f * sc, beltY + 13.0f * sc, BW * 0.50f + 6.0f * sc },
                                                   4.5f * sc, 4.5f * sc, 10, { 70, 96, 58, 255 });
                            } else if (kind == 1) {   // technical: cabine na frente + carroceria ARMADA atras
                                float cabinX =  L * 0.15f;
                                float cabinL =  L * 0.30f;
                                float bedX   = -L * 0.18f;                // centro da carroceria
                                float bedYf  =  beltY + 5.5f * sc;        // assoalho da carroceria
                                // cabine: vidro na frente + teto
                                rlPushMatrix();
                                rlTranslatef(cabinX + cabinL * 0.5f, beltY + roofH * 0.50f, 0.0f);
                                rlRotatef(-34.0f, 0.0f, 0.0f, 1.0f);
                                DrawCube({ 0.0f, 0.0f, 0.0f }, L * 0.10f, roofH * 1.00f, BW * 0.62f, glass);
                                rlPopMatrix();
                                DrawCube({ cabinX, bedYf + roofH * 0.15f, 0.0f }, cabinL * 0.70f, roofH * 0.55f, BW * 0.60f, body);
                                // carroceria ABERTA (sem teto) com laterais e porta-malas
                                DrawCube({ bedX, bedYf - 4.5f * sc, 0.0f }, L * 0.62f, 6.0f * sc, BW * 0.88f, body);
                                DrawCube({ bedX, bedYf - 2.5f * sc,  BW * 0.46f }, L * 0.62f, 7.0f * sc, 2.2f * sc, dark);
                                DrawCube({ bedX, bedYf - 2.5f * sc, -BW * 0.46f }, L * 0.62f, 7.0f * sc, 2.2f * sc, dark);
                                DrawCube({ bedX - L * 0.31f, bedYf - 2.0f * sc, 0.0f }, 3.0f * sc, 7.0f * sc, BW * 0.80f, dark);
                                // AMETRALLHADORA montada na carroceria (tripé + cano + caixa de municao)
                                DrawCylinderEx({ bedX - 4.0f * sc, bedYf + 3.0f * sc, 0.0f },
                                               { bedX + 22.0f * sc, bedYf + 9.0f * sc, 0.0f },
                                               1.8f * sc, 1.8f * sc, 6, { 52, 54, 58, 255 });
                                DrawCube({ bedX + 24.0f * sc, bedYf + 7.0f * sc, 0.0f },
                                         5.0f * sc, 3.4f * sc, 2.4f * sc, { 52, 54, 58, 255 });
                                DrawCube({ bedX + 18.0f * sc, bedYf - 0.5f * sc, 0.0f },
                                         7.0f * sc, 4.0f * sc, 6.0f * sc, { 86, 70, 44, 255 });
                                // sacos de sandia reforcando a borda traseira
                                for (int sI = 0; sI < 3; ++sI)
                                    DrawSphereEx({ bedX - L * 0.28f + sI * 6.5f * sc, bedYf + 1.0f * sc,
                                                   (sI % 2 ? 6.0f : -6.0f) * sc }, 3.4f * sc, 6, 6, { 96, 82, 46, 255 });
                            } else {   // seda civil CARBONIZADO: capo aberto com motor exposto
                                float cabinX = -L * 0.05f;
                                float cabinL = L * 0.34f;
                                float roofY  = beltY + roofH * 0.96f;
                                // foco de fogo no capo: motor EXPOSTO (viga + blocos + tubos)
                                rlPushMatrix();                        // capo aberto levantado
                                rlTranslatef(L * 0.20f, beltY - 1.0f * sc, 0.0f);
                                rlRotatef(52.0f, 0.0f, 0.0f, 1.0f);
                                DrawCube({ 0.0f, 0.0f, 0.0f }, L * 0.20f, 2.2f * sc, BW * 0.72f, dark);
                                rlPopMatrix();
                                DrawCube({ L * 0.30f, beltY - 4.5f * sc, 0.0f }, L * 0.22f, 7.0f * sc, BW * 0.58f,
                                         { 74, 76, 82, 255 });          // bloco do motor
                                DrawCylinderEx({ L * 0.26f, beltY + 4.0f * sc,  8.0f * sc },
                                               { L * 0.26f, beltY + 9.0f * sc,  8.0f * sc }, 2.6f * sc, 2.6f * sc, 6, soot);
                                DrawCylinderEx({ L * 0.26f, beltY + 4.0f * sc, -8.0f * sc },
                                               { L * 0.26f, beltY + 9.0f * sc, -8.0f * sc }, 2.6f * sc, 2.6f * sc, 6, soot);
                                DrawCylinderEx({ L * 0.40f, beltY + 1.5f * sc, 0.0f },
                                               { L * 0.56f, beltY + 1.5f * sc, 0.0f }, 4.2f * sc, 4.2f * sc, 8, chrome);
                                // vidro quebrado: faixa escura, sem reflexo + interior sumido
                                DrawCube({ cabinX, beltY + roofH * 0.55f, 0.0f }, cabinL, roofH * 0.80f, BW * 0.58f, glass);
                                DrawCube({ cabinX, roofY - 1.4f * sc, 0.0f }, cabinL * 0.86f, 2.4f * sc, BW * 0.70f, soot);
                                rlPushMatrix();                        // para-brisa rachado
                                rlTranslatef(cabinX + cabinL * 0.46f, beltY + roofH * 0.50f, 0.0f);
                                rlRotatef(-34.0f, 0.0f, 0.0f, 1.0f);
                                DrawCube({ 0.0f, 0.0f, 0.0f }, L * 0.11f, roofH * 1.00f, BW * 0.60f, glass);
                                rlPopMatrix();
                                // queimadura: fumaça/fagulha - fumaça ondulada subindo
                                for (int fI = 0; fI < 4; ++fI) {
                                    float fy = roofY + 4.0f * sc + fI * 5.0f * sc;
                                    float fx = L * 0.30f + sinf(fI * 1.3f) * 6.0f * sc;
                                    DrawSphereEx({ fx, fy, 0.0f }, (5.0f - fI * 0.7f) * sc, 6, 6,
                                                 ColorAlpha(Color{ 20, 20, 22, 255 }, 0.28f - fI * 0.05f));
                                }
                            }

                            // ── rodas: pneus estourados, aros nus e arcos ──
                            for (int wI = 0; wI < 4; ++wI) {
                                float wx = (wI < 2 ? -L * 0.31f : L * 0.31f);
                                float wz = ((wI & 1) ? track : -track);
                                bool burnOut = ((hsh >> (6 + wI)) & 1) != 0;    // roda queimada/piso removida
                                if (!burnOut) {
                                    DrawCylinderEx({ wx, wr, wz - 2.5f * sc }, { wx, wr, wz + 2.5f * sc },
                                                   wr * 1.16f, wr * 1.16f, 12, dark);         // arco
                                    DrawCylinderEx({ wx, wr, wz - ww * 0.5f }, { wx, wr, wz + ww * 0.5f },
                                                   wr, wr, 12, tire);                        // pneu
                                    DrawCylinderEx({ wx, wr - 0.4f * sc, wz - ww * 0.62f }, { wx, wr - 0.4f * sc, wz + ww * 0.62f },
                                                   wr * 0.40f, wr * 0.40f, 8, chrome);       // calota
                                } else {
                                    DrawCylinderEx({ wx, wr * 0.55f, wz - ww * 0.5f }, { wx, wr * 0.55f, wz + ww * 0.5f },
                                                   wr * 0.70f, wr * 0.70f, 10, { 26, 26, 28, 255 });   // aro sem pneu
                                    DrawCylinderEx({ wx, wr * 0.55f, wz - ww * 0.62f }, { wx, wr * 0.55f, wz + ww * 0.62f },
                                                   wr * 0.30f, wr * 0.30f, 8, chrome);       // cubo
                                }
                            }

                            // ── frente de combate: faróis cegados/apagados + para-choque blindado ──
                            if (kind == 2 || kind == 1) {
                                DrawCube({  L * 0.495f, beltY - 1.5f * sc, 0.0f }, 3.0f * sc, 6.0f * sc, BW * 0.86f, dark);
                                DrawCube({ -L * 0.495f, beltY - 1.5f * sc, 0.0f }, 3.0f * sc, 6.0f * sc, BW * 0.86f, dark);
                            } else {
                                // farois estilhacados: so ocao escuro no lugar da lente
                                for (int sI = 0; sI < 2; ++sI) {
                                    float sz2 = (sI ? 1.0f : -1.0f) * BW * 0.28f;
                                    DrawSphereEx({ L * 0.49f, beltY - 1.0f * sc, sz2 }, 2.0f * sc, 6, 6, soot);
                                }
                            }
                            rlPopMatrix();
                            w = L; h = beltY + roofH + 40.0f * sc;
                        } break;
                        case 13: { // MARCAS NO CHAO: riscos, trilhas e cascalho
                            // A textura do piso tem 128px por tile de 64u: nessa
                            // distancia de camera o mipmap come o detalhe e o chao
                            // vira cinza chapado. Marcas em ESCALA DE MUNDO resolvem.
                            // NADA de disco escuro grande: circulo cheio no chao le
                            // como buraco/cratera, nao como sujeira.
                            if (lodFar) break;   // marca de chao so aparece perto
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            unsigned hsh = (unsigned)(x * 0.9f) * 2654435761u ^ (unsigned)(zz * 0.9f) * 2246822519u;
                            float R = 17.0f * sc;
                            rlDisableDepthMask();
                            if (hsh & 1) {                      // riscos/trilhas finas
                                for (int dI = 0; dI < 3; ++dI) {
                                    float a   = obj.rotation + dI * 0.22f;
                                    float len = R * (0.9f + (float)((hsh >> (dI*5)) & 7) / 7.0f * 0.5f);
                                    float off = (dI - 1) * R * 0.28f;
                                    float ox  = cosf(a) * len, oz = sinf(a) * len;
                                    float px2 = x - sinf(a) * off, pz2 = zz + cosf(a) * off;
                                    DrawCylinderEx({ px2-ox, 1.80f, pz2-oz }, { px2+ox, 1.83f, pz2+oz },
                                                   1.4f*sc, 0.7f*sc, 4, ColorAlpha(Color{16,15,14,255}, 0.13f));
                                }
                            } else {                            // cascalho: pontos CLAROS, nao mancha escura
                                for (int dI = 0; dI < 7; ++dI) {
                                    float a  = dI * 0.897f + (float)((hsh >> (dI*3)) & 7) * 0.24f;
                                    float rd = R * (0.25f + (float)((hsh >> (dI*2)) & 7) / 7.0f * 0.85f);
                                    float ox = cosf(a) * rd, oz = sinf(a) * rd;
                                    float pr = (1.6f + (float)((hsh >> dI) & 3) * 0.7f) * sc;
                                    DrawCylinderEx({ x+ox, 1.80f, zz+oz }, { x+ox, 1.84f, zz+oz },
                                                   pr, pr, 6, ColorAlpha(cp.gravel, 0.16f));
                                }
                            }
                            rlEnableDepthMask();
                            w = R * 2.0f; h = 0.0f;
                        } break;
                        case 11: { // grama: tufo denso, alturas/tons variados, ancorado no chao
                            // Antes: 5 laminas iguais, mesma altura, mesma cor, mesmo balanco —
                            // lia como espetinhos 2D flutuando. Agora o tufo tem base escura
                            // no chao, 9 laminas com altura/tom/fase proprios.
                            // COR POR BIOMA: grama verde no inferno era o maior delator de
                            // "mesmo mundo". Seca em LA/fantasma, queimada no inferno,
                            // cinza-purpura nas catacumbas, cyan no nexus.
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            float gh = 9.0f * sc, gr = 8.0f * sc;   // ~25% da altura do heroi (era 2x!)
                            float t  = (float)GetTime();
                            unsigned hsh = (unsigned)(x * 1.3f) * 374761393u ^ (unsigned)(zz * 1.3f) * 668265263u;
                            // mancha de terra/raiz: cola o tufo no piso (sem ela ele "flutua")
                            if (!lodFar)
                                DrawCylinderEx({ x, 0.13f, zz }, { x, 0.16f, zz }, gr*0.95f, gr*0.95f, 9,
                                               ColorAlpha(cp.grassDirt, 0.55f));
                            int blades = lodFar ? 4 : 9;   // LOD: longe nao da pra ver 9 laminas
                            for (int bld = 0; bld < blades; ++bld) {
                                float u  = (float)((hsh >> (bld * 2)) & 31) / 31.0f;   // 0..1 estavel
                                float a  = bld * 0.698f + u * 0.9f;
                                float rad = gr * (0.18f + u * 0.62f);
                                float ox = cosf(a) * rad, oz = sinf(a) * rad;
                                float bh = gh * (0.55f + u * 0.65f);                   // alturas diferentes
                                float sway = sinf(t * 1.7f + x * 0.05f + bld * 0.8f) * bh * 0.26f;
                                float k = 0.62f + u * 0.55f;                           // tons diferentes
                                Color gc = { (unsigned char)(cp.grass.r * k), (unsigned char)(cp.grass.g * k),
                                             (unsigned char)(cp.grass.b * k), 255 };
                                DrawCylinderEx({ x+ox, 0.0f, zz+oz },
                                               { x+ox+sway, bh, zz+oz+sway*0.35f },
                                               gr*0.085f, gr*0.012f, 3, gc);
                            }
                            w = gr * 2.0f; h = gh;
                        } break;
                        case 12: { // pedras/detritos: cluster facetado com tons variados
                            const ClutterPalette& cp = clutterPaletteFor(tilemap.biomeAtWorld(x, zz));
                            float pr = 7.0f * sc;
                            unsigned hsh = (unsigned)(x) * 2654435761u ^ (unsigned)(zz) * 40503u;
                            const float px2[5] = { 0.0f,  0.62f, -0.55f,  0.30f, -0.28f };
                            const float pz2[5] = { 0.0f,  0.30f,  0.22f, -0.58f, -0.40f };
                            const float ps [5] = { 0.68f, 0.44f,  0.40f,  0.32f,  0.26f };
                            for (int i = 0; i < (lodFar ? 2 : 5); ++i) {
                                float k = 0.74f + (float)((hsh >> (i * 3)) & 7) / 7.0f * 0.44f;
                                Color rc = { (unsigned char)(cp.rock.r * k), (unsigned char)(cp.rock.g * k),
                                             (unsigned char)(cp.rock.b * k), 255 };
                                // 5 segmentos = facetas visiveis (pedra), nao bolinha lisa
                                DrawSphereEx({ x + px2[i]*pr, pr*ps[i]*0.75f, zz + pz2[i]*pr },
                                             pr*ps[i], 5, 5, rc);
                            }
                            w = pr*2.0f; h = pr;
                        } break;
                        case 23: { // ESTANDE DE MERCADO (LUNA): balcao + lona listrada + mercadoria
                            float t2 = (float)GetTime();
                            float L = 92.0f * sc, W2 = 62.0f * sc, HH = 76.0f * sc;
                            Color wood2 = { 104, 78, 48, 255 }, darkT = { 70, 52, 34, 255 };
                            Color awning = { 226, 168, 66, 255 }, awning2 = { 122, 94, 44, 255 };
                            // 4 pes
                            for (int k2 = 0; k2 < 4; ++k2) {
                                float px = (k2 & 1 ? -1.0f : 1.0f) * L * 0.40f;
                                float pz = (k2 & 2 ? -1.0f : 1.0f) * W2 * 0.40f;
                                DrawCylinderEx({ x+px, 0, zz+pz }, { x+px, 20.0f, zz+pz }, 4.5f*sc, 4.5f*sc, 6, darkT);
                            }
                            DrawCubeV({ x, 56.0f*sc, zz }, { L, 18.0f*sc, W2 }, wood2);    // balcao
                            DrawCubeV({ x, 59.0f*sc, zz }, { L*0.92f, 8.0f*sc, W2*0.92f }, darkT);
                            for (int k2 = 0; k2 < 3; ++k2)                                 // mercadoria no balcao
                                DrawCubeV({ x + (k2 - 1) * L*0.26f, 74.0f*sc, zz },
                                          { 24.0f*sc, 9.0f*sc, 20.0f*sc }, { 140, 120, 84, 255 });
                            DrawCubeV({ x - L*0.30f, 74.0f*sc, zz + W2*0.40f }, { 26.0f*sc, 14.0f*sc, 18.0f*sc }, { 96, 86, 64, 255 });
                            for (int k2 = 0; k2 < 2; ++k2) {                              // mastros da lona
                                float mx = (k2 ? 1.0f : -1.0f) * L * 0.48f;
                                DrawCylinderEx({ x+mx, 0, zz }, { x+mx, 66.0f*sc, zz }, 3.5f*sc, 2.6f*sc, 6, wood2);
                            }
                            for (int strip = 0; strip < 4; ++strip)                       // lona listrada (viva)
                                DrawCubeV({ x - L*0.42f + strip * L*0.21f, 66.0f*sc + 4.0f*sc - (float)(strip & 1)*1.6f, zz },
                                          { L*0.20f, 4.0f*sc, W2*1.05f }, (strip & 1) ? awning : awning2);
                            DrawSphereEx({ x, 62.0f*sc, zz }, 5.5f*sc, 6, 6, ColorAlpha({255,214,150,255}, 0.55f + 0.45f*sinf(t2*2.4f)));
                            w = L * 1.15f; h = 80.0f * sc;
                        } break;
                        case 24: { // FORJA DO FERREIRO (KANE): bigorna + fornalha + mesa de metal
                            float t2 = (float)GetTime(), pl2 = 0.55f + 0.45f * sinf(t2 * 2.8f);
                            float L = 96.0f * sc;
                            Color stone2 = { 72, 70, 74, 255 }, metal2 = { 120, 116, 112, 255 }, hot = { 255, 120, 40, 255 };
                            DrawCubeV({ x, 8.0f*sc, zz }, { L, 16.0f*sc, L*0.70f }, stone2);          // base
                            DrawCubeV({ x - L*0.26f, 34.0f*sc, zz + 6.0f*sc }, { 22.0f*sc, 44.0f*sc, 16.0f*sc }, { 46,46,52,255 });
                            DrawCubeV({ x - L*0.26f, 52.0f*sc, zz + 6.0f*sc }, { 14.0f*sc, 12.0f*sc, 14.0f*sc }, metal2);
                            DrawCubeV({ x - L*0.26f, 58.0f*sc, zz + 6.0f*sc }, { 34.0f*sc, 6.0f*sc, 20.0f*sc }, metal2);   // bigorna
                            DrawCubeV({ x + L*0.22f, 40.0f*sc, zz }, { 40.0f*sc, 80.0f*sc, 36.0f*sc }, { 56, 52, 50, 255 }); // fornalha
                            DrawCubeV({ x + L*0.22f, 26.0f*sc, zz + 20.0f*sc }, { 26.0f*sc, 12.0f*sc, 10.0f*sc }, ColorAlpha(hot, pl2));
                            DrawSphereEx({ x + L*0.22f, 38.0f*sc, zz + 20.0f*sc }, 9.0f*sc, 6, 6, ColorAlpha(hot, 0.35f + 0.30f*pl2)); // brasa
                            for (int sI = 0; sI < 3; ++sI)                                                                  // chaminé
                                DrawCylinderEx({ x + L*0.22f, (80.0f + sI*10.0f)*sc, zz }, { x + L*0.22f, (96.0f + sI*10.0f)*sc, zz },
                                               (12.0f - sI*2.5f)*sc, (14.0f - sI*2.5f)*sc, 6, { 44, 42, 42, 255 });
                            DrawCubeV({ x + L*0.45f, 16.0f*sc, zz - L*0.28f }, { 36.0f*sc, 32.0f*sc, 30.0f*sc }, metal2);    // mesa
                            DrawCubeV({ x + L*0.54f, 26.0f*sc, zz - L*0.28f }, { 14.0f*sc, 16.0f*sc, 12.0f*sc }, { 150, 96, 40, 255 });
                            for (int sI = 0; sI < 4; ++sI)                                                                  // metal bruto
                                DrawCubeV({ x - L*0.35f + (sI%2)*10.0f*sc, (16.0f + sI%3*7.0f)*sc, zz - L*0.32f },
                                          { 16.0f*sc, 7.0f*sc, 12.0f*sc }, { 104, 100, 96, 255 });
                            DrawPlane({ x + L*0.22f, 0.30f, zz }, { 150.0f*sc, 120.0f*sc },
                                      ColorAlpha(Color{255,150,60,255}, 0.05f + 0.04f*pl2));                                 // luz da forja
                            w = L * 1.3f; h = 108.0f * sc;
                        } break;
                        case 25: { // POSTO DE COMANDO (VANCE RIOS): mesa + radio + estandarte NEXUS
                            float t2 = (float)GetTime(), pl2 = 0.5f + 0.5f * sinf(t2 * 1.8f);
                            float L = 100.0f * sc;
                            Color metal3 = { 90, 98, 110, 255 }, darkM = { 58, 64, 74, 255 };
                            Color banner = { 90, 200, 255, 255 }, bannerD = { 54, 120, 180, 255 };
                            for (int k2 = 0; k2 < 4; ++k2) {
                                float px = (k2 & 1 ? -1.0f : 1.0f) * L * 0.40f, pz = (k2 & 2 ? -1.0f : 1.0f) * 26.0f * sc;
                                DrawCylinderEx({ x+px, 0, zz+pz }, { x+px, 16.0f*sc, zz+pz }, 4.0f*sc, 4.0f*sc, 6, darkM);
                            }
                            DrawCubeV({ x, 20.0f*sc, zz }, { L, 8.0f*sc, 60.0f*sc }, metal3);    // mesa de comando
                            DrawCubeV({ x, 9.0f*sc, zz }, { L*0.92f, 20.0f*sc, 56.0f*sc }, darkM);
                            DrawPlane({ x, 24.6f*sc, zz }, { L*0.82f, 54.0f*sc }, ColorAlpha(bannerD, 0.5f));  // mapa luminescente
                            DrawPlane({ x, 25.2f*sc, zz }, { L*0.60f, 40.0f*sc }, ColorAlpha(banner, 0.35f + 0.20f*pl2));
                            DrawCylinderEx({ x - L*0.35f, 0, zz + 26.0f*sc }, { x - L*0.35f, 26.0f*sc, zz + 26.0f*sc }, 3.0f*sc, 1.6f*sc, 5, metal3); // radio
                            DrawSphereEx({ x - L*0.35f, 28.0f*sc, zz + 26.0f*sc }, 3.6f*sc, 8, 8, { 255, 90, 60, 255 });
                            float mx = x + L*0.34f;                                                                         // estandarte
                            DrawCylinderEx({ mx, 0, zz - 6.0f*sc }, { mx, 150.0f*sc, zz - 6.0f*sc }, 3.4f*sc, 2.2f*sc, 6, { 70, 76, 88, 255 });
                            DrawSphereEx({ mx, 154.0f*sc, zz - 6.0f*sc }, 4.5f*sc, 6, 6, { 220, 220, 226, 255 });
                            for (int sI = 0; sI < 3; ++sI) {
                                float sh = (float)sI / 3.0f, sway = sinf(t2 * 2.2f + sI) * 5.0f * sc;
                                DrawCubeV({ mx + 28.0f*sc + sway, (150.0f - sh*36.0f)*sc, zz - 6.0f*sc },
                                          { 56.0f*sc, 13.0f*sc, 2.5f*sc }, (sI & 1) ? banner : bannerD);
                            }
                            DrawPlane({ mx, 0.30f, zz - 6.0f*sc }, { 160.0f*sc, 160.0f*sc },
                                      ColorAlpha(bannerD, 0.06f + 0.05f*pl2));
                            w = L * 1.2f; h = 156.0f * sc;
                        } break;
                        case 26: { // LABORATORIO DE IMPLANTES (DR. CHEN): bancada + holoprojetor ciano
                            float t2 = (float)GetTime(), pl2 = 0.5f + 0.5f * sinf(t2 * 2.0f);
                            float L = 84.0f * sc;
                            Color lab = { 150, 156, 166, 255 }, labD = { 96, 102, 112, 255 };
                            Color glow = { 90, 220, 255, 255 };
                            for (int k2 = 0; k2 < 4; ++k2) {
                                float px = (k2 & 1 ? -1.0f : 1.0f) * L * 0.40f, pz = (k2 & 2 ? -1.0f : 1.0f) * 20.0f * sc;
                                DrawCylinderEx({ x+px, 0, zz+pz }, { x+px, 20.0f*sc, zz+pz }, 4.0f*sc, 4.0f*sc, 6, labD);
                            }
                            DrawCubeV({ x, 18.0f*sc, zz }, { L, 10.0f*sc, 46.0f*sc }, lab);     // bancada
                            DrawCubeV({ x, 36.0f*sc, zz }, { L*0.90f, 8.0f*sc, 42.0f*sc }, labD);
                            DrawCapsule({ x - L*0.26f, 40.0f*sc, zz }, { x - L*0.26f, 66.0f*sc, zz }, 10.0f*sc, 6, 6, { 60, 74, 86, 255 }); // tanque
                            DrawSphereEx({ x - L*0.26f, 40.0f*sc, zz }, 12.0f*sc, 6, 6, ColorAlpha(glow, 0.20f + 0.18f*pl2));
                            DrawCylinderEx({ x - L*0.26f, 66.0f*sc, zz }, { x - L*0.26f, 82.0f*sc, zz }, 3.4f*sc, 1.2f*sc, 6, labD);
                            DrawCubeV({ x + L*0.26f, 22.0f*sc, zz }, { 24.0f*sc, 12.0f*sc, 18.0f*sc }, labD);   // terminal
                            for (int sI = 0; sI < 4; ++sI) {                                                        // holoprojeção
                                float pr = (5.0f - sI * 0.8f) * sc, ph2 = (40.0f + sI * 12.0f) * sc;
                                DrawCylinderEx({ x + L*0.26f, ph2, zz }, { x + L*0.26f, ph2 + 8.0f*sc, zz }, pr, pr * 0.6f, 4,
                                               ColorAlpha(glow, 0.30f + 0.25f*pl2));
                            }
                            DrawPlane({ x + L*0.26f, 0.30f, zz }, { 140.0f*sc, 140.0f*sc },
                                      ColorAlpha(glow, 0.05f + 0.04f*pl2));
                            w = L * 1.1f; h = 92.0f * sc;
                        } break;
                        case 27: { // ARSENAL (ZARA): rack de armas + steels + caixa de municao
                            float t2 = (float)GetTime(), pl2 = 0.5f + 0.5f * sinf(t2 * 2.4f);
                            float L = 76.0f * sc;
                            Color steel = { 96, 102, 110, 255 }, steelD = { 62, 66, 74, 255 };
                            Color amber = { 255, 196, 90, 255 };
                            DrawCubeV({ x, 6.0f*sc, zz }, { L, 12.0f*sc, 44.0f*sc }, steelD);     // base
                            for (int k2 = 0; k2 < 2; ++k2) {
                                float px = (k2 ? -1.0f : 1.0f) * L * 0.44f;
                                DrawCylinderEx({ x+px, 12.0f*sc, zz }, { x+px, 96.0f*sc, zz }, 5.0f*sc, 3.6f*sc, 6, steel);
                            }
                            for (int sI = 0; sI < 3; ++sI)                                     // travessas do rack
                                DrawCubeV({ x, (96.0f - sI*26.0f)*sc, zz }, { L*0.78f, 5.0f*sc, 24.0f*sc }, steel);
                            for (int sI = 0; sI < 4; ++sI) {                                   // armas apoiadas (canos)
                                float ax = (sI - 1.5f) * L * 0.16f, lnn = (sI & 1 ? -1.0f : 1.0f);
                                DrawCylinderEx({ x+ax, (72.0f - (sI&1 ? 20.0f : 0.0f))*sc, zz + 10.0f*sc },
                                               { x+ax + lnn*8.0f*sc, (72.0f - (sI&1 ? 44.0f : 6.0f))*sc, zz + 10.0f*sc },
                                               2.4f*sc, 2.4f*sc, 6, (sI & 1) ? steelD : amber);
                            }
                            for (int k2 = 0; k2 < 2; ++k2)                                     // caixas de municao
                                DrawCubeV({ x + (k2 ? -1.0f : 1.0f)*L*0.30f, 10.0f*sc, zz + 26.0f*sc },
                                          { 26.0f*sc, 20.0f*sc, 18.0f*sc }, { 120, 112, 84, 255 });
                            DrawCylinderEx({ x + L*0.30f, 20.0f*sc, zz + 26.0f*sc }, { x + L*0.30f, 32.0f*sc, zz + 26.0f*sc },
                                           7.0f*sc, 7.0f*sc, 6, amber);
                            DrawSphereEx({ x, 102.0f*sc, zz }, 5.0f*sc, 8, 8, ColorAlpha(amber, 0.5f + 0.5f*pl2));  // luz de trabalho
                            w = L * 1.2f; h = 106.0f * sc;
                        } break;
                        default: if (hasSprite) { // fallback billboard so p/ tipos sem 3D
                            int variant = ((int)(obj.position.x*0.13f+obj.position.y*0.07f)) % SpriteBank::SCENERY_VARIANTS;
                            if (variant<0) variant+=SpriteBank::SCENERY_VARIANTS;
                            Texture2D tx=sb.scenery[obj.type][variant]; float K=1.7f*sc;
                            w=tx.width*K; h=tx.height*K;
                            Rectangle src={0.0f,0.0f,(float)tx.width,-(float)tx.height};
                            DrawBillboardRec(camera3D, tx, src, {x,h*0.5f,zz}, {w,h}, WHITE);
                        } break;
                    }
                }

                // Sombra de contato: era um RETANGULO preto chapado (borda dura,
                // formato errado). 3 discos concentricos com alpha decrescente dao
                // penumbra e assentam o objeto no chao.
                {
                    float sr = w * 0.46f;
                    if (sr > 1.0f && !(smallProp && lodFar)) {
                        const float rk[3] = { 1.00f, 0.68f, 0.40f };
                        const float ak[3] = { 0.13f, 0.15f, 0.17f };
                        for (int sI = 0; sI < 3; ++sI)
                            DrawCylinderEx({ obj.position.x, 2.20f + sI * 0.06f, obj.position.y },
                                           { obj.position.x, 2.24f + sI * 0.06f, obj.position.y },
                                           sr * rk[sI], sr * rk[sI], 14, ColorAlpha(BLACK, ak[sI]));
                    }
                }

                // Poste de luz: cone/poça de luz amarela no chão
                bool lights = (obj.tint.r > 128);
                if (lights && obj.type == 5) {
                    float fl = std::sin(time * 7.3f + obj.position.x) * 0.5f + std::sin(time * 2.1f + obj.position.y) * 0.5f;
                    float pulse = 0.55f + 0.30f * fl;
                    if (pulse < 0.2f) pulse = 0.2f;
                    DrawPlane({ obj.position.x, 0.14f, obj.position.y }, { w * 1.0f, h * 0.20f }, ColorAlpha(Color{255,210,130,255}, 0.07f * pulse));
                }
            }
        }

        // ── CIRCULO DE ENERGIA da ZONA SEGURA: a base le como perimetro protegido.
        //    Piso translucido + 2 aneis tracejados pulsando + balizas + varredura
        //    rotativa + coluna central. So rende perto do hub (custo ~0).
        if (openWorldMode) {
            float tt = (float)GetTime();
            const float hx2 = safeZoneCenter.x, hz2 = safeZoneCenter.y, RR = 345.0f;
            float pdx = player.position.x - hx2, pdz = player.position.y - hz2;
            if (pdx * pdx + pdz * pdz < 1250.0f * 1250.0f) {
                float pl = 0.55f + 0.45f * sinf(tt * 2.3f);
                Color cy = { 80, 210, 255, 255 }, amb = { 255, 200, 90, 255 };
                DrawCylinderEx({ hx2, 0.28f, hz2 }, { hx2, 0.30f, hz2 }, RR, RR, 48, ColorAlpha(cy, 0.05f));
                DrawCylinderEx({ hx2, 0.30f, hz2 }, { hx2, 0.32f, hz2 }, RR * 0.97f, RR * 0.97f, 48, ColorAlpha(amb, 0.05f));
                for (int ring = 0; ring < 2; ++ring) {                                  // aneis tracejados
                    float r0 = RR + ring * 18.0f;
                    float a0 = ring * 0.7f + tt * (ring ? -0.28f : 0.22f);
                    Color rc = ring ? amb : cy;
                    for (int k2 = 0; k2 < 36; ++k2) {
                        float a1 = a0 + (float)k2 * (2.0f * PI / 36.0f);
                        float a2 = a1 + (2.0f * PI / 36.0f) * 0.55f;
                        Vector3 v1 = { hx2 + cosf(a1) * r0, 3.0f, hz2 + sinf(a1) * r0 };
                        Vector3 v2 = { hx2 + cosf(a2) * r0, 3.0f, hz2 + sinf(a2) * r0 };
                        DrawLine3D(v1, v2, ColorAlpha(rc, 0.20f + 0.30f * pl * (ring ? 0.6f : 1.0f)));
                    }
                }
                for (int k2 = 0; k2 < 8; ++k2) {                                        // balizas
                    float a = (float)k2 * (2.0f * PI / 8.0f);
                    float bx2 = hx2 + cosf(a) * RR, bz2 = hz2 + sinf(a) * RR;
                    DrawCylinderEx({ bx2, 0, bz2 }, { bx2, 22.0f, bz2 }, 3.0f, 2.0f, 6, { 46, 58, 66, 255 });
                    DrawSphereEx({ bx2, 25.0f, bz2 }, 3.6f, 8, 8, ColorAlpha(cy, 0.45f + 0.55f * pl));
                }
                float rot = tt * 0.5f;                                                  // varredura
                DrawLine3D({ hx2, 2.0f, hz2 }, { hx2 + cosf(rot) * RR, 2.0f, hz2 + sinf(rot) * RR },
                           ColorAlpha(cy, 0.30f + 0.25f * pl));
                DrawLine3D({ hx2, 2.0f, hz2 }, { hx2 + cosf(rot + PI) * RR * 0.8f, 2.0f, hz2 + sinf(rot + PI) * RR * 0.8f },
                           ColorAlpha(amb, 0.25f + 0.20f * pl));
                DrawCylinderEx({ hx2, 0.0f, hz2 }, { hx2, 60.0f + pl * 18.0f, hz2 }, 4.0f, 1.0f, 8,
                               ColorAlpha(cy, 0.18f));                                  // coluna central (marco)
                DrawSphereEx({ hx2, 64.0f + pl * 18.0f, hz2 }, 7.0f, 8, 8, ColorAlpha(cy, 0.30f));
            }
        }

        // ── Sombras REDONDAS suaves no chão (disco achatado, não retângulo) ──
        auto shadow = [](Vector2 pos, float r, float a) {
            DrawCylinderEx({ pos.x, 0.10f, pos.y }, { pos.x, 0.118f, pos.y }, r, r, 16, ColorAlpha(BLACK, a));
        };
        // player/inimigos/NPCs/companheiros têm SOMBRA PROJETADA (silhueta) no drawVoxel
        if (netActive)
            for (const auto& p : net.peers()) shadow({ p.x, p.y }, 11.0f, 0.34f);

        for (auto& it : items) shadow(it.position, 5.5f, 0.26f);

        for (const auto& a : animals) {
            if (std::fabs(a.position.x - camera.target.x) > 1100 || std::fabs(a.position.y - camera.target.y) > 700) continue;
            shadow(a.position, 6.0f, 0.26f);
        }

        // Sombras dos nós de recursos
        for (const auto& n : resourceNodes) {
            if (n.depleted) continue;
            if (std::fabs(n.position.x - camera.target.x) > 1100 || std::fabs(n.position.y - camera.target.y) > 700) continue;
            DrawPlane({ n.position.x, 0.1f, n.position.y }, { 24.0f, 12.0f }, ColorAlpha(BLACK, 0.35f));
        }

        // Sombras dos equipamentos no chão
        for (const auto& ge : groundEquips) {
            if (ge.collected) continue;
            if (std::fabs(ge.position.x - camera.target.x) > 1100 || std::fabs(ge.position.y - camera.target.y) > 700) continue;
            DrawPlane({ ge.position.x, 0.1f, ge.position.y }, { 16.0f, 8.0f }, ColorAlpha(BLACK, 0.3f));
        }

        // Sombras das construções
        for (const auto& b : buildingSystem.buildings) {
            if (!b.built) continue;
            DrawPlane({ b.position.x, 0.1f, b.position.y }, { 80.0f, 40.0f }, ColorAlpha(BLACK, 0.35f));
        }


        // ── Desenho dos Billboards 3D Reais (com oclusão e depth buffer) ──

        // ── BARREIRA DA FASE ──────────────────────────────────────────────────
        // So o arco proximo do jogador e desenhado: a borda inteira seriam
        // centenas de painentes fora de tela. Ela aparece de longe pra o jogador
        // entender que o mundo da fase TEM fim - e o que faz virar "fase".
        if (openWorldMode) {
            Vector2 rel = { player.position.x - safeZoneCenter.x,
                            player.position.y - safeZoneCenter.y };
            float pd = sqrtf(rel.x*rel.x + rel.y*rel.y);
            if (pd > owPhaseRadius - 1500.0f) {
                float base = atan2f(rel.y, rel.x);
                float tt   = (float)GetTime();
                for (int i = -9; i <= 9; ++i) {
                    float a  = base + i * 0.045f;
                    float bx = safeZoneCenter.x + cosf(a) * owPhaseRadius;
                    float bz = safeZoneCenter.y + sinf(a) * owPhaseRadius;
                    float pulse = 0.16f + 0.10f * sinf(tt * 2.0f + i * 0.7f);
                    // painel vertical de energia
                    DrawCylinderEx({ bx, 0.0f, bz }, { bx, 210.0f, bz }, 26.0f, 20.0f, 6,
                                   ColorAlpha(Color{ 70, 190, 255, 255 }, pulse));
                    DrawCylinderEx({ bx, 0.0f, bz }, { bx, 8.0f, bz }, 30.0f, 30.0f, 8,
                                   ColorAlpha(Color{ 140, 230, 255, 255 }, 0.35f));
                }
            }
        }

        // ── PORTAL DA FASE ────────────────────────────────────────────────────
        if (openWorldMode && owPortalOpen) {
            float px = owPortalPos.x, pz = owPortalPos.y;
            float t  = (float)GetTime();
            // base/plataforma
            DrawCylinderEx({px, 0.10f, pz}, {px, 4.0f, pz}, 62.0f, 58.0f, 24,
                           Color{38, 44, 58, 255});
            // anel girando (3 aros inclinados)
            for (int r = 0; r < 3; ++r) {
                float rr = 46.0f - r * 7.0f;
                float yy = 30.0f + r * 26.0f;
                float ph = t * (1.1f + r * 0.35f);
                for (int seg = 0; seg < 16; ++seg) {
                    float a0 = seg * 0.3927f + ph, a1 = a0 + 0.28f;
                    DrawCylinderEx({px + cosf(a0)*rr, yy + sinf(a0)*4.0f, pz + sinf(a0)*rr},
                                   {px + cosf(a1)*rr, yy + sinf(a1)*4.0f, pz + sinf(a1)*rr},
                                   3.2f, 3.2f, 5, Color{0, 220, 255, 255});
                }
            }
            // coluna de energia
            for (int c = 0; c < 5; ++c) {
                float k = 1.0f - c * 0.17f;
                float pulse = 0.6f + 0.4f * sinf(t * 3.0f + c);
                DrawCylinderEx({px, 4.0f, pz}, {px, 4.0f + 150.0f * k, pz},
                               34.0f * k, 10.0f * k, 14,
                               ColorAlpha(Color{90, 210, 255, 255}, 0.16f * pulse));
            }
            lightSystem.addLight(owPortalPos, 300.0f, 0.9f, Color{80,210,255,255}, true);
        }

        // Player — modelo voxel 3D do PRÓPRIO personagem do jogo (não genérico).
        // usa a VELOCIDADE real, NAO player.isMoving: Player::update zera a flag
        // depois do handleInput (que move o bot), entao a flag chega SEMPRE false
        // no render → pose 0 congelada + sem passo = personagem deslizando/flutuando.
        Vector2 pv = player.velocity;
        bool pMoving = (pv.x*pv.x + pv.y*pv.y) > 144.0f;   // > 12 px/s
        drawVoxel(1000 + (int)player.charClass * 1000 + player.visualSignature(),
                  player.position, 0.0f, player.walkAnimTimer, pMoving);

        // ── PICARETA: golpe animado quando minerando um nó perto ──
        if (mineFxIdx >= 0 && mineFxIdx < (int)resourceNodes.size() && !resourceNodes[mineFxIdx].depleted) {
            const auto& mn = resourceNodes[mineFxIdx];
            Vector2 dir = { mn.position.x - player.position.x, mn.position.y - player.position.y };
            float dl = std::sqrt(dir.x*dir.x + dir.y*dir.y); if (dl < 1.0f) { dir = {1,0}; dl = 1; }
            dir.x /= dl; dir.y /= dl;
            float sw  = 1.0f - mineSwingAnim;                 // 0=erguida, 1=batendo
            float ang = 1.15f - sw * 1.65f;                   // arco: ergue (+) → desce (-)
            float L = 28.0f;
            Vector3 piv = { player.position.x + dir.x*10.0f, 22.0f, player.position.y + dir.y*10.0f };
            Vector3 tip = { piv.x + dir.x * L * cosf(ang), piv.y + L * sinf(ang), piv.z + dir.y * L * cosf(ang) };
            DrawCylinderEx(piv, tip, 1.7f, 1.4f, 6, Color{120, 78, 40, 255});           // cabo
            DrawCubeV(tip, {9.0f, 4.0f, 4.0f}, Color{180, 185, 195, 255});               // cabeça (metal)
            // Impacto no nó no momento do golpe
            if (sw > 0.7f) {
                Vector3 ip = { mn.position.x, 12.0f, mn.position.y };
                DrawSphereEx(ip, 4.0f + (sw-0.7f)*14.0f, 6, 6, ColorAlpha(Color{255,245,200,255}, 0.6f));
            }
        }

        // Culling de entidades — o caminho 2D sempre teve (inView); o 3D nao tinha
        // NENHUM, e drawVoxel desenha 2x (silhueta de sombra + modelo). Mesmos
        // limites usados pelos civis.
        auto offScreen = [&](Vector2 p) {
            return std::fabs(p.x - camera.target.x) > 1200.0f ||
                   std::fabs(p.y - camera.target.y) > 800.0f;
        };

        // NPCs — modelos 3D próprios do jogo
        for (auto& n : npcs) {
            if (offScreen(n.position)) continue;
            drawVoxel(300 + (int)n.role, n.position, 0.0f);   // NPC de posto: parado
        }

        // Civis da cidade (cada um na sua tarefa) — com culling pra perto da câmera
        for (auto& f : cityFolk) {
            if (std::fabs(f.position.x - camera.target.x) > 1200 ||
                std::fabs(f.position.y - camera.target.y) > 800) continue;
            drawVoxel(300 + f.role, f.position, 0.0f, f.walkPhase, !f.atStation);
            // Trabalhador martelando: faísca pulsante (sinal de "fazendo algo")
            if (f.job == FolkJob::Worker && f.atStation) {
                float ph = std::sin(f.work * 9.0f);
                if (ph > 0.2f) {
                    Vector3 sp = { f.position.x + 10.0f * f.facing, 30.0f + ph * 4.0f, f.position.y };
                    DrawSphereEx(sp, 2.2f, 5, 5, Color{255, 210, 90, 255});
                    DrawSphereEx(sp, 3.6f, 5, 5, ColorAlpha(Color{255,160,40,255}, 0.4f));
                }
            }
        }

        // Companheiros — modelos 3D próprios do jogo
        for (auto& c : companions) {
            if (!c.active || offScreen(c.position)) continue;
            drawVoxel(500 + (int)c.type, c.position, 0.0f, c.walkTimer, true);
        }

        // Inimigos — modelos 3D próprios do jogo (cada tipo com sua silhueta)
        for (auto& e : enemies) {
            if (offScreen(e.position)) continue;
            drawVoxel(100 + (int)e.type, e.position, 0.0f, e.walkAnimTimer, true);
        }

        // Itens — modelos 3D próprios do jogo
        for (auto& it : items) {
            if (offScreen(it.position)) continue;
            it.render3D();
        }

        // Outros jogadores (Peers)
        if (netActive) {
            static const Color cols[6] = {
                {60,120,220,255},{220,80,140,255},{150,160,175,255},
                {120,80,220,255},{180,120,255,255},{200,130,60,255}
            };
            for (const auto& p : net.peers()) {
                Color c = cols[(p.charClass >= 0 && p.charClass < 6) ? p.charClass : 0];
                DrawCylinderEx({p.x, 0.12f, p.y}, {p.x, 0.13f, p.y}, 11.0f, 11.0f, 12, ColorAlpha(BLACK, 0.34f));
                DrawCapsule({p.x, 6.0f, p.y}, {p.x, 34.0f, p.y}, 7.0f, 8, 8, c);
                DrawSphereEx({p.x, 44.0f, p.y}, 8.0f, 8, 8, c);
            }
        }

        // Animais / Vida Selvagem
        for (const auto& a : animals) {
            if (std::fabs(a.position.x - camera.target.x) > 1100 || std::fabs(a.position.y - camera.target.y) > 700) continue;
            {
                float x = a.position.x, zz = a.position.y;
                float bob = std::sin(a.animTimer * 8.0f) * 1.2f;
                DrawCylinderEx({x,0.12f,zz},{x,0.13f,zz}, 12.0f,12.0f,10, ColorAlpha(BLACK,0.30f));
                switch (a.type) {
                    case AnimalType::Deer: {
                        Color body={150,110,70,255};
                        DrawCapsule({x-9,16.0f+bob,zz},{x+9,16.0f+bob,zz}, 6.0f,8,8, body);
                        DrawSphereEx({x+12,24.0f+bob,zz}, 5.0f,7,7, body);
                        DrawCylinderEx({x+12,28.0f,zz},{x+10,36.0f,zz}, 1.2f,0.4f,5, Color{90,60,30,255});
                        DrawCylinderEx({x+14,28.0f,zz},{x+16,36.0f,zz}, 1.2f,0.4f,5, Color{90,60,30,255});
                        for(int lg=0;lg<4;++lg){float lx=x+(lg<2?-7:7),lz=zz+((lg%2)?4:-4);DrawCylinderEx({lx,0,lz},{lx,12.0f,lz},1.6f,1.6f,5,Color{110,80,50,255});}
                    } break;
                    case AnimalType::Rabbit: {
                        Color body={210,200,190,255};
                        DrawSphereEx({x,6.0f+bob,zz}, 6.0f,7,7, body);
                        DrawCapsule({x-2,10.0f,zz},{x-2,17.0f,zz}, 1.6f,5,5, body);
                        DrawCapsule({x+2,10.0f,zz},{x+2,17.0f,zz}, 1.6f,5,5, body);
                    } break;
                    case AnimalType::Boar: {
                        Color body={90,70,60,255};
                        DrawCapsule({x-10,9.0f+bob,zz},{x+8,9.0f+bob,zz}, 7.0f,8,8, body);
                        DrawSphereEx({x+12,9.0f+bob,zz}, 5.0f,7,7, body);
                        for(int lg=0;lg<4;++lg){float lx=x+(lg<2?-6:6),lz=zz+((lg%2)?4:-4);DrawCylinderEx({lx,0,lz},{lx,6.0f,lz},1.8f,1.8f,5,Color{70,55,48,255});}
                    } break;
                    case AnimalType::Wolf: {
                        Color body=a.fleeing?Color{120,120,130,255}:Color{90,95,105,255};
                        DrawCapsule({x-10,11.0f+bob,zz},{x+8,11.0f+bob,zz}, 5.0f,8,8, body);
                        DrawSphereEx({x+12,14.0f+bob,zz}, 4.5f,7,7, body);
                        for(int lg=0;lg<4;++lg){float lx=x+(lg<2?-7:7),lz=zz+((lg%2)?4:-4);DrawCylinderEx({lx,0,lz},{lx,9.0f,lz},1.5f,1.5f,5,body);}
                    } break;
                    default: {
                        float fl = std::sin(a.animTimer*12.0f)*5.0f;
                        Color body={60,60,70,255};
                        DrawSphereEx({x,34.0f+bob*2,zz}, 3.2f,6,6, body);
                        DrawCapsule({x-7,34.0f+fl,zz},{x,34.0f,zz}, 1.4f,5,5, body);
                        DrawCapsule({x+7,34.0f+fl,zz},{x,34.0f,zz}, 1.4f,5,5, body);
                    } break;
                }
            }
        }

        // Nós de Recursos Naturais
        for (int i = 0; i < (int)resourceNodes.size(); ++i) {
            const auto& n = resourceNodes[i];
            if (n.depleted) continue;
            if (std::fabs(n.position.x - camera.target.x) > 1100 || std::fabs(n.position.y - camera.target.y) > 700) continue;

            {
                float sx = (n.shake > 0.0f) ? std::sin(n.shake * 30.0f) * 2.0f : 0.0f;
                float x = n.position.x + sx, zz = n.position.y;
                Color c = resourceColor(n.type);
                DrawCylinderEx({x, 0.12f, zz}, {x, 0.14f, zz}, 14.0f, 14.0f, 12, ColorAlpha(BLACK, 0.35f));
                if (n.type == ResourceType::Wood) {
                    DrawCylinderEx({x,0,zz},{x,26.0f,zz}, 5.0f, 3.5f, 8, Color{82,56,30,255});
                    DrawSphereEx({x, 40.0f, zz}, 20.0f, 8, 8, Color{30,86,42,255});
                    DrawSphereEx({x-12, 32.0f, zz}, 13.0f, 8, 8, Color{26,72,36,255});
                    DrawSphereEx({x+12, 34.0f, zz}, 14.0f, 8, 8, Color{36,96,46,255});
                } else if (n.type == ResourceType::Stone) {
                    DrawSphereEx({x, 9.0f, zz}, 14.0f, 8, 8, Color{120,120,128,255});
                    DrawSphereEx({x-8, 6.0f, zz+5}, 9.0f, 7, 7, Color{145,145,155,255});
                    DrawSphereEx({x+8, 5.0f, zz-4}, 8.0f, 7, 7, Color{100,100,110,255});
                } else {
                    DrawSphereEx({x, 9.0f, zz}, 14.0f, 8, 8, Color{80,72,66,255});
                    DrawSphereEx({x-5, 6.0f, zz+3}, 8.0f, 7, 7, Color{96,88,80,255});
                    for (int v = 0; v < 6; ++v) { float a = v * 1.05f + i;
                        DrawSphereEx({x + cosf(a)*9.0f, 12.0f + sinf(a)*4.0f, zz + sinf(a)*9.0f}, 2.8f, 5, 5, c); }
                }
            }
        }

        // Equipamentos no chão
        for (const auto& ge : groundEquips) {
            if (ge.collected) continue;
            if (std::fabs(ge.position.x - camera.target.x) > 1100 || std::fabs(ge.position.y - camera.target.y) > 700) continue;

            {
                float pulse = 0.5f + 0.5f * std::sin(ge.pulseTimer * 4.0f);
                Color ec = ge.equip.color;
                float fade = (ge.lifetime < 5.0f) ? ge.lifetime / 5.0f : 1.0f;
                float gy = 13.0f + std::sin(ge.pulseTimer * 2.0f) * 3.0f;   // gema flutua suave
                DrawCylinderEx({ge.position.x,0.12f,ge.position.y},{ge.position.x,0.14f,ge.position.y},
                               14.0f + pulse*4.0f, 14.0f + pulse*4.0f, 16, ColorAlpha(ec, 0.30f * fade));
                DrawSphereEx({ge.position.x, gy, ge.position.y}, 9.0f, 8, 8, ColorAlpha(ec, 0.40f * fade));
                DrawSphereEx({ge.position.x, gy, ge.position.y}, 5.5f, 8, 8, ColorAlpha(ec, fade));
                DrawSphereEx({ge.position.x, gy + 1.5f, ge.position.y}, 2.5f, 6, 6, ColorAlpha(WHITE, 0.8f * fade));
            }
        }

        // Construções, Tanques e Soldados (Building System / RTS)
        // Arca/Casa: modelos MEDIEVAIS (castle.obj/house.obj) so em zona rural/
        // gotica. Em zona urbana/sci-fi a Arca vira bunker de concreto com
        // antenas e a Casa vira estrutura moderna (auditoria P1: telhado de
        // telha e torre de castelo no asfalto quebravam a cidade).
        const bool medievalZone = isMedievalZone(currentZone);
        for (const auto& b : buildingSystem.buildings) {
            if (m_modelsLoaded && b.built) {
                if (b.type == BuildingType::Ark && medievalZone && m_castleModel.meshCount > 0) {
                    DrawModelEx(m_castleModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_castleScale, m_castleScale, m_castleScale }, WHITE);
                } else if (b.type == BuildingType::Ark && !medievalZone) {
                    drawArkStructure(b.position);
                } else if (b.type == BuildingType::House && medievalZone && m_houseModel.meshCount > 0) {
                    DrawModelEx(m_houseModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_houseScale, m_houseScale, m_houseScale }, WHITE);
                } else if (b.type == BuildingType::House && !medievalZone) {
                    drawGenericStructure(b.position, 1.0f);   // casa moderna (concreto+metal)
                } else if (b.type == BuildingType::Barracks && m_barracksModel.meshCount > 0) {
                    DrawModelEx(m_barracksModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_barracksScale, m_barracksScale, m_barracksScale }, WHITE);
                } else if (b.type == BuildingType::Turret && m_turretModel.meshCount > 0) {
                    DrawModelEx(m_turretModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_turretScale, m_turretScale, m_turretScale }, WHITE);
                } else if (b.type == BuildingType::TankFactory && m_marketModel.meshCount > 0) {
                    DrawModelEx(m_marketModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_marketScale, m_marketScale, m_marketScale }, WHITE);
                } else if (b.type == BuildingType::MedBay && m_wellModel.meshCount > 0) {
                    DrawModelEx(m_wellModel, { b.position.x, 0.0f, b.position.y }, { 0.0f, 1.0f, 0.0f }, 0.0f, { m_wellScale, m_wellScale, m_wellScale }, WHITE);
                } else if (b.type == BuildingType::Wall) {
                    SpriteBank& sb = SpriteBank::get();
                    if (sb.ready) {
                        DrawCubeTexture(sb.tileWall[(int)currentZone], { b.position.x, 32.0f, b.position.y }, 64.0f, 64.0f, 64.0f, WHITE);
                    } else {
                        DrawCube({ b.position.x, 32.0f, b.position.y }, 64.0f, 64.0f, 64.0f, GRAY);
                    }
                } else {
                    drawGenericStructure(b.position, 1.0f);
                }
            } else {
                drawGenericStructure(b.position, 1.0f);
            }
        }
        // Tanque com escala e forma de tanque: 96u de casco contra 66u de heroi.
        // Os 28u antigos faziam o "tanque" caber embaixo do braco do personagem.
        for (const auto& t : buildingSystem.tanks) {
            if (t.isDead()) continue;
            const Color HULL = { 78, 96, 74, 255 };
            const Color TRK  = { 44, 50, 44, 255 };
            const Color TUR  = { 92, 112, 88, 255 };
            float tx = t.position.x, tz = t.position.y;
            DrawCylinderEx({tx, 0.12f, tz}, {tx, 0.13f, tz}, 46.0f, 40.0f, 16,
                           ColorAlpha(BLACK, 0.34f));
            DrawCylinderEx({tx, 0.14f, tz}, {tx, 0.15f, tz}, 20.0f, 16.0f, 14,
                           ColorAlpha(Color{60, 230, 120, 255}, 0.30f));   // marca de aliado
            for (int sI = 0; sI < 2; ++sI)                                  // esteiras
                DrawCubeV({tx, 11.0f, tz + (sI ? 30.0f : -30.0f)}, {96.0f, 22.0f, 18.0f}, TRK);
            DrawCubeV({tx, 20.0f, tz}, {92.0f, 20.0f, 62.0f}, HULL);        // casco
            DrawCubeV({tx, 34.0f, tz}, {52.0f, 18.0f, 44.0f}, TUR);         // torre
            DrawCylinderEx({tx + 20.0f, 36.0f, tz}, {tx + 74.0f, 36.0f, tz},
                           5.0f, 4.0f, 8, TRK);                              // canhao
            DrawSphereEx({tx - 14.0f, 46.0f, tz}, 5.0f, 6, 6, TUR);          // escotilha
        }
        // Aliados usam o MESMO modelo dos NPCs (soldado). Antes eram uma capsula
        // verde com uma bola bege por cabeca: um "pino" andando pelo cenario, sem
        // nenhuma relacao com o resto da arte do jogo.
        for (const auto& s : buildingSystem.soldiers) {
            if (s.isDead()) continue;
            // anel verde no chao = marca de ALIADO (o modelo e o mesmo dos NPCs)
            DrawCylinderEx({s.position.x, 0.12f, s.position.y},
                           {s.position.x, 0.13f, s.position.y}, 15.0f, 12.0f, 14,
                           ColorAlpha(Color{60, 230, 120, 255}, 0.35f));
            drawVoxel(300 + (int)NPCRole::Soldier, s.position, 0.0f,
                      (float)GetTime() * 6.0f + s.position.x * 0.05f, true);
        }

        // RTS Building Preview Ghost
        if (buildingSystem.buildModeActive) {
            Vector2 mouseWorld = mouseGround3D();
            {
                BuildingType preview = static_cast<BuildingType>(buildingSystem.selectedType); (void)preview;
                bool canPlace = true;
                for (const auto& b : buildingSystem.buildings) {
                    if (Vector2Distance(b.position, mouseWorld) < 80.f) { canPlace = false; break; }
                }
                Color pc = canPlace ? Color{0,255,100,255} : Color{255,50,50,255};
                DrawPlane({mouseWorld.x, 0.2f, mouseWorld.y}, {64.0f, 64.0f}, ColorAlpha(pc, 0.25f));
                DrawCubeWires({mouseWorld.x, 32.0f, mouseWorld.y}, 64.0f, 64.0f, 64.0f, pc);
            }
        }

        // ── VIDA AMBIENTE: partículas flutuando (poeira/brasas/pólen) por TEMA ──
        {
            float t = (float)GetTime();
            Color mc;
            switch (currentZone) {
                case ZoneID::InfernoZone: case ZoneID::KronosForge:
                    mc = {255,150,60,255}; break;                                  // brasas
                case ZoneID::Cemetery: case ZoneID::GhostCity: case ZoneID::AbandonedManor:
                    mc = {180,200,235,255}; break;                                 // névoa fria
                case ZoneID::DarkForest: case ZoneID::CursedFarm:
                    mc = {170,235,150,255}; break;                                 // pólen/vaga-lumes
                default:
                    mc = {255,215,160,255}; break;                                 // poeira dourada
            }
            const float RANGE = 720.0f;
            for (int i = 0; i < 120; ++i) {
                float hx = sinf(i * 12.9898f) * 43758.5453f; hx -= floorf(hx);
                float hz = sinf(i * 78.233f)  * 43758.5453f; hz -= floorf(hz);
                float hy = sinf(i * 37.719f)  * 43758.5453f; hy -= floorf(hy);
                float px = player.position.x + (hx - 0.5f) * 2.0f * RANGE + sinf(t * 0.25f + i) * 28.0f;
                float pz = player.position.y + (hz - 0.5f) * 2.0f * RANGE + cosf(t * 0.22f + i * 1.7f) * 28.0f;
                float py = 14.0f + hy * 160.0f + sinf(t * 0.6f + i * 1.3f) * 14.0f;
                DrawSphereEx({ px, py, pz }, 1.1f + hy * 1.3f, 4, 4, mc);
            }
        }

        // ── FX em 3D (com PROFUNDIDADE/OCLUSÃO): projéteis e feixes de loot ──
        for (auto& p : projectiles) {
            DrawSphereEx({ p.position.x, 14.0f, p.position.y }, p.radius * 0.9f, 7, 7, p.color);
            DrawSphereEx({ p.position.x, 14.0f, p.position.y }, p.radius * 1.6f, 6, 6, ColorAlpha(p.color, 0.28f));
        }
        for (auto& p : enemyProjectiles) {
            DrawSphereEx({ p.position.x, 13.0f, p.position.y }, p.radius * 0.9f, 7, 7, p.color);
            DrawSphereEx({ p.position.x, 13.0f, p.position.y }, p.radius * 1.6f, 6, 6, ColorAlpha(p.color, 0.28f));
        }
        // Feixe de luz vertical do loot (raro+) — pilar 3D que ilumina e é ocluído
        for (auto& item : items) {
            if (item.dropBeamTimer > 0.0f && item.rarity >= ItemRarity::Uncommon) {
                float beamH = 160.0f + (int)item.rarity * 70.0f;
                float rB    = 3.0f + (int)item.rarity * 1.6f;
                DrawCylinderEx({ item.position.x, 0.2f, item.position.y },
                               { item.position.x, beamH, item.position.y },
                               rB, rB * 0.35f, 10, ColorAlpha(item.rarityColor, 0.40f));
                DrawCylinderEx({ item.position.x, 0.2f, item.position.y },
                               { item.position.x, beamH * 0.9f, item.position.y },
                               rB * 0.4f, rB * 0.12f, 8, ColorAlpha(item.rarityColor, 0.85f));
            }
        }

    EndMode3D();

    // Máscara de luz/noite + vignette ANTES dos overlays — assim barras de vida,
    // nomes e prompts ficam por cima e LEGÍVEIS mesmo no escuro/noite.
    lightSystem.applyMask();
    DrawVignette(screenWidth, screenHeight);

    // ── 2. Overlay 2D Projetado: Projéteis, Partículas, Nomes e UI ────────────
    auto proj = [&](Vector2 w, float h) {
        return GetWorldToScreenEx({ w.x, h, w.y }, camera3D, screenWidth, screenHeight);
    };

    // ── RTS Unit Selection Indicators in 2D projected space ──
    for (const auto& t : buildingSystem.tanks) {
        if (!t.selected || t.isDead()) continue;
        Vector2 s = proj(t.position, 0.0f);
        DrawEllipseLines((int)s.x, (int)s.y, 22, 10, Color{0,255,80,255});
        if (t.hasMoveOrder) {
            Vector2 mS = proj(t.moveOrder, 0.0f);
            DrawLineEx(s, mS, 1.0f, ColorAlpha(Color{0,255,80,255}, 0.35f));
        }
    }
    for (const auto& s : buildingSystem.soldiers) {
        if (!s.selected || s.isDead()) continue;
        Vector2 feetS = proj(s.position, 0.0f);
        DrawEllipseLines((int)feetS.x, (int)feetS.y, 14, 7, Color{0,255,80,255});
        if (s.hasMoveOrder) {
            Vector2 mS = proj(s.moveOrder, 0.0f);
            DrawLineEx(feetS, mS, 1.0f, ColorAlpha(Color{0,255,80,255}, 0.35f));
        }
    }

    // ── Prompts de Produção dos Prédios RTS ──
    for (const auto& b : buildingSystem.buildings) {
        if (!b.built) continue;
        bool isFactory  = (b.type == BuildingType::TankFactory);
        bool isBarracks = (b.type == BuildingType::Barracks);
        if (!isFactory && !isBarracks) continue;

        int n   = isFactory ? (int)buildingSystem.tanks.size()    : (int)buildingSystem.soldiers.size();
        int cap = isFactory ? 8                      : 12;
        int cost= isFactory ? 40                     : 20;
        const char* unit = isFactory ? "Tanque" : "Soldado";

        Vector2 s = proj(b.position, 56.0f); // altura 56
        const char* lbl = TextFormat("[CLIQUE] %s  $%d   %d/%d", unit, cost, n, cap);
        int tw = MeasureText(lbl, 11);
        DrawRectangle((int)(s.x - tw/2 - 4), (int)(s.y - 2), tw + 8, 16, ColorAlpha(BLACK, 0.7f));
        DrawRectangleLines((int)(s.x - tw/2 - 4), (int)(s.y - 2), tw + 8, 16, ColorAlpha(Color{120,200,255,255}, 0.7f));
        DrawText(lbl, (int)(s.x - tw/2), (int)s.y, 11, Color{180,220,255,255});

        // Barra de producao automatica
        float pct = b.productionTimer / b.productionRate;
        DrawRectangle((int)(s.x - 24), (int)(s.y + 16), 48, 4, ColorAlpha(BLACK, 0.6f));
        DrawRectangle((int)(s.x - 24), (int)(s.y + 16), (int)(48 * pct), 4, Color{255,200,0,255});
    }

    // ── Prédios RTS Info (Level up e evolução) ──
    for (const auto& b : buildingSystem.buildings) {
        if (!b.built) continue;
        const char* name = BuildingSystem::COSTS[(int)b.type].name;
        // Badge de nivel
        const char* lvTxt = TextFormat("Lv%d", b.level);
        Vector2 s = proj(b.position, 0.0f);
        DrawText(lvTxt, (int)(s.x + 16), (int)(s.y - 38), 11,
                 b.level >= Building::MAX_LEVEL ? Color{255,215,0,255} : Color{120,220,255,255});

        // Painel completo se perto do jogador
        if (Vector2Distance(player.position, b.position) <= 120.f) {
            Vector2 pS = proj(b.position, 0.0f);
            int px = (int)pS.x;
            int py = (int)pS.y - 92;
            int pw = 230, ph = 64;
            DrawRectangle(px - pw/2, py, pw, ph, ColorAlpha(BLACK, 0.8f));
            DrawRectangleLinesEx({(float)(px-pw/2),(float)py,(float)pw,(float)ph}, 1.0f,
                                 ColorAlpha(Color{0,200,255,255}, 0.7f));
            DrawText(TextFormat("%s  [Lv %d/%d]", name, b.level, Building::MAX_LEVEL),
                     px - pw/2 + 6, py + 4, 12, Color{0,220,255,255});
            DrawText(BuildingSystem::COSTS[(int)b.type].desc, px - pw/2 + 6, py + 20, 9, Color{200,200,210,255});
            if (b.level < Building::MAX_LEVEL) {
                DrawText(TextFormat("[U] Evoluir Lv%d->Lv%d  ($%d)",
                         b.level, b.level+1, buildingSystem.upgradeCostFor(b)),
                         px - pw/2 + 6, py + 44, 11, Color{255,215,0,255});
            } else {
                DrawText("NIVEL MAXIMO", px - pw/2 + 6, py + 44, 11, Color{255,215,0,255});
            }
        }
    }

    // ── HUD/Indicador do nó de recurso mais próximo ──
    if (nearResourceIdx >= 0 && nearResourceIdx < (int)resourceNodes.size()) {
        const auto& n = resourceNodes[nearResourceIdx];
        if (!n.depleted) {
            Vector2 s = proj(n.position, 0.0f);
            Color c = resourceColor(n.type);
            DrawCircleLines((int)s.x, (int)s.y, 22.0f, ColorAlpha(c, 0.8f));
            const char* lbl = TextFormat("[H] Minerar %s (%d)", resourceName(n.type), n.amount);
            int w = MeasureText(lbl, 11);
            DrawRectangle((int)s.x - w/2 - 4, (int)s.y - 46, w + 8, 16, ColorAlpha(BLACK, 0.7f));
            DrawText(lbl, (int)s.x - w/2, (int)s.y - 44, 11, c);
            if (n.harvestProg > 0.0f) {
                DrawRectangle((int)s.x - 20, (int)s.y - 28, 40, 5, ColorAlpha(BLACK, 0.6f));
                DrawRectangle((int)s.x - 20, (int)s.y - 28, (int)(40 * n.harvestProg), 5, c);
            }
        }
    }

    // ── HUD de Equipamentos no chão ──
    for (const auto& ge : groundEquips) {
        if (ge.collected) continue;
        if (std::fabs(ge.position.x - camera.target.x) > 1100 || std::fabs(ge.position.y - camera.target.y) > 700) continue;
        float fade = (ge.lifetime < 5.0f) ? ge.lifetime / 5.0f : 1.0f;
        Vector2 s = proj(ge.position, 0.0f);
        // Name label
        const char* eName = ge.equip.name.c_str();
        int ew = MeasureText(eName, 11);
        DrawText(eName, (int)(s.x - ew/2), (int)(s.y - 30), 11, ColorAlpha(ge.equip.color, fade));
        // E-prompt when player nearby
        float dist = Vector2Distance(player.position, ge.position);
        if (dist < 60.0f) {
            const char* pr = "[E] EQUIPAR";
            int pw = MeasureText(pr, 12);
            DrawRectangle((int)(s.x - pw/2 - 4), (int)(s.y - 50), pw + 8, 18, ColorAlpha(BLACK, 0.7f));
            DrawText(pr, (int)(s.x - pw/2), (int)(s.y - 48), 12, ColorAlpha({255,210,0,255}, 1.0f));
        }
    }

    // (Feixes de loot agora são pilares 3D dentro do BeginMode3D — com oclusão)

    // Partículas
    for (const auto& p : particles.particles) {
        if (!p.active) continue;
        Vector2 s = proj(p.position, 8.0f);
        Particle tempP = p;
        tempP.position = s;
        tempP.render();
    }

    // (Projéteis do player e dos inimigos agora são esferas 3D dentro do BeginMode3D)

    // Remote Players (Names and Online Indicators)
    if (netActive) {
        for (const auto& p : net.peers()) {
            Vector2 headS = proj({p.x, p.y}, 48.0f);
            int w = MeasureText(p.name, 11);
            DrawRectangle((int)headS.x - w/2 - 3, (int)headS.y - 12, w + 6, 14, ColorAlpha(BLACK, 0.6f));
            DrawText(p.name, (int)headS.x - w/2, (int)headS.y - 10, 11, ColorAlpha(WHITE, 0.95f));
            DrawCircle((int)headS.x + w/2 + 8, (int)headS.y - 5, 3.0f, Color{0,255,80,255}); // online dot
        }
    }

    // NPCs (Nomes e Tags de Quest)
    for (auto& n : npcs) {
        Vector2 feetS = proj(n.position, 0.0f);
        DrawText(n.name.c_str(), (int)feetS.x - (int)n.name.size() * 4, (int)feetS.y + 12, 14, WHITE);

        if (n.hasQuest) {
            Vector2 headS = proj(n.position, 48.0f);
            DrawText("!", (int)headS.x - 4, (int)headS.y - 12, 28, GOLD);
        } else if (n.hasNewDialogue && n.dialogues.size() > 0) {
            Vector2 headS = proj(n.position, 50.0f);
            DrawText("!", (int)headS.x + 6, (int)headS.y - 14, 20, Color{255, 220, 0, 255});
        }
    }

    // Inimigos (Barras de Vida e Nomes de Elite)
    for (auto& e : enemies) {
        Vector2 barPos = proj(e.position, e.radius + 16.0f);

        // Barra de HP
        float barW  = e.isBoss() ? 70.0f : (e.type == EnemyType::Tank ? 48.0f : 36.0f);
        float hpPct = e.health / e.maxHealth;
        Color hpCol = hpPct > 0.5f ? Color{0,220,80,255} : hpPct > 0.25f ? YELLOW : RED;
        DrawHealthBar(barPos, hpPct, barW, 5, hpCol);

        // Label de Tier
        if (e.evolTier > 0) {
            const char* tierLabel = e.evolTier == 1 ? "[VET]" : e.evolTier == 2 ? "[ELT]" : "[LND]";
            Color tierCol = e.evolTier == 1 ? Color{0,220,100,255} :
                            e.evolTier == 2 ? Color{100,180,255,255} : Color{255,160,0,255};
            int tw = MeasureText(tierLabel, 9);
            DrawText(tierLabel, (int)(barPos.x - tw/2), (int)(barPos.y - 11), 9, tierCol);
        }

        // Nome de Elite Mod
        if (e.isElite) {
            Color eliteCol;
            switch (e.eliteMod) {
                case 0:  eliteCol = {255, 60,  0,   255}; break;
                case 1:  eliteCol = {180, 180, 255, 255}; break;
                default: eliteCol = {255, 0,   200, 255}; break;
            }
            const char* tag = (e.eliteMod == 0) ? "BERSERK" : (e.eliteMod == 1) ? "BLINDADO" : "VOLATIL";
            DrawText(tag, (int)(barPos.x - MeasureText(tag, 10)/2), (int)(barPos.y - 21), 10, ColorAlpha(eliteCol, 0.9f));
        }
    }

    // ── HOVER TOOLTIP universal: passe o mouse em cima e veja O QUE É ─────────
    // (NPCs, armas/equipamentos no chão, itens/consumíveis e recursos ouro/prata/ferro).
    if (!showInventory && !shopSystem.open && !craftingSystem.open && !dialogOpen
        && !buildingSystem.buildModeActive) {
        Vector2 vm = virtualizeMousePos(GetMousePosition());
        float bestD = 1e9f; bool hov = false;
        std::string hTitle, hDesc; Color hCol = {255,255,255,255};
        auto consider = [&](Vector2 wp, float wh, float rad, std::string title, std::string desc, Color col) {
            if (Vector2Distance(wp, camera.target) > 1500.0f) return;          // cull longe
            Vector2 s = proj(wp, wh);
            float d = Vector2Distance(s, vm);
            if (d <= rad && d < bestD) { bestD = d; hTitle = std::move(title); hDesc = std::move(desc); hCol = col; hov = true; }
        };
        for (const auto& n : npcs)
            consider(n.position, 30.0f, 44.0f, n.name,
                     (n.title.empty() ? std::string("Personagem") : n.title) + "   [E] falar", n.color);
        for (const auto& ge : groundEquips) {
            if (ge.collected) continue;
            consider(ge.position, 14.0f, 34.0f, ge.equip.name,
                     ge.equip.description + "   Tier " + std::to_string(ge.equip.tier) + "   [E] equipar",
                     ge.equip.color);
        }
        for (const auto& it : items) {
            if (it.pickedUp) continue;
            consider(it.position, 10.0f, 38.0f, it.name,
                     std::string(Item::rarityToName(it.rarity)) + "   item", it.rarityColor);
        }
        for (const auto& nd : resourceNodes) {
            if (nd.depleted) continue;
            consider(nd.position, 12.0f, 34.0f, resourceName(nd.type),
                     "Recurso   x" + std::to_string(nd.amount) + "   [H] minerar (picareta)", resourceColor(nd.type));
        }
        if (hov) {
            int padX = 10, padY = 8;
            int tw = MeasureText(hTitle.c_str(), 16);
            int dw = MeasureText(hDesc.c_str(), 12);
            int boxW = (tw > dw ? tw : dw) + padX * 2;
            int boxH = 46;
            int bx = (int)vm.x + 18, by = (int)vm.y + 12;
            if (bx + boxW > screenWidth)  bx = (int)vm.x - boxW - 12;   // não sai da tela
            if (by + boxH > screenHeight) by = screenHeight - boxH - 4;
            DrawRectangleRounded({(float)bx,(float)by,(float)boxW,(float)boxH}, 0.12f, 5, ColorAlpha(Color{8,12,22,255}, 0.95f));
            DrawRectangleLinesEx({(float)bx,(float)by,(float)boxW,(float)boxH}, 1.5f, hCol);
            DrawText(hTitle.c_str(), bx + padX, by + padY, 16, hCol);
            DrawText(hDesc.c_str(),  bx + padX, by + padY + 21, 12, ColorAlpha(WHITE, 0.82f));
        }
    }

    // (máscara de luz + vignette já aplicadas logo após EndMode3D — overlays acima ficam legíveis)

    drawFloatingNumbers(true);   // dano/creditos/cura/recursos — NAO existiam no 3D

    // ── 3. Interface e HUD Final ─────────────────────────────────────────────
    drawUI();
    drawHudAndOverlays();   // recursos/ameaça + pause/levelup/evolução/loja/crafting/party (faltava no 3D!)

    EndTextureMode();
}

