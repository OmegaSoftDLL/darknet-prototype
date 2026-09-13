#include "BuildingSystem.h"
#include "Enemy.h"
#include "EnemyProjectile.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

// ─── Cost table ──────────────────────────────────────────────────────────────

const BuildingCost BuildingSystem::COSTS[BuildingSystem::NUM_TYPES] = {
    { 200, 0,  0,  "Arca",           "Nucleo da base. Renasce aqui. Cura + aura de bonus."     },
    { 80,  0,  0,  "Base de Apoio",  "Gera 10 creditos a cada 8s. Sua fonte de renda."         },
    { 150, 5,  0,  "Quartel",        "Produz soldados aliados que atacam inimigos."             },
    { 300, 8,  3,  "Fabrica de Tank","Produz tanques amigos a cada 30s."                       },
    { 120, 3,  1,  "Torre",          "Atira automaticamente nos inimigos em 220px."             },
    { 100, 4,  2,  "Extrator",       "Gera MetalScrap passivamente."                           },
    { 50,  2,  0,  "Parede",         "Barreira que bloqueia inimigos. 800 HP."                 },
    { 180, 6,  0,  "MedBay",         "Cura jogador e companions em 120px a cada 3s."           },
};

// ─── Building constructor ─────────────────────────────────────────────────────

Building::Building(Vector2 pos, BuildingType t) : position(pos), type(t) {
    switch (t) {
        case BuildingType::Ark:
            health = maxHealth = 600.f;
            healRadius = 240.f; healAmount = 35.f; healRate = 2.0f; // cura mais forte + raio maior
            tintColor = {0, 255, 200, 255};
            break;
        case BuildingType::House:
            health = maxHealth = 200.f;
            genRate = 8.f;
            tintColor = {200, 180, 120, 255};
            break;
        case BuildingType::Barracks:
            health = maxHealth = 300.f;
            productionRate = 20.f;
            spawnTime = 5.f;
            tintColor = {80, 160, 255, 255};
            break;
        case BuildingType::TankFactory:
            health = maxHealth = 400.f;
            productionRate = 30.f;
            spawnTime = 9.f;
            tintColor = {255, 140, 0, 255};
            break;
        case BuildingType::Turret:
            health = maxHealth = 150.f;
            shootRange = 220.f; shootRate = 1.5f; shootDamage = 20.f;
            tintColor = {255, 60, 60, 255};
            break;
        case BuildingType::ResourceNode:
            health = maxHealth = 180.f;
            genRate = 12.f;
            tintColor = {180, 100, 255, 255};
            break;
        case BuildingType::Wall:
            health = maxHealth = 800.f;
            isWall = true;
            tintColor = {160, 160, 160, 255};
            break;
        case BuildingType::MedBay:
            health = maxHealth = 250.f;
            healRadius = 120.f; healAmount = 15.f; healRate = 3.f;
            tintColor = {0, 255, 100, 255};
            break;
    }
    buildTimer = 0.f;
    built = false;
    animTimer = 0.f;
}

// ─── BuildingSystem ───────────────────────────────────────────────────────────

BuildingSystem::BuildingSystem() {}

void BuildingSystem::toggleBuildMode() {
    buildModeActive = !buildModeActive;
}

bool BuildingSystem::tryPlace(Vector2 worldPos, int playerCredits, int playerMetal, int playerCarapace,
                              int& outCreditCost, int& outMetalCost, int& outCarapaceCost) {
    if (selectedType < 0 || selectedType >= NUM_TYPES) return false;
    const auto& cost = COSTS[selectedType];
    outCreditCost   = cost.credits;
    outMetalCost    = cost.metalScrap;
    outCarapaceCost = cost.alienCarapace;

    if (playerCredits < cost.credits) return false;
    if (playerMetal   < cost.metalScrap) return false;
    if (playerCarapace < cost.alienCarapace) return false;

    // Don't overlap existing buildings
    for (const auto& b : buildings) {
        if (Vector2Distance(b.position, worldPos) < 80.f) return false;
    }

    buildings.emplace_back(worldPos, static_cast<BuildingType>(selectedType));
    return true;
}

// ─── Update ──────────────────────────────────────────────────────────────────

void BuildingSystem::update(float dt, Vector2 playerPos,
                            std::vector<EnemyProjectile>* enemyProj,
                            const std::vector<Enemy*>& enemies) {
    pendingShots.clear();
    pendingCredits   = 0;
    pendingMaterials = 0;

    for (auto& b : buildings) {
        if (!b.active) continue;
        updateBuilding(b, dt, playerPos, enemies);
    }

    // Projecteis inimigos derrubam construcoes e unidades aliadas.
    // Paredes barram o tiro; predios comuns levam dano mas deixa o projétil passar.
    if (enemyProj) {
        for (auto& ep : *enemyProj) {
            bool wallHit = false;
            for (auto& b : buildings) {
                if (!b.active || !b.built) continue;
                bool isWall = (b.type == BuildingType::Wall);
                float w = isWall ? 64.f : 48.f;
                float h = isWall ? 16.f : 48.f;
                Rectangle br = {b.position.x - w/2.f, b.position.y - h/2.f, w, h};
                if (CheckCollisionCircleRec(ep.position, ep.radius, br)) {
                    b.health -= ep.damage;
                    if (isWall) { ep.active = false; wallHit = true; }
                    if (b.health <= 0.f) b.active = false;
                    break;   // primeira construcao atingida por este projetil
                }
            }
            if (wallHit) continue;
            for (auto& t : tanks) {
                if (!t.active || t.isDead()) continue;
                if (Vector2Distance(ep.position, t.position) < ep.radius + 20.f)
                    t.health -= ep.damage;
            }
            for (auto& s : soldiers) {
                if (!s.active || s.isDead()) continue;
                if (Vector2Distance(ep.position, s.position) < ep.radius + 12.f)
                    s.health -= ep.damage;
            }
        }
    }

    // Remove dead buildings
    buildings.erase(std::remove_if(buildings.begin(), buildings.end(),
        [](const Building& b){ return !b.active; }), buildings.end());

    // Update tanks
    for (auto& t : tanks) {
        if (!t.active) continue;
        t.update(dt, enemies);
        // Aura da Arca: regenera HP e da +50% de dano nos tiros
        bool aura = isInArkAura(t.position);
        if (aura && t.health < t.maxHealth) t.health = fminf(t.maxHealth, t.health + 12.f * dt);
        if (t.wantsToShoot) {
            float dmg = aura ? 45.f : 30.f;
            Color col = aura ? Color{0,255,200,255} : Color{255,120,0,255};
            pendingShots.push_back({t.position, t.shootDir, dmg, col, 350.f});
            t.wantsToShoot = false;
        }
    }
    tanks.erase(std::remove_if(tanks.begin(), tanks.end(),
        [](const FriendlyTank& t){ return !t.active || t.isDead(); }), tanks.end());

    // Update soldiers
    for (auto& s : soldiers) {
        if (!s.active) continue;
        s.update(dt, playerPos, enemies);
        bool aura = isInArkAura(s.position);
        if (aura && s.health < s.maxHealth) s.health = fminf(s.maxHealth, s.health + 10.f * dt);
        if (s.wantsToShoot) {
            float dmg = aura ? 18.f : 12.f;
            Color col = aura ? Color{0,255,200,255} : Color{100,200,255,255};
            pendingShots.push_back({s.position, s.shootDir, dmg, col, 320.f});
            s.wantsToShoot = false;
        }
    }
    soldiers.erase(std::remove_if(soldiers.begin(), soldiers.end(),
        [](const FriendlySoldier& s){ return !s.active || s.isDead(); }), soldiers.end());
}

