// Game_Resources.cpp — coleta of resources naturais (mineracao) and animais/health selvagem.
// Extraido of Game.cpp. Same class Game.
#include "Game.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>

// ─── Coleta of Recursos Naturais ─────────────────────────────────────────────

const char* Game::resourceName(ResourceType t) {
    switch (t) {
        case ResourceType::Wood:   return "Wood";
        case ResourceType::Stone:  return "Stone";
        case ResourceType::Iron:   return "Iron";
        case ResourceType::Silver: return "Silver";
        case ResourceType::Gold:   return "Gold";
        default: return "";
    }
}

Color Game::resourceColor(ResourceType t) {
    switch (t) {
        case ResourceType::Wood:   return {120, 80, 40, 255};
        case ResourceType::Stone:  return {150, 150, 160, 255};
        case ResourceType::Iron:   return {110, 90, 80, 255};
        case ResourceType::Silver: return {210, 220, 235, 255};
        case ResourceType::Gold:   return {255, 200, 40, 255};
        default: return GRAY;
    }
}

void Game::setupResourceNodes() {
    resourceNodes.clear();
    if (!openWorldMode) return;
    unsigned int rng = 0x5EED1234u;
    auto rnd = [&]() { rng = rng*1664525u+1013904223u; return (rng>>8) & 0x7FFF; };

    auto addNode = [&](Vector2 pos, ResourceType t, int amt) {
        ResourceNode n;
        n.position = pos; n.type = t; n.amount = amt; n.maxAmount = amt;
        n.harvestProg = 0.0f; n.respawnTimer = 0.0f; n.depleted = false; n.shake = 0.0f;
        resourceNodes.push_back(n);
    };

    // Distribui in the by region conforme the biome
    for (const auto& r : worldRegions) {
        Rectangle b = r.bounds;
        auto randPos = [&](float margin) -> Vector2 {
            // Guarda: with region menor that 2*margem the span virava 0/negative and the
            // `% span` era divisao by zero (UB / crash).
            int spanW = (int)(b.width  - 2*margin); if (spanW < 1) spanW = 1;
            int spanH = (int)(b.height - 2*margin); if (spanH < 1) spanH = 1;
            return { b.x + margin + (float)(rnd() % spanW),
                     b.y + margin + (float)(rnd() % spanH) };
        };
        // contagens by biome
        int wood=8, stone=6, iron=3, silver=1, gold=1;
        switch (r.zoneType) {
            case ZoneID::DarkForest:   wood=26; stone=6;  iron=2; silver=1; gold=0; break;
            case ZoneID::CursedFarm:   wood=18; stone=8;  iron=3; silver=1; gold=0; break;
            case ZoneID::Cemetery:     wood=10; stone=14; iron=4; silver=2; gold=1; break;
            case ZoneID::KronosForge:  wood=2;  stone=16; iron=10;silver=4; gold=3; break; // rica in metal
            case ZoneID::LARuins:      wood=6;  stone=12; iron=6; silver=2; gold=1; break;
            case ZoneID::Bunker:       wood=4;  stone=10; iron=8; silver=3; gold=2; break;
            case ZoneID::GhostCity:    wood=4;  stone=14; iron=5; silver=2; gold=1; break;
            case ZoneID::AbandonedManor:wood=10;stone=10; iron=4; silver=3; gold=2; break;
            case ZoneID::KronosNexus:  wood=2;  stone=8;  iron=6; silver=4; gold=4; break;
            default: break;
        }
        for (int i=0;i<wood;i++)   addNode(randPos(120), ResourceType::Wood,   GetRandomValue(3,6));
        for (int i=0;i<stone;i++)  addNode(randPos(120), ResourceType::Stone,  GetRandomValue(3,6));
        for (int i=0;i<iron;i++)   addNode(randPos(140), ResourceType::Iron,   GetRandomValue(2,4));
        for (int i=0;i<silver;i++) addNode(randPos(160), ResourceType::Silver, GetRandomValue(1,3));
        for (int i=0;i<gold;i++)   addNode(randPos(160), ResourceType::Gold,   GetRandomValue(1,2));
    }
}

