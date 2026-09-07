#include "Player.h"
#include "Effects.h"
#include "SkillTree.h"
#include <cmath>
#include <string>
extern bool g_voxelCapture;

Player::Player() {
    // Skills balanceadas: basico fast/down damage, controle medio, burst high/cd longo
    skills.emplace_back("Laser",     "Radius of high energy that perfura",  1.0f, 48.0f,  560.0f, KEY_ONE);
    skills.emplace_back("EMP",       "Pulso EMP in area - atordoa",       5.0f, 95.0f,  180.0f, KEY_TWO);
    skills.emplace_back("Grenade",   "Grenade of plasma with AoE massivo", 7.0f,140.0f,  180.0f, KEY_THREE);
    skills.emplace_back("Overload","Damage +50% Vel +30% by 8s",        16.0f, 0.0f,   0.0f,  KEY_FOUR);
    skills.emplace_back("Barrier",  "Campo of strength: immune 3s",         24.0f, 0.0f,   0.0f,  KEY_FIVE);
    skills.emplace_back("Burst",    "Saves: 8 projetos in leque",        9.0f, 42.0f,  440.0f, KEY_SIX);

    // Guarda the damage/range/cooldown base of cada skill to aplicar mods
    // (skillPower of the class + perks) of forma idempotent
    for (auto& s : skills) {
        baseSkillDamage.push_back(s.damage);
        baseSkillRange.push_back(s.range);
        baseSkillCool.push_back(s.cooldown);
    }

    applyClass(CharacterClass::Soldado);  // class padrao
}

// ── Sistema of classes ───────────────────────────────────────────────────────
void Player::applyClass(CharacterClass c) {
    charClass = c;
    float skillMult = 1.0f;
    switch (c) {
        case CharacterClass::Soldado:
            baseMaxHealth=100; baseSpeed=165; baseAttackDamage=25; baseAttackRange=90; baseDefense=8;
            classPrimary={35,55,120,255}; classSecondary={55,90,180,255}; classAccent={0,210,255,255};
            classSkin={180,135,105,255}; classTrim={160,170,185,255}; skillMult=1.0f; break;
        case CharacterClass::Guerreira:
            baseMaxHealth=85; baseSpeed=195; baseAttackDamage=30; baseAttackRange=80; baseDefense=3;
            classPrimary={120,30,60,255}; classSecondary={200,60,90,255}; classAccent={255,90,140,255};
            classSkin={205,160,130,255}; classTrim={230,210,140,255}; skillMult=1.0f; break;
        case CharacterClass::Robo:
            baseMaxHealth=160; baseSpeed=135; baseAttackDamage=24; baseAttackRange=95; baseDefense=22;
            classPrimary={90,95,105,255}; classSecondary={140,150,165,255}; classAccent={255,140,0,255};
            classSkin={120,130,140,255}; classTrim={205,210,220,255}; skillMult=0.9f; break;
        case CharacterClass::Mago:
            baseMaxHealth=70; baseSpeed=160; baseAttackDamage=14; baseAttackRange=120; baseDefense=2;
            classPrimary={40,30,110,255}; classSecondary={80,60,200,255}; classAccent={120,200,255,255};
            classSkin={210,180,150,255}; classTrim={235,210,120,255}; skillMult=2.0f; break;  // glass cannon magic
        case CharacterClass::Bruxa:
            baseMaxHealth=90; baseSpeed=175; baseAttackDamage=22; baseAttackRange=100; baseDefense=5;
            classPrimary={60,20,80,255}; classSecondary={120,50,150,255}; classAccent={180,120,255,255};
            classSkin={215,175,150,255}; classTrim={120,230,160,255}; skillMult=1.35f; break;
        case CharacterClass::HomemFera:
            baseMaxHealth=120; baseSpeed=185; baseAttackDamage=40; baseAttackRange=70; baseDefense=10;
            classPrimary={90,60,30,255}; classSecondary={140,95,50,255}; classAccent={255,180,40,255};
            classSkin={150,110,70,255}; classTrim={60,45,25,255}; skillMult=1.0f; break;
        default: break;
    }
    skillPower = skillMult;
    refreshSkillVectors();

    applyEquipmentStats();   // mantem bonus of equipment/evolution
    health = maxHealth;
}

void Player::refreshSkillVectors() {
    if (baseSkillDamage.size() != skills.size()) return;
    auto st = SkillTree::statsFor(perkMask);
    for (size_t i = 0; i < skills.size(); ++i) {
        skills[i].damage   = baseSkillDamage[i]  * skillPower * st.skillMult;
        skills[i].range    = baseSkillRange[i]   * st.rangeMult;
        skills[i].cooldown = baseSkillCool[i]    * cdEvoMult * st.cdMult;
    }
}

const char* Player::className(CharacterClass c) {
    switch (c) {
        case CharacterClass::Soldado:   return "SOLDADO";
        case CharacterClass::Guerreira: return "GUERREIRA";
        case CharacterClass::Robo:      return "ROBO DE COMBAT";
        case CharacterClass::Mago:      return "MAGO";
        case CharacterClass::Bruxa:     return "BRUXA";
        case CharacterClass::HomemFera: return "HOMEM-FERA";
        default: return "?";
    }
}

const char* Player::classDescription(CharacterClass c) {
    switch (c) {
        case CharacterClass::Soldado:   return "Equilibrado. Armor, rifle. Bom to begin.";
        case CharacterClass::Guerreira: return "Rapida and agil. Pistolas duplas. Little defense.";
        case CharacterClass::Robo:      return "Tanque. Muito HP and defense, mas slow.";
        case CharacterClass::Mago:      return "Frageil, mas skills devastadoras (x2 damage).";
        case CharacterClass::Bruxa:     return "Magica equilibrada. Boa mobilidade and skills.";
        case CharacterClass::HomemFera: return "Brutamonte. Damage body the body altissimo, fast.";
        default: return "";
    }
}

const char* Player::classFantasy(CharacterClass c) {
    switch (c) {
        case CharacterClass::Soldado:   return "Veterano of the stamina NEXUS, middle homem middle maquina.";
        case CharacterClass::Guerreira: return "Cacadora that never errou um alvo in movement.";
        case CharacterClass::Robo:      return "Unidade aliada reprogramada to protect the humanidade.";
        case CharacterClass::Mago:      return "Last arcanista the dominar codigo and feitico to the same time.";
        case CharacterClass::Bruxa:     return "Tece magic antiga contra the fria logica of the KRONOS.";
        case CharacterClass::HomemFera: return "Maldicao genetica virou your maior weapon in the guerra.";
        default: return "";
    }
}

Color Player::accentNow() const {
    if (isShielded())   return {0,255,200,255};
    if (isOverloaded()) return {255,190,0,255};
    return classAccent;
}

void Player::move(Vector2 direction, float dt) {
    float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len < 0.01f) return;

    direction.x /= len;
    direction.y /= len;
    float currentSpeed = speed * (isOverloaded() ? 1.3f : 1.0f) * (sprinting ? 1.7f : 1.0f);

    // Acceleration responsiva mas with peso. Sem drag competindo (see update),
    // the speed atinge 100% of the alvo — end of the sensation "slow/float".
    float t = 1.0f - std::exp(-24.0f * dt);
    velocity.x += (direction.x * currentSpeed - velocity.x) * t;
    velocity.y += (direction.y * currentSpeed - velocity.y) * t;

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    isMoving      = true;
    moveRequested = true;
    if (velocity.x >  12.0f) facing =  1;
    if (velocity.x < -12.0f) facing = -1;
    // A cadencia of the passada and avancada in update() (evita contar 2x and acelerar demais).
}

void Player::update(float dt) {
    isMoving = false;

    // Healing: cooldown of the potion + buff of regeneracao active (estilo Diablo 3).
    if (healCooldown > 0.0f) healCooldown -= dt;
    if (regenTimer   > 0.0f) {
        regenTimer -= dt;
        heal(maxHealth * 0.06f * dt);   // ~6% maxHP by second enquanto hard
    }

    // Perk Hack Tree: Regenerador (HP/s) + timer of the Protocolo Imortal
    {
        auto st = SkillTree::statsFor(perkMask);
        if (st.regen > 0.0f) heal(st.regen * dt);
        if (!reviveReady) {
            reviveTimer -= dt;
            if (reviveTimer <= 0.0f) reviveReady = true;
        }
    }

    // Coast/freio: SO desacelera when NOT houve input in the frame previous.
    // Enquanto anda, not ha drag competindo with the acceleration -> speed plena.
    if (!moveRequested) {
        float drag = 1.0f - std::exp(-22.0f * dt);  // stopped firme, without deslizar
        velocity.x -= velocity.x * drag;
        velocity.y -= velocity.y * drag;
    }
    moveRequested = false;

    // Avanca the animation of caminhada pela speed real (anima same in the inercia)
    {
        float vm = std::sqrt(velocity.x*velocity.x + velocity.y*velocity.y);
        if (vm > 12.0f) walkAnimTimer += dt * (4.0f + 5.0f * (vm / (speed + 1.0f)));
    }

    // Pulo (z-height) — arco of salto to cruzar obstaculos/go up
    if (isJumping) {
        jumpZ    += jumpVel * dt;
        jumpVel  -= 900.0f * dt;       // gravity
        if (jumpZ <= 0.0f) { jumpZ = 0.0f; jumpVel = 0.0f; isJumping = false; }
    }

    for (auto& skill : skills) skill.update(dt);

    if (overloadTimer > 0.0f) overloadTimer -= dt;
    if (shieldTimer   > 0.0f) shieldTimer   -= dt;

    if (levelUpTimer > 0.0f) levelUpTimer -= dt;
    leveledUp = levelUpTimer > 0.0f;
    evolutionPulse += dt;
}

void Player::absorbBossEssence(int kind) {
    // Mexe in the stat BASE (not in the efetivo) for the buff be PERMANENTE.
    switch (kind) {
        case 0: baseMaxHealth    *= 1.10f; break;
        case 1: baseAttackDamage *= 1.10f; break;
        case 2: baseDefense      += 4.0f;  break;
        case 3: baseSpeed        *= 1.06f; break;
        default: break;
    }
    applyEquipmentStats();
    health = maxHealth;
}

