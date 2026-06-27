#include "TutorialSystem.h"
#include <cmath>
#include <algorithm>

void TutorialSystem::init() {
    hints.clear();
    completedCount   = 0;
    currentStep      = TutorialStep::Welcome;
    active           = true;
    fadingIn         = true;
    fadeAlpha        = 0.0f;
    stepTimer        = 0.0f;

    auto add = [&](TutorialStep s, const char* title, const char* text,
                   const char* key, Color kc, float dt = 0.0f,
                   bool arrow = false, Vector2 arrowPos = {0,0}) {
        TutorialHint h;
        h.step        = s;
        h.title       = title;
        h.text        = text;
        h.key         = key;
        h.keyColor    = kc;
        h.displayTime = dt;
        h.completed   = false;
        h.hasArrow    = arrow;
        h.arrowTarget = arrowPos;
        hints.push_back(h);
    };

    add(TutorialStep::Welcome,
        "BEM-VINDO AO DARKNET",
        "O KRONOS tomou o controle. Voce e a resistencia.\nSobreviva, evolua, destrua os portais.",
        "ENTER / E para continuar",
        {0, 220, 255, 255}, 0.0f);

    add(TutorialStep::Movement,
        "MOVIMENTO",
        "Use WASD ou as setas direcionais para se mover.\nEvite inimigos enquanto ganha XP.",
        "W A S D",
        {255, 220, 0, 255}, 0.0f);

    add(TutorialStep::Attack,
        "ATACAR",
        "Clique com o BOTAO ESQUERDO do mouse para atirar.\nSegure para fogo continuo.",
        "BOTAO ESQUERDO",
        {255, 100, 60, 255}, 0.0f);

    add(TutorialStep::UseSkill,
        "HABILIDADES",
        "Pressione 1-6 para usar habilidades especiais.\nCada habilidade tem cooldown proprio.",
        "1  2  3  4  5  6",
        {0, 200, 255, 255}, 0.0f);

    add(TutorialStep::PickupItem,
        "COLETAR ITENS",
        "Inimigos dropam itens ao morrer.\nPasse por cima ou pressione E para coletar.",
        "E  /  Passar por cima",
        {100, 255, 100, 255}, 0.0f);

    add(TutorialStep::TalkToNPC,
        "CONVERSAR COM NPCs",
        "Aproxime-se de personagens com ! acima da cabeca.\nEles dao missoes, itens e lore da historia.",
        "E  perto do NPC",
        {255, 200, 0, 255}, 0.0f);

    add(TutorialStep::OpenShop,
        "LOJA",
        "Pressione TAB a qualquer momento para abrir a loja.\nGaste creditos em equipamentos melhores.",
        "TAB",
        {0, 220, 180, 255}, 0.0f);

    add(TutorialStep::Portal,
        "PORTAIS DE ANOMALIA",
        "Tempestades abrem portais que spawnham inimigos.\nFeche-os atacando o nucleo do portal!",
        "Ataque o portal para fecha-lo",
        {180, 60, 255, 255}, 0.0f);

    add(TutorialStep::LevelUp,
        "EVOLUCAO",
        "Acumule XP eliminando inimigos e completando missoes.\nSuba de nivel para ficar mais forte.",
        "Mata inimigos para ganhar XP",
        {255, 220, 0, 255}, 0.0f);

    add(TutorialStep::Completed,
        "TUTORIAL CONCLUIDO!",
        "+500 XP   +100 CREDITOS\nBoa sorte, operativo. O KRONOS nao ira esperar.",
        "",
        {0, 255, 150, 255}, 4.0f);
}

int TutorialSystem::stepIndex(TutorialStep s) const {
    for (int i = 0; i < (int)hints.size(); i++)
        if (hints[i].step == s) return i;
    return -1;
}

TutorialHint* TutorialSystem::getCurrentHint() {
    int idx = stepIndex(currentStep);
    if (idx < 0 || idx >= (int)hints.size()) return nullptr;
    return &hints[idx];
}

void TutorialSystem::completeStep(TutorialStep step) {
    int idx = stepIndex(step);
    if (idx >= 0 && !hints[idx].completed) {
        hints[idx].completed = true;
        completedCount++;
        fadingIn  = false;
        stepTimer = 0.0f;

        // Avança para próximo passo
        int nextIdx = idx + 1;
        if (nextIdx < (int)hints.size()) {
            currentStep = hints[nextIdx].step;
            fadingIn    = true;
            stepTimer   = 0.0f;
        } else {
            currentStep = TutorialStep::Completed;
            active      = false;
            completeBadgeTimer = 5.0f;
        }
    }
}

void TutorialSystem::skipTutorial() {
    for (auto& h : hints) h.completed = true;
    currentStep        = TutorialStep::Completed;
    active             = false;
    completeBadgeTimer = 4.0f;
}

