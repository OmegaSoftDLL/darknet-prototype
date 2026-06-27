#include "Companion.h"
#include "Enemy.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
extern bool g_renderPass3D;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Contador para distribuir companheiros em "vagas" de formacao distintas
static int s_companionSpawnIndex = 0;

// ─── Constructor ─────────────────────────────────────────────────────────────

Companion::Companion(CompanionType t) : type(t) {
    formationPhase = (float)(s_companionSpawnIndex++ % 6) * (float)(M_PI / 3.0);

    switch (t) {
        case CompanionType::MarcoVeil:
            name = "MARCO VEIL"; health = maxHealth = 90.f; speed = 145.f; radius = 13.f;
            damage = 10.f; attackRate = 1.6f; skillRate = 6.5f;
            shootDamage = 16.f; shootSpeed = 430.f; shootRange = 300.f; shootRate2 = 1.5f;
            projectileColor = {0, 220, 255, 255};
            break;
        case CompanionType::Steel:
            name = "STEEL"; health = maxHealth = 240.f; speed = 95.f; radius = 18.f;
            damage = 30.f; attackRate = 1.1f; skillRate = 9.f;
            shootDamage = 0.f; shootRange = 42.f; aoeRadius = 90.f; aoeDamage = 45.f;
            break;
        case CompanionType::Rex:
            name = "REX"; health = maxHealth = 70.f; speed = 190.f; radius = 10.f;
            damage = 22.f; attackRate = 0.7f; skillRate = 7.f;
            shootDamage = 0.f; shootRange = 0.f;
            break;
        case CompanionType::Guardian:
            name = "ATLAS"; health = maxHealth = 320.f; speed = 105.f; radius = 19.f;
            damage = 26.f; attackRate = 1.2f; skillRate = 8.f;
            shootDamage = 0.f; shootRange = 46.f; aoeRadius = 100.f; aoeDamage = 38.f;
            break;
        case CompanionType::Sniper:
            name = "VIPER"; health = maxHealth = 65.f; speed = 130.f; radius = 12.f;
            damage = 8.f; attackRate = 2.4f; skillRate = 10.f;
            shootDamage = 60.f; shootSpeed = 720.f; shootRange = 520.f; shootRate2 = 2.6f;
            projectileColor = {255, 80, 60, 255};
            break;
        case CompanionType::Healer:
            name = "MEDIC"; health = maxHealth = 80.f; speed = 150.f; radius = 12.f;
            damage = 0.f; attackRate = 1.f; skillRate = 12.f;
            shootDamage = 0.f; shootRange = 0.f; healRadius = 220.f;
            projectileColor = {80, 255, 140, 255};
            break;
        case CompanionType::LootDrone:
            name = "MAGNET"; health = maxHealth = 60.f; speed = 210.f; radius = 11.f;
            damage = 0.f; collectRadius = 180.f;
            projectileColor = {255, 210, 60, 255};
            break;
    }
    active = true;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

void Companion::emote(const char* txt, float dur) {
    emoteText = txt; emoteTimer = dur;
}

void Companion::followFormation(float dt, Vector2 playerPos, float desiredDist) {
    // Orbita lentamente o player numa vaga propria -> companheiros se espalham
    formationPhase += dt * 0.4f;
    Vector2 slot = {
        playerPos.x + std::cos(formationPhase) * desiredDist,
        playerPos.y + std::sin(formationPhase) * desiredDist
    };
    Vector2 to = Vector2Subtract(slot, position);
    float d = Vector2Length(to);
    if (d > 6.f) {
        Vector2 dir = Vector2Scale(to, 1.f / d);
        float sp = speed * (d > desiredDist * 1.8f ? 1.4f : 0.85f); // corre se ficou longe
        position.x += dir.x * sp * dt;
        position.y += dir.y * sp * dt;
        if (std::fabs(dir.x) > 0.2f) facing = (dir.x >= 0.f) ? 1 : -1;
    }
}

void Companion::retreatTo(float dt, Vector2 playerPos) {
    Vector2 to = Vector2Subtract(playerPos, position);
    float d = Vector2Length(to);
    if (d > 35.f) {
        Vector2 dir = Vector2Scale(to, 1.f / d);
        position.x += dir.x * speed * 1.3f * dt;
        position.y += dir.y * speed * 1.3f * dt;
        facing = (dir.x >= 0.f) ? 1 : -1;
    }
}

Enemy* Companion::findNearestEnemy(const std::vector<Enemy*>& enemies, float maxRange) const {
    Enemy* nearest = nullptr; float best = maxRange;
    for (Enemy* e : enemies) {
        if (!e || e->isDead()) continue;
        float d = Vector2Distance(position, e->position);
        if (d < best) { best = d; nearest = e; }
    }
    return nearest;
}

int Companion::countEnemies(const std::vector<Enemy*>& enemies, Vector2 c, float r) const {
    int n = 0;
    for (Enemy* e : enemies) if (e && !e->isDead() && Vector2Distance(c, e->position) <= r) n++;
    return n;
}

// ─── Update ──────────────────────────────────────────────────────────────────

void Companion::update(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies) {
    wantsToShoot = false; wantsSkill = false; wantsAoE = false;
    wantsHeal = false; healAmount = 0.f; wantsCollect = false; taunting = false;

    if (hitFlash   > 0.f) hitFlash   -= dt;
    if (skillFlash > 0.f) skillFlash -= dt;
    if (emoteTimer > 0.f) emoteTimer -= dt;

    if (isDead()) {
        deadTimer += dt;
        // Revive mais rapido se o player estiver por perto (resgate)
        float rate = (Vector2Distance(position, playerPos) < 90.f) ? 1.8f : 1.0f;
        deadTimer += dt * (rate - 1.0f);
        if (deadTimer >= deadDuration) { revive(playerPos); emote("Voltei a ativa!"); }
        return;
    }

    // Estado de alerta + emote ao avistar inimigos
    bool wasAlerted = alerted;
    alerted = (findNearestEnemy(nearbyEnemies, 360.f) != nullptr);
    if (alerted && !wasAlerted) {
        const char* lines[] = {"Inimigos!", "Em guarda!", "Eu cubro voce!", "Vamos nessa!"};
        emote(lines[GetRandomValue(0,3)], 1.6f);
    }

    // Emote ocioso ocasional
    idleEmoteCD -= dt;
    if (!alerted && idleEmoteCD <= 0.f) {
        idleEmoteCD = (float)GetRandomValue(8, 16);
        const char* idle[] = {"Tudo limpo.", "De olho no perimetro.", "Comigo, chefe.", "..."};
        emote(idle[GetRandomValue(0,3)], 2.0f);
    }

    // HP baixo -> recua para perto do player
    if (health < maxHealth * 0.25f && alerted) {
        retreatTo(dt, playerPos);
        if (GetRandomValue(0, 400) == 0) emote("Preciso recuar!");
        walkTimer += dt;
        // ainda pode atirar de longe se for atirador
        if (type == CompanionType::MarcoVeil || type == CompanionType::Sniper) {
            Enemy* t = findNearestEnemy(nearbyEnemies, shootRange);
            if (t && shootCooldown <= 0.f) {
                shootCooldown = shootRate2;
                Vector2 d = Vector2Normalize(Vector2Subtract(t->position, position));
                shootDir = d; wantsToShoot = true; facing = (d.x >= 0 ? 1 : -1);
            }
        }
        if (shootCooldown > 0.f) shootCooldown -= dt;
        return;
    }

    switch (type) {
        case CompanionType::MarcoVeil: updateMarcoVeil(dt, playerPos, nearbyEnemies); break;
        case CompanionType::Steel:     updateSteel    (dt, playerPos, nearbyEnemies); break;
        case CompanionType::Rex:       updateRex      (dt, playerPos, nearbyEnemies); break;
        case CompanionType::Guardian:  updateGuardian (dt, playerPos, nearbyEnemies); break;
        case CompanionType::Sniper:    updateSniper   (dt, playerPos, nearbyEnemies); break;
        case CompanionType::Healer:    updateHealer   (dt, playerPos, nearbyEnemies); break;
        case CompanionType::LootDrone: updateLootDrone(dt, playerPos, nearbyEnemies); break;
    }
    walkTimer += dt;
}

void Companion::updateMarcoVeil(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies) {
    if (burstCount > 0) {
        burstTimer -= dt;
        if (burstTimer <= 0.f) { wantsToShoot = true; burstCount--; burstTimer = burstInterval; }
    }
    if (shootCooldown > 0.f) shootCooldown -= dt;
    if (skillCooldown > 0.f) skillCooldown -= dt;

    Enemy* target = findNearestEnemy(nearbyEnemies, shootRange);
    if (target) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(target->position, position));
        facing = (dir.x >= 0.f) ? 1 : -1;
        float distToEnemy = Vector2Distance(position, target->position);
        // Kiting: mantem distancia media (~150px)
        if (distToEnemy < 130.f) { position.x -= dir.x * speed * 0.7f * dt; position.y -= dir.y * speed * 0.7f * dt; }
        else if (distToEnemy > 240.f) { position.x += dir.x * speed * 0.5f * dt; position.y += dir.y * speed * 0.5f * dt; }
        if (shootCooldown <= 0.f && burstCount == 0) { shootCooldown = shootRate2; wantsToShoot = true; shootDir = dir; }
        if (skillCooldown <= 0.f && burstCount == 0) {
            skillCooldown = skillRate; burstCount = 2; burstTimer = burstInterval;
            wantsToShoot = true; shootDir = dir; wantsSkill = true; skillFlash = 0.3f;
            emote("Rajada!");
        }
    } else followFormation(dt, playerPos, 70.f);
}