void Game::updateResourceGathering(float dt) {
    nearResourceIdx = -1;
    float bestDist = 64.0f;
    for (int i = 0; i < (int)resourceNodes.size(); ++i) {
        auto& n = resourceNodes[i];
        if (n.shake > 0.0f) n.shake -= dt * 6.0f;
        if (n.depleted) {
            n.respawnTimer -= dt;
            if (n.respawnTimer <= 0.0f) {       // ressurge full
                n.depleted = false; n.amount = n.maxAmount; n.harvestProg = 0.0f;
            }
            continue;
        }
        float d = Vector2Distance(player.position, n.position);
        if (d < bestDist) { bestDist = d; nearResourceIdx = i; }
    }

    // ── MINERACAO COM PICARETA: segura H near the in the → bate in GOLPES (cadencia) ──
    if (mineSwingCD   > 0.0f) mineSwingCD   -= dt;
    if (mineSwingAnim > 0.0f) mineSwingAnim -= dt * 4.5f;   // anim of the golpe decai
    // O click only podia CONTINUE um golpe (exigia nearResourceIdx == mineFxIdx, and
    // mineFxIdx only era written here inside) — i.and., never comecava. Agora inicia,
    // since that the button not esteja servindo the other coisa (dialogo/structure/RTS).
    bool mouseFree = !dialogOpen && !buildingSystem.buildModeActive && !rtsDragging;
    bool mineHeld  = IsKeyDown(KEY_H) || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mouseFree);
    if (nearResourceIdx >= 0 && mineHeld) {
        mineFxIdx = nearResourceIdx;
        auto& n = resourceNodes[nearResourceIdx];
        // cadencia of the picareta: wood fast, metais lentos (picareta "pesa" more)
        float swingTime = (n.type == ResourceType::Wood) ? 0.34f :
                          (n.type == ResourceType::Stone) ? 0.42f : 0.52f;
        if (mineSwingCD <= 0.0f) {                          // ★ um GOLPE
            mineSwingCD   = swingTime;
            mineSwingAnim = 1.0f;
            n.shake = 1.3f;                                  // the in the leva the impacto
            Color oc = resourceColor(n.type);
            particles.spawnHit(n.position, oc, 14);          // estilhacos/faiscas
            particles.spawnHit({n.position.x, n.position.y - 6.0f}, Color{230,230,230,255}, 6);
            audio.playPickup();
            triggerShake(2.0f, 0.10f);                       // tranco of the picareta
            int got = (n.type == ResourceType::Wood) ? 2 : 1;  // cada golpe extrai
            if (got > n.amount) got = n.amount;
            playerResources[(int)n.type] += got;
            n.amount -= got;
            // prefixo SEM the number: the render already concatena dn.value ("Wood +" + "2").
            damageNumbers.push_back({{n.position.x, n.position.y - 10.0f}, (float)got, oc, 1.0f,
                                     std::string(resourceName(n.type)) + " +"});
            n.harvestProg = (n.maxAmount > 0) ? 1.0f - (float)n.amount / (float)n.maxAmount : 0.0f;
            if (n.amount <= 0) {                             // esgotou — ressurge after
                n.depleted = true; n.respawnTimer = 35.0f; n.harvestProg = 0.0f;
                particles.spawnHit(n.position, oc, 26);      // estoura to the quebrar
            }
        }
    } else if (!IsKeyDown(KEY_H) && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        mineFxIdx = -1;
    }
}