void BuildingSystem::updateBuilding(Building& b, float dt, Vector2 playerPos,
                                    const std::vector<Enemy*>& enemies) {
    b.animTimer += dt;

    // Construction phase
    if (!b.built) {
        b.buildTimer += dt / 3.0f; // 3 seconds to build
        if (b.buildTimer >= 1.0f) { b.buildTimer = 1.0f; b.built = true; b.builtAt = (float)GetTime(); }
        return;
    }

    switch (b.type) {
        case BuildingType::House:
            b.genTimer += dt;
            if (b.genTimer >= b.genRate) {
                b.genTimer = 0.f;
                pendingCredits += 10;
            }
            break;

        case BuildingType::Barracks:
            b.productionTimer += dt;
            if (b.productionTimer >= b.productionRate) {
                b.productionTimer = 0.f;
                if ((int)soldiers.size() + b.spawnQueue < 12) b.spawnQueue++;
            }
            break;

        case BuildingType::TankFactory:
            b.productionTimer += dt;
            if (b.productionTimer >= b.productionRate) {
                b.productionTimer = 0.f;
                if ((int)tanks.size() + b.spawnQueue < 8) b.spawnQueue++;
            }
            break;

        case BuildingType::Turret: {
            b.shootCooldown -= dt;
            // Otimizacao: so varre inimigos quando a torre pode atirar (evita um
            // scan O(inimigos) por torre a cada frame — critico com muitas torres).
            if (b.shootCooldown <= 0.f) {
                const Enemy* target = nearestEnemy(b.position, b.shootRange, enemies);
                if (target) {
                    Vector2 dir = Vector2Normalize(Vector2Subtract(target->position, b.position));
                    b.shootDir = dir;
                    b.wantsToShoot = true;
                    b.shootCooldown = 1.0f / b.shootRate;
                    pendingShots.push_back({b.position, dir, b.shootDamage, {255,50,50,255}, 380.f});
                }
            }
            break;
        }

        case BuildingType::ResourceNode:
            b.genTimer += dt;
            if (b.genTimer >= b.genRate) {
                b.genTimer = 0.f;
                pendingMaterials += 1;
            }
            break;

        case BuildingType::Ark:
        case BuildingType::MedBay:
            b.healTimer += dt;
            // Actual healing done in healPlayerIfNear()
            if (b.healTimer >= b.healRate) b.healTimer = 0.f;
            break;

        default: break;
    }

    // ── Fila de producao ── base de produção constrói a unidade ao longo do
    // tempo (spawnTime); ela NAO nasce pronto na hora.
    if (b.spawnQueue > 0 && (b.type == BuildingType::Barracks ||
                             b.type == BuildingType::TankFactory)) {
        float m = 1.0f + (b.level - 1) * 0.5f;   // nivel reduz o tempo de producao
        b.spawnTimer += dt;
        if (b.spawnTimer >= b.spawnTime / m) {
            b.spawnTimer = 0.f;
            b.spawnQueue--;
            if (b.type == BuildingType::Barracks) spawnSoldier(b.position);
            else                                 spawnTank(b.position);
        }
    }
}

void BuildingSystem::healPlayerIfNear(Vector2 playerPos, float& playerHealth, float playerMaxHealth) {
    for (auto& b : buildings) {
        if (!b.active || !b.built) continue;
        if (b.type != BuildingType::Ark && b.type != BuildingType::MedBay) continue;
        if (Vector2Distance(playerPos, b.position) > b.healRadius) continue;
        if (b.healTimer < b.healRate - 0.05f) continue; // just triggered
        playerHealth = std::min(playerMaxHealth, playerHealth + b.healAmount);
    }
}

bool BuildingSystem::getArkPosition(Vector2& out) const {
    for (const auto& b : buildings) {
        if (b.active && b.built && b.type == BuildingType::Ark) {
            out = b.position;
            return true;
        }
    }
    return false;
}

bool BuildingSystem::isInArkAura(Vector2 pos) const {
    for (const auto& b : buildings) {
        if (b.active && b.built && b.type == BuildingType::Ark) {
            if (Vector2Distance(pos, b.position) <= b.healRadius) return true;
        }
    }
    return false;
}

int BuildingSystem::collectCredits() {
    int c = pendingCredits; pendingCredits = 0; return c;
}

int BuildingSystem::collectMaterials() {
    int m = pendingMaterials; pendingMaterials = 0; return m;
}

const Enemy* BuildingSystem::nearestEnemy(Vector2 from, float range,
                                           const std::vector<Enemy*>& enemies) const {
    const Enemy* best = nullptr;
    float bestDist = range;
    for (const auto* e : enemies) {
        if (!e || e->isDead()) continue;
        float d = Vector2Distance(from, e->position);
        if (d < bestDist) { bestDist = d; best = e; }
    }
    return best;
}

void BuildingSystem::spawnTank(Vector2 factoryPos) {
    FriendlyTank t;
    // Angulo deterministico (sem GetRandomValue) — mantem o world-state reproduzivel
    float angle = (float)tanks.size() * 1.7f + 2.0f;
    t.position   = {factoryPos.x + cosf(angle) * 55.f, factoryPos.y + sinf(angle) * 55.f};
    t.factoryPos = factoryPos;
    t.orbitAngle = angle;
    tanks.push_back(t);
}

int BuildingSystem::clickProduce(Vector2 worldPos, int& playerCredits) {
    for (auto& b : buildings) {
        if (!b.built) continue;
        bool isFactory  = (b.type == BuildingType::TankFactory);
        bool isBarracks = (b.type == BuildingType::Barracks);
        if (!isFactory && !isBarracks) continue;
        if (Vector2Distance(worldPos, b.position) > 60.0f) continue;

        if (isFactory) {
            if ((int)tanks.size() + b.spawnQueue >= 8) return 3;   // limite
            int cost = 40;
            if (playerCredits < cost) return 2;          // sem creditos
            playerCredits -= cost;
            b.spawnQueue++;                              // entra na fila de producao
        } else {
            if ((int)soldiers.size() + b.spawnQueue >= 12) return 3;
            int cost = 20;
            if (playerCredits < cost) return 2;
            playerCredits -= cost;
            b.spawnQueue++;
        }
        return 1; // enfileirou
    }
    return 0; // nao clicou em predio de producao
}

// ── Evolucao de predios ──────────────────────────────────────────────────────

void BuildingSystem::applyLevelStats(Building& b) {
    float m = 1.0f + (b.level - 1) * 0.5f;             // Lv1=1.0 Lv2=1.5 Lv3=2.0
    float hpFrac = b.maxHealth > 0.f ? b.health / b.maxHealth : 1.0f;
    int   lv = b.level;
    switch (b.type) {
        case BuildingType::Ark:
            b.maxHealth = 600.f * m; b.healAmount = 35.f * m; b.healRadius = 240.f + (lv-1)*40.f; break;
        case BuildingType::House:
            b.maxHealth = 200.f * m; b.genRate = 8.f / m; break;
        case BuildingType::Barracks:
            b.maxHealth = 300.f * m; b.productionRate = 20.f / m; break;
        case BuildingType::TankFactory:
            b.maxHealth = 400.f * m; b.productionRate = 30.f / m; break;
        case BuildingType::Turret:
            b.maxHealth = 150.f * m; b.shootDamage = 20.f * m;
            b.shootRange = 220.f + (lv-1)*40.f; b.shootRate = 1.5f * m; break;
        case BuildingType::ResourceNode:
            b.maxHealth = 180.f * m; b.genRate = 12.f / m; break;
        case BuildingType::Wall:
            b.maxHealth = 800.f * m; break;
        case BuildingType::MedBay:
            b.maxHealth = 250.f * m; b.healAmount = 15.f * m; b.healRadius = 120.f + (lv-1)*40.f; break;
    }
    b.health = b.maxHealth * hpFrac;  // preserva a fracao de vida
}