void Companion::updateSteel(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies) {
    if (attackCooldown > 0.f) attackCooldown -= dt;
    if (skillCooldown  > 0.f) skillCooldown  -= dt;
    Enemy* target = findNearestEnemy(nearbyEnemies, 220.f);
    if (target) {
        taunting = true; // postura de guarda (Game pode usar p/ atrair aggro futuramente)
        // Guardiao: posiciona-se ENTRE o player e o inimigo (body-block)
        Vector2 mid = { (playerPos.x + target->position.x) * 0.5f,
                        (playerPos.y + target->position.y) * 0.5f };
        Vector2 dir = Vector2Subtract(target->position, position);
        float dist = Vector2Length(dir); if (dist > 0.f) dir = Vector2Scale(dir, 1.f/dist);
        facing = (dir.x >= 0.f) ? 1 : -1;
        Vector2 toMid = Vector2Subtract(mid, position);
        float dMid = Vector2Length(toMid);
        if (dist > shootRange) {
            // anda em direcao ao ponto de bloqueio entre player e inimigo
            Vector2 m = (dMid > 20.f) ? Vector2Scale(toMid, 1.f/dMid) : dir;
            position.x += m.x * speed * dt; position.y += m.y * speed * dt;
        } else {
            if (attackCooldown <= 0.f) { attackCooldown = attackRate; target->takeDamage(damage); }
            if (skillCooldown <= 0.f) { skillCooldown = skillRate; wantsAoE = true; wantsSkill = true; skillFlash = 0.3f; emote("PISADA!"); }
        }
    } else followFormation(dt, playerPos, 55.f);
}

