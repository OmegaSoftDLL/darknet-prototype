#include "Player.h"
#include "Effects.h"
#include <cmath>
#include <string>
extern bool g_renderPass3D;

Player::Player() {
    // Habilidades balanceadas: basico rapido/baixo dano, controle medio, burst alto/cd longo
    skills.emplace_back("Laser",     "Raio de alta energia que perfura",  1.0f, 48.0f,  560.0f, KEY_ONE);
    skills.emplace_back("EMP",       "Pulso EMP em area - atordoa",       5.0f, 95.0f,  180.0f, KEY_TWO);
    skills.emplace_back("Granada",   "Granada de plasma com AoE massivo", 7.0f,140.0f,  180.0f, KEY_THREE);
    skills.emplace_back("Sobrecarga","Dano +50% Vel +30% por 8s",        16.0f, 0.0f,   0.0f,  KEY_FOUR);
    skills.emplace_back("Barreira",  "Campo de forca: imune 3s",         24.0f, 0.0f,   0.0f,  KEY_FIVE);
    skills.emplace_back("Rajada",    "Salva: 8 projetos em leque",        9.0f, 42.0f,  440.0f, KEY_SIX);

    // Guarda o dano base de cada skill para aplicar o skillPower da classe de forma idempotente
    for (auto& s : skills) baseSkillDamage.push_back(s.damage);

    applyClass(CharacterClass::Soldado);  // classe padrao
}

// ── Sistema de classes ───────────────────────────────────────────────────────
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
            classSkin={210,180,150,255}; classTrim={235,210,120,255}; skillMult=2.0f; break;  // glass cannon magico
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
    // Dano de habilidade da classe (idempotente — recalcula do base guardado)
    if (baseSkillDamage.size() == skills.size())
        for (size_t i = 0; i < skills.size(); ++i)
            skills[i].damage = baseSkillDamage[i] * skillMult;

    applyEquipmentStats();   // mantem bonus de equipamento/evolucao
    health = maxHealth;
}

const char* Player::className(CharacterClass c) {
    switch (c) {
        case CharacterClass::Soldado:   return "SOLDADO";
        case CharacterClass::Guerreira: return "GUERREIRA";
        case CharacterClass::Robo:      return "ROBO DE COMBATE";
        case CharacterClass::Mago:      return "MAGO";
        case CharacterClass::Bruxa:     return "BRUXA";
        case CharacterClass::HomemFera: return "HOMEM-FERA";
        default: return "?";
    }
}

const char* Player::classDescription(CharacterClass c) {
    switch (c) {
        case CharacterClass::Soldado:   return "Equilibrado. Armadura, rifle. Bom para comecar.";
        case CharacterClass::Guerreira: return "Rapida e agil. Pistolas duplas. Pouca defesa.";
        case CharacterClass::Robo:      return "Tanque. Muito HP e defesa, mas lento.";
        case CharacterClass::Mago:      return "Frageil, mas habilidades devastadoras (x2 dano).";
        case CharacterClass::Bruxa:     return "Magica equilibrada. Boa mobilidade e skills.";
        case CharacterClass::HomemFera: return "Brutamonte. Dano corpo a corpo altissimo, rapido.";
        default: return "";
    }
}