void Player::render() const {
    // Ciclo of caminhada with peso: legs alternam and the body afunda the cada
    // pisada (footfall), dando sensation of pisar in the floor instead of flutuar.
    // "Andando" pela SPEED real (not pela flag isMoving, that and zerada in
    // update() before the render -> causava character deslizando/"voando").
    float velMag = std::sqrt(velocity.x*velocity.x + velocity.y*velocity.y);
    bool  moving = velMag > 12.0f;
    float walk   = moving ? std::sin(walkAnimTimer) : 0.0f;
    float walkAbs= moving ? std::fabs(walk) : 0.0f;
    float legL   =  walk * 19.0f;   // passos maiores and visiveis
    float legR   = -walk * 19.0f;
    // Pisada: the body afunda in the momento in that um foot plant (|walk| in the pico).
    // bob negative in the middle of the passo (foot in the ar) and zero to the plantar -> sensation of peso.
    float bodyBob = moving ? (walkAbs * 4.0f - 2.0f) : 0.0f;
    // Shadow esmaga/estica conforme the pisada (more larga and dark to the plantar the foot)
    float shadowW = 20.0f + (moving ? walkAbs * 6.0f : 0.0f);
    float shadowH = 5.5f  - (moving ? walkAbs * 1.6f : 0.0f);
    float shadowA = 0.45f + (moving ? walkAbs * 0.20f : 0.0f);

    bool  ol = isOverloaded();
    bool  sh = isShielded();

    // Colors
    Color C_hull    = ol ? Color{40,20,5,255}    : Color{18,22,38,255};
    Color C_plate   = ol ? Color{180,65,0,255}   : Color{35,55,120,255};
    Color C_plateLt = ol ? Color{230,100,10,255} : Color{55,90,180,255};
    Color C_servo   = ol ? Color{100,35,0,255}   : Color{25,35,80,255};
    Color C_chrome  = {160,170,185,255};
    Color C_neon    = sh ? Color{0,255,200,255}  : ol ? Color{255,190,0,255} : Color{0,210,255,255};
    Color C_dark    = {12,14,22,255};
    // Human half colors (right side = organic)
    Color C_skin    = {180,135,105,255};  // tan skin
    Color C_jacket  = {50,60,45,255};     // dark olive jacket
    Color C_jackLt  = {70,85,60,255};

    // Pulo levanta the body (z); the shadow encolhe conforme the height
    float lift = jumpZ;
    float px = position.x, py = position.y + bodyBob - lift; // dip down + goes up to the pular
    float f  = (float)facing;
    if (lift > 1.0f) {
        float k = 1.0f / (1.0f + lift * 0.02f);
        shadowW *= k; shadowH *= k; shadowA *= k;   // shadow menor/more fraca in the ar
    }

    // Attack range indicator (subtle)
    DrawCircleLines((int)px, (int)py, attackRange, ColorAlpha(C_neon, 0.07f));

    // Ground shadow the€" fixed at feet level (not affected by body dip), with squash
    if (!g_voxelCapture) {
        DrawEllipse((int)position.x, (int)(position.y + 51.0f), shadowW, shadowH, ColorAlpha(BLACK, shadowA));
    }

    // Body distinto by class (Soldier usa the arte cyborg detalhada below)
    switch (charClass) {
        case CharacterClass::Guerreira: renderGuerreira(px,py,f,legL,legR); break;
        case CharacterClass::Robo:      renderRobo(px,py,f,legL,legR);      break;
        case CharacterClass::Mago:      renderMago(px,py,f,legL,legR);      break;
        case CharacterClass::Bruxa:     renderBruxa(px,py,f,legL,legR);     break;
        case CharacterClass::HomemFera: renderHomemFera(px,py,f,legL,legR); break;
        default: break;
    }

    // ── OVERLAY DE EQUIPMENT — the character MUDA conforme the that is equipado.
    //    Funciona to all the classes (drawn by up of the body base).
    {
        // Armor: peitoral + ombreiras tingidas pela color of the armor; glow
        // aumenta with tier/upgrade -> visibly more "blindado".
        if (!equippedArmor.isEmpty()) {
            Color ac = equippedArmor.color;
            float pwr = 0.45f + 0.12f * (float)(equippedArmor.tier + equippedArmor.upgradeLevel);
            if (pwr > 0.9f) pwr = 0.9f;
            // peitoral
            DrawRectangle((int)(px-8), (int)(py-10), 16, 16, ColorAlpha(ac, pwr*0.55f));
            DrawRectangleLinesEx({px-8, py-10, 16, 16}, 1.5f, ColorAlpha(ac, pwr));
            // ombreiras
            DrawCircleV({px-9, py-10}, 4.5f, ColorAlpha(ac, pwr));
            DrawCircleV({px+9, py-10}, 4.5f, ColorAlpha(ac, pwr));
            // estrelas of upgrade glowing in the chest
            for (int s = 0; s < equippedArmor.upgradeLevel; ++s)
                DrawCircleV({px-4.0f + s*4.0f, py-2.0f}, 1.3f, ColorAlpha(WHITE, 0.9f));
        }

        // Implant: orb bright pulsante near the head.
        if (!equippedImplant.isEmpty()) {
            Color ic = equippedImplant.color;
            float pul = 0.6f + 0.4f * std::sin(walkAnimTimer * 4.0f + position.x * 0.05f);
            DrawCircleV({px + f*6.0f, py - 20.0f}, 3.5f + pul*1.5f, ColorAlpha(ic, 0.35f));
            DrawCircleV({px + f*6.0f, py - 20.0f}, 2.0f, ic);
        }

        // Weapon: barril in the hand tingido pela color of the weapon, length cresce with tier.
        // O Soldier JA draws your own weapon detalhada (tingida pela color) — evita
        // weapon duplicada drawing the overlay only to the demais classes.
        if (!equippedWeapon.isEmpty() && charClass != CharacterClass::Soldado) {
            Color wc = equippedWeapon.color;
            float wlen = 16.0f + 5.0f * (float)equippedWeapon.tier;
            float hx = px + f*12.0f, hy = py + 2.0f;
            DrawRectangle((int)hx, (int)(hy-2), (int)(f*wlen), 4, ColorAlpha(wc, 0.95f));
            DrawCircleV({hx + f*wlen, hy}, 2.5f + 0.5f*equippedWeapon.tier, ColorAlpha(wc, 0.8f));
        }
    }

    // the"€the"€ LEGS the€" left leg = mechanical exo, right leg = human in armored boot the"€the"€
    if (charClass == CharacterClass::Soldado) {
    {   // LEFT LEG (machine side)
        float lx = px - 10, ky = py + 28;
        DrawRectangle((int)(lx + legL),     (int)(py + 12), 9, 17, C_plate);
        DrawRectangle((int)(lx + legL + 2), (int)(py + 13), 5, 4,  C_plateLt);
        DrawCircleV({lx+4.5f+legL, ky}, 5.5f, C_chrome);
        DrawCircleV({lx+4.5f+legL, ky}, 2.5f, C_dark);
        DrawRectangle((int)(lx+1+legL), (int)(ky+4),  7, 14, C_servo);
        DrawRectangle((int)(lx+2+legL), (int)(ky+5),  5, 5,  C_plateLt);
        DrawLineEx({lx+4+legL, ky+4}, {lx+4+legL, ky+16}, 1.5f, ColorAlpha(C_neon, 0.5f));
        DrawRectangle((int)(lx-2+legL), (int)(ky+17), 13, 5, C_chrome);
        DrawRectangle((int)(lx-4+legL), (int)(ky+20), 15, 3, C_dark);
    }
    {   // RIGHT LEG (human side the€" thigh + calf in torn pants + armored boot)
        float rx = px + 1, ky = py + 28;
        // Thigh the€" dark pants
        DrawRectangle((int)(rx+legR),     (int)(py+12), 9, 17, C_jacket);
        DrawRectangle((int)(rx+legR+1),   (int)(py+13), 7, 15, C_jackLt);
        // Knee
        DrawCircleV({rx+4.5f+legR, ky}, 5.0f, C_chrome);
        DrawCircleV({rx+4.5f+legR, ky}, 2.0f, C_dark);
        // Calf the€" exposed muscle line (torn)
        DrawRectangle((int)(rx+1+legR),  (int)(ky+4),  7, 14, C_jacket);
        DrawLineEx({rx+5+legR, ky+4}, {rx+5+legR, ky+14}, 1.5f, ColorAlpha(C_skin, 0.4f)); // torn seam
        // Boot (armored)
        DrawRectangle((int)(rx-2+legR),  (int)(ky+17), 13, 5, C_servo);
        DrawRectangle((int)(rx-4+legR),  (int)(ky+20), 15, 3, C_dark);
    }

    // the"€the"€ PELVIS the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€
    DrawRectangle((int)(px-13), (int)(py+8), 13, 6, C_servo);   // left = machine
    DrawRectangle((int)(px),    (int)(py+8), 13, 6, C_jacket);  // right = jacket
    DrawRectangle((int)(px-11), (int)(py+9), 22, 4, C_plate);
    DrawCircleV({px-9, py+12}, 4, C_chrome);
    DrawCircleV({px+9, py+12}, 4, C_chrome);

    // the"€the"€ TORSO the€" LEFT HALF MACHINE / RIGHT HALF HUMAN the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€
    // Back plate
    DrawRectangle((int)(px-15), (int)(py-14), 30, 24, C_hull);
    // Left machine half
    DrawRectangle((int)(px-13), (int)(py-13), 13, 22, C_plate);
    DrawRectangle((int)(px-12), (int)(py-12), 11, 19, C_plateLt);
    for (int i = 0; i < 3; ++i) {
        float ry = py-9+i*7.0f;
        DrawLineEx({px-11,ry},{px-2,ry},1.0f,ColorAlpha(C_chrome,0.35f));
    }
    // Right human half the€" jacket
    DrawRectangle((int)(px),    (int)(py-13), 13, 22, C_jacket);
    DrawRectangle((int)(px+1),  (int)(py-12), 11, 19, C_jackLt);
    // Torn jacket seam showing ribs
    DrawLineEx({px+4,py-10},{px+7,py-4},1.5f,ColorAlpha(C_skin,0.45f));
    DrawLineEx({px+7,py-4},{px+4,py+2},1.5f,ColorAlpha(C_skin,0.35f));
    // Dividing line (flesh to metal split)
    DrawRectangle((int)(px-1), (int)(py-13), 2, 22, {10,8,12,255});
    // Central energy core (bionic implant)
    DrawRectangle((int)(px-4), (int)(py-5), 8, 8, C_dark);
    DrawGlowCircle({px,py}, 5.0f, C_neon, 4.0f);
    DrawCircleV({px,py}, 3.0f, C_neon);

    // the"€the"€ LEFT SHOULDER + MECHANICAL ARM the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€
    {
        float sx = px - 22;
        DrawRectangle((int)(sx),   (int)(py-15), 12, 22, C_servo);
        DrawRectangle((int)(sx+1), (int)(py-14), 10, 8,  C_plateLt);
        DrawCircleV({sx+6, py-14}, 5, C_chrome);
        DrawRectangle((int)(sx+1), (int)(py-9),  8, 14, C_plate);
        DrawCircleV({sx+5, py+6}, 4, C_chrome);
        DrawCircleV({sx+5, py+6}, 1.5f, C_dark);
        DrawRectangle((int)(sx+2), (int)(py+6),  6, 12, C_servo);
        DrawLineEx({sx+5, py+6}, {sx+5, py+16}, 1.5f, ColorAlpha(C_neon, 0.4f));
        // Mechanical claw hand
        DrawRectangle((int)(sx+1), (int)(py+17), 8, 5, C_chrome);
        DrawLineEx({sx+2,py+22},{sx,py+26},2.0f,C_chrome);
        DrawLineEx({sx+5,py+22},{sx+4,py+27},2.0f,C_chrome);
        DrawLineEx({sx+8,py+22},{sx+10,py+26},2.0f,C_chrome);
    }

    // the"€the"€ RIGHT SHOULDER + HUMAN ARM + WEAPON the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€
    {
        float sx = px + 10;
        // Human shoulder in jacket
        DrawRectangle((int)(sx),   (int)(py-15), 12, 22, C_jacket);
        DrawRectangle((int)(sx+1), (int)(py-14), 10, 8,  C_jackLt);
        DrawCircleV({sx+6, py-14}, 5, C_chrome); // joint implant visible
        // Upper arm the€" human flesh + jacket sleeve
        DrawRectangle((int)(sx+1), (int)(py-9), 8, 14, C_jacket);
        DrawRectangle((int)(sx+2), (int)(py-8), 6, 10, C_jackLt);
        // Elbow the€" human
        DrawCircleV({sx+5, py+6}, 4, C_chrome);
        DrawCircleV({sx+5, py+6}, 1.5f, C_dark);
        // Forearm the€" partly exposed skin
        DrawRectangle((int)(sx+2), (int)(py+6),  5, 10, C_jacket);
        DrawRectangle((int)(sx+3), (int)(py+8),  3, 5, C_skin);  // bare forearm
    }

    // the"€the"€ WEAPON the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€
    Color C_gun = !equippedWeapon.isEmpty() ? equippedWeapon.color : C_neon;
    float wx = px + f*14;
    float wy = py - 3;
    DrawRectangle((int)(wx),       (int)(wy-4), (int)(f*6),  10, C_chrome);
    DrawRectangle((int)(wx+f*6),   (int)(wy-5), (int)(f*10), 12, C_hull);
    DrawRectangle((int)(wx+f*7),   (int)(wy-4), (int)(f*8),  10, C_servo);
    DrawRectangle((int)(wx+f*16),  (int)(wy-2), (int)(f*18),  5, C_chrome);
    DrawRectangle((int)(wx+f*10),  (int)(wy-8), (int)(f*8),   4, C_dark);
    DrawCircleV({wx+f*13, wy-6}, 2, ColorAlpha(C_gun,0.7f));
    Vector2 muzzle = {wx+f*36, wy};
    DrawGlowCircle(muzzle, 5.0f, C_gun, 4.0f);
    DrawCircleV(muzzle, 2.5f, C_gun);

    // the"€the"€ NECK the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€
    // Left (machine) neck
    DrawRectangle((int)(px-4), (int)(py-18), 4, 6, C_servo);
    DrawRectangle((int)(px-3), (int)(py-17), 3, 3, C_chrome);
    // Right (human) neck
    DrawRectangle((int)(px),   (int)(py-18), 4, 6, C_skin);

    // the"€the"€ HEAD the€" LEFT HALF MACHINE / RIGHT HALF HUMAN the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€
    // Machine half (left) the€" IRON-VIII skull
    DrawRectangle((int)(px-12), (int)(py-40), 12, 24, C_hull);
    DrawRectangle((int)(px-11), (int)(py-38),  5, 20, C_plate);
    DrawRectangle((int)(px- 6), (int)(py-38),  6, 22, C_servo);
    DrawRectangle((int)(px-10), (int)(py-40), 10, 4, C_plateLt);
    // Machine eye the€" glowing red/orange
    {
        Color eyeCol = ol ? Color{255,130,0,255} : Color{220,20,0,255};
        DrawRectangle((int)(px-11), (int)(py-32), 9, 7, ColorAlpha(C_dark,0.9f));
        DrawGlowLine({px-10,py-29},{px-4,py-29}, 3.0f, eyeCol);
        DrawGlowCircle({px-7,py-29}, 4.0f, eyeCol, 4.0f);
        DrawCircleV({px-7,py-29}, 2.0f, WHITE);
    }
    // Jaw (machine side)
    DrawRectangle((int)(px-10), (int)(py-17), 10, 4, C_servo);
    DrawRectangle((int)(px-9),  (int)(py-16), 8, 2, C_dark);

    // Human half (right) the€" scarred organic face
    DrawRectangle((int)(px),  (int)(py-40), 12, 24, C_skin);
    // Scar line (diagonal across cheek the€" from old battle)
    DrawLineEx({px+2,py-35},{px+10,py-22}, 1.5f, ColorAlpha({100,60,60,255},0.55f));
    // Human eye (right) the€" dark iris
    DrawRectangle((int)(px+2), (int)(py-33), 8, 6, ColorAlpha({20,15,30,255},0.6f));
    DrawCircleV({px+6, py-30}, 3.0f, {30,40,90,255});  // iris
    DrawCircleV({px+6, py-30}, 1.5f, {10,12,30,255});  // pupil
    DrawGlowCircle({px+6,py-30}, 1.5f, C_neon, 1.5f);  // tiny neon highlight
    // Eyebrow (human)
    DrawRectangle((int)(px+2), (int)(py-35), 8, 2, {60,45,30,255});
    // Jaw (human)
    DrawRectangle((int)(px),   (int)(py-17), 10, 4, C_skin);
    // Dividing crack (machine/flesh border on face)
    DrawLineEx({px,py-40},{px,py-16}, 2.0f, ColorAlpha(C_dark,0.9f));
    // Forehead ridge (full width)
    DrawRectangle((int)(px-10), (int)(py-41), 22, 3, C_plateLt);

    // Antenna (machine side, above left)
    DrawRectangle((int)(px-5), (int)(py-50), 3, 10, C_chrome);
    DrawGlowCircle({px-3.5f, py-51}, 3, C_neon, 3.0f);
    } // end of the body Soldier

    // the"€the"€ STATUS EFFECTS the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€the"€
    if (sh) {
        float st = std::sin(shieldTimer * 10.0f);
        float r  = radius + 24 + st * 5;
        DrawCircleLines((int)px, (int)py, r,      ColorAlpha({0,255,200,255}, 0.75f));
        DrawCircleLines((int)px, (int)py, r + 6,  ColorAlpha({0,200,255,255}, 0.35f));
        DrawCircleLines((int)px, (int)py, r + 12, ColorAlpha({0,255,255,255}, 0.15f));
        for (int i = 0; i < 6; ++i) {
            float the  = i * 1.047f + shieldTimer * 1.2f;
            Vector2 hp = {px + std::cos(the)*r, py + std::sin(the)*r};
            DrawCircleV(hp, 4, ColorAlpha({0,255,200,255}, 0.7f));
        }
    }
    if (ol) {
        float st = std::sin(overloadTimer * 14.0f);
        float r  = radius + 16 + st * 4;
        DrawCircleLines((int)px, (int)py, r,     ColorAlpha({255,120,0,255}, 0.75f));
        DrawCircleLines((int)px, (int)py, r + 7, ColorAlpha({255,200,0,255}, 0.3f));
        for (int i = 0; i < 6; ++i) {
            float the  = i * 1.047f + overloadTimer * 6.0f;
            Vector2 sp = {px + std::cos(the)*(r+3), py + std::sin(the)*(r+3)};
            DrawCircleV(sp, 3, ColorAlpha({255,220,0,255}, 0.85f));
        }
    }
    if (leveledUp) {
        float st = std::sin(levelUpTimer * 14.0f);
        DrawCircleLines((int)px, (int)py, radius + 16 + st*5, ColorAlpha(GOLD, 0.9f));
        DrawCircleLines((int)px, (int)py, radius + 28 + st*5, ColorAlpha(GOLD, 0.4f));
    }
    // Buff of healing/regeneracao (estilo Diablo 3): aura green + cruzes subindo.
    if (regenTimer > 0.0f) {
        Color heal = {0, 255, 120, 255};
        float pul = 0.5f + 0.5f * std::sin((float)GetTime() * 6.0f);
        DrawCircleLines((int)px, (int)py, radius + 12 + pul*4, ColorAlpha(heal, 0.7f));
        for (int i = 0; i < 4; ++i) {
            float t2 = std::fmod((float)GetTime() * 0.8f + i * 0.25f, 1.0f);
            float cxh = px + std::sin((float)GetTime()*2.0f + i*1.6f) * 14.0f;
            float cyh = py + 6 - t2 * 38.0f;
            float the   = (1.0f - t2) * 0.9f;
            // cruz of healing
            DrawRectangle((int)(cxh-3), (int)(cyh-1), 6, 2, ColorAlpha(heal, the));
            DrawRectangle((int)(cxh-1), (int)(cyh-3), 2, 6, ColorAlpha(heal, the));
        }
    }

    // ── EVOLUTION PATH VISUALS ────────────────────────────────────────────────
    if (evolutionPath == EvolutionPath::CyborgSoldier) {
        // Metal shoulder plate (left)
        DrawRectangle((int)(px-28),(int)(py-16),14,8,{140,150,165,255});
        DrawRectangle((int)(px-27),(int)(py-15),12,5,{180,190,200,255});
        // Bionic right forearm overlay
        DrawRectangle((int)(px+12),(int)(py+2),8,14,{100,110,125,255});
        DrawRectangle((int)(px+13),(int)(py+3),6,6, {140,150,165,255});
        // Red cyber eye (left)
        DrawCircleV({px-7,py-29}, 3.5f, {255,0,0,255});
        DrawGlowCircle({px-7,py-29}, 3.5f, {255,40,0,255}, 4.0f);
    } else if (evolutionPath == EvolutionPath::HackerFantasma) {
        // Digital aura — 6 hex nodes orbiting
        for (int i=0;i<6;++i) {
            float angle = i*1.047f + walkAnimTimer*0.5f;
            float ar = radius+22.0f;
            Vector2 np={px+std::cos(angle)*ar, py+std::sin(angle)*ar};
            float ap=0.5f+0.4f*std::sin(walkAnimTimer*2.0f+i*1.1f);
            DrawCircleV(np,3.5f,ColorAlpha({0,220,255,255},ap));
            DrawLineEx({px,py},np,1.0f,ColorAlpha({0,180,255,255},ap*0.25f));
        }
        // Digital hood / capuz
        DrawTriangle({px-10,py-40},{px+10,py-40},{px,py-58},
                     ColorAlpha({10,20,40,255},0.85f));
    } else if (evolutionPath == EvolutionPath::ExecutorOmega) {
        // Heavy shoulder pauldrons
        DrawRectangle((int)(px-32),(int)(py-18),16,12,{100,10,10,255});
        DrawRectangle((int)(px+16),(int)(py-18),16,12,{100,10,10,255});
        DrawRectangle((int)(px-31),(int)(py-17),14,6, {140,20,20,255});
        DrawRectangle((int)(px+17),(int)(py-17),14,6, {140,20,20,255});
        // Chest plate overlay
        DrawRectangle((int)(px-14),(int)(py-12),28,18,ColorAlpha({120,15,15,255},0.7f));
        // Orange eye glow
        DrawGlowCircle({px+6,py-30}, 3.0f, {255,120,0,255}, 5.0f);
    }

    // ══ COSMETICOS ════════════════════════════════════════════════════════════
    // 1) Tinta of the shop common (credits): modula the cores of the body with uma demao
    //    translucida in the color escolhida (chest + ombros + head).
    if (hasCosmeticTint) {
        Color t = cosmeticTint;
        DrawRectangle((int)(px-9), (int)(py-12), 18, 20, ColorAlpha(t, 0.38f)); // torso
        DrawCircleV({px, py-30}, 10.0f, ColorAlpha(t, 0.30f));                  // head
        DrawCircleV({px-9, py-12}, 5.0f, ColorAlpha(t, 0.40f));                 // ombro E
        DrawCircleV({px+9, py-12}, 5.0f, ColorAlpha(t, 0.40f));                 // ombro D
    }

    // 2) Skin Neon (Gems): glow neon pulsante ADITIVO around of the model.
    if (skinNeon) {
        float pul = 0.55f + 0.45f * std::sin((float)GetTime() * 4.0f);
        Color neon = {0, 255, 200, 255};
        BeginBlendMode(BLEND_ADDITIVE);
        DrawCircleLines((int)px, (int)py, radius + 14 + pul*4, ColorAlpha(neon, 0.55f));
        DrawCircleLines((int)px, (int)py, radius + 20 + pul*5, ColorAlpha(neon, 0.30f));
        DrawCircleV({px, py-30}, 11.0f, ColorAlpha(Color{0,255,200,255}, 0.18f*pul));
        DrawRectangle((int)(px-10), (int)(py-12), 20, 22, ColorAlpha(neon, 0.10f*pul));
        EndBlendMode();
    }

    // 3) Skin Dragao (Gems): tonalidade hot + brasas laranjas subindo to the andar.
    if (skinDragon) {
        DrawRectangle((int)(px-9), (int)(py-12), 18, 20, ColorAlpha(Color{255,90,0,255}, 0.22f));
        // eyes orange/red incandescentes
        DrawCircleV({px-4, py-30}, 2.0f, Color{255,80,0,255});
        DrawCircleV({px+5, py-30}, 2.0f, Color{255,80,0,255});
        DrawGlowCircle({px-4, py-30}, 2.0f, Color{255,120,0,255}, 3.0f);
        DrawGlowCircle({px+5, py-30}, 2.0f, Color{255,120,0,255}, 3.0f);
        // escamas/lines of fire in the chest
        for (int i = 0; i < 3; ++i)
            DrawLineEx({px-6.0f+i*6, py-8}, {px-6.0f+i*6, py+4}, 1.5f,
                       ColorAlpha(Color{255,140,0,255}, 0.6f));
        // brasas subindo (more intensas in movement)
        int embers = moving ? 5 : 2;
        for (int i = 0; i < embers; ++i) {
            float t2 = std::fmod((float)GetTime() * 0.6f + i * 0.21f, 1.0f);
            float ex = px + std::sin((float)GetTime()*3.0f + i) * 10.0f;
            float ey = py - 8 - t2 * 34.0f;
            float the  = (1.0f - t2) * 0.8f;
            DrawCircleV({ex, ey}, 1.5f + (1.0f-t2)*2.0f,
                        ColorAlpha(Color{255, (unsigned char)(120+(int)(100*t2)), 0, 255}, the));
        }
    }

    // 4) Drone of Estimacao (Gems): drone pixel-art flutuando above-to the-left.
    if (petDrone) {
        float hov = std::sin((float)GetTime() * 3.0f) * 4.0f;   // voo stable
        float dx = px - 16.0f;            // above and the left of the head
        float dy = py - 36.0f + hov;
        // body
        DrawRectangle((int)(dx-6), (int)(dy-4), 12, 8, Color{70,80,95,255});
        DrawRectangle((int)(dx-5), (int)(dy-3), 10, 3, Color{120,135,150,255});
        // helices
        DrawLineEx({dx-9, dy-5}, {dx-3, dy-5}, 1.5f, Color{160,170,185,255});
        DrawLineEx({dx+3, dy-5}, {dx+9, dy-5}, 1.5f, Color{160,170,185,255});
        // led piscante
        float blink = (std::sin((float)GetTime()*6.0f) > 0.0f) ? 1.0f : 0.3f;
        DrawCircleV({dx, dy}, 2.0f, ColorAlpha(Color{255,40,40,255}, blink));
        // feixe of light apontando for the floor
        BeginBlendMode(BLEND_ADDITIVE);
        DrawTriangle({dx-3, dy+3}, {dx+3, dy+3}, {dx, dy+22},
                     ColorAlpha(Color{0,200,255,255}, 0.18f));
        EndBlendMode();
    }
}