int BuildingSystem::upgradeCostFor(const Building& b) const {
    return COSTS[(int)b.type].credits * b.level;  // Lv1->2 = base, Lv2->3 = base*2
}

int BuildingSystem::upgradeNearby(Vector2 playerPos, int& playerCredits) {
    Building* nearest = nullptr; float best = 95.f;
    for (auto& b : buildings) {
        if (!b.built) continue;
        float d = Vector2Distance(playerPos, b.position);
        if (d < best) { best = d; nearest = &b; }
    }
    if (!nearest) return 0;
    if (nearest->level >= Building::MAX_LEVEL) return 3;
    int cost = upgradeCostFor(*nearest);
    if (playerCredits < cost) return 2;
    playerCredits -= cost;
    nearest->level++;
    applyLevelStats(*nearest);
    return 1;
}

void BuildingSystem::renderBuildingInfo(Vector2 playerPos) const {
    for (const auto& b : buildings) {
        if (!b.built) continue;
        const char* name = COSTS[(int)b.type].name;
        // Badge de nivel sempre visivel
        const char* lvTxt = TextFormat("Lv%d", b.level);
        DrawText(lvTxt, (int)(b.position.x + 16), (int)(b.position.y - 38), 11,
                 b.level >= Building::MAX_LEVEL ? Color{255,215,0,255} : Color{120,220,255,255});

        // Painel completo quando o jogador esta perto
        if (Vector2Distance(playerPos, b.position) > 120.f) continue;
        int   px = (int)b.position.x;
        int   py = (int)b.position.y - 92;
        int   pw = 230, ph = 64;
        DrawRectangle(px - pw/2, py, pw, ph, ColorAlpha(BLACK, 0.8f));
        DrawRectangleLinesEx({(float)(px-pw/2),(float)py,(float)pw,(float)ph}, 1.0f,
                             ColorAlpha(Color{0,200,255,255}, 0.7f));
        DrawText(TextFormat("%s  [Lv %d/%d]", name, b.level, Building::MAX_LEVEL),
                 px - pw/2 + 6, py + 4, 12, Color{0,220,255,255});
        DrawText(COSTS[(int)b.type].desc, px - pw/2 + 6, py + 20, 9, Color{200,200,210,255});
        if (b.level < Building::MAX_LEVEL) {
            DrawText(TextFormat("[U] Evoluir Lv%d->Lv%d  ($%d)",
                     b.level, b.level+1, upgradeCostFor(b)),
                     px - pw/2 + 6, py + 44, 11, Color{255,215,0,255});
        } else {
            DrawText("NIVEL MAXIMO", px - pw/2 + 6, py + 44, 11, Color{255,215,0,255});
        }
    }
}

// ── Controle RTS ─────────────────────────────────────────────────────────────

int BuildingSystem::selectUnitsInBox(Rectangle box) {
    // Normaliza caixa (largura/altura positivas)
    if (box.width  < 0) { box.x += box.width;  box.width  = -box.width; }
    if (box.height < 0) { box.y += box.height; box.height = -box.height; }
    int count = 0;
    for (auto& t : tanks) {
        t.selected = (!t.isDead() && CheckCollisionPointRec(t.position, box));
        if (t.selected) count++;
    }
    for (auto& s : soldiers) {
        s.selected = (!s.isDead() && CheckCollisionPointRec(s.position, box));
        if (s.selected) count++;
    }
    return count;
}

void BuildingSystem::clearSelection() {
    for (auto& t : tanks)    t.selected = false;
    for (auto& s : soldiers) s.selected = false;
}

void BuildingSystem::orderMove(Vector2 dest) {
    // Espalha as unidades num pequeno raio ao redor do destino (formacao)
    int idx = 0;
    auto place = [&](Vector2& order, bool& has) {
        float ang = idx * 0.7f;
        float rad = 18.0f + (idx / 8) * 26.0f;
        order = {dest.x + cosf(ang) * rad, dest.y + sinf(ang) * rad};
        has = true;
        idx++;
    };
    for (auto& t : tanks)    if (t.selected && !t.isDead()) place(t.moveOrder, t.hasMoveOrder);
    for (auto& s : soldiers) if (s.selected && !s.isDead()) place(s.moveOrder, s.hasMoveOrder);
}

int BuildingSystem::selectedCount() const {
    int c = 0;
    for (const auto& t : tanks)    if (t.selected) c++;
    for (const auto& s : soldiers) if (s.selected) c++;
    return c;
}

void BuildingSystem::renderSelection() const {
    for (const auto& t : tanks) {
        if (!t.selected || t.isDead()) continue;
        DrawEllipseLines((int)t.position.x, (int)(t.position.y + 6), 22, 10, Color{0,255,80,255});
        if (t.hasMoveOrder)
            DrawLineEx(t.position, t.moveOrder, 1.0f, ColorAlpha(Color{0,255,80,255}, 0.35f));
    }
    for (const auto& s : soldiers) {
        if (!s.selected || s.isDead()) continue;
        DrawEllipseLines((int)s.position.x, (int)(s.position.y + 6), 14, 7, Color{0,255,80,255});
        if (s.hasMoveOrder)
            DrawLineEx(s.position, s.moveOrder, 1.0f, ColorAlpha(Color{0,255,80,255}, 0.35f));
    }
}

void BuildingSystem::renderUnitPrompts() const {
    for (const auto& b : buildings) {
        if (!b.built) continue;
        bool isFactory  = (b.type == BuildingType::TankFactory);
        bool isBarracks = (b.type == BuildingType::Barracks);
        if (!isFactory && !isBarracks) continue;

        int n   = isFactory ? (int)tanks.size()    : (int)soldiers.size();
        int cap = isFactory ? 8                      : 12;
        int cost= isFactory ? 40                     : 20;
        const char* unit = isFactory ? "Tanque" : "Soldado";

        float bx = b.position.x, by = b.position.y - 56.0f;
        // Fundo
        const char* lbl = TextFormat("[CLIQUE] %s  $%d   %d/%d", unit, cost, n + b.spawnQueue, cap);
        int tw = MeasureText(lbl, 11);
        DrawRectangle((int)(bx - tw/2 - 4), (int)(by - 2), tw + 8, 16, ColorAlpha(BLACK, 0.7f));
        DrawRectangleLines((int)(bx - tw/2 - 4), (int)(by - 2), tw + 8, 16,
                           ColorAlpha(Color{120,200,255,255}, 0.7f));
        DrawText(lbl, (int)(bx - tw/2), (int)by, 11, Color{180,220,255,255});

        // Barra de producao: unidade em construcao (cyan) ou ciclo automatico (amarelo)
        if (b.spawnQueue > 0) {
            float pct = fminf(1.0f, (float)b.spawnTimer / b.spawnTime);
            DrawRectangle((int)(bx - 26), (int)(by + 16), 52, 4, ColorAlpha(BLACK, 0.6f));
            DrawRectangle((int)(bx - 26), (int)(by + 16), (int)(52 * pct), 4, Color{80,220,255,255});
            DrawText(TextFormat("Construindo... %d na fila", b.spawnQueue),
                     (int)(bx - 34), (int)(by + 22), 9, Color{140,220,255,255});
        } else {
            float pct = b.productionTimer / b.productionRate;
            DrawRectangle((int)(bx - 24), (int)(by + 16), 48, 4, ColorAlpha(BLACK, 0.6f));
            DrawRectangle((int)(bx - 24), (int)(by + 16), (int)(48 * pct), 4, Color{255,200,0,255});
        }
    }
}