void Companion::updateRex(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies) {
    if (attackCooldown > 0.f) attackCooldown -= dt;
    if (skillCooldown  > 0.f) skillCooldown  -= dt;
    if (isRushing) {
        rushTimer -= dt;
        if (rushTimer <= 0.f || Vector2Distance(position, rushTarget) < 12.f) isRushing = false;
        else {
            Vector2 dir = Vector2Subtract(rushTarget, position);
            float len = Vector2Length(dir);
            if (len > 0.f) {
                dir = Vector2Scale(dir, 1.f/len);
                position.x += dir.x * speed * 2.6f * dt; position.y += dir.y * speed * 2.6f * dt;
                facing = (dir.x >= 0.f) ? 1 : -1;
                for (Enemy* e : nearbyEnemies) {
                    if (!e || e->isDead()) continue;
                    if (Vector2Distance(position, e->position) <= radius + e->radius + 5.f && attackCooldown <= 0.f) {
                        attackCooldown = attackRate; e->takeDamage(damage * 1.6f);
                    }
                }
            }
        }
        return;
    }
    Enemy* target = findNearestEnemy(nearbyEnemies, 230.f);
    if (target) {
        Vector2 dir = Vector2Subtract(target->position, position);
        float dist = Vector2Length(dir); if (dist > 0.f) dir = Vector2Scale(dir, 1.f/dist);
        facing = (dir.x >= 0.f) ? 1 : -1;
        if (dist > radius + target->radius) { position.x += dir.x * speed * dt; position.y += dir.y * speed * dt; }
        else if (attackCooldown <= 0.f) { attackCooldown = attackRate; target->takeDamage(damage); }
        if (skillCooldown <= 0.f) {
            skillCooldown = skillRate; isRushing = true; rushTimer = 3.f; skillFlash = 0.3f;
            float ang = std::atan2(dir.y, dir.x) + (float)M_PI * 0.5f;
            rushTarget = { target->position.x + std::cos(ang)*90.f, target->position.y + std::sin(ang)*90.f };
            wantsSkill = true; emote("AU AU!");
        }
    } else { isRushing = false; followFormation(dt, playerPos, 50.f); }
}