// ── Bodies by class (formatos distintos, reconheciveis) ────────────────────

void Player::renderGuerreira(float px, float py, float f, float legL, float legR) const {
    Color body = classPrimary, bodyLt = classSecondary, skin = classSkin;
    Color hair = {40,25,20,255}, ac = accentNow(), trim = classTrim;
    // Legs esguias with botas
    DrawRectangle((int)(px-7+legL),(int)(py+12),5,16,bodyLt);
    DrawRectangle((int)(px+2+legR),(int)(py+12),5,16,bodyLt);
    DrawRectangle((int)(px-8+legL),(int)(py+26),7,5,{30,30,40,255});
    DrawRectangle((int)(px+1+legR),(int)(py+26),7,5,{30,30,40,255});
    // Quadril (cintura marcada)
    DrawRectangle((int)(px-7),(int)(py+8),14,6,body);
    // Tronco esguio (cintura fina, ombros)
    DrawRectangle((int)(px-8),(int)(py-12),16,20,body);
    DrawRectangle((int)(px-6),(int)(py-11),12,10,bodyLt);
    DrawRectangle((int)(px-7),(int)(py-2),14,6,{20,20,30,255}); // cinto
    // Arms
    DrawRectangle((int)(px-12),(int)(py-10),5,16,skin);
    DrawRectangle((int)(px+7),(int)(py-10),5,16,skin);
    // Head + rabo of cavalo
    DrawCircleV({px,py-20},8,skin);
    DrawRectangle((int)(px-8),(int)(py-28),16,7,hair);          // franja
    DrawRectangle((int)(px+6),(int)(py-26),5,18,hair);          // rabo of cavalo
    DrawCircleV({px-3,py-20},1.6f,ac); DrawCircleV({px+3,py-20},1.6f,ac); // eyes
    // Duas pistolas (uma in cada hand)
    DrawRectangle((int)(px+8),(int)(py-2),10,4,trim);
    DrawRectangle((int)(px-18),(int)(py+2),10,4,trim);
    DrawGlowCircle({px+20,py},3.0f,ac,3.0f);
    DrawGlowCircle({px-20,py+4},2.5f,ac,2.5f);
}