void BuildingSystem::spawnSoldier(Vector2 barracksPos) {
    FriendlySoldier s;
    // Angulo deterministico (sem GetRandomValue) — mantem o world-state reproduzivel
    float angle = (float)soldiers.size() * 0.9f + 7.0f;
    s.position   = {barracksPos.x + cosf(angle) * 50.f, barracksPos.y + sinf(angle) * 50.f};
    s.barracksPos = barracksPos;
    soldiers.push_back(s);
}

// ─── FriendlyTank Update ─────────────────────────────────────────────────────

void FriendlyTank::update(float dt, const std::vector<Enemy*>& enemies) {
    if (isDead()) { active = false; return; }

    // Find nearest enemy
    const Enemy* target = nullptr;
    float bestDist = shootRange;
    for (const auto* e : enemies) {
        if (!e || e->isDead()) continue;
        float d = Vector2Distance(position, e->position);
        if (d < bestDist) { bestDist = d; target = e; }
    }

    // Ordem de mover (RTS) tem prioridade — attack-move: anda ate o destino mas
    // ainda atira em inimigos no alcance pelo caminho.
    if (hasMoveOrder) {
        Vector2 toDest = Vector2Subtract(moveOrder, position);
        float d = Vector2Length(toDest);
        if (d > 12.f) {
            Vector2 dir = Vector2Scale(toDest, 1.0f / d);
            position.x += dir.x * speed * dt;
            position.y += dir.y * speed * dt;
        } else {
            hasMoveOrder = false; // chegou
        }
        // dispara em inimigos no alcance sem desviar do destino
        if (target) {
            shootCooldown -= dt;
            if (shootCooldown <= 0.f && bestDist <= shootRange) {
                shootDir = Vector2Normalize(Vector2Subtract(target->position, position));
                wantsToShoot = true;
                shootCooldown = 1.0f / shootRate;
            }
        }
    } else if (target) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(target->position, position));
        position.x += dir.x * speed * dt;
        position.y += dir.y * speed * dt;
        shootCooldown -= dt;
        if (shootCooldown <= 0.f && bestDist <= shootRange) {
            shootDir = dir;
            wantsToShoot = true;
            shootCooldown = 1.0f / shootRate;
        }
    } else {
        // Orbit factory when no enemies
        orbitAngle += dt * 0.5f;
        float dist = 80.f;
        Vector2 orbitTarget = {
            factoryPos.x + cosf(orbitAngle) * dist,
            factoryPos.y + sinf(orbitAngle) * dist
        };
        Vector2 toOrbit = Vector2Subtract(orbitTarget, position);
        float d = Vector2Length(toOrbit);
        if (d > 5.f) {
            Vector2 moveDir = Vector2Scale(toOrbit, 1.0f / d);
            position.x += moveDir.x * speed * dt;
            position.y += moveDir.y * speed * dt;
        }
    }
}

void FriendlyTank::render() const {
    if (!active || isDead()) return;

    // ── APC Void-Tech — casco escuro + nucleo neon ─────────────────────────────
    Color hullC = {42, 48, 66, 255};
    Color coreC = {0, 240, 255, 255};
    DrawRectangle((int)(position.x - 18), (int)(position.y - 12), 36, 24, hullC);
    DrawRectangleLinesEx({position.x - 18, position.y - 12, 36, 24}, 1.f,
                         ColorAlpha(coreC, 0.4f));
    DrawRectangle((int)(position.x - 14), (int)(position.y - 8),  28, 16, {56, 62, 82, 255});
    // Nucleo de energia central
    DrawCircle((int)position.x, (int)position.y, 7.f, {0, 200, 255, 255});
    DrawCircleLines((int)position.x, (int)position.y, 9.f, ColorAlpha(coreC, 0.5f));
    // Canhao de energia
    Vector2 barrelEnd = {position.x + shootDir.x * 18.f, position.y + shootDir.y * 18.f};
    DrawLineEx(position, barrelEnd, 4.f, {0, 220, 255, 255});
    // HP bar
    float pct = (maxHealth > 0.0f) ? (health / maxHealth) : 0.0f;
    DrawRectangle((int)(position.x - 18), (int)(position.y - 20), 36, 4, ColorAlpha(BLACK, 0.6f));
    DrawRectangle((int)(position.x - 18), (int)(position.y - 20), (int)(36 * pct), 4,
                  {0, 240, 255, 255});
}

// ─── FriendlySoldier Update ──────────────────────────────────────────────────

void FriendlySoldier::update(float dt, Vector2 playerPos, const std::vector<Enemy*>& enemies) {
    if (isDead()) { active = false; return; }

    const Enemy* target = nullptr;
    float bestDist = shootRange;
    for (const auto* e : enemies) {
        if (!e || e->isDead()) continue;
        float d = Vector2Distance(position, e->position);
        if (d < bestDist) { bestDist = d; target = e; }
    }

    // Ordem de mover (RTS) tem prioridade — attack-move
    if (hasMoveOrder) {
        Vector2 toDest = Vector2Subtract(moveOrder, position);
        float d = Vector2Length(toDest);
        if (d > 10.f) {
            Vector2 dir = Vector2Scale(toDest, 1.0f / d);
            position.x += dir.x * speed * dt;
            position.y += dir.y * speed * dt;
        } else {
            hasMoveOrder = false;
        }
        if (target) {
            shootCooldown -= dt;
            if (shootCooldown <= 0.f) {
                shootDir = Vector2Normalize(Vector2Subtract(target->position, position));
                wantsToShoot = true;
                shootCooldown = 1.0f / shootRate;
            }
        }
    } else if (target) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(target->position, position));
        // Advance to attack range
        if (bestDist > 100.f) {
            position.x += dir.x * speed * dt;
            position.y += dir.y * speed * dt;
        }
        shootCooldown -= dt;
        if (shootCooldown <= 0.f) {
            shootDir = dir;
            wantsToShoot = true;
            shootCooldown = 1.0f / shootRate;
        }
    } else {
        // Follow player loosely at ~80px
        float d = Vector2Distance(position, playerPos);
        if (d > 80.f) {
            Vector2 dir = Vector2Normalize(Vector2Subtract(playerPos, position));
            position.x += dir.x * speed * dt;
            position.y += dir.y * speed * dt;
        }
    }
}