bool TutorialSystem::isBlockingInput() const {
    return active && currentStep == TutorialStep::Welcome;
}

// ─── Triggers ───────────────────────────────────────────────────────────────
void TutorialSystem::onPlayerMoved()    { if (currentStep == TutorialStep::Movement)  completeStep(TutorialStep::Movement); }
void TutorialSystem::onPlayerAttacked() { if (currentStep == TutorialStep::Attack)    completeStep(TutorialStep::Attack); }
void TutorialSystem::onSkillUsed()      { if (currentStep == TutorialStep::UseSkill)  completeStep(TutorialStep::UseSkill); }
void TutorialSystem::onItemPickedUp()   { if (currentStep == TutorialStep::PickupItem) completeStep(TutorialStep::PickupItem); }
void TutorialSystem::onMapOpened()      { if (currentStep == TutorialStep::CheckMap)  completeStep(TutorialStep::CheckMap); }
void TutorialSystem::onNPCTalked()      { if (currentStep == TutorialStep::TalkToNPC) completeStep(TutorialStep::TalkToNPC); }
void TutorialSystem::onShopOpened()     { if (currentStep == TutorialStep::OpenShop)  completeStep(TutorialStep::OpenShop); }
void TutorialSystem::onLeveledUp()      { if (currentStep == TutorialStep::LevelUp)   completeStep(TutorialStep::LevelUp); }
void TutorialSystem::onPortalFound()    { if (currentStep == TutorialStep::Portal)    completeStep(TutorialStep::Portal); }

// ─── Update ──────────────────────────────────────────────────────────────────
void TutorialSystem::update(float dt) {
    if (completeBadgeTimer > 0) completeBadgeTimer -= dt;

    // Tela de boas vindas: E/Enter completa
    if (active && currentStep == TutorialStep::Welcome) {
        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            completeStep(TutorialStep::Welcome);
        }
    }

    // Auto-completar passos com displayTime (ex: Completed mostra por 4s)
    if (active) {
        TutorialHint* h = getCurrentHint();
        if (h && h->displayTime > 0.0f) {
            stepTimer += dt;
            if (stepTimer >= h->displayTime) {
                completeStep(currentStep);
            }
        }
    }

    // Fade alpha
    if (fadingIn) {
        fadeAlpha = std::min(fadeAlpha + dt * 4.0f, 1.0f);
    } else {
        fadeAlpha = std::max(fadeAlpha - dt * 4.0f, 0.0f);
    }

    // Skip tutorial com ESC
    if (active && IsKeyPressed(KEY_F1)) {
        skipTutorial();
    }
}

// ─── Render ──────────────────────────────────────────────────────────────────
void TutorialSystem::render(int screenW, int screenH) const {
    if (completeBadgeTimer > 0 && currentStep == TutorialStep::Completed) {
        renderCompletionBadge(screenW, screenH);
        return;
    }
    if (!active || fadeAlpha <= 0.01f) return;
    renderCurrentHint(screenW, screenH);
}