void Companion::updateGuardian(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies) {
    // Igual ao Steel porem mais resistente e sempre fica na frente da maior ameaca
    updateSteel(dt, playerPos, nearbyEnemies);
    taunting = true;
}

void Companion::updateSniper(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies) {
    if (shootCooldown > 0.f) shootCooldown -= dt;
    Enemy* target = findNearestEnemy(nearbyEnemies, shootRange);
    if (target) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(target->position, position));
        facing = (dir.x >= 0.f) ? 1 : -1;
        float d = Vector2Distance(position, target->position);
        // mantem MUITA distancia
        if (d < 260.f) { position.x -= dir.x * speed * dt; position.y -= dir.y * speed * dt; }
        if (shootCooldown <= 0.f) {
            shootCooldown = shootRate2; wantsToShoot = true; shootDir = dir; skillFlash = 0.25f;
            if (GetRandomValue(0,3)==0) emote("Alvo travado.");
        }
    } else followFormation(dt, playerPos, 90.f);
}

void Companion::updateHealer(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies) {
    (void)nearbyEnemies;
    // Fica perto do player e emite cura (Game precisa LER wantsHeal/healAmount)
    followFormation(dt, playerPos, 60.f);
    if (skillCooldown > 0.f) skillCooldown -= dt;
    if (Vector2Distance(position, playerPos) <= healRadius) {
        wantsHeal = true;
        healAmount = 8.0f * dt;       // cura por segundo (Game aplica ao player)
        if (skillCooldown <= 0.f) { skillCooldown = skillRate; skillFlash = 0.3f; emote("Curando!"); }
    }
}

void Companion::updateLootDrone(float dt, Vector2 playerPos, const std::vector<Enemy*>& nearbyEnemies) {
    (void)nearbyEnemies;
    // Vagueia rapido ao redor coletando (Game precisa LER wantsCollect/collectRadius)
    followFormation(dt, playerPos, 75.f);
    wantsCollect = true;
}

// ─── Damage / Death / Revive ─────────────────────────────────────────────────

void Companion::takeDamage(float dmg) {
    if (isDead()) return;
    float before = health;
    health -= dmg; hitFlash = 0.15f;
    if (health < 0.f) health = 0.f;
    if (health <= 0.f && before > 0.f) emote("Aaargh!");
}

bool Companion::isDead() const { return health <= 0.f; }

void Companion::revive(Vector2 pos) {
    health = maxHealth * 0.4f; position = pos; deadTimer = 0.f;
    isRushing = false; burstCount = 0;
}