void Player::renderRobo(float px, float py, float f, float legL, float legR) const {
    Color metal = classPrimary, metalLt = classSecondary, ac = accentNow(), trim = classTrim;
    // Legs pesadas/hidraulicas
    DrawRectangle((int)(px-11+legL*0.5f),(int)(py+12),9,18,metal);
    DrawRectangle((int)(px+2+legR*0.5f),(int)(py+12),9,18,metal);
    DrawRectangle((int)(px-13),(int)(py+28),11,4,{20,20,25,255});
    DrawRectangle((int)(px+2),(int)(py+28),11,4,{20,20,25,255});
    // Body retangular metallic (chassi)
    DrawRectangle((int)(px-13),(int)(py-16),26,28,metal);
    DrawRectangle((int)(px-11),(int)(py-14),22,24,metalLt);
    // Paineis + parafusos
    DrawRectangleLinesEx({px-11,py-14,22,24},1.0f,ColorAlpha(trim,0.5f));
    DrawCircleV({px-8,py-11},1.5f,trim); DrawCircleV({px+8,py-11},1.5f,trim);
    DrawCircleV({px-8,py+8},1.5f,trim);  DrawCircleV({px+8,py+8},1.5f,trim);
    // Reator central
    DrawGlowCircle({px,py-2},6.0f,ac,5.0f); DrawCircleV({px,py-2},3.0f,ac);
    // Arms blocados
    DrawRectangle((int)(px-20),(int)(py-14),7,20,metal);
    DrawRectangle((int)(px+13),(int)(py-14),7,20,metal);
    DrawRectangle((int)(px-21),(int)(py+5),9,7,trim);   // garra/canhao
    DrawRectangle((int)(px+12),(int)(py+5),9,7,trim);
    // Head caixa with view only
    DrawRectangle((int)(px-9),(int)(py-32),18,16,metalLt);
    DrawRectangle((int)(px-7),(int)(py-28),14,5,{10,10,15,255});
    DrawGlowLine({px-6,py-25},{px+6,py-25},4.0f,ac);    // view varrendo
    // Antena
    DrawRectangle((int)(px-1),(int)(py-40),2,8,trim);
    DrawGlowCircle({px,py-41},2.5f,ac,3.0f);
    // Weapon pesada
    DrawRectangle((int)(px+f*14),(int)(py-4),(int)(f*16),8,trim);
    DrawGlowCircle({px+f*32,py},4.0f,ac,4.0f);
}