void Game::renderResourceNodes() const {
    Vector2 cam = camera.target;
    for (int i = 0; i < (int)resourceNodes.size(); ++i) {
        const auto& n = resourceNodes[i];
        if (n.depleted) continue;
        // culling
        if (std::fabs(n.position.x - cam.x) > 1100 || std::fabs(n.position.y - cam.y) > 700) continue;
        float sx = (n.shake > 0.0f) ? std::sin(n.shake * 30.0f) * 2.0f : 0.0f;
        float x = n.position.x + sx, y = n.position.y;
        Color c = resourceColor(n.type);
        // shadow
        DrawEllipse((int)n.position.x, (int)(y + 14), 16.0f, 5.0f, ColorAlpha(BLACK, 0.35f));
        switch (n.type) {
            case ResourceType::Wood: { // tree
                DrawRectangle((int)(x-4), (int)(y-6), 8, 22, Color{90,60,30,255});
                DrawCircleV({x, y-22}, 18.0f, Color{30,90,40,255});
                DrawCircleV({x-10, y-14}, 12.0f, Color{36,100,46,255});
                DrawCircleV({x+10, y-14}, 12.0f, Color{28,84,38,255});
                break;
            }
            case ResourceType::Stone: { // stone
                DrawCircleV({x, y}, 15.0f, Color{120,120,128,255});
                DrawCircleV({x-6, y+2}, 9.0f, Color{145,145,155,255});
                DrawCircleV({x+7, y-1}, 8.0f, Color{100,100,110,255});
                break;
            }
            default: { // ore (iron/silver/gold) — rocha with veios coloridos
                DrawCircleV({x, y}, 15.0f, Color{80,72,66,255});
                DrawCircleV({x-5, y+2}, 8.0f, Color{96,88,80,255});
                for (int v = 0; v < 5; ++v) {
                    float the = v * 1.2f + i;
                    DrawCircleV({x + std::cos(the)*7.0f, y + std::sin(the)*7.0f}, 2.6f, c);
                }
                break;
            }
        }
        // Indicador when is the in the coletavel more next
        if (i == nearResourceIdx) {
            DrawCircleLines((int)x, (int)y, 22.0f, ColorAlpha(c, 0.8f));
            const char* lbl = TextFormat("[H] Minerar %s (%d)", resourceName(n.type), n.amount);
            int w = MeasureText(lbl, 11);
            DrawRectangle((int)x - w/2 - 4, (int)y - 46, w + 8, 16, ColorAlpha(BLACK, 0.7f));
            DrawText(lbl, (int)x - w/2, (int)y - 44, 11, c);
            if (n.harvestProg > 0.0f) {
                DrawRectangle((int)x - 20, (int)y - 28, 40, 5, ColorAlpha(BLACK, 0.6f));
                DrawRectangle((int)x - 20, (int)y - 28, (int)(40 * n.harvestProg), 5, c);
            }
        }
    }
}

void Game::drawResourceHUD() const {
    // Painel of resources — dock ESQUERDA, soon below the pill of the PHASE (outside the
    // center). Cada resource: square colorido + quantity + NOME.
    int n = (int)ResourceType::COUNT;
    int ew = 96;
    int pw = ew * n + 10;
    int px = 10, py = 80;
    DrawRectangle(px, py, pw, 40, ColorAlpha({8,12,26,255}, 0.86f));
    DrawRectangle(px, py, 3, 40, ColorAlpha({0,235,255,255}, 0.9f));
    for (int i = 0; i < n; ++i) {
        ResourceType t = (ResourceType)i;
        Color c = resourceColor(t);
        int ix = px + 10 + i * ew;
        DrawRectangle(ix, py + 6, 10, 10, c);
        DrawRectangleLinesEx({(float)ix, (float)(py+6), 10.0f, 10.0f}, 1,
                             ColorAlpha(WHITE, 0.3f));
        DrawText(TextFormat("%d", playerResources[i]), ix + 15, py + 3, 13,
                 ColorAlpha(WHITE, 0.95f));
        DrawText(resourceName(t), ix, py + 24, 10, ColorAlpha(c, 0.95f));
    }
}

// ─── Animais / Health Selvagem ─────────────────────────────────────────────────