void FriendlySoldier::render() const {
    if (!active || isDead()) return;
    // ── Infantaria Void — armadura escura + visor neon ─────────────────────────
    // Helmet com visor
    DrawCircle((int)position.x, (int)(position.y - 8), 8.f, {34, 40, 56, 255});
    DrawCircleLines((int)position.x, (int)(position.y - 8), 8.f,
                    ColorAlpha({0, 240, 255, 255}, 0.35f));
    DrawCircle((int)position.x, (int)(position.y - 9), 3.f, {0, 240, 255, 255});
    // Torso de armadura
    DrawRectangle((int)(position.x - 5), (int)position.y, 10, 16, {28, 34, 50, 255});
    // Rifle de energia
    Vector2 rifleEnd = {position.x + shootDir.x * 16.f, position.y - 8.f + shootDir.y * 16.f};
    DrawLineEx({position.x, position.y - 8.f}, rifleEnd, 2.5f, {0, 220, 255, 255});
    // HP
    float pct = (maxHealth > 0.0f) ? (health / maxHealth) : 0.0f;
    DrawRectangle((int)(position.x - 12), (int)(position.y - 18), 24, 3, ColorAlpha(BLACK, 0.6f));
    DrawRectangle((int)(position.x - 12), (int)(position.y - 18), (int)(24 * pct), 3,
                  {0, 220, 255, 255});
}

// ─── Render ──────────────────────────────────────────────────────────────────

void BuildingSystem::render(Vector2 playerPos, Vector2 mouseWorldPos) const {
    for (const auto& t : tanks)    t.render();
    for (const auto& s : soldiers) s.render();
    for (const auto& b : buildings) renderBuilding(b);

    // Build mode preview
    if (buildModeActive) {
        BuildingType preview = static_cast<BuildingType>(selectedType);
        Color previewCol = {0, 255, 180, 100};

        // Check overlap
        bool canPlace = true;
        for (const auto& b : buildings) {
            if (Vector2Distance(b.position, mouseWorldPos) < 80.f) { canPlace = false; break; }
        }
        previewCol = canPlace ? Color{0, 255, 100, 80} : Color{255, 50, 50, 80};

        // Draw ghost
        DrawRectangle((int)(mouseWorldPos.x - 32), (int)(mouseWorldPos.y - 32), 64, 64, previewCol);
        DrawRectangleLinesEx({mouseWorldPos.x - 32, mouseWorldPos.y - 32, 64, 64},
                             2.f, canPlace ? Color{0, 255, 100, 200} : Color{255, 50, 50, 200});
        DrawText(COSTS[selectedType].name, (int)(mouseWorldPos.x - 28), (int)(mouseWorldPos.y - 12),
                 14, WHITE);
    }
}

void BuildingSystem::renderBuilding(const Building& b) const {
    switch (b.type) {
        case BuildingType::Ark:          renderArkBuilding(b); break;
        case BuildingType::House:        renderHouseBuilding(b); break;
        case BuildingType::Barracks:     renderBarracks(b); break;
        case BuildingType::TankFactory:  renderTankFactory(b); break;
        case BuildingType::Turret:       renderTurret(b); break;
        case BuildingType::ResourceNode: renderResourceNode(b); break;
        case BuildingType::Wall:         renderWall(b); break;
        case BuildingType::MedBay:       renderMedBay(b); break;
    }

    // HP bar for all built structures
    if (b.built && b.type != BuildingType::Wall) {
        float pct = b.health / b.maxHealth;
        int bw = 60;
        DrawRectangle((int)(b.position.x - bw/2), (int)(b.position.y - 48), bw, 5,
                      ColorAlpha(BLACK, 0.7f));
        DrawRectangle((int)(b.position.x - bw/2), (int)(b.position.y - 48), (int)(bw * pct), 5,
                      pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED));
    }
}

void BuildingSystem::renderArkBuilding(const Building& b) const {
    float pulse = 0.5f + 0.5f * sinf(b.animTimer * 2.0f);

    if (!b.built) {
        // Scaffold holografico sob construcao
        DrawRectangleLinesEx({b.position.x - 36, b.position.y - 36, 72, 72}, 1.5f,
                             ColorAlpha({0, 220, 255, 255}, 0.4f + pulse * 0.4f));
        float prog = b.buildTimer * 72.f;   // era (int) num float: truncava e gerava C4244
        DrawRectangle((int)(b.position.x - 36), (int)(b.position.y + 28), (int)prog, 8,
                      Color{0, 255, 220, 255});
        DrawText("MONTANDO...", (int)(b.position.x - 36), (int)(b.position.y - 50), 12,
                 Color{0, 220, 255, 255});
        return;
    }

    // Base
    DrawRectangle((int)(b.position.x - 36), (int)(b.position.y - 24), 72, 48, {40, 60, 80, 255});
    DrawRectangle((int)(b.position.x - 28), (int)(b.position.y - 32), 56, 16, {60, 80, 100, 255});

    // Dome top
    for (int i = 0; i < 5; i++) {
        float r = 28.f - i * 4.f;
        DrawCircle((int)b.position.x, (int)(b.position.y - 32), r,
                   ColorAlpha({0, 200, 255, 255}, 0.15f + i * 0.04f));
    }
    DrawCircleLines((int)b.position.x, (int)(b.position.y - 32), 28.f, {0, 200, 255, 255});

    // Heal radius ring
    DrawCircleLines((int)b.position.x, (int)b.position.y, b.healRadius,
                    ColorAlpha({0, 255, 200, 255}, 0.12f + pulse * 0.08f));

    // Antenna beacon
    DrawLineEx({b.position.x, b.position.y - 52.f}, {b.position.x, b.position.y - 32.f}, 2.f,
               {0, 255, 200, 255});
    DrawCircle((int)b.position.x, (int)(b.position.y - 54), 4.f,
               ColorAlpha({0, 255, 200, 255}, 0.4f + pulse * 0.6f));

    DrawText("ARCA", (int)(b.position.x - 16), (int)(b.position.y + 28), 12, {0, 255, 200, 255});
}

void BuildingSystem::renderHouseBuilding(const Building& b) const {
    float pulse = 0.5f + 0.5f * sinf(b.animTimer * 2.2f);

    if (!b.built) {
        // Scaffold holografico de construcao
        DrawRectangleLinesEx({b.position.x - 24, b.position.y - 24, 48, 48}, 1.5f,
                             ColorAlpha({0, 220, 255, 255}, 0.4f + pulse * 0.4f));
        DrawRectangle((int)(b.position.x - 24), (int)(b.position.y + 18), (int)(b.buildTimer * 48), 6,
                      Color{0, 255, 220, 255});
        DrawText("MONTANDO...", (int)(b.position.x - 30), (int)(b.position.y - 44), 10,
                 Color{0, 220, 255, 255});
        return;
    }

    // ── Base de Apoio (relay de energia) — nada de fazenda ────────────────────
    // Plataforma escura com painel
    DrawRectangle((int)(b.position.x - 22), (int)(b.position.y - 12), 44, 30, {26, 32, 48, 255});
    DrawRectangleLinesEx({b.position.x - 22, b.position.y - 12, 44, 30}, 1.5f,
                         ColorAlpha({0, 200, 255, 255}, 0.35f + pulse * 0.3f));

    // Núcleo de energia acima da plataforma
    DrawCircle((int)b.position.x, (int)(b.position.y - 18), 9.f + pulse * 2.f,
               ColorAlpha({0, 220, 255, 255}, 0.18f));
    DrawCircle((int)b.position.x, (int)(b.position.y - 18), 5.f, {130, 230, 255, 255});
    DrawCircleLines((int)b.position.x, (int)(b.position.y - 18), 9.f,
                    ColorAlpha({0, 220, 255, 255}, 0.6f + pulse * 0.4f));

    // Antena beacon
    DrawLineEx({b.position.x, b.position.y - 12.f}, {b.position.x, b.position.y + 2.f}, 2.f,
               {0, 220, 255, 255});

    // Condutos de energia nas laterais
    for (int i = -1; i <= 1; i += 2) {
        DrawLineEx({b.position.x + i * 14.f, b.position.y - 8.f},
                   {b.position.x + i * 20.f, b.position.y + 14.f}, 2.f,
                   ColorAlpha({80, 180, 255, 255}, 0.5f + pulse * 0.4f));
    }

    // Barra de geracao de creditos (cyan)
    float genPct = b.genTimer / b.genRate;
    DrawRectangle((int)(b.position.x - 20), (int)(b.position.y + 22), (int)(40 * genPct), 4,
                  {0, 220, 255, 200});
    DrawRectangleLinesEx({b.position.x - 20, b.position.y + 22, 40, 4}, 1.f, {0, 170, 220, 255});
    DrawText("PWR", (int)(b.position.x - 12), (int)(b.position.y + 28), 10, {0, 220, 255, 255});
}