void Player::renderMago(float px, float py, float f, float legL, float legR) const {
    Color robe = classPrimary, robeLt = classSecondary, ac = accentNow(), skin = classSkin, trim = classTrim;
    // Tunica longa (saia triangular) — esconde the legs, balanca um little
    float sway = (legL - legR) * 0.2f;
    DrawTriangle({px - 16 + sway, py + 30}, {px + 16 + sway, py + 30}, {px, py - 6}, robe);
    DrawTriangle({px - 11 + sway, py + 28}, {px + 11 + sway, py + 28}, {px, py + 2}, robeLt);
    // Barra lower decorada
    DrawRectangle((int)(px-16+sway),(int)(py+27),32,3,trim);
    // Tronco
    DrawRectangle((int)(px-8),(int)(py-12),16,16,robe);
    // Arms in the mangas
    DrawRectangle((int)(px-13),(int)(py-10),6,14,robeLt);
    DrawRectangle((int)(px+7),(int)(py-10),6,14,robeLt);
    // Capuz + rosto sombreado + barba
    DrawTriangle({px-10,py-16},{px+10,py-16},{px,py-40},robe);
    DrawCircleV({px,py-22},7,{30,25,45,255});       // shadow of the capuz
    DrawCircleV({px-3,py-22},1.6f,ac); DrawCircleV({px+3,py-22},1.6f,ac);
    DrawTriangle({px-5,py-17},{px+5,py-17},{px,py-8},{230,230,235,255}); // barba branca
    // Staff with crystal bright
    float stx = px + f*13;
    DrawRectangle((int)(stx),(int)(py-26),3,46,{90,60,30,255});
    DrawGlowCircle({stx+1.5f,py-30},6.0f,ac,7.0f);
    DrawCircleV({stx+1.5f,py-30},3.0f,ac);
}

void Player::renderBruxa(float px, float py, float f, float legL, float legR) const {
    Color dress = classPrimary, dressLt = classSecondary, ac = accentNow(), skin = classSkin, trim = classTrim;
    // Vestido (saia) with light balanco
    float sway = (legL - legR) * 0.25f;
    DrawTriangle({px - 14 + sway, py + 28}, {px + 14 + sway, py + 28}, {px, py - 2}, dress);
    DrawTriangle({px - 9 + sway, py + 26}, {px + 9 + sway, py + 26}, {px, py + 4}, dressLt);
    // Tronco + cinto
    DrawRectangle((int)(px-7),(int)(py-12),14,14,dress);
    DrawRectangle((int)(px-7),(int)(py-1),14,3,trim);
    // Arms
    DrawRectangle((int)(px-11),(int)(py-10),5,13,dressLt);
    DrawRectangle((int)(px+6),(int)(py-10),5,13,dressLt);
    // Head
    DrawCircleV({px,py-19},7,skin);
    DrawCircleV({px-2.5f,py-19},1.5f,ac); DrawCircleV({px+2.5f,py-19},1.5f,ac);
    // Cabelo side
    DrawRectangle((int)(px-7),(int)(py-20),3,12,{30,20,30,255});
    DrawRectangle((int)(px+4),(int)(py-20),3,12,{30,20,30,255});
    // CHAPEU PONTUDO
    DrawTriangle({px-12,py-24},{px+12,py-24},{px-2,py-48},dress);  // cone (inclinado)
    DrawEllipse((int)px,(int)(py-24),15.0f,4.0f,{20,10,30,255});   // aba
    DrawRectangle((int)(px-13),(int)(py-26),26,3,trim);            // fita
    DrawGlowCircle({px-2,py-46},2.5f,ac,3.0f);                     // ponta bright
    // Capa esvoacante behind
    DrawTriangle({px-8,py-12},{px-18-sway,py+22},{px-4,py+16},ColorAlpha(dress,0.8f));
    // Varinha with glow
    float wx = px + f*12;
    DrawRectangle((int)(wx),(int)(py-6),(int)(f*12),2,{120,90,50,255});
    DrawGlowCircle({wx+f*13,py-5},5.0f,ac,6.0f);
}

void Player::renderHomemFera(float px, float py, float f, float legL, float legR) const {
    Color fur = classPrimary, furLt = classSecondary, ac = accentNow(), trim = classTrim;
    Color claw = {235,235,240,255};
    // Legs digitigradas musculosas
    DrawRectangle((int)(px-10+legL),(int)(py+10),8,16,fur);
    DrawRectangle((int)(px+2+legR),(int)(py+10),8,16,fur);
    DrawRectangle((int)(px-11+legL),(int)(py+24),9,6,furLt); // patas
    DrawRectangle((int)(px+2+legR),(int)(py+24),9,6,furLt);
    // Garras of the feet
    for (int i=0;i<3;i++){ DrawLineEx({px-9+legL+i*3,py+30},{px-10+legL+i*3,py+33},1.5f,claw); }
    // Tronco musculoso wide (postura curvada)
    DrawRectangle((int)(px-13),(int)(py-10),26,22,fur);
    DrawRectangle((int)(px-10),(int)(py-8),20,14,furLt);
    DrawLineEx({px,py-8},{px,py+8},1.5f,ColorAlpha(trim,0.6f)); // peitoral
    // Arms grossos
    DrawRectangle((int)(px-19),(int)(py-10),7,18,fur);
    DrawRectangle((int)(px+12),(int)(py-10),7,18,fur);
    // GARRAS in the hands
    for (int i=0;i<3;i++){
        DrawLineEx({px-18+i*3.0f,py+8},{px-20+i*3.0f,py+14},2.0f,claw);
        DrawLineEx({px+13+i*3.0f,py+8},{px+15+i*3.0f,py+14},2.0f,claw);
    }
    // Head of fera (focinho + orelhas)
    DrawCircleV({px,py-20},9,fur);
    DrawTriangle({px-9,py-24},{px-3,py-24},{px-7,py-34},fur);  // orelha esq
    DrawTriangle({px+3,py-24},{px+9,py-24},{px+7,py-34},fur);  // orelha dir
    DrawTriangle({px-2,py-18},{px+8,py-19},{px+2,py-13},furLt); // focinho
    DrawCircleV({px+6,py-17},1.5f,{15,10,8,255});               // nariz
    // Eyes ferozes glowing
    DrawCircleV({px-3,py-22},1.8f,ac); DrawCircleV({px+3,py-22},1.8f,ac);
    // Presas
    DrawTriangle({px-1,py-13},{px+1,py-13},{px,py-9},claw);
}

void Player::addItem(const Item& item) {
    inventory.push_back(item);
}

void Player::useSkill(int index, Vector2 /*target*/) {
    if (index >= 0 && index < (int)skills.size()) {
        skills[index].use();
    }
}

// ── Icones procedurais (weapons/items "reais") for the inventory visual ──
static void invGearIcon(float cx, float cy, float s, EquipSlot slot, Color col) {
    switch (slot) {
        case EquipSlot::Weapon:
            DrawRectangle((int)(cx-s*0.42f),(int)(cy-s*0.09f),(int)(s*0.70f),(int)(s*0.20f), col);
            DrawRectangle((int)(cx+s*0.18f),(int)(cy-s*0.04f),(int)(s*0.34f),(int)(s*0.09f), col);
            DrawRectangle((int)(cx-s*0.42f),(int)(cy+s*0.10f),(int)(s*0.18f),(int)(s*0.24f), col);
            DrawRectangle((int)(cx-s*0.14f),(int)(cy+s*0.10f),(int)(s*0.10f),(int)(s*0.16f), ColorAlpha(col,0.7f));
            break;
        case EquipSlot::Armor:
            DrawTriangle({cx,cy-s*0.40f},{cx-s*0.38f,cy-s*0.18f},{cx+s*0.38f,cy-s*0.18f}, col);
            DrawRectangle((int)(cx-s*0.34f),(int)(cy-s*0.20f),(int)(s*0.68f),(int)(s*0.48f), col);
            DrawTriangle({cx-s*0.34f,cy+s*0.28f},{cx,cy+s*0.46f},{cx+s*0.34f,cy+s*0.28f}, col);
            DrawRectangle((int)(cx-s*0.04f),(int)(cy-s*0.16f),(int)(s*0.08f),(int)(s*0.42f), ColorAlpha(BLACK,0.3f));
            break;
        default: // implant / chip
            DrawRectangle((int)(cx-s*0.30f),(int)(cy-s*0.30f),(int)(s*0.60f),(int)(s*0.60f), col);
            for (int i=0;i<4;i++){ float the=-s*0.22f+i*s*0.15f;
                DrawRectangle((int)(cx+the),(int)(cy-s*0.44f),(int)(s*0.06f),(int)(s*0.14f), col);
                DrawRectangle((int)(cx+the),(int)(cy+s*0.30f),(int)(s*0.06f),(int)(s*0.14f), col);
                DrawRectangle((int)(cx-s*0.44f),(int)(cy+the),(int)(s*0.14f),(int)(s*0.06f), col);
                DrawRectangle((int)(cx+s*0.30f),(int)(cy+the),(int)(s*0.14f),(int)(s*0.06f), col); }
            DrawRectangle((int)(cx-s*0.12f),(int)(cy-s*0.12f),(int)(s*0.24f),(int)(s*0.24f), ColorAlpha(BLACK,0.4f));
            break;
    }
}
static void invItemIcon(float cx, float cy, float s, ItemType t, Color col) {
    switch (t) {
        case ItemType::HealthPack:
            DrawRectangle((int)(cx-s*0.30f),(int)(cy-s*0.30f),(int)(s*0.60f),(int)(s*0.60f), col);
            DrawRectangle((int)(cx-s*0.06f),(int)(cy-s*0.20f),(int)(s*0.12f),(int)(s*0.40f), WHITE);
            DrawRectangle((int)(cx-s*0.20f),(int)(cy-s*0.06f),(int)(s*0.40f),(int)(s*0.12f), WHITE);
            break;
        case ItemType::NanoCore:
            DrawCircle((int)cx,(int)cy,s*0.32f, col); DrawCircle((int)cx,(int)cy,s*0.15f, ColorAlpha(WHITE,0.85f));
            break;
        case ItemType::PlasmaCell:
            DrawRectangle((int)(cx-s*0.18f),(int)(cy-s*0.30f),(int)(s*0.36f),(int)(s*0.58f), col);
            DrawRectangle((int)(cx-s*0.08f),(int)(cy-s*0.40f),(int)(s*0.16f),(int)(s*0.10f), col);
            DrawRectangle((int)(cx-s*0.10f),(int)(cy-s*0.10f),(int)(s*0.20f),(int)(s*0.06f), ColorAlpha(WHITE,0.7f));
            break;
        case ItemType::EnergyCore: {
            Vector2 h[6]; for(int i=0;i<6;i++){ float the=i*PI/3.0f; h[i]={cx+cosf(the)*s*0.32f, cy+sinf(the)*s*0.32f}; }
            for(int i=0;i<6;i++) DrawTriangle({cx,cy}, h[(i+1)%6], h[i], col);
            DrawCircle((int)cx,(int)cy,s*0.10f, WHITE);
        } break;
        case ItemType::WeaponPart: {
            for(int i=0;i<8;i++){ float the=i*PI/4.0f;
                DrawRectangle((int)(cx+cosf(the)*s*0.26f-s*0.05f),(int)(cy+sinf(the)*s*0.26f-s*0.05f),(int)(s*0.10f),(int)(s*0.10f), col); }
            DrawCircle((int)cx,(int)cy,s*0.22f, col); DrawCircle((int)cx,(int)cy,s*0.10f, ColorAlpha(BLACK,0.45f));
        } break;
        default: // chip / techchip / scrap
            DrawRectangle((int)(cx-s*0.26f),(int)(cy-s*0.26f),(int)(s*0.52f),(int)(s*0.52f), col);
            DrawRectangle((int)(cx-s*0.10f),(int)(cy-s*0.10f),(int)(s*0.20f),(int)(s*0.20f), ColorAlpha(BLACK,0.4f));
            DrawRectangleLinesEx({cx-s*0.26f,cy-s*0.26f,s*0.52f,s*0.52f},1.0f, ColorAlpha(WHITE,0.3f));
            break;
    }
}