void Game::setupAnimals() {
    animals.clear();
    if (!openWorldMode) return;
    unsigned int rng = 0xA417BEEFu;
    auto rnd = [&]() { rng = rng*1664525u+1013904223u; return (rng>>8) & 0x7FFF; };

    for (const auto& r : worldRegions) {
        Rectangle b = r.bounds;
        // biome define that animais aparecem
        struct Spawn { AnimalType t; int count; };
        std::vector<Spawn> spawns;
        switch (r.zoneType) {
            case ZoneID::DarkForest:
                spawns = {{AnimalType::Deer,8},{AnimalType::Rabbit,10},{AnimalType::Boar,4},{AnimalType::Wolf,5},{AnimalType::Bird,8}}; break;
            case ZoneID::CursedFarm:
                spawns = {{AnimalType::Deer,6},{AnimalType::Rabbit,8},{AnimalType::Boar,5},{AnimalType::Bird,6}}; break;
            case ZoneID::LARuins: case ZoneID::Bunker:
                spawns = {{AnimalType::Rabbit,5},{AnimalType::Bird,5},{AnimalType::Wolf,2}}; break;
            case ZoneID::Cemetery: case ZoneID::AbandonedManor:
                spawns = {{AnimalType::Wolf,4},{AnimalType::Bird,4},{AnimalType::Rabbit,3}}; break;
            case ZoneID::GhostCity:
                spawns = {{AnimalType::Rabbit,3},{AnimalType::Wolf,3},{AnimalType::Bird,3}}; break;
            default:
                spawns = {{AnimalType::Rabbit,3},{AnimalType::Bird,3}}; break;
        }
        for (auto& sp : spawns) {
            for (int i = 0; i < sp.count; ++i) {
                Animal the{};
                the.position = { b.x + 100 + (float)(rnd() % (int)(b.width  - 200)),
                               b.y + 100 + (float)(rnd() % (int)(b.height - 200)) };
                the.type = sp.t;
                the.hostile = (sp.t == AnimalType::Wolf);
                float hp = (sp.t == AnimalType::Boar) ? 60.f : (sp.t == AnimalType::Wolf) ? 45.f :
                           (sp.t == AnimalType::Deer) ? 35.f : 15.f;
                the.health = the.maxHealth = hp;
                the.wanderDir = { (float)(rnd()%100-50)/50.f, (float)(rnd()%100-50)/50.f };
                the.wanderTimer = (float)(rnd()%300)/100.f;
                the.dead = false; the.fleeing = false; the.animTimer = (float)(rnd()%628)/100.f; the.attackCD = 0.f;
                animals.push_back(the);
            }
        }
    }
}

void Game::updateAnimals(float dt) {
    if (!openWorldMode) return;
    for (auto it = animals.begin(); it != animals.end();) {
        Animal& the = *it;
        the.animTimer += dt;
        if (the.attackCD > 0.f) the.attackCD -= dt;

        float distToPlayer = Vector2Distance(the.position, player.position);

        // Damage of projectiles of the player
        for (auto& p : projectiles) {
            if (Vector2Distance(p.position, the.position) < 18.0f) {
                the.health -= 25.0f; the.fleeing = true;
                particles.spawnHit(the.position, Color{200,60,40,255}, 6);
            }
        }
        // Damage melee (click direito near)
        if (botMeleeRequest || (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !dialogOpen)) {
            if (distToPlayer < player.attackRange + 20.0f && meleeCooldown <= 0.05f) {
                the.health -= 30.0f; the.fleeing = true;
            }
        }

        if (the.health <= 0.0f) {
            // Caca reward: credits + XP + "flesh"
            int cred = (the.type == AnimalType::Boar) ? GetRandomValue(20,40) :
                       (the.type == AnimalType::Deer) ? GetRandomValue(15,30) :
                       (the.type == AnimalType::Wolf) ? GetRandomValue(12,24) : GetRandomValue(4,10);
            player.credits += cred;
            totalCreditsEarned += cred;
            achievements.onCreditsEarned(totalCreditsEarned);
            xpOrbs.emplace_back(the.position, cred / 2 + 5);
            damageNumbers.push_back({the.position, (float)cred, Color{255,200,0,255}, 1.0f, "$"});
            particles.spawnHit(the.position, Color{180,40,30,255}, 14);
            audio.playEnemyDeath(false);
            it = animals.erase(it);
            continue;
        }

        // Movement
        Vector2 move{0,0};
        if (the.hostile && distToPlayer < 320.0f && distToPlayer > 28.0f && !the.fleeing) {
            // wolf persegue
            move = Vector2Normalize(Vector2Subtract(player.position, the.position));
            if (distToPlayer < 40.0f && the.attackCD <= 0.f && !player.isShielded()) {
                player.takeDamage(8.0f); noteHurtDir(the.position); hitFlashTimer = 0.2f; the.attackCD = 1.2f;
                audio.playPlayerHurt();
            }
        } else if (the.fleeing || (!the.hostile && distToPlayer < 160.0f)) {
            // foge of the player
            the.fleeing = (distToPlayer < 280.0f);
            move = Vector2Normalize(Vector2Subtract(the.position, player.position));
        } else {
            // vagueia
            the.wanderTimer -= dt;
            if (the.wanderTimer <= 0.f) {
                the.wanderDir = { (float)(GetRandomValue(-100,100))/100.f,
                                (float)(GetRandomValue(-100,100))/100.f };
                the.wanderTimer = (float)GetRandomValue(2,5);
            }
            move = the.wanderDir;
        }
        float spd = (the.type==AnimalType::Rabbit||the.type==AnimalType::Bird) ? 140.f :
                    (the.type==AnimalType::Wolf) ? 165.f :
                    the.fleeing ? 180.f : 55.f;
        the.position.x += move.x * spd * dt;
        the.position.y += move.y * spd * dt;
        ++it;
    }
}