const char* Player::classFantasy(CharacterClass c) {
    switch (c) {
        case CharacterClass::Soldado:   return "Veterano da resistencia NEXUS, meio homem meio maquina.";
        case CharacterClass::Guerreira: return "Cacadora que nunca errou um alvo em movimento.";
        case CharacterClass::Robo:      return "Unidade aliada reprogramada para proteger a humanidade.";
        case CharacterClass::Mago:      return "Ultimo arcanista a dominar codigo e feitico ao mesmo tempo.";
        case CharacterClass::Bruxa:     return "Tece magia antiga contra a fria logica do KRONOS.";
        case CharacterClass::HomemFera: return "Maldicao genetica virou sua maior arma na guerra.";
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

    // Aceleracao responsiva mas com peso. Sem drag competindo (ver update),
    // a velocidade atinge 100% do alvo — fim da sensacao "lenta/flutuante".
    float t = 1.0f - std::exp(-24.0f * dt);
    velocity.x += (direction.x * currentSpeed - velocity.x) * t;
    velocity.y += (direction.y * currentSpeed - velocity.y) * t;

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    isMoving      = true;
    moveRequested = true;
    if (velocity.x >  12.0f) facing =  1;
    if (velocity.x < -12.0f) facing = -1;
    // A cadencia da passada e avancada em update() (evita contar 2x e acelerar demais).
}

void Player::update(float dt) {
    isMoving = false;

    // Cura: cooldown da poção + buff de regeneração ativo (estilo Diablo 3).
    if (healCooldown > 0.0f) healCooldown -= dt;
    if (regenTimer   > 0.0f) {
        regenTimer -= dt;
        heal(maxHealth * 0.06f * dt);   // ~6% maxHP por segundo enquanto dura
    }

    // Coast/freio: SO desacelera quando NAO houve input no frame anterior.
    // Enquanto anda, nao ha drag competindo com a aceleracao -> velocidade plena.
    if (!moveRequested) {
        float drag = 1.0f - std::exp(-22.0f * dt);  // parada firme, sem deslizar
        velocity.x -= velocity.x * drag;
        velocity.y -= velocity.y * drag;
    }
    moveRequested = false;

    // Avanca a animacao de caminhada pela velocidade real (anima mesmo na inercia)
    {
        float vm = std::sqrt(velocity.x*velocity.x + velocity.y*velocity.y);
        if (vm > 12.0f) walkAnimTimer += dt * (4.0f + 5.0f * (vm / (speed + 1.0f)));
    }

    // Pulo (z-height) — arco de salto para cruzar obstaculos/subir
    if (isJumping) {
        jumpZ    += jumpVel * dt;
        jumpVel  -= 900.0f * dt;       // gravidade
        if (jumpZ <= 0.0f) { jumpZ = 0.0f; jumpVel = 0.0f; isJumping = false; }
    }

    for (auto& skill : skills) skill.update(dt);

    if (overloadTimer > 0.0f) overloadTimer -= dt;
    if (shieldTimer   > 0.0f) shieldTimer   -= dt;

    if (levelUpTimer > 0.0f) levelUpTimer -= dt;
    leveledUp = levelUpTimer > 0.0f;
    evolutionPulse += dt;
}

void Player::render() const {
    // Ciclo de caminhada com peso: pernas alternam e o corpo afunda a cada
    // pisada (footfall), dando sensacao de pisar no chao em vez de flutuar.
    // "Andando" pela VELOCIDADE real (nao pela flag isMoving, que e zerada em
    // update() antes do render -> causava personagem deslizando/"voando").
    float velMag = std::sqrt(velocity.x*velocity.x + velocity.y*velocity.y);
    bool  moving = velMag > 12.0f;
    float walk   = moving ? std::sin(walkAnimTimer) : 0.0f;
    float walkAbs= moving ? std::fabs(walk) : 0.0f;
    float legL   =  walk * 19.0f;   // passos maiores e visiveis
    float legR   = -walk * 19.0f;
    // Pisada: o corpo afunda no momento em que um pe planta (|walk| no pico).
    // bob negativo no meio do passo (pe no ar) e zero ao plantar -> sensacao de peso.
    float bodyBob = moving ? (walkAbs * 4.0f - 2.0f) : 0.0f;
    // Sombra esmaga/estica conforme a pisada (mais larga e escura ao plantar o pe)
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

    // Pulo levanta o corpo (z); a sombra encolhe conforme a altura
    float lift = jumpZ;
    float px = position.x, py = position.y + bodyBob - lift; // dip down + sobe ao pular
    float f  = (float)facing;
    if (lift > 1.0f) {
        float k = 1.0f / (1.0f + lift * 0.02f);
        shadowW *= k; shadowH *= k; shadowA *= k;   // sombra menor/mais fraca no ar
    }

    // Attack range indicator (subtle)
    DrawCircleLines((int)px, (int)py, attackRange, ColorAlpha(C_neon, 0.07f));

    // Ground shadow â€" fixed at feet level (not affected by body dip), com squash
    if (!g_renderPass3D) {
        DrawEllipse((int)position.x, (int)(position.y + 51.0f), shadowW, shadowH, ColorAlpha(BLACK, shadowA));
    }

    // Corpo distinto por classe (Soldado usa a arte cyborg detalhada abaixo)
    switch (charClass) {
        case CharacterClass::Guerreira: renderGuerreira(px,py,f,legL,legR); break;
        case CharacterClass::Robo:      renderRobo(px,py,f,legL,legR);      break;
        case CharacterClass::Mago:      renderMago(px,py,f,legL,legR);      break;
        case CharacterClass::Bruxa:     renderBruxa(px,py,f,legL,legR);     break;
        case CharacterClass::HomemFera: renderHomemFera(px,py,f,legL,legR); break;
        default: break;
    }

    // ── OVERLAY DE EQUIPAMENTO — o personagem MUDA conforme o que esta equipado.
    //    Funciona para todas as classes (desenhado por cima do corpo base).
    {
        // Armadura: peitoral + ombreiras tingidas pela cor da armadura; brilho
        // aumenta com tier/upgrade -> visivelmente mais "blindado".
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
            // estrelas de upgrade brilhando no peito
            for (int s = 0; s < equippedArmor.upgradeLevel; ++s)
                DrawCircleV({px-4.0f + s*4.0f, py-2.0f}, 1.3f, ColorAlpha(WHITE, 0.9f));
        }

        // Implante: orbe brilhante pulsante perto da cabeca.
        if (!equippedImplant.isEmpty()) {
            Color ic = equippedImplant.color;
            float pul = 0.6f + 0.4f * std::sin(walkAnimTimer * 4.0f + position.x * 0.05f);
            DrawCircleV({px + f*6.0f, py - 20.0f}, 3.5f + pul*1.5f, ColorAlpha(ic, 0.35f));
            DrawCircleV({px + f*6.0f, py - 20.0f}, 2.0f, ic);
        }

        // Arma: barril na mao tingido pela cor da arma, comprimento cresce com tier.
        // O Soldado JA desenha sua propria arma detalhada (tingida pela cor) — evita
        // arma duplicada desenhando o overlay so para as demais classes.
        if (!equippedWeapon.isEmpty() && charClass != CharacterClass::Soldado) {
            Color wc = equippedWeapon.color;
            float wlen = 16.0f + 5.0f * (float)equippedWeapon.tier;
            float hx = px + f*12.0f, hy = py + 2.0f;
            DrawRectangle((int)hx, (int)(hy-2), (int)(f*wlen), 4, ColorAlpha(wc, 0.95f));
            DrawCircleV({hx + f*wlen, hy}, 2.5f + 0.5f*equippedWeapon.tier, ColorAlpha(wc, 0.8f));
        }
    }

    // â"€â"€ LEGS â€" left leg = mechanical exo, right leg = human in armored boot â"€â"€
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
    {   // RIGHT LEG (human side â€" thigh + calf in torn pants + armored boot)
        float rx = px + 1, ky = py + 28;
        // Thigh â€" dark pants
        DrawRectangle((int)(rx+legR),     (int)(py+12), 9, 17, C_jacket);
        DrawRectangle((int)(rx+legR+1),   (int)(py+13), 7, 15, C_jackLt);
        // Knee
        DrawCircleV({rx+4.5f+legR, ky}, 5.0f, C_chrome);
        DrawCircleV({rx+4.5f+legR, ky}, 2.0f, C_dark);
        // Calf â€" exposed muscle line (torn)
        DrawRectangle((int)(rx+1+legR),  (int)(ky+4),  7, 14, C_jacket);
        DrawLineEx({rx+5+legR, ky+4}, {rx+5+legR, ky+14}, 1.5f, ColorAlpha(C_skin, 0.4f)); // torn seam
        // Boot (armored)
        DrawRectangle((int)(rx-2+legR),  (int)(ky+17), 13, 5, C_servo);
        DrawRectangle((int)(rx-4+legR),  (int)(ky+20), 15, 3, C_dark);
    }

    // â"€â"€ PELVIS â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    DrawRectangle((int)(px-13), (int)(py+8), 13, 6, C_servo);   // left = machine
    DrawRectangle((int)(px),    (int)(py+8), 13, 6, C_jacket);  // right = jacket
    DrawRectangle((int)(px-11), (int)(py+9), 22, 4, C_plate);
    DrawCircleV({px-9, py+12}, 4, C_chrome);
    DrawCircleV({px+9, py+12}, 4, C_chrome);

    // â"€â"€ TORSO â€" LEFT HALF MACHINE / RIGHT HALF HUMAN â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    // Back plate
    DrawRectangle((int)(px-15), (int)(py-14), 30, 24, C_hull);
    // Left machine half
    DrawRectangle((int)(px-13), (int)(py-13), 13, 22, C_plate);
    DrawRectangle((int)(px-12), (int)(py-12), 11, 19, C_plateLt);
    for (int i = 0; i < 3; ++i) {
        float ry = py-9+i*7.0f;
        DrawLineEx({px-11,ry},{px-2,ry},1.0f,ColorAlpha(C_chrome,0.35f));
    }
    // Right human half â€" jacket
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

    // â"€â"€ LEFT SHOULDER + MECHANICAL ARM â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
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

    // â"€â"€ RIGHT SHOULDER + HUMAN ARM + WEAPON â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    {
        float sx = px + 10;
        // Human shoulder in jacket
        DrawRectangle((int)(sx),   (int)(py-15), 12, 22, C_jacket);
        DrawRectangle((int)(sx+1), (int)(py-14), 10, 8,  C_jackLt);
        DrawCircleV({sx+6, py-14}, 5, C_chrome); // joint implant visible
        // Upper arm â€" human flesh + jacket sleeve
        DrawRectangle((int)(sx+1), (int)(py-9), 8, 14, C_jacket);
        DrawRectangle((int)(sx+2), (int)(py-8), 6, 10, C_jackLt);
        // Elbow â€" human
        DrawCircleV({sx+5, py+6}, 4, C_chrome);
        DrawCircleV({sx+5, py+6}, 1.5f, C_dark);
        // Forearm â€" partly exposed skin
        DrawRectangle((int)(sx+2), (int)(py+6),  5, 10, C_jacket);
        DrawRectangle((int)(sx+3), (int)(py+8),  3, 5, C_skin);  // bare forearm
    }

    // â"€â"€ WEAPON â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
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

    // â"€â"€ NECK â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    // Left (machine) neck
    DrawRectangle((int)(px-4), (int)(py-18), 4, 6, C_servo);
    DrawRectangle((int)(px-3), (int)(py-17), 3, 3, C_chrome);
    // Right (human) neck
    DrawRectangle((int)(px),   (int)(py-18), 4, 6, C_skin);

    // â"€â"€ HEAD â€" LEFT HALF MACHINE / RIGHT HALF HUMAN â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    // Machine half (left) â€" IRON-VIII skull
    DrawRectangle((int)(px-12), (int)(py-40), 12, 24, C_hull);
    DrawRectangle((int)(px-11), (int)(py-38),  5, 20, C_plate);
    DrawRectangle((int)(px- 6), (int)(py-38),  6, 22, C_servo);
    DrawRectangle((int)(px-10), (int)(py-40), 10, 4, C_plateLt);
    // Machine eye â€" glowing red/orange
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

    // Human half (right) â€" scarred organic face
    DrawRectangle((int)(px),  (int)(py-40), 12, 24, C_skin);
    // Scar line (diagonal across cheek â€" from old battle)
    DrawLineEx({px+2,py-35},{px+10,py-22}, 1.5f, ColorAlpha({100,60,60,255},0.55f));
    // Human eye (right) â€" dark iris
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
    } // fim do corpo Soldado

    // â"€â"€ STATUS EFFECTS â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    if (sh) {
        float st = std::sin(shieldTimer * 10.0f);
        float r  = radius + 24 + st * 5;
        DrawCircleLines((int)px, (int)py, r,      ColorAlpha({0,255,200,255}, 0.75f));
        DrawCircleLines((int)px, (int)py, r + 6,  ColorAlpha({0,200,255,255}, 0.35f));
        DrawCircleLines((int)px, (int)py, r + 12, ColorAlpha({0,255,255,255}, 0.15f));
        for (int i = 0; i < 6; ++i) {
            float a  = i * 1.047f + shieldTimer * 1.2f;
            Vector2 hp = {px + std::cos(a)*r, py + std::sin(a)*r};
            DrawCircleV(hp, 4, ColorAlpha({0,255,200,255}, 0.7f));
        }
    }
    if (ol) {
        float st = std::sin(overloadTimer * 14.0f);
        float r  = radius + 16 + st * 4;
        DrawCircleLines((int)px, (int)py, r,     ColorAlpha({255,120,0,255}, 0.75f));
        DrawCircleLines((int)px, (int)py, r + 7, ColorAlpha({255,200,0,255}, 0.3f));
        for (int i = 0; i < 6; ++i) {
            float a  = i * 1.047f + overloadTimer * 6.0f;
            Vector2 sp = {px + std::cos(a)*(r+3), py + std::sin(a)*(r+3)};
            DrawCircleV(sp, 3, ColorAlpha({255,220,0,255}, 0.85f));
        }
    }
    if (leveledUp) {
        float st = std::sin(levelUpTimer * 14.0f);
        DrawCircleLines((int)px, (int)py, radius + 16 + st*5, ColorAlpha(GOLD, 0.9f));
        DrawCircleLines((int)px, (int)py, radius + 28 + st*5, ColorAlpha(GOLD, 0.4f));
    }
    // Buff de cura/regeneração (estilo Diablo 3): aura verde + cruzes subindo.
    if (regenTimer > 0.0f) {
        Color heal = {0, 255, 120, 255};
        float pul = 0.5f + 0.5f * std::sin((float)GetTime() * 6.0f);
        DrawCircleLines((int)px, (int)py, radius + 12 + pul*4, ColorAlpha(heal, 0.7f));
        for (int i = 0; i < 4; ++i) {
            float t2 = std::fmod((float)GetTime() * 0.8f + i * 0.25f, 1.0f);
            float cxh = px + std::sin((float)GetTime()*2.0f + i*1.6f) * 14.0f;
            float cyh = py + 6 - t2 * 38.0f;
            float a   = (1.0f - t2) * 0.9f;
            // cruz de cura
            DrawRectangle((int)(cxh-3), (int)(cyh-1), 6, 2, ColorAlpha(heal, a));
            DrawRectangle((int)(cxh-1), (int)(cyh-3), 2, 6, ColorAlpha(heal, a));
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

    // ══ COSMÉTICOS ════════════════════════════════════════════════════════════
    // 1) Tinta da loja comum (creditos): modula as cores do corpo com uma demao
    //    translucida na cor escolhida (peito + ombros + cabeca).
    if (hasCosmeticTint) {
        Color t = cosmeticTint;
        DrawRectangle((int)(px-9), (int)(py-12), 18, 20, ColorAlpha(t, 0.38f)); // torso
        DrawCircleV({px, py-30}, 10.0f, ColorAlpha(t, 0.30f));                  // cabeca
        DrawCircleV({px-9, py-12}, 5.0f, ColorAlpha(t, 0.40f));                 // ombro E
        DrawCircleV({px+9, py-12}, 5.0f, ColorAlpha(t, 0.40f));                 // ombro D
    }

    // 2) Skin Neon (Gems): brilho neon pulsante ADITIVO ao redor do modelo.
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

    // 3) Skin Dragão (Gems): tonalidade quente + brasas laranjas subindo ao andar.
    if (skinDragon) {
        DrawRectangle((int)(px-9), (int)(py-12), 18, 20, ColorAlpha(Color{255,90,0,255}, 0.22f));
        // olhos laranja/vermelho incandescentes
        DrawCircleV({px-4, py-30}, 2.0f, Color{255,80,0,255});
        DrawCircleV({px+5, py-30}, 2.0f, Color{255,80,0,255});
        DrawGlowCircle({px-4, py-30}, 2.0f, Color{255,120,0,255}, 3.0f);
        DrawGlowCircle({px+5, py-30}, 2.0f, Color{255,120,0,255}, 3.0f);
        // escamas/linhas de fogo no peito
        for (int i = 0; i < 3; ++i)
            DrawLineEx({px-6.0f+i*6, py-8}, {px-6.0f+i*6, py+4}, 1.5f,
                       ColorAlpha(Color{255,140,0,255}, 0.6f));
        // brasas subindo (mais intensas em movimento)
        int embers = moving ? 5 : 2;
        for (int i = 0; i < embers; ++i) {
            float t2 = std::fmod((float)GetTime() * 0.6f + i * 0.21f, 1.0f);
            float ex = px + std::sin((float)GetTime()*3.0f + i) * 10.0f;
            float ey = py - 8 - t2 * 34.0f;
            float a  = (1.0f - t2) * 0.8f;
            DrawCircleV({ex, ey}, 1.5f + (1.0f-t2)*2.0f,
                        ColorAlpha(Color{255, (unsigned char)(120+(int)(100*t2)), 0, 255}, a));
        }
    }

    // 4) Drone de Estimação (Gems): drone pixel-art flutuando acima-à-esquerda.
    if (petDrone) {
        float hov = std::sin((float)GetTime() * 3.0f) * 4.0f;   // voo estavel
        float dx = px - 16.0f;            // acima e a esquerda da cabeca
        float dy = py - 36.0f + hov;
        // corpo
        DrawRectangle((int)(dx-6), (int)(dy-4), 12, 8, Color{70,80,95,255});
        DrawRectangle((int)(dx-5), (int)(dy-3), 10, 3, Color{120,135,150,255});
        // helices
        DrawLineEx({dx-9, dy-5}, {dx-3, dy-5}, 1.5f, Color{160,170,185,255});
        DrawLineEx({dx+3, dy-5}, {dx+9, dy-5}, 1.5f, Color{160,170,185,255});
        // led piscante
        float blink = (std::sin((float)GetTime()*6.0f) > 0.0f) ? 1.0f : 0.3f;
        DrawCircleV({dx, dy}, 2.0f, ColorAlpha(Color{255,40,40,255}, blink));
        // feixe de luz apontando para o chao
        BeginBlendMode(BLEND_ADDITIVE);
        DrawTriangle({dx-3, dy+3}, {dx+3, dy+3}, {dx, dy+22},
                     ColorAlpha(Color{0,200,255,255}, 0.18f));
        EndBlendMode();
    }
}

// ── Corpos por classe (formatos distintos, reconheciveis) ────────────────────

void Player::renderGuerreira(float px, float py, float f, float legL, float legR) const {
    Color body = classPrimary, bodyLt = classSecondary, skin = classSkin;
    Color hair = {40,25,20,255}, ac = accentNow(), trim = classTrim;
    // Pernas esguias com botas
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
    // Bracos
    DrawRectangle((int)(px-12),(int)(py-10),5,16,skin);
    DrawRectangle((int)(px+7),(int)(py-10),5,16,skin);
    // Cabeca + rabo de cavalo
    DrawCircleV({px,py-20},8,skin);
    DrawRectangle((int)(px-8),(int)(py-28),16,7,hair);          // franja
    DrawRectangle((int)(px+6),(int)(py-26),5,18,hair);          // rabo de cavalo
    DrawCircleV({px-3,py-20},1.6f,ac); DrawCircleV({px+3,py-20},1.6f,ac); // olhos
    // Duas pistolas (uma em cada mao)
    DrawRectangle((int)(px+8),(int)(py-2),10,4,trim);
    DrawRectangle((int)(px-18),(int)(py+2),10,4,trim);
    DrawGlowCircle({px+20,py},3.0f,ac,3.0f);
    DrawGlowCircle({px-20,py+4},2.5f,ac,2.5f);
}

void Player::renderRobo(float px, float py, float f, float legL, float legR) const {
    Color metal = classPrimary, metalLt = classSecondary, ac = accentNow(), trim = classTrim;
    // Pernas pesadas/hidraulicas
    DrawRectangle((int)(px-11+legL*0.5f),(int)(py+12),9,18,metal);
    DrawRectangle((int)(px+2+legR*0.5f),(int)(py+12),9,18,metal);
    DrawRectangle((int)(px-13),(int)(py+28),11,4,{20,20,25,255});
    DrawRectangle((int)(px+2),(int)(py+28),11,4,{20,20,25,255});
    // Corpo retangular metalico (chassi)
    DrawRectangle((int)(px-13),(int)(py-16),26,28,metal);
    DrawRectangle((int)(px-11),(int)(py-14),22,24,metalLt);
    // Paineis + parafusos
    DrawRectangleLinesEx({px-11,py-14,22,24},1.0f,ColorAlpha(trim,0.5f));
    DrawCircleV({px-8,py-11},1.5f,trim); DrawCircleV({px+8,py-11},1.5f,trim);
    DrawCircleV({px-8,py+8},1.5f,trim);  DrawCircleV({px+8,py+8},1.5f,trim);
    // Reator central
    DrawGlowCircle({px,py-2},6.0f,ac,5.0f); DrawCircleV({px,py-2},3.0f,ac);
    // Bracos blocados
    DrawRectangle((int)(px-20),(int)(py-14),7,20,metal);
    DrawRectangle((int)(px+13),(int)(py-14),7,20,metal);
    DrawRectangle((int)(px-21),(int)(py+5),9,7,trim);   // garra/canhao
    DrawRectangle((int)(px+12),(int)(py+5),9,7,trim);
    // Cabeca caixa com visor unico
    DrawRectangle((int)(px-9),(int)(py-32),18,16,metalLt);
    DrawRectangle((int)(px-7),(int)(py-28),14,5,{10,10,15,255});
    DrawGlowLine({px-6,py-25},{px+6,py-25},4.0f,ac);    // visor varrendo
    // Antena
    DrawRectangle((int)(px-1),(int)(py-40),2,8,trim);
    DrawGlowCircle({px,py-41},2.5f,ac,3.0f);
    // Arma pesada
    DrawRectangle((int)(px+f*14),(int)(py-4),(int)(f*16),8,trim);
    DrawGlowCircle({px+f*32,py},4.0f,ac,4.0f);
}

void Player::renderMago(float px, float py, float f, float legL, float legR) const {
    Color robe = classPrimary, robeLt = classSecondary, ac = accentNow(), skin = classSkin, trim = classTrim;
    // Tunica longa (saia triangular) — esconde as pernas, balanca um pouco
    float sway = (legL - legR) * 0.2f;
    DrawTriangle({px - 16 + sway, py + 30}, {px + 16 + sway, py + 30}, {px, py - 6}, robe);
    DrawTriangle({px - 11 + sway, py + 28}, {px + 11 + sway, py + 28}, {px, py + 2}, robeLt);
    // Barra inferior decorada
    DrawRectangle((int)(px-16+sway),(int)(py+27),32,3,trim);
    // Tronco
    DrawRectangle((int)(px-8),(int)(py-12),16,16,robe);
    // Bracos nas mangas
    DrawRectangle((int)(px-13),(int)(py-10),6,14,robeLt);
    DrawRectangle((int)(px+7),(int)(py-10),6,14,robeLt);
    // Capuz + rosto sombreado + barba
    DrawTriangle({px-10,py-16},{px+10,py-16},{px,py-40},robe);
    DrawCircleV({px,py-22},7,{30,25,45,255});       // sombra do capuz
    DrawCircleV({px-3,py-22},1.6f,ac); DrawCircleV({px+3,py-22},1.6f,ac);
    DrawTriangle({px-5,py-17},{px+5,py-17},{px,py-8},{230,230,235,255}); // barba branca
    // Cajado com cristal brilhante
    float stx = px + f*13;
    DrawRectangle((int)(stx),(int)(py-26),3,46,{90,60,30,255});
    DrawGlowCircle({stx+1.5f,py-30},6.0f,ac,7.0f);
    DrawCircleV({stx+1.5f,py-30},3.0f,ac);
}

void Player::renderBruxa(float px, float py, float f, float legL, float legR) const {
    Color dress = classPrimary, dressLt = classSecondary, ac = accentNow(), skin = classSkin, trim = classTrim;
    // Vestido (saia) com leve balanco
    float sway = (legL - legR) * 0.25f;
    DrawTriangle({px - 14 + sway, py + 28}, {px + 14 + sway, py + 28}, {px, py - 2}, dress);
    DrawTriangle({px - 9 + sway, py + 26}, {px + 9 + sway, py + 26}, {px, py + 4}, dressLt);
    // Tronco + cinto
    DrawRectangle((int)(px-7),(int)(py-12),14,14,dress);
    DrawRectangle((int)(px-7),(int)(py-1),14,3,trim);
    // Bracos
    DrawRectangle((int)(px-11),(int)(py-10),5,13,dressLt);
    DrawRectangle((int)(px+6),(int)(py-10),5,13,dressLt);
    // Cabeca
    DrawCircleV({px,py-19},7,skin);
    DrawCircleV({px-2.5f,py-19},1.5f,ac); DrawCircleV({px+2.5f,py-19},1.5f,ac);
    // Cabelo lateral
    DrawRectangle((int)(px-7),(int)(py-20),3,12,{30,20,30,255});
    DrawRectangle((int)(px+4),(int)(py-20),3,12,{30,20,30,255});
    // CHAPEU PONTUDO
    DrawTriangle({px-12,py-24},{px+12,py-24},{px-2,py-48},dress);  // cone (inclinado)
    DrawEllipse((int)px,(int)(py-24),15.0f,4.0f,{20,10,30,255});   // aba
    DrawRectangle((int)(px-13),(int)(py-26),26,3,trim);            // fita
    DrawGlowCircle({px-2,py-46},2.5f,ac,3.0f);                     // ponta brilhante
    // Capa esvoacante atras
    DrawTriangle({px-8,py-12},{px-18-sway,py+22},{px-4,py+16},ColorAlpha(dress,0.8f));
    // Varinha com brilho
    float wx = px + f*12;
    DrawRectangle((int)(wx),(int)(py-6),(int)(f*12),2,{120,90,50,255});
    DrawGlowCircle({wx+f*13,py-5},5.0f,ac,6.0f);
}

void Player::renderHomemFera(float px, float py, float f, float legL, float legR) const {
    Color fur = classPrimary, furLt = classSecondary, ac = accentNow(), trim = classTrim;
    Color claw = {235,235,240,255};
    // Pernas digitígradas musculosas
    DrawRectangle((int)(px-10+legL),(int)(py+10),8,16,fur);
    DrawRectangle((int)(px+2+legR),(int)(py+10),8,16,fur);
    DrawRectangle((int)(px-11+legL),(int)(py+24),9,6,furLt); // patas
    DrawRectangle((int)(px+2+legR),(int)(py+24),9,6,furLt);
    // Garras dos pes
    for (int i=0;i<3;i++){ DrawLineEx({px-9+legL+i*3,py+30},{px-10+legL+i*3,py+33},1.5f,claw); }
    // Tronco musculoso largo (postura curvada)
    DrawRectangle((int)(px-13),(int)(py-10),26,22,fur);
    DrawRectangle((int)(px-10),(int)(py-8),20,14,furLt);
    DrawLineEx({px,py-8},{px,py+8},1.5f,ColorAlpha(trim,0.6f)); // peitoral
    // Bracos grossos
    DrawRectangle((int)(px-19),(int)(py-10),7,18,fur);
    DrawRectangle((int)(px+12),(int)(py-10),7,18,fur);
    // GARRAS nas maos
    for (int i=0;i<3;i++){
        DrawLineEx({px-18+i*3.0f,py+8},{px-20+i*3.0f,py+14},2.0f,claw);
        DrawLineEx({px+13+i*3.0f,py+8},{px+15+i*3.0f,py+14},2.0f,claw);
    }
    // Cabeca de fera (focinho + orelhas)
    DrawCircleV({px,py-20},9,fur);
    DrawTriangle({px-9,py-24},{px-3,py-24},{px-7,py-34},fur);  // orelha esq
    DrawTriangle({px+3,py-24},{px+9,py-24},{px+7,py-34},fur);  // orelha dir
    DrawTriangle({px-2,py-18},{px+8,py-19},{px+2,py-13},furLt); // focinho
    DrawCircleV({px+6,py-17},1.5f,{15,10,8,255});               // nariz
    // Olhos ferozes brilhando
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

void Player::drawInventory() const {
    const int SW = 1280, SH = 720;
    const int PX = 60, PY = 50, PW = SW - 120, PH = SH - 100;
    Color C_cyan = {0,210,255,255};
    Color C_gold = {255,190,0,255};
    Color C_green= {0,210,80,255};

    // Dark overlay background
    DrawRectangle(0, 0, SW, SH, ColorAlpha(BLACK, 0.80f));

    // Main panel
    DrawRectangle(PX, PY, PW, PH, ColorAlpha({8,10,22,255}, 0.96f));
    // Cyberpunk border
    int c = 10;
    Color brd = C_cyan;
    DrawLine(PX+c, PY,   PX+PW-c, PY,   ColorAlpha(brd, 0.8f));
    DrawLine(PX, PY+c,   PX, PY+PH-c,   ColorAlpha(brd, 0.6f));
    DrawLine(PX+c, PY+PH,PX+PW-c, PY+PH,ColorAlpha(brd, 0.8f));
    DrawLine(PX+PW, PY+c,PX+PW, PY+PH-c,ColorAlpha(brd, 0.6f));
    DrawLine(PX, PY+c, PX+c, PY, ColorAlpha(brd, 0.7f));
    DrawLine(PX+PW-c, PY, PX+PW, PY+c, ColorAlpha(brd, 0.7f));
    DrawLine(PX, PY+PH-c, PX+c, PY+PH, ColorAlpha(brd, 0.7f));
    DrawLine(PX+PW-c, PY+PH, PX+PW, PY+PH-c, ColorAlpha(brd, 0.7f));

    // Title bar
    DrawRectangle(PX, PY, PW, 30, ColorAlpha({0,180,220,255}, 0.12f));
    DrawText("INVENTARIO  /  BUILD", PX+14, PY+8, 18, C_cyan);
    DrawText("[I] FECHAR  [1/2/3] SLOT  [U] UPGRADE  [Q/E] ITEM  [F] USAR  [R] FUNDIR  [SETAS/T] EQUIPAR",
             PX + 200, PY+9, 12, ColorAlpha(WHITE, 0.55f));

    // â"€â"€ LEFT COLUMN: Character stats â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    int lx = PX + 14, ly = PY + 44;
    DrawText("PERSONAGEM", lx, ly, 14, ColorAlpha(C_gold, 0.9f));
    DrawLine(lx, ly+18, lx+220, ly+18, ColorAlpha(C_cyan, 0.3f));
    ly += 24;

    auto statRow = [&](const char* label, const char* val, Color col) {
        DrawText(label, lx, ly, 13, ColorAlpha(WHITE, 0.6f));
        DrawText(val, lx + 110, ly, 13, col);
        ly += 18;
    };
    statRow("NIVEL",     TextFormat("%d",        level),                 C_gold);
    statRow("HP",        TextFormat("%.0f / %.0f", health, maxHealth),   C_green);
    statRow("DANO",      TextFormat("%.0f",       attackDamage),         {255,100,80,255});
    statRow("ALCANCE",   TextFormat("%.0f",       attackRange),          {180,140,255,255});
    statRow("DEFESA",    TextFormat("%.0f%%",     defense),              {100,200,255,255});
    statRow("VELOC.",    TextFormat("%.0f",       speed),                {255,200,80,255});
    statRow("XP MULT",   TextFormat("x%.1f",     xpMultiplier),         {0,255,180,255});
    statRow("CREDITOS",  TextFormat("$ %d",       credits),              C_gold);

    ly += 6;
    DrawLine(lx, ly, lx+220, ly, ColorAlpha(C_cyan, 0.2f));
    ly += 8;
    DrawText("SKILLS", lx, ly, 13, ColorAlpha(C_gold, 0.8f));
    ly += 18;
    for (const auto& sk : skills) {
        bool rdy = sk.isReady();
        Color sc = rdy ? C_green : ColorAlpha(WHITE, 0.4f);
        DrawText(TextFormat("%s", sk.name.c_str()),    lx,      ly, 12, sc);
        DrawText(TextFormat("CD:%.1fs Dano:%.0f", sk.cooldown, sk.damage),
                 lx + 90, ly, 11, ColorAlpha(sc, 0.7f));
        ly += 15;
    }

    // â"€â"€ CENTER COLUMN: Equipped slots â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    int cx = PX + 250, cy = PY + 44;
    int slotW = 330, slotH = 80;
    DrawText("EQUIPAMENTOS", cx, cy, 14, ColorAlpha(C_gold, 0.9f));
    DrawLine(cx, cy+18, cx+slotW, cy+18, ColorAlpha(C_cyan, 0.3f));
    cy += 26;

    const Equipment* slots[3] = {&equippedWeapon, &equippedArmor, &equippedImplant};
    const char* slotNames[3]  = {"[1] ARMA", "[2] ARMADURA", "[3] IMPLANTE"};
    const char* slotKeys[3]   = {"DMG", "HP", "VEL"};
    const char* slot2Keys[3]  = {"Alc", "Def%", "XPx"};
    Color slotColors[3]       = {{255,100,80,255}, {100,200,255,255}, {255,200,80,255}};

    for (int s = 0; s < 3; ++s) {
        bool selected = (selectedEquipSlot == s);
        Color bg = selected ? ColorAlpha(slotColors[s], 0.16f) : ColorAlpha(BLACK, 0.3f);
        Color bd = selected ? slotColors[s] : ColorAlpha(C_cyan, 0.35f);
        DrawRectangle(cx, cy, slotW, slotH, bg);
        DrawRectangleLinesEx({(float)cx,(float)cy,(float)slotW,(float)slotH},
                              selected ? 2.0f : 1.0f, bd);

        // Slot label
        DrawText(slotNames[s], cx+8, cy+6, 13, ColorAlpha(bd, 0.9f));

        const Equipment& eq = *slots[s];
        if (eq.isEmpty()) {
            DrawText("--- vazio ---", cx+10, cy+26, 14, ColorAlpha(DARKGRAY, 0.8f));
        } else {
            // Name + upgrade stars
            DrawText(eq.name.c_str(), cx+10, cy+26, 15, eq.color);
            // Stars
            for (int st = 0; st < 3; ++st) {
                Color starC = (st < eq.upgradeLevel) ? C_gold : ColorAlpha(DARKGRAY, 0.5f);
                DrawText("â˜…", cx + 10 + st*16 + MeasureText(eq.name.c_str(),15) + 8,
                         cy+26, 13, starC);
            }
            // Tier badge
            DrawText(TextFormat("T%d", eq.tier), cx+slotW-34, cy+6, 11,
                     ColorAlpha(C_gold, 0.8f));

            // Stats
            float ep = eq.getEffectivePrimary();
            float es = eq.getEffectiveSecondary();
            DrawText(TextFormat("%s: %.0f", slotKeys[s],  ep), cx+10, cy+46, 12, ColorAlpha(WHITE,0.8f));
            if (eq.upgradeLevel > 0 && eq.primary > 0)
                DrawText(TextFormat("(base %.0f)", eq.primary), cx+90, cy+46, 10, ColorAlpha(WHITE,0.45f));
            DrawText(TextFormat("%s: %.1f", slot2Keys[s], es), cx+180, cy+46, 12, ColorAlpha(WHITE,0.8f));

            // Upgrade cost / max
            if (eq.canUpgrade()) {
                DrawText(TextFormat("[U] Upgrade: $%d", eq.upgradeCost()),
                         cx+10, cy+62, 11, selected ? C_gold : ColorAlpha(C_gold,0.5f));
            } else if (eq.upgradeLevel >= 3) {
                DrawText("MAXIMO ALCANCADO", cx+10, cy+62, 11, ColorAlpha(C_gold,0.7f));
            }
        }
        cy += slotH + 10;
    }

    // ── CENTER COLUMN (cont.): Equipamentos guardados (mochila de gear) ────────
    {
        int gx = PX + 250, gy = cy + 4;
        DrawText("EQUIPAMENTOS GUARDADOS", gx, gy, 14, ColorAlpha(C_gold, 0.9f));
        DrawText("[Setas] navegar  [T] equipar", gx + 200, gy + 2, 11, ColorAlpha(WHITE, 0.5f));
        DrawLine(gx, gy+18, gx+slotW, gy+18, ColorAlpha(C_cyan, 0.3f));
        gy += 24;

        if (equipBag.empty()) {
            DrawText("Vazio. Itens trocados ficam guardados aqui.",
                     gx+6, gy+6, 12, ColorAlpha(WHITE, 0.4f));
        } else {
            // Janela rolavel centrada no item selecionado
            int rowH = 40, maxRows = 4;
            int start = std::max(0, selectedBagEquip - maxRows/2);
            if (start + maxRows > (int)equipBag.size())
                start = std::max(0, (int)equipBag.size() - maxRows);
            for (int i = start; i < (int)equipBag.size() && i < start + maxRows; ++i) {
                const Equipment& e = equipBag[i];
                bool sel = (i == selectedBagEquip);
                int ey = gy + (i - start) * (rowH + 4);
                Color bd = sel ? e.color : ColorAlpha(e.color, 0.5f);
                DrawRectangle(gx, ey, slotW, rowH, sel ? ColorAlpha(e.color,0.18f) : ColorAlpha(BLACK,0.35f));
                DrawRectangleLinesEx({(float)gx,(float)ey,(float)slotW,(float)rowH}, sel?2.0f:1.0f, bd);
                // tipo de slot
                const char* sn = (e.slot==EquipSlot::Weapon)?"ARMA":
                                 (e.slot==EquipSlot::Armor)?"ARMADURA":"IMPLANTE";
                DrawText(sn, gx+6, ey+4, 10, ColorAlpha(C_cyan, 0.8f));
                DrawText(e.name.c_str(), gx+6, ey+18, 14, e.color);
                // estrelas de upgrade + tier
                for (int st = 0; st < 3; ++st) {
                    Color sc = (st < e.upgradeLevel) ? C_gold : ColorAlpha(DARKGRAY,0.5f);
                    DrawText("*", gx+slotW-90+st*10, ey+4, 12, sc);
                }
                DrawText(TextFormat("T%d", e.tier), gx+slotW-34, ey+4, 11, ColorAlpha(C_gold,0.8f));
                // poderes / atributos
                const char* p1 = (e.slot==EquipSlot::Weapon)?"DMG":(e.slot==EquipSlot::Armor)?"HP":"VEL";
                const char* p2 = (e.slot==EquipSlot::Weapon)?"Alc":(e.slot==EquipSlot::Armor)?"Def%":"XPx";
                DrawText(TextFormat("%s +%.0f   %s +%.1f", p1, e.getEffectivePrimary(),
                         p2, e.getEffectiveSecondary()),
                         gx+slotW-200, ey+20, 12, ColorAlpha(WHITE, 0.85f));
            }
            if ((int)equipBag.size() > maxRows)
                DrawText(TextFormat("%d/%d", selectedBagEquip+1, (int)equipBag.size()),
                         gx+slotW-44, gy + maxRows*(rowH+4), 11, ColorAlpha(WHITE,0.5f));
        }
    }

    // â"€â"€ RIGHT COLUMN: Item bag â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    int rx = PX + 610, ry = PY + 44;
    int itemCols = 2;
    DrawText("BOLSA DE ITENS", rx, ry, 14, ColorAlpha(C_gold, 0.9f));
    DrawLine(rx, ry+18, rx+560, ry+18, ColorAlpha(C_cyan, 0.3f));
    ry += 26;

    if (inventory.empty()) {
        DrawText("Nenhum item coletado ainda.", rx+8, ry+10, 13, ColorAlpha(WHITE,0.4f));
        DrawText("Derrote inimigos para encontrar itens!", rx+8, ry+28, 12, ColorAlpha(WHITE,0.35f));
    } else {
        // Count items per type for fusion hints
        int typeCounts[8] = {};
        for (const auto& it : inventory) {
            int t = (int)it.type;
            if (t >= 0 && t < 8) typeCounts[t]++;
        }

        int col = 0;
        int cellW = 270, cellH = 50;
        for (int i = 0; i < (int)inventory.size() && i < 16; ++i) {
            const Item& it = inventory[i];
            int ix = rx + col * (cellW + 8);
            int iy = ry + (i / itemCols) * (cellH + 4);
            bool selected = (i == selectedInvItem);

            Color bgCol = selected ? ColorAlpha(it.color, 0.20f) : ColorAlpha(BLACK, 0.45f);
            float bdW   = selected ? 2.0f : 1.0f;
            Color bdCol = selected ? it.color : ColorAlpha(it.color, 0.55f);

            DrawRectangle(ix, iy, cellW, cellH, bgCol);
            DrawRectangleLinesEx({(float)ix,(float)iy,(float)cellW,(float)cellH}, bdW, bdCol);
            DrawCircleV({(float)(ix+22),(float)(iy+25)}, 10.0f, ColorAlpha(it.color, 0.5f));
            DrawCircleV({(float)(ix+22),(float)(iy+25)}, 6.0f, it.color);
            DrawText(it.name.c_str(), ix+40, iy+6, 13, it.color);

            const char* hint = "";
            switch (it.type) {
                case ItemType::HealthPack:  hint = "[F] Usar: +30 HP"; break;
                case ItemType::NanoCore:    hint = "[F] Usar: +25 MaxHP permanente"; break;
                case ItemType::TechChip:    hint = "[F] Usar: +50 XP"; break;
                case ItemType::PlasmaCell:  hint = "[F] Usar: Zerar CDs"; break;
                case ItemType::EnergyCore:  hint = "[F] Usar: Escudo 2.5s"; break;
                case ItemType::ScrapMetal:  hint = "[F] Usar: +8 HP / +$8"; break;
                case ItemType::WeaponPart:  hint = "[F] Usar: +$20"; break;
                default: hint = ""; break;
            }
            DrawText(hint, ix+40, iy+24, 11, ColorAlpha(WHITE, selected ? 0.9f : 0.55f));

            // Fusion counter badge
            int tc = typeCounts[(int)it.type];
            if (tc >= 2) {
                Color fusCol = (tc >= 3) ? C_gold : ColorAlpha(C_gold,0.5f);
                DrawText(TextFormat("x%d", tc), ix+cellW-26, iy+6, 11, fusCol);
                if (tc >= 3)
                    DrawText("FUNDIR!", ix+cellW-46, iy+34, 9, ColorAlpha(C_gold,0.85f));
            }

            col = 1 - col;
        }
        if ((int)inventory.size() > 16) {
            DrawText(TextFormat("...+%d mais", (int)inventory.size()-16),
                     rx+8, ry + 8*(cellH+4) + 4, 12, ColorAlpha(WHITE,0.4f));
        }

        // Fusion panel at bottom of item column
        int fusY = ry + 8*(cellH+4) + 18;
        DrawLine(rx, fusY, rx+560, fusY, ColorAlpha(C_gold,0.25f));
        fusY += 6;
        DrawText("[R] FUNDIR 3x mesmo tipo", rx, fusY, 12, ColorAlpha(C_gold,0.7f));
        DrawText("=> efeito duplicado + bonus permanente", rx+220, fusY, 11, ColorAlpha(WHITE,0.45f));
    }

    // â"€â"€ Bottom help bar â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€â"€
    int bby = PY + PH - 28;
    DrawRectangle(PX, bby, PW, 28, ColorAlpha(BLACK, 0.6f));
    DrawLine(PX, bby, PX+PW, bby, ColorAlpha(C_cyan, 0.3f));
    DrawText("DICA: Colete NanoCore para aumentar MaxHP  |  TechChip = XP bonus  |  PlasmaCell = Skills mais rapidas",
             PX+12, bby+8, 12, ColorAlpha(WHITE, 0.45f));
}

void Player::drawEquipment() const {
    // Legacy â€" full inventory now shown via drawInventory
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
            // Remove 3 of this type
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

    // Mochila de equipamentos: setas navegam, T equipa o selecionado
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

// Poção/poder de cura (estilo Diablo 3): cura ~35% da vida na hora e ativa uma
// regeneração por alguns segundos. Cooldown impede spam.
void Player::usePotion() {
    if (healCooldown > 0.0f) return;
    if (health <= 0.0f) return;
    heal(maxHealth * 0.35f);
    regenTimer   = 4.0f;     // regen buff
    healCooldown = 14.0f;    // cooldown como o globo/poção do D3
}

void Player::increaseBaseMaxHP(float amount) {
    baseMaxHealth += amount;
    applyEquipmentStats();
}

void Player::takeDamage(float amount) {
    if (isShielded()) return;
    float reduced = amount * (1.0f - defense / 100.0f);
    health -= reduced;
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
    // Curva mais ingreme = progressao mais lenta e merecida (pedido do usuario).
    // base 450, expoente 2.05 -> Lv1→2 ~450, Lv5→6 ~12k, Lv10→11 ~50k, Lv20→21 ~210k.
    xpToNextLevel = (int)(450.0f * powf((float)level, 2.05f));
    // Ganhos por nivel: HP e dano fortes (progressao perceptivel); alcance e
    // velocidade crescem devagar para nao virar bola de neve injusta.
    baseMaxHealth    += 26.0f;
    baseAttackDamage += 8.0f;
    baseAttackRange  += 2.5f;
    baseSpeed        += 2.0f;

    // Passive milestones — spaced to match longer XP curve
    lastPassive.clear();
    switch (level) {
        case 5:  baseMaxHealth    += 40.0f;
                 lastPassive = "PASSIVA: Blindagem de Campo +40 HP"; break;
        case 8:  baseAttackDamage += 15.0f;
                 lastPassive = "PASSIVA: Calibracao de Combate +15 Dano"; break;
        case 10: baseAttackRange  += 30.0f; baseSpeed += 15.0f;
                 lastPassive = "PASSIVA: Amplificador Neural +30 Alcance +15 Vel"; break;
        case 15: baseMaxHealth    += 80.0f; baseAttackDamage += 20.0f;
                 lastPassive = "PASSIVA: Protocolo IRON-VIII +80 HP +20 Dano"; break;
        case 20: baseAttackDamage += 30.0f; baseMaxHealth += 60.0f;
                 lastPassive = "PASSIVA: Sobrecarga Neural +30 Dano +60 HP"; break;
        case 25: baseAttackDamage += 40.0f; baseMaxHealth += 100.0f; baseSpeed += 20.0f;
                 lastPassive = "PASSIVA: EXECUTOR LENDARIO +40 Dano +100 HP +20 Vel"; break;
        case 30: baseAttackDamage += 60.0f; baseMaxHealth += 150.0f;
                 lastPassive = "PASSIVA: ASCENSAO OMEGA +60 Dano +150 HP"; break;
        case 40: baseMaxHealth    += 250.0f; baseAttackDamage += 80.0f; baseSpeed += 30.0f;
                 lastPassive = "PASSIVA: TRANSCENDENCIA KRONOS +250 HP +80 Dano +30 Vel"; break;
        default: break;
    }

    // Skill cooldown reduction at evolution levels: 10, 25, 40, 60
    static const int CD_LEVELS[] = {10, 25, 40, 60};
    for (int cl : CD_LEVELS) {
        if (level == cl) {
            for (auto& sk : skills) sk.cooldown *= 0.88f;
            break;
        }
    }

    applyEquipmentStats();
    health = maxHealth;
    leveledUp    = true;
    levelUpTimer = 2.5f;

    // Level-up speech
    if (level % 10 == 0)
        say("Level " + std::to_string(level) + "! Poder maximo se aproxima.", 3.5f, {255,200,0,255});
    else if (level % 5 == 0)
        say("Level " + std::to_string(level) + "! Ficando mais forte.", 2.5f, {0,220,255,255});
}

void Player::equipItem(const Equipment& equip) {
    // Guarda o equipamento atual do slot na mochila (nao descarta — estilo Diablo).
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
            if (equipBag.size() > 24) equipBag.erase(equipBag.begin()); // teto
        }
        *slot = equip;
    }
    applyEquipmentStats();
}

// Equipa o item idx da mochila; o que estava equipado volta para a mochila.
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
        if (!slot->isEmpty()) equipBag.push_back(*slot); // o antigo volta p/ a bolsa
        *slot = chosen;
    }
    if (selectedBagEquip >= (int)equipBag.size())
        selectedBagEquip = std::max(0, (int)equipBag.size() - 1);
    applyEquipmentStats();
}