// ─── Render ──────────────────────────────────────────────────────────────────

void Companion::renderHpBar() const {
    int bw = 36, bx = (int)position.x - bw/2, by = (int)position.y - (int)radius - 16;
    float pct = health / maxHealth;
    DrawRectangle(bx, by, bw, 6, {30,30,30,200});
    Color hp = (pct>0.5f)?Color{0,220,60,255}:(pct>0.25f)?Color{220,180,0,255}:Color{220,40,40,255};
    DrawRectangle(bx, by, (int)(bw*pct), 6, hp);
    DrawRectangleLines(bx, by, bw, 6, {180,180,180,160});
    // indicador de skill pronta
    if (skillCooldown <= 0.05f && type != CompanionType::LootDrone)
        DrawCircle(bx + bw + 6, by + 3, 3.0f, {255,220,0,255});
}

void Companion::renderEmote() const {
    if (emoteTimer <= 0.f || emoteText.empty()) return;
    int w = MeasureText(emoteText.c_str(), 11);
    int ex = (int)position.x - w/2 - 4;
    int ey = (int)position.y - (int)radius - 44;
    DrawRectangle(ex, ey, w + 8, 16, ColorAlpha(BLACK, 0.7f));
    DrawText(emoteText.c_str(), ex + 4, ey + 3, 11, {255,255,210,255});
}

void Companion::renderSkillReady() const {
    if (skillFlash > 0.f) DrawCircleLines((int)position.x, (int)position.y,
        radius + 6.0f + skillFlash * 20.0f, ColorAlpha(projectileColor, skillFlash));
}

void Companion::render() const {
    if (!active) return;
    if (isDead()) {
        Color deadCol = {80,80,80,180};
        if (!g_renderPass3D) {
            DrawEllipse((int)position.x, (int)position.y + 6, radius*1.8f, radius*0.6f, deadCol);
        }
        float pct = deadTimer / deadDuration; int bw = 36;
        DrawRectangle((int)position.x - bw/2, (int)position.y - 28, bw, 5, {50,50,50,200});
        DrawRectangle((int)position.x - bw/2, (int)position.y - 28, (int)(bw*pct), 5, {200,200,0,220});
        DrawText("CAIDO", (int)position.x - 16, (int)position.y - 40, 10, {255,200,0,200});
        return;
    }
    renderSkillReady();
    renderHpBar();
    switch (type) {
        case CompanionType::MarcoVeil: renderMarcoVeil(); break;
        case CompanionType::Steel:     renderSteel();     break;
        case CompanionType::Rex:       renderRex();       break;
        case CompanionType::Guardian:  renderGuardian();  break;
        case CompanionType::Sniper:    renderSniper();    break;
        case CompanionType::Healer:    renderDrone({80,255,140,255});  break;
        case CompanionType::LootDrone: renderDrone({255,210,60,255});  break;
    }
    renderEmote();
}

void Companion::renderMarcoVeil() const {
    bool fl = hitFlash > 0.f && ((int)(hitFlash*20.f)%2==0);
    Color body = fl?WHITE:Color{100,75,40,255}, pants = fl?WHITE:Color{55,70,35,255};
    Color skin = fl?WHITE:Color{200,160,120,255}, helmet = fl?WHITE:Color{60,55,35,255};
    float bob = std::sin(walkTimer*8.f)*3.f; int px=(int)position.x, py=(int)position.y;
    DrawRectangle(px-6, py+4, 5, 9+(int)bob, pants);
    DrawRectangle(px+1, py+4, 5, 9-(int)bob, pants);
    DrawRectangle(px-8, py-8, 16, 14, body);
    DrawRectangle(px-12, py-6, 5, 10, body); DrawRectangle(px+7, py-6, 5, 10, body);
    DrawCircle(px, py-14, 9, skin);
    DrawRectangle(px-9, py-22, 18, 10, helmet); DrawRectangle(px-6, py-26, 12, 5, helmet);
    int rfx = px + (facing>=0?8:-8);
    DrawRectangle(rfx-2, py-5, 4, 14, {70,70,70,255}); DrawRectangle(rfx-1, py-3, 2, 18, {50,50,50,255});
    int nw = MeasureText(name.c_str(),10);
    DrawText(name.c_str(), px-nw/2, py-(int)radius-28, 10, {100,220,255,230});
}