void Player::drawInventory() const {
    const int SW=1280, SH=720;
    const Color C_cyan={0,235,255,255}, C_gold={255,190,0,255}, C_green={0,210,80,255};
    DrawRectangle(0,0,SW,SH, ColorAlpha(BLACK,0.82f));
    int PX=64, PY=44, PW=SW-128, PH=SH-88;
    DrawRectangle(PX,PY,PW,PH, ColorAlpha(Color{10,12,24,255},0.97f));
    DrawRectangleLinesEx({(float)PX,(float)PY,(float)PW,(float)PH},2.0f, ColorAlpha(C_cyan,0.7f));
    DrawRectangle(PX,PY,PW,34, ColorAlpha(Color{0,180,220,255},0.14f));
    DrawText("INVENTORY", PX+16, PY+9, 20, C_cyan);
    DrawText("click p/ select/equipar/usar  |  [I] close  [U] upgrade  [R] fundir",
             PX+170, PY+12, 12, ColorAlpha(WHITE,0.55f));
    DrawRectangle(PX+PW-32, PY+6, 24, 24, ColorAlpha(Color{200,40,40,255},0.55f));
    DrawRectangleLinesEx({(float)(PX+PW-32),(float)(PY+6),24,24}, 1.0f, Color{255,120,120,255});
    DrawText("X", PX+PW-25, PY+9, 18, WHITE);

    // ===== ESQUERDA: stats + slots equipados =====
    int lx=PX+18, ly=PY+50;
    DrawText("CHARACTER", lx, ly, 15, C_gold); ly+=22;
    auto stat=[&](const char* the,const char* b,Color col){ DrawText(the,lx,ly,13,ColorAlpha(WHITE,0.6f)); DrawText(b,lx+120,ly,13,col); ly+=17; };
    stat("LESPEED",    TextFormat("%d",level),                  C_gold);
    stat("HP",       TextFormat("%.0f/%.0f",health,maxHealth),C_green);
    stat("DAMAGE",     TextFormat("%.0f",attackDamage),         Color{255,100,80,255});
    stat("DEFENSE",   TextFormat("%.0f%%",defense),            Color{100,200,255,255});
    stat("SPEEDOC.",   TextFormat("%.0f",speed),                Color{255,200,80,255});
    stat("CREDITS", TextFormat("$%d",credits),               C_gold);
    ly+=10;
    DrawText("EQUIPADO", lx, ly, 15, C_gold); ly+=24;

    const Equipment* eqs[3]={&equippedWeapon,&equippedArmor,&equippedImplant};
    const char* enames[3]={"ARMA","ARMOR","IMPLANTE"};
    EquipSlot eslot[3]={EquipSlot::Weapon,EquipSlot::Armor,EquipSlot::Implant};
    const char* p1n[3]={"DAMAGE","HP","SPEED"}; const char* p2n[3]={"Rng","Def%","XPx"};
    int sw=304, sh=82;
    for(int s=0;s<3;s++){
        int sx=lx, sy=ly+s*(sh+8);
        bool sel=(selectedEquipSlot==s);
        const Equipment& and=*eqs[s];
        Color accent = and.isEmpty()? ColorAlpha(C_cyan,0.45f) : and.color;
        DrawRectangle(sx,sy,sw,sh, sel?ColorAlpha(accent,0.16f):ColorAlpha(BLACK,0.35f));
        DrawRectangleLinesEx({(float)sx,(float)sy,(float)sw,(float)sh}, sel?2.5f:1.0f, sel?accent:ColorAlpha(accent,0.6f));
        int ib=sh-12;
        DrawRectangle(sx+6,sy+6,ib,ib, ColorAlpha(BLACK,0.5f));
        DrawRectangleLinesEx({(float)(sx+6),(float)(sy+6),(float)ib,(float)ib},1.0f, ColorAlpha(accent,0.5f));
        invGearIcon(sx+6+ib*0.5f, sy+6+ib*0.5f, ib*0.78f, eslot[s], and.isEmpty()?ColorAlpha(WHITE,0.16f):and.color);
        DrawText(TextFormat("[%d] %s",s+1,enames[s]), sx+ib+16, sy+8, 12, ColorAlpha(accent,0.9f));
        if(and.isEmpty()){
            DrawText("empty", sx+ib+16, sy+34, 16, ColorAlpha(DARKGRAY,0.9f));
        } else {
            DrawText(and.name.c_str(), sx+ib+16, sy+26, 15, and.color);
            DrawText(TextFormat("%s +%.0f   %s +%.1f", p1n[s], and.getEffectivePrimary(), p2n[s], and.getEffectiveSecondary()),
                     sx+ib+16, sy+46, 12, ColorAlpha(WHITE,0.8f));
            DrawText(TextFormat("T%d",and.tier), sx+sw-30, sy+8, 11, ColorAlpha(C_gold,0.8f));
            for(int st=0;st<3;st++) DrawText("*", sx+ib+16+st*10, sy+62, 12, st<and.upgradeLevel?C_gold:ColorAlpha(DARKGRAY,0.5f));
            if(and.canUpgrade() && sel) DrawText(TextFormat("[U] $%d",and.upgradeCost()), sx+sw-92, sy+62, 11, C_gold);
        }
    }

    // ===== DIREITA: grades of slots (gear collected + items) =====
    int gx=PX+362, gy=PY+50;
    const int cell=62, gap=6, cols=11;
    auto cellAt=[&](int idx,int baseY)->Rectangle{ int r=idx/cols, c2=idx%cols;
        return Rectangle{ (float)(gx+c2*(cell+gap)), (float)(baseY+r*(cell+gap)), (float)cell,(float)cell }; };

    DrawText("EQUIPAMENTOS COLETADOS   [ARROWS] navegar   [T] equipar", gx, gy, 14, C_gold); gy+=22;
    int gearRows=2;
    for(int i=0;i<gearRows*cols;i++){
        Rectangle cr=cellAt(i,gy);
        DrawRectangle((int)cr.x,(int)cr.y,(int)cr.width,(int)cr.height, ColorAlpha(BLACK,0.4f));
        if(i<(int)equipBag.size()){
            const Equipment& and=equipBag[i]; bool sel=(i==selectedBagEquip);
            DrawRectangle((int)cr.x,(int)cr.y,(int)cr.width,(int)cr.height, ColorAlpha(and.color,sel?0.28f:0.10f));
            DrawRectangleLinesEx(cr, sel?2.5f:1.0f, sel?and.color:ColorAlpha(and.color,0.6f));
            invGearIcon(cr.x+cr.width*0.5f, cr.y+cr.height*0.42f, cell*0.62f, and.slot, and.color);
            DrawText(TextFormat("T%d",and.tier), (int)cr.x+4, (int)(cr.y+cr.height)-13, 10, ColorAlpha(C_gold,0.9f));
            const char* sn=(and.slot==EquipSlot::Weapon)?"ARM":(and.slot==EquipSlot::Armor)?"DEF":"IMP";
            DrawText(sn, (int)(cr.x+cr.width)-MeasureText(sn,10)-4, (int)(cr.y+cr.height)-13, 10, ColorAlpha(C_cyan,0.85f));
        } else {
            DrawRectangleLinesEx(cr,1.0f, ColorAlpha(C_cyan,0.15f));
        }
    }
    gy += gearRows*(cell+gap) + 14;

    DrawText("ITENS COLETADOS   [Q/E] navegar   [F] usar   [R] fundir 3x iguais", gx, gy, 14, C_gold); gy+=22;
    int itemRows=3;
    int tcount[16]={};
    for(const auto& it:inventory){ int t=(int)it.type; if(t>=0&&t<16) tcount[t]++; }
    for(int i=0;i<itemRows*cols;i++){
        Rectangle cr=cellAt(i,gy);
        DrawRectangle((int)cr.x,(int)cr.y,(int)cr.width,(int)cr.height, ColorAlpha(BLACK,0.4f));
        if(i<(int)inventory.size()){
            const Item& it=inventory[i]; bool sel=(i==selectedInvItem);
            DrawRectangle((int)cr.x,(int)cr.y,(int)cr.width,(int)cr.height, ColorAlpha(it.color,sel?0.28f:0.10f));
            DrawRectangleLinesEx(cr, sel?2.5f:1.0f, sel?it.color:ColorAlpha(it.color,0.6f));
            invItemIcon(cr.x+cr.width*0.5f, cr.y+cr.height*0.42f, cell*0.62f, it.type, it.color);
            int tc=tcount[(int)it.type];
            if(tc>=2) DrawText(TextFormat("x%d",tc), (int)(cr.x+cr.width)-22, (int)cr.y+4, 11, tc>=3?C_gold:ColorAlpha(C_gold,0.6f));
        } else {
            DrawRectangleLinesEx(cr,1.0f, ColorAlpha(C_cyan,0.15f));
        }
    }
    gy += itemRows*(cell+gap) + 12;

    // Detalhe of the item selected
    if(selectedInvItem>=0 && selectedInvItem<(int)inventory.size()){
        const Item& it=inventory[selectedInvItem];
        DrawText(it.name.c_str(), gx, gy, 16, it.color);
        const char* hint="";
        switch(it.type){
            case ItemType::HealthPack: hint="+30 HP"; break;
            case ItemType::NanoCore:   hint="+25 MaxHP permanent"; break;
            case ItemType::TechChip:   hint="+50 XP"; break;
            case ItemType::PlasmaCell: hint="zera cooldowns"; break;
            case ItemType::EnergyCore: hint="shield 2.5s"; break;
            case ItemType::ScrapMetal: hint="+8 HP / +$8"; break;
            case ItemType::WeaponPart: hint="+$20"; break;
            default: break;
        }
        DrawText(TextFormat("[F] usar:  %s", hint), gx, gy+20, 13, ColorAlpha(WHITE,0.85f));
    }
}