void Player::applyEquipmentStats() {
    attackDamage = baseAttackDamage;
    attackRange  = baseAttackRange;
    maxHealth    = baseMaxHealth;
    speed        = baseSpeed;
    defense      = baseDefense;   // defesa base da classe (armadura soma por cima)
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

    // Bonus de evolucao aplicados AQUI (e nao com *= direto), para nao serem
    // apagados a cada level-up/equip. Idempotente e persistente.
    switch (evolutionPath) {
        case EvolutionPath::CyborgSoldier:  attackDamage *= 1.30f; break;       // +30% dano
        case EvolutionPath::HackerFantasma: speed *= 1.40f; xpMultiplier *= 1.15f; break; // +40% vel
        case EvolutionPath::ExecutorOmega:  maxHealth *= 1.50f; attackDamage *= 1.10f; break; // +50% HP
        default: break;
    }
    // Tiers de evolucao extra acumulam um pequeno bonus geral
    if (evolutionTier > 1) {
        float t = 1.0f + (evolutionTier - 1) * 0.06f;
        attackDamage *= t;
        maxHealth    *= t;
    }

    // Limites de seguranca: defesa nao passa de 80% e velocidade tem teto
    if (defense > 80.0f) defense = 80.0f;
    if (speed   > 260.0f) speed  = 260.0f;

    if (health > maxHealth) health = maxHealth;
}