void Game::renderAnimals() const {
    Vector2 cam = camera.target;
    for (const auto& the : animals) {
        if (std::fabs(the.position.x - cam.x) > 1100 || std::fabs(the.position.y - cam.y) > 700) continue;
        float x = the.position.x, y = the.position.y;
        float bob = std::sin(the.animTimer * 8.0f) * 1.5f;
        DrawEllipse((int)x, (int)(y+8), 12.0f, 4.0f, ColorAlpha(BLACK, 0.3f));
        switch (the.type) {
            case AnimalType::Deer: {
                Color body={150,110,70,255};
                DrawEllipse((int)x,(int)(y+bob),12.0f,8.0f,body);
                DrawCircleV({x+9,y-6+bob},5.0f,body);             // head
                DrawLine((int)x+9,(int)(y-10+bob),(int)x+6,(int)(y-16+bob),Color{90,60,30,255}); // chifre
                DrawLine((int)x+11,(int)(y-10+bob),(int)x+14,(int)(y-16+bob),Color{90,60,30,255});
                DrawRectangle((int)x-8,(int)(y+6),2,8,body); DrawRectangle((int)x+6,(int)(y+6),2,8,body);
                break;
            }
            case AnimalType::Rabbit: {
                Color body={210,200,190,255};
                DrawCircleV({x,y+bob},6.0f,body);
                DrawEllipse((int)(x-2),(int)(y-8+bob),2.0f,5.0f,body); // orelhas
                DrawEllipse((int)(x+2),(int)(y-8+bob),2.0f,5.0f,body);
                break;
            }
            case AnimalType::Boar: {
                Color body={90,70,60,255};
                DrawEllipse((int)x,(int)(y+bob),13.0f,8.0f,body);
                DrawCircleV({x+10,y+bob},5.0f,body);
                DrawCircleV({x+13,y+bob},2.0f,Color{40,30,25,255}); // focinho
                break;
            }
            case AnimalType::Wolf: {
                Color body=the.fleeing?Color{120,120,130,255}:Color{90,95,105,255};
                DrawEllipse((int)x,(int)(y+bob),12.0f,7.0f,body);
                DrawCircleV({x+9,y-3+bob},5.0f,body);
                DrawLine((int)x+7,(int)(y-7+bob),(int)x+6,(int)(y-11+bob),body); // orelha
                DrawLine((int)x+11,(int)(y-7+bob),(int)x+12,(int)(y-11+bob),body);
                DrawCircleV({x+11,y-3+bob},1.5f,Color{255,200,0,255}); // eye
                break;
            }
            default: { // Bird
                float fl = std::sin(the.animTimer*12.0f)*4.0f;
                Color body={60,60,70,255};
                DrawCircleV({x,y-20+bob*2},3.0f,body);
                DrawLine((int)x,(int)(y-20+bob*2),(int)(x-6),(int)(y-20-fl+bob*2),body);
                DrawLine((int)x,(int)(y-20+bob*2),(int)(x+6),(int)(y-20-fl+bob*2),body);
                break;
            }
        }
    }
}