// Mouse in the inventory (coords already virtualizadas p/ 1280x720). Layout casado with
// drawInventory(): slots equipados to the left; grades of gear/items to the right.
bool Player::handleInventoryMouse(Vector2 m, bool leftClick, bool rightClick) {
    const int PX = 64, PY = 44, PW = 1280 - 128;
    // Button FECHAR (X)
    Rectangle closeR = { (float)(PX + PW - 32), (float)(PY + 6), 24, 24 };
    if (leftClick && CheckCollisionPointRec(m, closeR)) return true;
    if (!leftClick && !rightClick) return false;

    // Slots equipados (left): x=82, y=252+s*90, 304x82
    for (int s = 0; s < 3; ++s) {
        Rectangle r = { 82.0f, (float)(252 + s * 90), 304.0f, 82.0f };
        if (CheckCollisionPointRec(m, r)) {
            selectedEquipSlot = s;
            if (rightClick) tryUpgradeEquip(s);     // button direito = upgrade
            return false;
        }
    }
    const int gx = 426, cell = 62, step = 68, cols = 11;
    // Grade EQUIPAMENTOS COLETADOS (baseY=116, 2 lines) — click esquerdo EQUIPA
    for (int i = 0; i < (int)equipBag.size() && i < 2 * cols; ++i) {
        Rectangle r = { (float)(gx + (i % cols) * step), (float)(116 + (i / cols) * step), (float)cell, (float)cell };
        if (CheckCollisionPointRec(m, r)) {
            selectedBagEquip = i;
            if (leftClick) equipFromBag(i);
            return false;
        }
    }
    // COLLECTED ITEMS GRID (baseY=288, 3 lines) — click selects; in the already-selected (ou direito) USA
    for (int i = 0; i < (int)inventory.size() && i < 3 * cols; ++i) {
        Rectangle r = { (float)(gx + (i % cols) * step), (float)(288 + (i / cols) * step), (float)cell, (float)cell };
        if (CheckCollisionPointRec(m, r)) {
            if (rightClick || (leftClick && selectedInvItem == i)) useInventoryItem(i);
            else selectedInvItem = i;
            return false;
        }
    }
    return false;
}
void Player::drawEquipment() const {
    // Legacy the€" full inventory now shown via drawInventory
    drawInventory();
}

bool Player::tryUpgradeEquip(int slot) {
    Equipment* eq = nullptr;
    if (slot == 0) eq = &equippedWeapon;
    else if (slot == 1) eq = &equippedArmor;
    else if (slot == 2) eq = &equippedImplant;
    if (!eq || eq->isEmpty() || !eq->canUpgrade()) return false;
    int cost = eq->upgradeCost();
    if (credits < cost) return false;
    credits -= cost;
    eq->upgradeLevel++;
    applyEquipmentStats();
    return true;
}

bool Player::useInventoryItem(int index) {
    if (index < 0 || index >= (int)inventory.size()) return false;
    const Item& it = inventory[index];
    switch (it.type) {
        case ItemType::HealthPack:
            heal(30.0f);
            break;
        case ItemType::NanoCore:
            baseMaxHealth += 25.0f;
            applyEquipmentStats();
            health = std::min(health + 25.0f, maxHealth);
            break;
        case ItemType::TechChip:
            addXP(50);
            break;
        case ItemType::PlasmaCell:
            for (auto& sk : skills) sk.currentCooldown = 0.0f;
            break;
        case ItemType::EnergyCore:
            shieldTimer = std::max(shieldTimer, 2.5f);
            break;
        case ItemType::ScrapMetal:
            heal(8.0f);
            credits += 8;
            break;
        case ItemType::WeaponPart:
            credits += 20;
            break;
        case ItemType::Credits:
            credits += it.value;
            break;
    }
    inventory.erase(inventory.begin() + index);
    if (selectedInvItem >= (int)inventory.size())
        selectedInvItem = std::max(0, (int)inventory.size() - 1);
    return true;
}

bool Player::tryFuseItems() {
    // Count each item type
    for (int t = 0; t < 8; ++t) {
        ItemType target = static_cast<ItemType>(t);
        int count = 0;
        for (const auto& it : inventory)
            if (it.type == target) count++;
        if (count >= 3) {
            // Removes 3 of this type
            int removed = 0;
            for (auto it = inventory.begin(); it != inventory.end() && removed < 3; ) {
                if (it->type == target) { it = inventory.erase(it); removed++; }
                else ++it;
            }
            // Grant evolved effect: double the standard bonus
            switch (target) {
                case ItemType::HealthPack:  heal(80.0f); break;
                case ItemType::NanoCore:
                    baseMaxHealth += 60.0f; applyEquipmentStats();
                    health = std::min(health + 60.0f, maxHealth); break;
                case ItemType::TechChip:    addXP(200); break;
                case ItemType::PlasmaCell:
                    for (auto& sk : skills) { sk.cooldown *= 0.85f; sk.currentCooldown = 0.0f; } break;
                case ItemType::EnergyCore:
                    shieldTimer = 6.0f; overloadTimer = 4.0f; break;
                case ItemType::ScrapMetal:
                    baseAttackDamage += 5.0f; applyEquipmentStats(); break;
                case ItemType::WeaponPart:
                    baseAttackDamage += 12.0f; baseAttackRange += 15.0f; applyEquipmentStats(); break;
                case ItemType::Credits:     credits += 200; break;
            }
            if (selectedInvItem >= (int)inventory.size())
                selectedInvItem = std::max(0, (int)inventory.size() - 1);
            return true;
        }
    }
    return false;
}

void Player::handleInventoryInput() {
    if (IsKeyPressed(KEY_ONE))   selectedEquipSlot = 0;
    if (IsKeyPressed(KEY_TWO))   selectedEquipSlot = 1;
    if (IsKeyPressed(KEY_THREE)) selectedEquipSlot = 2;
    if (IsKeyPressed(KEY_U))     tryUpgradeEquip(selectedEquipSlot);

    // Item bag navigation
    int invSize = (int)inventory.size();
    if (invSize > 0) {
        if (IsKeyPressed(KEY_Q)) selectedInvItem = (selectedInvItem - 1 + invSize) % invSize;
        if (IsKeyPressed(KEY_E)) selectedInvItem = (selectedInvItem + 1) % invSize;
        if (IsKeyPressed(KEY_F)) useInventoryItem(selectedInvItem);
        if (IsKeyPressed(KEY_R)) tryFuseItems();
    }

    // Equipment backpack: arrows navegam, T equipa the selected
    int bagSize = (int)equipBag.size();
    if (bagSize > 0) {
        if (IsKeyPressed(KEY_DOWN)) selectedBagEquip = (selectedBagEquip + 1) % bagSize;
        if (IsKeyPressed(KEY_UP))   selectedBagEquip = (selectedBagEquip - 1 + bagSize) % bagSize;
        if (IsKeyPressed(KEY_T))    equipFromBag(selectedBagEquip);
    }
}

void Player::heal(float amount) {
    health = std::min(health + amount, maxHealth);
}

// Potion/power of healing (estilo Diablo 3): healing ~35% of the health in the hour and ativa uma
// regeneracao by some seconds. Cooldown impede spam.
void Player::usePotion() {
    if (healCooldown > 0.0f) return;
    if (health <= 0.0f) return;
    heal(maxHealth * 0.35f);
    regenTimer   = 4.0f;     // regen buff
    healCooldown = 14.0f;    // cooldown as the globo/potion of the D3
}

void Player::increaseBaseMaxHP(float amount) {
    baseMaxHealth += amount;
    applyEquipmentStats();
}

void Player::increaseBaseAttackDamage(float amount) {
    baseAttackDamage += amount;
    applyEquipmentStats();
}

void Player::increaseBaseSpeed(float amount) {
    baseSpeed += amount;
    applyEquipmentStats();
}

void Player::increaseBaseAttackRange(float amount) {
    baseAttackRange += amount;
    applyEquipmentStats();
}

void Player::increaseBaseDefense(float amount) {
    baseDefense += amount;
    applyEquipmentStats();
}

void Player::takeDamage(float amount) {
    if (inSafeRefuge) return;   // refuge = invulneravel (in the one kills you in the city)
    if (isShielded()) return;
    auto st = SkillTree::statsFor(perkMask);
    if (st.evade > 0.0f && (rand() % 100) < (int)(st.evade * 100.0f)) return;
    float reduced = amount * (1.0f - defense / 100.0f);
    health -= reduced;
    if (health < 0.0f && st.revive && reviveReady) {
        reviveReady = false;
        reviveTimer = 60.0f;
        health      = maxHealth * 0.40f;
        say("PROTOCOLO IMORTAL! Reconstruindo.", 3.0f, {0, 220, 160, 255});
        return;
    }
    if (health < 0.0f) health = 0.0f;
}

void Player::addXP(int amount) {
    xp += static_cast<int>(amount * xpMultiplier);
    while (xp >= xpToNextLevel) {
        xp -= xpToNextLevel;
        levelUp();
    }
}

void Player::levelUp() {
    level++;
    // Curva more ingreme = progressao more slow and merecida (pedido of the user).
    // base 450, expoente 2.05 -> Lv1→2 ~450, Lv5→6 ~12k, Lv10→11 ~50k, Lv20→21 ~210k.
    xpToNextLevel = (int)(450.0f * powf((float)level, 2.05f));
    // Ganhos by level: HP and damage fortes (progressao perceptivel); range and
    // speed crescem devagar to not virar bola of snow injusta.
    baseMaxHealth    += 26.0f;
    baseAttackDamage += 8.0f;
    baseAttackRange  += 2.5f;
    baseSpeed        += 2.0f;

    // Passive milestones — spaced to match longer XP curve
    lastPassive.clear();
    switch (level) {
        case 5:  baseMaxHealth    += 40.0f;
                 lastPassive = "PASSIVA: Blindagem of Campo +40 HP"; break;
        case 8:  baseAttackDamage += 15.0f;
                 lastPassive = "PASSIVA: Calibracao of Combat +15 Damage"; break;
        case 10: baseAttackRange  += 30.0f; baseSpeed += 15.0f;
                 lastPassive = "PASSIVA: Amplificador Neural +30 Range +15 Vel"; break;
        case 15: baseMaxHealth    += 80.0f; baseAttackDamage += 20.0f;
                 lastPassive = "PASSIVA: Protocolo IRON-VIII +80 HP +20 Damage"; break;
        case 20: baseAttackDamage += 30.0f; baseMaxHealth += 60.0f;
                 lastPassive = "PASSIVA: Overload Neural +30 Damage +60 HP"; break;
        case 25: baseAttackDamage += 40.0f; baseMaxHealth += 100.0f; baseSpeed += 20.0f;
                 lastPassive = "PASSIVA: EXECUTOR LENDARIO +40 Damage +100 HP +20 Vel"; break;
        case 30: baseAttackDamage += 60.0f; baseMaxHealth += 150.0f;
                 lastPassive = "PASSIVA: ASCENSAO OMEGA +60 Damage +150 HP"; break;
        case 40: baseMaxHealth    += 250.0f; baseAttackDamage += 80.0f; baseSpeed += 30.0f;
                 lastPassive = "PASSIVA: TRANSCENDENCIA KRONOS +250 HP +80 Damage +30 Vel"; break;
        default: break;
    }

    // Skill cooldown reduction at evolution levels: 10, 25, 40, 60
    static const int CD_LESPEEDS[] = {10, 25, 40, 60};
    for (int cl : CD_LESPEEDS) {
        if (level == cl) {
            cdEvoMult *= 0.88f;
            break;
        }
    }

    applyEquipmentStats();
    refreshSkillVectors();
    health = maxHealth;
    leveledUp    = true;
    levelUpTimer = 2.5f;
    unclaimedLevels++;   // the Game drena isto (account TODOS the levels, inclusive multiplos)
    skillPoints++;       // Hack Tree: 1 point of perk by level

    // Level-up speech
    if (level % 10 == 0)
        say("Level " + std::to_string(level) + "! Power maximum if aproxima.", 3.5f, {255,200,0,255});
    else if (level % 5 == 0)
        say("Level " + std::to_string(level) + "! Staying more strong.", 2.5f, {0,220,255,255});
}