void BuildingSystem::renderBarracks(const Building& b) const {
    float pulse = 0.5f + 0.5f * sinf(b.animTimer * 2.6f);

    if (!b.built) {
        DrawRectangleLinesEx({b.position.x - 32, b.position.y - 20, 64, 40}, 1.5f,
                             ColorAlpha({0, 220, 255, 255}, 0.4f + pulse * 0.4f));
        DrawRectangle((int)(b.position.x - 32), (int)(b.position.y + 16), (int)(b.buildTimer * 64), 6,
                      Color{0, 255, 220, 255});
        DrawText("MONTANDO...", (int)(b.position.x - 34), (int)(b.position.y - 40), 10,
                 Color{0, 220, 255, 255});
        return;
    }

    // ── Quartel holografico — casco escuro + viga de energia ──────────────────
    DrawRectangle((int)(b.position.x - 32), (int)(b.position.y - 20), 64, 40, {24, 30, 46, 255});
    DrawRectangleLinesEx({b.position.x - 32, b.position.y - 20, 64, 40}, 1.5f,
                         ColorAlpha({0, 200, 255, 255}, 0.35f + pulse * 0.3f));

    // Costelas de energia ao longo do casco
    for (int x = -24; x <= 24; x += 12) {
        DrawLineEx({b.position.x + x, b.position.y - 18.f},
                   {b.position.x + x, b.position.y + 18.f}, 1.5f,
                   ColorAlpha({80, 200, 255, 255}, 0.3f + pulse * 0.25f));
    }

    // Porta de deploy — arco luminoso
    DrawRectangleLinesEx({b.position.x - 9, b.position.y + 0, 18, 20}, 2.f,
                         ColorAlpha({0, 220, 255, 255}, 0.45f + pulse * 0.5f));

    // Antena no teto
    DrawLineEx({b.position.x, b.position.y - 20.f}, {b.position.x, b.position.y - 30.f}, 2.f,
               {0, 220, 255, 255});
    DrawCircle((int)b.position.x, (int)(b.position.y - 32), 2.f,
               ColorAlpha({0, 255, 220, 255}, 0.4f + pulse * 0.6f));

    DrawText("QUARTEL", (int)(b.position.x - 26), (int)(b.position.y + 24), 10, {80, 220, 255, 255});
}

void BuildingSystem::renderTankFactory(const Building& b) const {
    float pulse = 0.5f + 0.5f * sinf(b.animTimer * 3.0f);

    if (!b.built) {
        DrawRectangleLinesEx({b.position.x - 40, b.position.y - 28, 80, 56}, 1.5f,
                             ColorAlpha({255, 150, 30, 255}, 0.4f + pulse * 0.4f));
        DrawRectangle((int)(b.position.x - 40), (int)(b.position.y + 24), (int)(b.buildTimer * 80), 6,
                      Color{255, 180, 60, 255});
        DrawText("MONTANDO...", (int)(b.position.x - 34), (int)(b.position.y - 48), 10,
                 Color{255, 150, 30, 255});
        return;
    }

    // ── Fabrica Void-Tech — casco escuro + nucleo de energia ───────────────────
    DrawRectangle((int)(b.position.x - 40), (int)(b.position.y - 28), 80, 56, {34, 28, 26, 255});
    DrawRectangleLinesEx({b.position.x - 40, b.position.y - 28, 80, 56}, 1.5f,
                         ColorAlpha({255, 150, 30, 255}, 0.35f + pulse * 0.3f));

    // Nucleo de energia central pulsante
    DrawCircle((int)b.position.x, (int)(b.position.y - 2), 13.f + pulse * 3.f,
               ColorAlpha({255, 150, 30, 255}, 0.16f));
    DrawCircle((int)b.position.x, (int)(b.position.y - 2), 7.f, {255, 180, 70, 255});
    DrawCircleLines((int)b.position.x, (int)(b.position.y - 2), 11.f,
                    ColorAlpha({255, 150, 30, 255}, 0.55f + pulse * 0.45f));

    // Condutos de energia para as bordas
    DrawLineEx({b.position.x - 6.f, b.position.y - 2.f},
               {b.position.x - 36.f, b.position.y - 2.f}, 2.5f,
               ColorAlpha({255, 160, 50, 255}, 0.4f + pulse * 0.4f));
    DrawLineEx({b.position.x + 6.f, b.position.y - 2.f},
               {b.position.x + 36.f, b.position.y - 2.f}, 2.5f,
               ColorAlpha({255, 160, 50, 255}, 0.4f + pulse * 0.4f));

    // Particulas de carga subindo (substitui a fumaca)
    for (int i = 0; i < 3; i++) {
        float y = b.position.y - 26.f - fmodf(b.animTimer * 14.f + i * 8.f, 30.f);
        DrawCircle((int)(b.position.x - 24 + i * 24), (int)y, 2.5f - i * 0.6f,
                   ColorAlpha({255, 190, 90, 255}, 0.5f - i * 0.12f));
    }

    // Barra de producao
    float prodPct = b.productionTimer / b.productionRate;
    DrawRectangle((int)(b.position.x - 36), (int)(b.position.y + 32), (int)(72 * prodPct), 5,
                  {255, 160, 50, 200});
    DrawRectangleLinesEx({b.position.x - 36, b.position.y + 32, 72, 5}, 1.f, {220, 120, 30, 255});
    DrawText("FABRICA", (int)(b.position.x - 26), (int)(b.position.y + 40), 10, {255, 160, 50, 255});
}

void BuildingSystem::renderTurret(const Building& b) const {
    float pulse = 0.5f + 0.5f * sinf(b.animTimer * 4.0f);

    if (!b.built) {
        DrawCircleLines((int)b.position.x, (int)b.position.y, 20.f,
                        ColorAlpha({255, 60, 60, 255}, 0.4f + pulse * 0.4f));
        DrawRectangle((int)(b.position.x - 20), (int)(b.position.y + 18), (int)(b.buildTimer * 40), 5,
                      Color{255, 90, 60, 255});
        return;
    }

    // Base platform
    DrawCircle((int)b.position.x, (int)b.position.y, 18.f, {60, 60, 70, 255});
    DrawCircleLines((int)b.position.x, (int)b.position.y, 18.f,
                    ColorAlpha({255, 60, 60, 255}, 0.6f + pulse * 0.4f));

    // Rotating turret head
    DrawCircle((int)b.position.x, (int)b.position.y, 10.f, {80, 30, 30, 255});

    // Barrel pointing toward shoot direction
    Vector2 barrelEnd = {
        b.position.x + b.shootDir.x * 22.f,
        b.position.y + b.shootDir.y * 22.f
    };
    DrawLineEx(b.position, barrelEnd, 5.f, {200, 50, 50, 255});

    // Range ring (subtle)
    DrawCircleLines((int)b.position.x, (int)b.position.y, b.shootRange,
                    ColorAlpha({255, 60, 60, 255}, 0.06f));
}