void TutorialSystem::renderCurrentHint(int screenW, int screenH) const {
    int idx = -1;
    for (int i = 0; i < (int)hints.size(); i++)
        if (hints[i].step == currentStep) { idx = i; break; }
    if (idx < 0) return;
    const auto& h = hints[idx];

    float alpha = fadeAlpha;

    // Painel Welcome especial: centro da tela
    if (currentStep == TutorialStep::Welcome) {
        int pw = 600, ph = 220;
        int px = (screenW - pw) / 2;
        int py = (screenH - ph) / 2;
        DrawRectangle(0, 0, screenW, screenH, ColorAlpha(BLACK, alpha * 0.7f));
        DrawRectangleRounded({(float)px,(float)py,(float)pw,(float)ph},
                              0.08f, 8, ColorAlpha(Color{5,5,25,255}, alpha * 0.95f));
        DrawRectangleLinesEx({(float)px,(float)py,(float)pw,(float)ph},
                             2.5f, ColorAlpha(Color{0,200,255,255}, alpha));

        // Titulo
        int tw = MeasureText(h.title.c_str(), 24);
        DrawText(h.title.c_str(), px+(pw-tw)/2, py+20, 24, ColorAlpha(Color{0,220,255,255}, alpha));

        // Linha
        DrawLine(px+20, py+52, px+pw-20, py+52, ColorAlpha(Color{0,180,255,100}, alpha));

        // Texto
        DrawText(h.text.c_str(), px+30, py+62, 14, ColorAlpha(WHITE, alpha));

        // Logotipo DARKNET
        int lw = MeasureText("DARKNET", 36);
        DrawText("DARKNET", px+(pw-lw)/2, py+100, 36, ColorAlpha(Color{0,200,255,255}, alpha));

        // Key prompt piscando
        bool blink = (int)(GetTime() * 2) % 2 == 0;
        if (blink) {
            int kw = MeasureText(h.key.c_str(), 14);
            DrawText(h.key.c_str(), px+(pw-kw)/2, py+ph-30, 14,
                     ColorAlpha(Color{0,200,255,255}, alpha));
        }

        // Skip
        DrawText("[F1] Pular Tutorial", px+10, py+ph-18, 10, ColorAlpha(GRAY, alpha * 0.6f));
        return;
    }

    // Painel normal: canto inferior esquerdo
    int pw = 340, ph = 120;
    int px = 10, py = screenH - ph - 10;

    DrawRectangleRounded({(float)px,(float)py,(float)pw,(float)ph},
                          0.08f, 6, ColorAlpha(Color{5,10,25,255}, alpha * 0.92f));
    DrawRectangleLinesEx({(float)px,(float)py,(float)pw,(float)ph},
                         1.5f, ColorAlpha(Color{0,180,255,255}, alpha * 0.8f));

    // Progresso
    int total = (int)hints.size() - 1; // exclude Welcome
    int done  = completedCount;
    std::string prog = std::to_string(done) + "/" + std::to_string(total);
    DrawText(prog.c_str(), px+pw-40, py+6, 10, ColorAlpha(GRAY, alpha));

    // Titulo
    DrawText(h.title.c_str(), px+10, py+8, 14, ColorAlpha(h.keyColor, alpha));
    DrawLine(px+8, py+26, px+pw-8, py+26, ColorAlpha(h.keyColor, alpha * 0.3f));

    // Texto
    DrawText(h.text.c_str(), px+10, py+32, 12, ColorAlpha(WHITE, alpha * 0.9f));

    // Key badge
    if (!h.key.empty()) {
        int kw = MeasureText(h.key.c_str(), 12) + 16;
        int kx = px+10, ky = py+ph-28;
        DrawRectangle(kx, ky, kw, 20, ColorAlpha(h.keyColor, alpha * 0.25f));
        DrawRectangleLinesEx({(float)kx,(float)ky,(float)kw,20.0f}, 1.0f,
                              ColorAlpha(h.keyColor, alpha * 0.7f));
        DrawText(h.key.c_str(), kx+8, ky+4, 12, ColorAlpha(h.keyColor, alpha));
    }

    // Skip
    DrawText("[F1] Pular", px+pw-70, py+ph-16, 9, ColorAlpha(GRAY, alpha * 0.5f));

    // Seta animada
    if (h.hasArrow && (h.arrowTarget.x != 0 || h.arrowTarget.y != 0)) {
        renderArrow(h.arrowTarget, screenW, screenH);
    }
}

void TutorialSystem::renderArrow(Vector2 target, int screenW, int screenH) const {
    // Seta pulsante apontando para target (screen-space)
    float pulse = sinf((float)GetTime() * 4.0f) * 6.0f;
    float ax = target.x, ay = target.y - 40.0f - pulse;

    // Triangulo seta
    Vector2 p1 = {ax,      ay};
    Vector2 p2 = {ax-12,   ay-20};
    Vector2 p3 = {ax+12,   ay-20};
    DrawTriangle(p1, p3, p2, ColorAlpha(Color{0,220,255,255}, fadeAlpha * 0.9f));
    DrawTriangleLines(p1, p3, p2, ColorAlpha(WHITE, fadeAlpha * 0.6f));
}

void TutorialSystem::renderCompletionBadge(int screenW, int screenH) const {
    float ratio = completeBadgeTimer / 5.0f;
    float alpha = ratio > 0.8f ? (1.0f - ratio) * 5.0f : (ratio < 0.2f ? ratio * 5.0f : 1.0f);
    alpha = std::max(0.0f, std::min(1.0f, alpha));

    int pw = 480, ph = 100;
    int px = (screenW - pw) / 2;
    int py = screenH / 2 - 80;

    DrawRectangleRounded({(float)px,(float)py,(float)pw,(float)ph},
                          0.1f, 8, ColorAlpha(Color{5,25,5,255}, alpha * 0.95f));
    DrawRectangleLinesEx({(float)px,(float)py,(float)pw,(float)ph},
                         2.0f, ColorAlpha(Color{0,255,120,255}, alpha));

    const char* title = "TUTORIAL CONCLUIDO!";
    int tw = MeasureText(title, 22);
    DrawText(title, px+(pw-tw)/2, py+12, 22, ColorAlpha(Color{0,255,150,255}, alpha));

    const char* reward = "+500 XP   +100 CREDITOS";
    int rw = MeasureText(reward, 16);
    DrawText(reward, px+(pw-rw)/2, py+44, 16, ColorAlpha(Color{255,220,0,255}, alpha));

    const char* sub = "Boa sorte, operativo.";
    int sw = MeasureText(sub, 13);
    DrawText(sub, px+(pw-sw)/2, py+68, 13, ColorAlpha(WHITE, alpha * 0.7f));
}