void Companion::renderSteel() const {
    bool fl = hitFlash > 0.f && ((int)(hitFlash*20.f)%2==0);
    Color metal=fl?WHITE:Color{160,160,165,255}, dark=fl?WHITE:Color{80,80,88,255}, joint=fl?WHITE:Color{110,110,118,255};
    Color eye={0,150,255,255};
    float bob = std::sin(walkTimer*5.f)*2.f; int px=(int)position.x, py=(int)position.y;
    DrawRectangle(px-7,py+4,6,12+(int)bob,metal); DrawRectangle(px+1,py+4,6,12-(int)bob,metal);
    DrawCircle(px-4,py+10,4,joint); DrawCircle(px+4,py+10,4,joint);
    DrawRectangle(px-10,py+15,9,4,dark); DrawRectangle(px+1,py+15,9,4,dark);
    DrawRectangle(px-10,py-10,20,16,metal);
    for (int r=0;r<3;++r) DrawLine(px-9,py-7+r*5,px+9,py-7+r*5,dark);
    DrawRectangle(px-15,py-9,5,14,metal); DrawRectangle(px+10,py-9,5,14,metal);
    DrawRectangle(px-16,py+4,7,6,dark); DrawRectangle(px+9,py+4,7,6,dark);
    DrawCircle(px,py-18,11,metal); DrawRectangle(px-7,py-14,14,6,dark);
    DrawCircle(px-4,py-20,4,eye); DrawCircle(px+4,py-20,4,eye);
    if (taunting) DrawCircleLines(px, py, radius + 10.0f, ColorAlpha(Color{0,180,255,255}, 0.4f));
    const char* tag = name.c_str(); int tw=MeasureText(tag,10);
    DrawText(tag, px-tw/2, py-(int)radius-30, 10, {0,200,255,230});
}

void Companion::renderRex() const {
    bool fl = hitFlash > 0.f && ((int)(hitFlash*20.f)%2==0);
    Color body=fl?WHITE:Color{60,65,70,255}, leg=fl?WHITE:Color{80,85,90,255}, sensor={0,255,180,255};
    float lb = std::sin(walkTimer*12.f)*4.f; int px=(int)position.x, py=(int)position.y;
    DrawRectangle(px-12,py-4,24,10,body);
    DrawRectangle(px-11,py+4,4,7+(int)lb,leg); DrawRectangle(px-13,py+10+(int)lb,7,3,leg);
    DrawRectangle(px-3,py+4,4,7-(int)lb,leg);  DrawRectangle(px-5,py+10-(int)lb,7,3,leg);
    DrawRectangle(px+4,py+4,4,7-(int)lb,leg);  DrawRectangle(px+2,py+10-(int)lb,7,3,leg);
    DrawRectangle(px+8,py+4,4,7+(int)lb,leg);  DrawRectangle(px+6,py+10+(int)lb,7,3,leg);
    int hx = px + facing*10; DrawRectangle(hx-6,py-7,12,9,body);
    DrawCircle(hx+facing*3, py-4, 4, sensor); DrawCircle(hx+facing*3, py-4, 2, WHITE);
    DrawLine(px-facing*12,py,px-facing*18,py-6,leg);
    if (isRushing) DrawCircleLines(px, py, radius + 4.0f, ColorAlpha(sensor, 0.6f));
    int nw=MeasureText(name.c_str(),10);
    DrawText(name.c_str(), px-nw/2, py-(int)radius-24, 10, {0,255,180,220});
}