void BuildingSystem::renderResourceNode(const Building& b) const {
    float pulse = 0.5f + 0.5f * sinf(b.animTimer * 1.5f);

    if (!b.built) {
        DrawRectangleLinesEx({b.position.x - 22, b.position.y - 22, 44, 44}, 1.5f,
                             ColorAlpha({180, 100, 255, 255}, 0.4f + pulse * 0.4f));
        DrawRectangle((int)(b.position.x - 22), (int)(b.position.y + 18), (int)(b.buildTimer * 44), 5,
                      Color{200, 130, 255, 255});
        return;
    }

    // Mining rig body
    DrawRectangle((int)(b.position.x - 18), (int)(b.position.y - 14), 36, 28, {70, 50, 90, 255});
    // Crystal cluster on top
    for (int i = -2; i <= 2; i++) {
        float h = 8.f + fabsf((float)i) * 3.f;
        DrawRectangle((int)(b.position.x + i * 6 - 3), (int)(b.position.y - 14 - h),
                      6, (int)h, ColorAlpha({180, 100, 255, 255}, 0.6f + pulse * 0.4f));
    }
    // Glow
    DrawCircle((int)b.position.x, (int)(b.position.y - 18), 8.f,
               ColorAlpha({180, 100, 255, 255}, 0.1f + pulse * 0.2f));

    float genPct = b.genTimer / b.genRate;
    DrawRectangle((int)(b.position.x - 18), (int)(b.position.y + 18), (int)(36 * genPct), 4,
                  {180, 100, 255, 200});
}

void BuildingSystem::renderWall(const Building& b) const {
    float pulse = 0.5f + 0.5f * sinf(b.animTimer * 3.4f);

    if (!b.built) {
        DrawRectangleLinesEx({b.position.x - 32, b.position.y - 8, 64, 16}, 1.5f,
                             ColorAlpha({0, 220, 255, 255}, 0.4f + pulse * 0.4f));
        DrawRectangle((int)(b.position.x - 32), (int)(b.position.y - 8),
                      (int)(b.buildTimer * 64), 16, ColorAlpha({0, 255, 220, 255}, 0.35f));
        return;
    }

    float pct = b.health / b.maxHealth;
    Color coreC = pct > 0.5f ? Color{0, 220, 255, 255} :
                  pct > 0.25f ? Color{255, 190, 60, 255} : Color{255, 90, 60, 255};

    // Barra de metal escuro
    DrawRectangle((int)(b.position.x - 32), (int)(b.position.y - 8), 64, 16, {32, 38, 52, 255});
    DrawRectangleLinesEx({b.position.x - 32, b.position.y - 8, 64, 16}, 1.f,
                         ColorAlpha(coreC, 0.55f + pulse * 0.45f));

    // Rendagem de energia (grade holografica)
    for (int x = -28; x <= 28; x += 7) {
        DrawLineEx({b.position.x + x, b.position.y - 6.f},
                   {b.position.x + x, b.position.y + 6.f}, 1.f,
                   ColorAlpha(coreC, 0.25f + pulse * 0.2f));
    }
    DrawLineEx({b.position.x - 30, b.position.y}, {b.position.x + 30, b.position.y}, 1.f,
               ColorAlpha(coreC, 0.4f + pulse * 0.3f));

    // Pilones de energia no topo
    for (int i = -2; i <= 2; i++) {
        DrawCircle((int)(b.position.x + i * 12), (int)(b.position.y - 12), 2.5f,
                   ColorAlpha(coreC, 0.5f + pulse * 0.5f));
    }

    // HP bar
    DrawRectangle((int)(b.position.x - 32), (int)(b.position.y - 20), 64, 3,
                  ColorAlpha(BLACK, 0.6f));
    DrawRectangle((int)(b.position.x - 32), (int)(b.position.y - 20), (int)(64 * pct), 3,
                  pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED));
}

void BuildingSystem::renderMedBay(const Building& b) const {
    float pulse = 0.5f + 0.5f * sinf(b.animTimer * 2.5f);

    if (!b.built) {
        DrawRectangleLinesEx({b.position.x - 26, b.position.y - 26, 52, 52}, 1.5f,
                             ColorAlpha({80, 255, 140, 255}, 0.4f + pulse * 0.4f));
        DrawRectangle((int)(b.position.x - 26), (int)(b.position.y + 22), (int)(b.buildTimer * 52), 5,
                      Color{80, 255, 140, 255});
        DrawText("MONTANDO...", (int)(b.position.x - 30), (int)(b.position.y - 46), 10,
                 Color{80, 255, 140, 255});
        return;
    }

    // ── Nano-MedBay — capsula elastica com cruz de bio-reparo ──────────────────
    DrawRectangle((int)(b.position.x - 24), (int)(b.position.y - 20), 48, 40, {30, 42, 40, 255});
    DrawRectangleLinesEx({b.position.x - 24, b.position.y - 20, 48, 40}, 1.5f,
                         ColorAlpha({80, 255, 140, 255}, 0.35f + pulse * 0.3f));

    // Cruz de bio-reparo pulsante
    DrawRectangle((int)(b.position.x - 3), (int)(b.position.y - 14), 6, 18,
                  ColorAlpha({80, 255, 140, 255}, 0.8f + pulse * 0.2f));
    DrawRectangle((int)(b.position.x - 9), (int)(b.position.y - 8), 18, 6,
                  ColorAlpha({80, 255, 140, 255}, 0.8f + pulse * 0.2f));

    // Aneis de nano-reparo
    DrawCircleLines((int)b.position.x, (int)b.position.y, 30.f + pulse * 2.f,
                    ColorAlpha({80, 255, 140, 255}, 0.25f + pulse * 0.2f));

    // Heal radius pulse ring
    DrawCircleLines((int)b.position.x, (int)b.position.y, b.healRadius,
                    ColorAlpha({80, 255, 140, 255}, 0.08f + pulse * 0.12f));

    DrawText("MED", (int)(b.position.x - 12), (int)(b.position.y + 24), 12, {80, 255, 140, 255});
}

// ─── Build Menu (screen space) ────────────────────────────────────────────────

int BuildingSystem::menuCellAt(Vector2 m, int screenW, int screenH) const {
    if (!buildModeActive) return -1;
    const int menuW = 360, menuH = 240;
    const int mx = (screenW - menuW) / 2;
    const int my = screenH - menuH - 10;
    // Fora do painel? nao e clique de menu
    if (m.x < mx || m.x > mx + menuW || m.y < my || m.y > my + menuH) return -1;
    const int cols = 4;
    const int cellW = menuW / cols;
    const int cellH = (menuH - 26) / 2;
    for (int i = 0; i < NUM_TYPES; i++) {
        int col = i % cols, row = i / cols;
        int cx = mx + col * cellW;
        int cy = my + 26 + row * cellH;
        if (m.x >= cx + 2 && m.x <= cx + cellW - 2 &&
            m.y >= cy + 2 && m.y <= cy + cellH - 2)
            return i;
    }
    return -1;
}