float Player::getEffectiveDamage() const {
    return attackDamage * (isOverloaded() ? 1.5f : 1.0f);
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

    if (killStreak.count == 5)
        say("Cinco seguidos!", 2.5f, {255, 200, 0, 255});
    else if (killStreak.count == 10)
        say("Imparavel!", 3.0f, {255, 120, 0, 255});
    else if (killStreak.count == 20)
        say("LENDARIO!", 3.5f, {255, 50, 0, 255});

    if (totalKills == 1)
        say("Primeiro sangue.", 2.0f);
    else if (totalKills == 100)
        say("100 eliminados. Isso e so o começo.", 3.5f);
    else if (totalKills == 500)
        say("500 kills. Sou uma maquina de guerra.", 3.5f, {255,120,0,255});
}

void Player::onBossFound() {
    say("Aqui esta o chefao...", 3.0f, {255, 60, 0, 255});
}

void Player::onPortalClosed() {
    say("Portal fechado. Um a menos.", 2.5f, {0, 200, 255, 255});
}

void Player::onEnterZone(int zoneId) {
    static const char* zoneSpeeches[] = {
        "Ruinas de Avalon. Territorio KRONOS.",     // 0
        "Bunker Nexus. Tensao no ar.",               // 1
        "Forja KRONOS. Calor intenso.",              // 2
        "Zona desconhecida.",                        // 3
        "Lugar sombrio... almas presas aqui.",       // 4 Cemetery
        "Fazenda maldita. Algo nao esta certo.",     // 5 CursedFarm
        "Cidade fantasma. Silencio mortal.",         // 6 GhostCity
        "Floresta negra. Perigo por toda parte.",    // 7 DarkForest
        "Catacumbas. Profundidade sem fim.",         // 8 Catacombs
        "Mansao abandonada. Ela ainda vive.",        // 9 Manor
        "Calor extremo. Cuidado com a lava.",        // 10 Inferno
    };
    int idx = zoneId;
    if (idx < 0 || idx > 10) idx = 3;
    say(zoneSpeeches[idx], 3.0f, {0, 220, 255, 255});
}