void Player::equipItem(const Equipment& equip) {
    // Guarda the equipment current of the slot in the mochila (not descarta — estilo Diablo).
    Equipment* slot = nullptr;
    switch (equip.slot) {
        case EquipSlot::Weapon:  slot = &equippedWeapon;  break;
        case EquipSlot::Armor:   slot = &equippedArmor;   break;
        case EquipSlot::Implant: slot = &equippedImplant; break;
        default: break;
    }
    if (slot) {
        if (!slot->isEmpty()) {
            equipBag.push_back(*slot);
            if (equipBag.size() > 24) equipBag.erase(equipBag.begin()); // ceiling
        }
        *slot = equip;
    }
    applyEquipmentStats();
}

// Equipa the item idx of the mochila; the that was equipado returns for the mochila.
void Player::equipFromBag(int idx) {
    if (idx < 0 || idx >= (int)equipBag.size()) return;
    Equipment chosen = equipBag[idx];
    equipBag.erase(equipBag.begin() + idx);

    Equipment* slot = nullptr;
    switch (chosen.slot) {
        case EquipSlot::Weapon:  slot = &equippedWeapon;  break;
        case EquipSlot::Armor:   slot = &equippedArmor;   break;
        case EquipSlot::Implant: slot = &equippedImplant; break;
        default: break;
    }
    if (slot) {
        if (!slot->isEmpty()) equipBag.push_back(*slot); // the old returns p/ the bolsa
        *slot = chosen;
    } else {
        // Slot invalid: devolve the item to the bolsa instead of perde-lo.
        equipBag.push_back(chosen);
    }
    if (selectedBagEquip >= (int)equipBag.size())
        selectedBagEquip = std::max(0, (int)equipBag.size() - 1);
    applyEquipmentStats();
}

int Player::visualSignature() const {
    auto h = [](const std::string& v, int seed) {
        int acc = seed;
        for (char c : v) acc = acc * 31 + (unsigned char)c;
        return acc;
    };
    int sig = h(equippedWeapon.name, 7);
    sig = h(equippedArmor.name,   sig);
    sig = h(equippedImplant.name, sig);
    sig = sig * 31 + equippedWeapon.upgradeLevel * 7 + equippedArmor.upgradeLevel * 13
                   + equippedImplant.upgradeLevel * 17;
    sig = sig * 31 + (int)cosmeticTint.r + (int)cosmeticTint.g * 3 + (int)cosmeticTint.b * 5;
    // Skins premium (Gems) also mudam the aparencia — without isto the model 3D ficava
    // with the visual old cacheado after the purchase.
    sig = sig * 31 + (hasCosmeticTint ? 1 : 0) + (skinNeon ? 2 : 0)
                   + (skinDragon ? 4 : 0) + (petDrone ? 8 : 0);
    return (sig & 0x7fffffff) % 997;
}

void Player::applyEquipmentStats() {
    attackDamage = baseAttackDamage;
    attackRange  = baseAttackRange;
    maxHealth    = baseMaxHealth;
    speed        = baseSpeed;
    defense      = baseDefense;   // defense base of the class (armor soma by up)
    xpMultiplier = 1.0f;

    if (!equippedWeapon.isEmpty()) {
        attackDamage += equippedWeapon.getEffectivePrimary();
        attackRange  += equippedWeapon.getEffectiveSecondary();
    }
    if (!equippedArmor.isEmpty()) {
        maxHealth += equippedArmor.getEffectivePrimary();
        defense   += equippedArmor.getEffectiveSecondary();
    }
    if (!equippedImplant.isEmpty()) {
        speed        += equippedImplant.getEffectivePrimary();
        xpMultiplier  = (equippedImplant.getEffectiveSecondary() > 0.0f) ? equippedImplant.getEffectiveSecondary() : 1.0f;
    }

    // Bonus of evolution aplicados AQUI (and not with *= direct), to not serem
    // apagados the cada level-up/equip. Idempotent and persistente.
    switch (evolutionPath) {
        case EvolutionPath::CyborgSoldier:  attackDamage *= 1.30f; break;       // +30% damage
        case EvolutionPath::HackerFantasma: speed *= 1.40f; xpMultiplier *= 1.15f; break; // +40% vel
        case EvolutionPath::ExecutorOmega:  maxHealth *= 1.50f; attackDamage *= 1.10f; break; // +50% HP
        default: break;
    }
    // Tiers of evolution extra acumulam um small bonus general
    if (evolutionTier > 1) {
        float t = 1.0f + (evolutionTier - 1) * 0.06f;
        attackDamage *= t;
        maxHealth    *= t;
    }

    // Limites of security: defense not passes of 80% and speed has ceiling
    if (defense > 80.0f) defense = 80.0f;
    if (speed   > 260.0f) speed  = 260.0f;

    if (health > maxHealth) health = maxHealth;
}

float Player::getEffectiveDamage() const {
    return attackDamage * (isOverloaded() ? 1.5f : 1.0f)
                        * SkillTree::statsFor(perkMask).weaponMult;
}

void Player::say(const std::string& text, float duration, Color col) {
    if (speech.active && speech.timer > 1.5f) return; // don't interrupt important speech
    speech.text     = text;
    speech.duration = duration;
    speech.timer    = duration;
    speech.active   = true;
    speech.color    = col;
}

void Player::updateSpeech(float dt) {
    if (speech.active) {
        speech.timer -= dt;
        if (speech.timer <= 0.0f) speech.active = false;
    }

    // Kill streak timer
    if (killStreak.count > 0) {
        killStreak.resetTimer -= dt;
        if (killStreak.resetTimer <= 0.0f) killStreak.count = 0;
    }

    // Low HP speech trigger
    bool isLowHP = health < maxHealth * 0.20f;
    if (isLowHP && !wasLowHP) {
        say("Preciso me recuperar!", 3.0f, {255, 80, 80, 255});
    }
    wasLowHP = isLowHP;
}

void Player::renderSpeech() const {
    if (!speech.active || speech.text.empty()) return;

    float alpha = 1.0f;
    if (speech.timer < 0.5f) alpha = speech.timer / 0.5f;  // fade out
    if (speech.duration - speech.timer < 0.2f)
        alpha = (speech.duration - speech.timer) / 0.2f;   // fade in

    const char* txt = speech.text.c_str();
    int tw  = MeasureText(txt, 13);
    int bw  = tw + 16;
    int bh  = 24;
    int bx  = (int)(position.x - bw/2);
    int by  = (int)(position.y - 70 - bh);

    // Bubble background
    DrawRectangle(bx, by, bw, bh, ColorAlpha({10, 20, 40, 255}, 0.82f * alpha));
    DrawRectangleLinesEx({(float)bx,(float)by,(float)bw,(float)bh}, 1.5f,
                         ColorAlpha(speech.color, 0.9f * alpha));
    // Tail pointing down
    DrawTriangle(
        {(float)(bx + bw/2 - 5), (float)(by + bh)},
        {(float)(bx + bw/2 + 5), (float)(by + bh)},
        {(float)(bx + bw/2),     (float)(by + bh + 8)},
        ColorAlpha({10, 20, 40, 255}, 0.82f * alpha)
    );
    DrawText(txt, bx + 8, by + 5, 13, ColorAlpha(speech.color, alpha));
}

void Player::onKill() {
    totalKills++;
    killStreak.count++;
    killStreak.resetTimer = 4.0f;

    auto st = SkillTree::statsFor(perkMask);
    if (st.lifesteal > 0.0f) heal(maxHealth * st.lifesteal);

    if (killStreak.count == 5)
        say("Cinco seguidos!", 2.5f, {255, 200, 0, 255});
    else if (killStreak.count == 10)
        say("Imparavel!", 3.0f, {255, 120, 0, 255});
    else if (killStreak.count == 20)
        say("LENDARIO!", 3.5f, {255, 50, 0, 255});

    if (totalKills == 1)
        say("Primeiro blood.", 2.0f);
    else if (totalKills == 100)
        say("100 eliminados. Isso and only the comeco.", 3.5f);
    else if (totalKills == 500)
        say("500 kills. Sou uma maquina of guerra.", 3.5f, {255,120,0,255});
}

void Player::onBossFound() {
    say("Aqui is the chefao...", 3.0f, {255, 60, 0, 255});
}

void Player::onPortalClosed() {
    say("Portal closed. Um the less.", 2.5f, {0, 200, 255, 255});
}

void Player::onEnterZone(int zoneId) {
    static const char* zoneSpeeches[] = {
        "Ruins of Avalon. Territorio KRONOS.",     // 0
        "Bunker Nexus. Tensao in the ar.",               // 1
        "Forge KRONOS. Calor intenso.",              // 2
        "Zone desconhecida.",                        // 3
        "Lugar sombrio... almas presas here.",       // 4 Cemetery
        "Farm maldita. Algo not is right.",     // 5 CursedFarm
        "City ghost. Silencio mortal.",         // 6 GhostCity
        "Forest negra. Perigo by all parte.",    // 7 DarkForest
        "Catacumbas. Profundidade without end.",         // 8 Catacombs
        "Manor abandonada. Ela still vive.",        // 9 Manor
        "Calor extremo. Cuidado with the lava.",        // 10 Inferno
    };
    int idx = zoneId;
    if (idx < 0 || idx > 10) idx = 3;
    say(zoneSpeeches[idx], 3.0f, {0, 220, 255, 255});
}