void BuildingSystem::renderBuildMenu(int screenW, int screenH) const {
    if (!buildModeActive) return;

    const int menuW = 360;
    const int menuH = 240;
    const int mx = (screenW - menuW) / 2;
    const int my = screenH - menuH - 10;

    // Background panel
    DrawRectangle(mx, my, menuW, menuH, ColorAlpha(BLACK, 0.85f));
    DrawRectangleLinesEx({(float)mx, (float)my, (float)menuW, (float)menuH}, 2.f,
                         {0, 200, 255, 200});

    DrawText("[ B ] CONSTRUCOES  - Clique para selecionar / Clique no mapa para colocar",
             mx + 8, my + 6, 10, {0, 200, 255, 200});

    // Grid 4 columns x 2 rows
    const int cols = 4;
    const int cellW = menuW / cols;
    const int cellH = (menuH - 26) / 2;

    for (int i = 0; i < NUM_TYPES; i++) {
        int col = i % cols;
        int row = i / cols;
        int cx = mx + col * cellW;
        int cy = my + 26 + row * cellH;

        bool selected = (i == selectedType);
        Color bg = selected ? Color{0, 80, 120, 220} : Color{20, 20, 30, 200};
        DrawRectangle(cx + 2, cy + 2, cellW - 4, cellH - 4, bg);
        DrawRectangleLinesEx({(float)(cx + 2), (float)(cy + 2),
                              (float)(cellW - 4), (float)(cellH - 4)}, 1.5f,
                             selected ? Color{0, 200, 255, 255} : Color{80, 80, 100, 200});

        DrawText(COSTS[i].name, cx + 6, cy + 6, 11, WHITE);

        // Cost line
        char costBuf[64];
        if (COSTS[i].metalScrap > 0 || COSTS[i].alienCarapace > 0) {
            snprintf(costBuf, sizeof(costBuf), "$%d  M:%d  A:%d",
                     COSTS[i].credits, COSTS[i].metalScrap, COSTS[i].alienCarapace);
        } else {
            snprintf(costBuf, sizeof(costBuf), "$%d", COSTS[i].credits);
        }
        DrawText(costBuf, cx + 6, cy + 22, 9, {180, 220, 255, 255});

        // Short desc
        DrawText(COSTS[i].desc, cx + 6, cy + 36, 8, {140, 140, 160, 255});
    }
}

// ─── Persistence ─────────────────────────────────────────────────────────────

void BuildingSystem::save(std::vector<std::string>& out) const {
    out.clear();
    out.push_back("BDG_V1");
    out.push_back("buildingCount " + std::to_string(buildings.size()));
    for (const auto& b : buildings) {
        char buf[512];
        snprintf(buf, sizeof(buf),
                 "bld %f %f %d %f %f %d %d %f %f %d %f %f %f %f %f",
                 b.position.x, b.position.y,
                 static_cast<int>(b.type),
                 b.health, b.maxHealth,
                 b.level,
                 b.built ? 1 : 0,
                 b.buildTimer,
                 b.productionTimer,
                 b.spawnQueue,
                 b.spawnTimer,
                 b.genTimer,
                 b.shootCooldown,
                 b.healTimer,
                 b.animTimer);
        out.push_back(buf);
    }
    out.push_back("tankCount " + std::to_string(tanks.size()));
    for (const auto& t : tanks) {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "tnk %f %f %f %d %f %f",
                 t.position.x, t.position.y,
                 t.health,
                 t.active ? 1 : 0,
                 t.shootCooldown,
                 t.orbitAngle);
        out.push_back(buf);
    }
    out.push_back("soldierCount " + std::to_string(soldiers.size()));
    for (const auto& s : soldiers) {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "sol %f %f %f %d %f",
                 s.position.x, s.position.y,
                 s.health,
                 s.active ? 1 : 0,
                 s.shootCooldown);
        out.push_back(buf);
    }
}

bool BuildingSystem::load(const std::vector<std::string>& in) {
    if (in.empty() || in[0] != "BDG_V1") return false;
    buildings.clear();
    tanks.clear();
    soldiers.clear();
    pendingCredits = 0;
    pendingMaterials = 0;

    size_t i = 1;
    auto nextInt = [&](int& out) -> bool {
        if (i >= in.size()) return false;
        out = std::atoi(in[i].c_str());
        ++i;
        return true;
    };

    int buildingCount = 0;
    while (i < in.size() && in[i].find("buildingCount ") != 0) ++i;
    if (i >= in.size()) return false;
    buildingCount = std::atoi(in[i].c_str() + 14);
    ++i;
    for (int b = 0; b < buildingCount && i < in.size(); ++b, ++i) {
        const std::string& line = in[i];
        if (line.rfind("bld ", 0) != 0) { --b; continue; }
        float x=0,y=0,health=0,maxHealth=0,buildTimer=0,productionTimer=0,spawnTimer=0,genTimer=0,shootCooldown=0,healTimer=0,animTimer=0;
        int typeInt=0,level=0,builtInt=0,spawnQueue=0;
        if (sscanf(line.c_str(),
                   "bld %f %f %d %f %f %d %d %f %f %d %f %f %f %f %f",
                   &x, &y, &typeInt, &health, &maxHealth, &level, &builtInt,
                   &buildTimer, &productionTimer, &spawnQueue, &spawnTimer,
                   &genTimer, &shootCooldown, &healTimer, &animTimer) < 11) continue;
        BuildingType t = static_cast<BuildingType>(typeInt);
        if (typeInt < 0 || typeInt >= NUM_TYPES) t = BuildingType::House;
        Building bd(Vector2{x,y}, t);
        bd.health = health;
        bd.maxHealth = maxHealth;
        bd.level = level;
        bd.built = builtInt != 0;
        bd.buildTimer = buildTimer;
        bd.productionTimer = productionTimer;
        bd.spawnQueue = spawnQueue;
        bd.spawnTimer = spawnTimer;
        bd.genTimer = genTimer;
        bd.shootCooldown = shootCooldown;
        bd.healTimer = healTimer;
        bd.animTimer = animTimer;
        applyLevelStats(bd);
        buildings.push_back(bd);
    }

    while (i < in.size() && in[i].find("tankCount ") != 0) ++i;
    if (i < in.size()) {
        int tankCount = std::atoi(in[i].c_str() + 10);
        ++i;
        for (int t = 0; t < tankCount && i < in.size(); ++t, ++i) {
            const std::string& line = in[i];
            if (line.rfind("tnk ", 0) != 0) { --t; continue; }
            float x=0,y=0,health=0,shootCooldown=0,orbitAngle=0;
            int active=0;
            if (sscanf(line.c_str(), "tnk %f %f %f %d %f %f",
                       &x, &y, &health, &active, &shootCooldown, &orbitAngle) < 4) continue;
            FriendlyTank ft;
            ft.position = {x,y};
            ft.health = health;
            ft.active = active != 0;
            ft.shootCooldown = shootCooldown;
            ft.orbitAngle = orbitAngle;
            tanks.push_back(ft);
        }
    }

    while (i < in.size() && in[i].find("soldierCount ") != 0) ++i;
    if (i < in.size()) {
        int soldierCount = std::atoi(in[i].c_str() + 13);
        ++i;
        for (int s = 0; s < soldierCount && i < in.size(); ++s, ++i) {
            const std::string& line = in[i];
            if (line.rfind("sol ", 0) != 0) { --s; continue; }
            float x=0,y=0,health=0,shootCooldown=0;
            int active=0;
            if (sscanf(line.c_str(), "sol %f %f %f %d %f",
                       &x, &y, &health, &active, &shootCooldown) < 4) continue;
            FriendlySoldier fs;
            fs.position = {x,y};
            fs.health = health;
            fs.active = active != 0;
            fs.shootCooldown = shootCooldown;
            soldiers.push_back(fs);
        }
    }
    return true;
}