// ── Render 3D low-poly do jogador ───────────────────────────────────────────
// Humanoide montado SO com primitivas arredondadas (sem cubos).
// Mapeamento: X3D = position.x, Z3D = position.y (o "y" 2D vira profundidade),
// e Y e a ALTURA (pes em Y=0, cabeca por volta de Y=46-52). O pulo soma jumpZ.
void Player::render3D() const {
    const float px = position.x;
    const float pz = position.y;
    const float f  = (float)facing;          // -1 ou 1: orienta arma/visor
    const float base = jumpZ;                 // levanta o corpo inteiro no pulo

    // Passada das pernas a partir da animacao de caminhada.
    float swing = isMoving ? std::sin(walkAnimTimer * 9.0f) * 5.0f : 0.0f;

    // Paleta da classe (membros existentes).
    Color skin = classSkin;
    Color ac   = accentNow();
    Color limb = classSecondary;             // pernas/braco
    Color torsoCol = classPrimary;           // tronco
    if (hasCosmeticTint) {                    // tinta cosmetica modula o tronco
        torsoCol.r = (unsigned char)(torsoCol.r * cosmeticTint.r / 255);
        torsoCol.g = (unsigned char)(torsoCol.g * cosmeticTint.g / 255);
        torsoCol.b = (unsigned char)(torsoCol.b * cosmeticTint.b / 255);
    }

    // ── Pernas (capsulas) — pe em Y=0, quadril ~Y=22 ──
    DrawCapsule({px - 5, base + 1,  pz - swing}, {px - 4, base + 22, pz}, 4.0f, 8, 6, limb);
    DrawCapsule({px + 5, base + 1,  pz + swing}, {px + 4, base + 22, pz}, 4.0f, 8, 6, limb);

    // ── Tronco (capsula mais grossa) — quadril ~Y=22 ate ombros ~Y=40 ──
    DrawCapsule({px, base + 22, pz}, {px, base + 40, pz}, 7.5f, 10, 8, torsoCol);

    // ── Bracos (capsulas) — do ombro para baixo, com balanco oposto ──
    DrawCapsule({px - 8, base + 39, pz + swing*0.5f}, {px - 9, base + 24, pz + swing}, 3.2f, 8, 6, limb);
    DrawCapsule({px + 8, base + 39, pz - swing*0.5f}, {px + 9, base + 24, pz - swing}, 3.2f, 8, 6, limb);

    // ── Cabeca (esfera) — pescoco curto + cranio ~Y=46-52 ──
    DrawCapsule({px, base + 40, pz}, {px, base + 44, pz}, 2.6f, 8, 6, skin);
    DrawSphere({px, base + 49, pz}, 5.5f, skin);

    // ── Visor/olho neon (esfera ciano pequena) na frente conforme facing ──
    DrawSphere({px + f * 4.2f, base + 50, pz}, 1.6f, ac);

    // ── Arma (cilindro horizontal saindo da mao na direcao facing) ──
    Color gun = !equippedWeapon.isEmpty() ? equippedWeapon.color : Color{0, 230, 255, 255};
    Vector3 handPos = { px + f * 9.0f, base + 24, pz };
    Vector3 gunTip  = { px + f * 24.0f, base + 25, pz };
    DrawCylinderEx(handPos, gunTip, 1.8f, 1.2f, 8, gun);
    DrawSphere(gunTip, 1.4f, ColorAlpha(ac, 0.9f));  // ponta brilhante
}