void Companion::renderGuardian() const {
    // Tanque pesado com escudo frontal
    bool fl = hitFlash > 0.f && ((int)(hitFlash*20.f)%2==0);
    Color metal=fl?WHITE:Color{120,130,150,255}, dark=fl?WHITE:Color{60,70,90,255};
    int px=(int)position.x, py=(int)position.y;
    float bob = std::sin(walkTimer*4.f)*2.f;
    DrawRectangle(px-8,py+4,7,12+(int)bob,metal); DrawRectangle(px+1,py+4,7,12-(int)bob,metal);
    DrawRectangle(px-12,py-12,24,20,metal);
    DrawRectangle(px-12,py-12,24,4,dark);
    DrawCircle(px,py-18,11,metal);
    DrawCircle(px-4,py-20,3,{0,200,255,255}); DrawCircle(px+4,py-20,3,{0,200,255,255});
    // escudo grande na frente
    int sx = px + facing*14;
    DrawRectangle(sx-3, py-16, 6, 30, {90,110,140,255});
    DrawRectangleLines(sx-3, py-16, 6, 30, {150,180,220,255});
    if (taunting) DrawCircleLines(px, py, radius + 12.0f, ColorAlpha(Color{0,180,255,255}, 0.45f));
    int nw=MeasureText(name.c_str(),10);
    DrawText(name.c_str(), px-nw/2, py-(int)radius-30, 10, {120,200,255,230});
}

void Companion::renderSniper() const {
    bool fl = hitFlash > 0.f && ((int)(hitFlash*20.f)%2==0);
    Color body=fl?WHITE:Color{40,50,60,255}, cloak=fl?WHITE:Color{30,60,50,255}, skin={200,160,120,255};
    int px=(int)position.x, py=(int)position.y; float bob=std::sin(walkTimer*7.f)*2.f;
    DrawRectangle(px-5,py+4,4,9+(int)bob,body); DrawRectangle(px+1,py+4,4,9-(int)bob,body);
    DrawRectangle(px-7,py-9,14,15,cloak);
    DrawCircle(px,py-14,8,skin);
    DrawRectangle(px-8,py-22,16,9,body);              // capuz
    DrawCircle(px+facing*2, py-15, 2, {255,80,60,255}); // mira/olho
    // rifle longo
    int rfx = px + facing*9;
    DrawRectangle(rfx-2, py-6, facing*22, 3, {40,40,45,255});
    DrawRectangle(rfx + facing*16, py-7, 3, 4, {255,80,60,200}); // scope glint
    int nw=MeasureText(name.c_str(),10);
    DrawText(name.c_str(), px-nw/2, py-(int)radius-28, 10, {255,120,100,230});
}

void Companion::renderDrone(Color tint) const {
    bool fl = hitFlash > 0.f && ((int)(hitFlash*20.f)%2==0);
    int px=(int)position.x, py=(int)position.y;
    float hover = std::sin(walkTimer*6.f)*3.f;        // flutua
    int cy = py - 6 + (int)hover;
    // sombra (flutuando -> sombra menor embaixo)
    if (!g_renderPass3D) {
        DrawEllipse(px, py+10, 8.0f, 3.0f, ColorAlpha(BLACK,0.3f));
    }
    // rotores
    float rot = walkTimer * 30.f;
    for (int i=0;i<2;i++){
        int rx = px + (i==0?-10:10);
        DrawLineEx({(float)rx-6,(float)cy-6},{(float)rx+6,(float)cy-6}, 2.0f, ColorAlpha(WHITE,0.5f));
        DrawCircle(rx, cy-6, 2.0f, {120,120,130,255});
        (void)rot;
    }
    // corpo
    Color body = fl?WHITE:Color{70,75,85,255};
    DrawRectangle(px-8, cy-4, 16, 10, body);
    DrawCircle(px, cy+1, 4.0f, tint);                  // nucleo brilhante
    DrawCircle(px, cy+1, 2.0f, WHITE);
    // feixe (cura/coleta)
    DrawCircleLines(px, cy, radius + 8.0f + std::sin(walkTimer*3.f)*3.0f, ColorAlpha(tint, 0.35f));
    int nw=MeasureText(name.c_str(),10);
    DrawText(name.c_str(), px-nw/2, py-(int)radius-26, 10, ColorAlpha(tint, 0.9f));
}
