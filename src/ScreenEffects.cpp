#include "ScreenEffects.h"
#include <cmath>
#include <algorithm>

// ─── Update ──────────────────────────────────────────────────────────────────

void ScreenEffectsSystem::update(float dt) {
    // Screen shake — trauma-based: a intensidade decai com t^2 (suave, organico)
    if (shake.timer > 0.0f) {
        shake.timer -= dt;
        float t      = shake.timer / shake.duration;
        float amount = shake.intensity * t * t;     // falloff quadratico
        shakeOffset.x = (float)GetRandomValue(-100, 100) / 100.0f * amount;
        shakeOffset.y = (float)GetRandomValue(-100, 100) / 100.0f * amount;
    } else {
        shakeOffset = {0.0f, 0.0f};
    }

    // Flash
    if (flash.timer > 0.0f) flash.timer -= dt;

    // Vignette
    if (vignette.pulse) {
        // oscillating vignette for low-HP
        vignette.timer += dt;
    } else {
        if (vignette.timer > 0.0f) vignette.timer -= dt;
    }

    // Boss warning
    if (bossWarning) {
        bossWarningTimer -= dt;
        if (bossWarningTimer <= 0.0f) bossWarning = false;
    }

    // Floating texts
    for (auto& ft : floatingTexts) {
        ft.position.x += ft.velocity.x * dt;
        ft.position.y += ft.velocity.y * dt;
        ft.velocity.y  -= 15.0f * dt; // decelerate rise
        ft.lifetime    -= dt;
    }
    floatingTexts.erase(
        std::remove_if(floatingTexts.begin(), floatingTexts.end(),
            [](const FloatingText& ft){ return ft.lifetime <= 0.0f; }),
        floatingTexts.end());

    // Kill feed
    for (auto& ke : killFeed) ke.timer -= dt;
    killFeed.erase(
        std::remove_if(killFeed.begin(), killFeed.end(),
            [](const KillEntry& k){ return k.timer <= 0.0f; }),
        killFeed.end());
}

// ─── Render ──────────────────────────────────────────────────────────────────

void ScreenEffectsSystem::render(int screenW, int screenH) const {
    renderVignette(screenW, screenH);
    renderFlash(screenW, screenH);
    renderFloatingTexts();
    renderKillFeed(screenW, screenH);
    renderBossWarning(screenW, screenH);
}

void ScreenEffectsSystem::renderFlash(int w, int h) const {
    if (flash.timer <= 0.0f || flash.duration <= 0.0f) return;
    float t     = flash.timer / flash.duration;
    float alpha = flash.alpha * t;
    DrawRectangle(0, 0, w, h, ColorAlpha(flash.color, alpha));
}

void ScreenEffectsSystem::renderVignette(int w, int h) const {
    float alpha = 0.0f;
    if (vignette.pulse) {
        // Breathing pulse for low-HP
        alpha = vignette.alpha * (0.5f + 0.5f * std::sin(vignette.timer * 3.5f));
    } else {
        if (vignette.timer <= 0.0f || vignette.duration <= 0.0f) return;
        float t = vignette.timer / vignette.duration;
        alpha = vignette.alpha * t;
    }
    if (alpha <= 0.005f) return;

    int border = 140;
    Color c = vignette.color;
    DrawRectangleGradientV(0,        0,        w,      border,  ColorAlpha(c, alpha), ColorAlpha(c, 0.0f));
    DrawRectangleGradientV(0,        h-border, w,      border,  ColorAlpha(c, 0.0f),  ColorAlpha(c, alpha));
    DrawRectangleGradientH(0,        0,        border, h,       ColorAlpha(c, alpha), ColorAlpha(c, 0.0f));
    DrawRectangleGradientH(w-border, 0,        border, h,       ColorAlpha(c, 0.0f),  ColorAlpha(c, alpha));
}

void ScreenEffectsSystem::renderFloatingTexts() const {
    for (const auto& ft : floatingTexts) {
        float t     = ft.lifetime / ft.maxLifetime;
        float alpha = std::min(1.0f, t * 2.0f); // fade in fast, hold, fade out at end
        if (ft.lifetime < 0.3f) alpha = ft.lifetime / 0.3f;

        int tw = MeasureText(ft.text.c_str(), ft.fontSize);
        int x  = (int)ft.position.x - tw / 2;
        int y  = (int)ft.position.y;

        if (ft.isCrit) {
            // Shadow + scale effect
            DrawText(ft.text.c_str(), x+2, y+2, ft.fontSize, ColorAlpha({0,0,0,255}, alpha*0.6f));
            DrawText(ft.text.c_str(), x,   y,   ft.fontSize, ColorAlpha(ft.color, alpha));
        } else if (ft.isLoot) {
            DrawRectangle(x-4, y-2, tw+8, ft.fontSize+4, ColorAlpha({0,0,0,255}, alpha*0.5f));
            DrawText(ft.text.c_str(), x, y, ft.fontSize, ColorAlpha(ft.color, alpha));
        } else {
            DrawText(ft.text.c_str(), x+1, y+1, ft.fontSize, ColorAlpha({0,0,0,255}, alpha*0.5f));
            DrawText(ft.text.c_str(), x,   y,   ft.fontSize, ColorAlpha(ft.color, alpha));
        }
    }
}

void ScreenEffectsSystem::renderKillFeed(int w, int /*h*/) const {
    int y = 80;
    for (int i = (int)killFeed.size()-1; i >= 0; --i) {
        const auto& ke = killFeed[i];
        float alpha = std::min(1.0f, ke.timer);
        int tw = MeasureText(ke.text.c_str(), 13);
        int x  = w - tw - 10;
        DrawRectangle(x-4, y-1, tw+8, 16, ColorAlpha({0,0,0,255}, alpha*0.55f));
        DrawText(ke.text.c_str(), x, y, 13, ColorAlpha(ke.col, alpha));
        y += 18;
    }
}

void ScreenEffectsSystem::renderBossWarning(int w, int h) const {
    if (!bossWarning) return;
    float t = bossWarningTimer / 4.0f;
    float alpha = std::sin(bossWarningTimer * 6.0f) * 0.5f + 0.5f;
    alpha *= std::min(1.0f, t * 2.0f);

    // Red border flash
    int border = 8;
    DrawRectangleLinesEx({0,0,(float)w,(float)h}, (float)border,
                          ColorAlpha({255,0,0,255}, alpha * 0.8f));

    // Warning text
    const char* warningLine = "! BOSS DETECTADO !";
    int tw = MeasureText(warningLine, 28);
    DrawText(warningLine, w/2 - tw/2 + 2, h/2 - 30 + 2, 28, ColorAlpha({0,0,0,255}, alpha));
    DrawText(warningLine, w/2 - tw/2,     h/2 - 30,     28, ColorAlpha({255,40,40,255}, alpha));

    int nw = MeasureText(bossWarningName.c_str(), 20);
    DrawText(bossWarningName.c_str(), w/2 - nw/2, h/2 + 4, 20, ColorAlpha({255,180,180,255}, alpha));
}

// ─── Add effects ──────────────────────────────────────────────────────────────

void ScreenEffectsSystem::addDamageText(Vector2 screenPos, int damage, bool isCrit) {
    FloatingText ft;
    ft.text        = isCrit ? ("CRIT " + std::to_string(damage) + "!") : std::to_string(damage);
    ft.position    = screenPos;
    ft.velocity    = {(float)GetRandomValue(-25, 25), -100.0f};
    ft.lifetime    = isCrit ? 1.4f : 0.85f;
    ft.maxLifetime = ft.lifetime;
    ft.color       = isCrit ? Color{255,220,0,255} : Color{255,80,80,255};
    ft.fontSize    = isCrit ? 22 : 15;
    ft.isCrit      = isCrit;
    floatingTexts.push_back(ft);
}

void ScreenEffectsSystem::addHealText(Vector2 screenPos, int amount) {
    FloatingText ft;
    ft.text        = "+" + std::to_string(amount) + " HP";
    ft.position    = screenPos;
    ft.velocity    = {(float)GetRandomValue(-15, 15), -90.0f};
    ft.lifetime    = 1.0f;
    ft.maxLifetime = ft.lifetime;
    ft.color       = {50, 255, 100, 255};
    ft.fontSize    = 16;
    ft.isHeal      = true;
    floatingTexts.push_back(ft);
}

void ScreenEffectsSystem::addLootText(Vector2 screenPos, const std::string& itemName, Color rarityColor) {
    FloatingText ft;
    ft.text        = itemName;
    ft.position    = screenPos;
    ft.velocity    = {0.0f, -60.0f};
    ft.lifetime    = 2.2f;
    ft.maxLifetime = ft.lifetime;
    ft.color       = rarityColor;
    ft.fontSize    = 14;
    ft.isLoot      = true;
    floatingTexts.push_back(ft);
}

void ScreenEffectsSystem::addXPText(Vector2 screenPos, int xp) {
    FloatingText ft;
    ft.text        = "+" + std::to_string(xp) + " XP";
    ft.position    = screenPos;
    ft.velocity    = {0.0f, -70.0f};
    ft.lifetime    = 0.9f;
    ft.maxLifetime = ft.lifetime;
    ft.color       = {100, 200, 255, 255};
    ft.fontSize    = 13;
    floatingTexts.push_back(ft);
}

// ─── Trigger effects ─────────────────────────────────────────────────────────

void ScreenEffectsSystem::triggerShake(float intensity, float duration) {
    if (intensity > shake.intensity || shake.timer <= 0.0f) {
        shake.intensity = intensity;
        shake.duration  = duration;
        shake.timer     = duration;
    }
}

void ScreenEffectsSystem::triggerFlash(Color col, float alpha, float duration) {
    flash.color    = col;
    flash.alpha    = alpha;
    flash.duration = duration;
    flash.timer    = duration;
}

void ScreenEffectsSystem::triggerVignette(Color col, float alpha, float duration, bool pulse) {
    if (!pulse) {
        vignette.pulse    = false;
        vignette.color    = col;
        vignette.alpha    = alpha;
        vignette.duration = duration;
        vignette.timer    = duration;
    } else {
        vignette.pulse = true;
        vignette.color = col;
        vignette.alpha = alpha;
    }
}

// ─── Convenience ─────────────────────────────────────────────────────────────

void ScreenEffectsSystem::onPlayerHit(Vector2 screenPos, int damage) {
    triggerShake(5.0f, 0.18f);
    triggerFlash({255,0,0,255}, 0.28f, 0.20f);
    addDamageText(screenPos, damage, false);
}

void ScreenEffectsSystem::onEnemyKill(Vector2 screenPos, const std::string& enemyName, int xp) {
    triggerShake(2.5f, 0.10f);
    addXPText(screenPos, xp);
    addKillFeedText(enemyName + " eliminado", {200,200,200,255});
}

void ScreenEffectsSystem::onLevelUp(int newLevel) {
    triggerFlash({255,220,50,255}, 0.55f, 0.40f);
    triggerShake(8.0f, 0.30f);
    addKillFeedText(">>> LEVEL UP " + std::to_string(newLevel) + " <<<", {255,220,50,255});
}

void ScreenEffectsSystem::onBossSpawn(const std::string& name) {
    bossWarning     = true;
    bossWarningTimer= 4.0f;
    bossWarningName = name;
    triggerFlash({200,0,0,255}, 0.6f, 0.5f);
    triggerVignette({150,0,0,255}, 0.4f, 3.0f, false);
    triggerShake(10.0f, 0.4f);
}

void ScreenEffectsSystem::onItemPickup(Vector2 screenPos, const std::string& name, Color rarityColor) {
    addLootText(screenPos, name, rarityColor);
}

void ScreenEffectsSystem::onPortalClosed(Vector2 screenPos) {
    triggerFlash({0,180,255,255}, 0.4f, 0.35f);
    triggerShake(4.0f, 0.2f);
    FloatingText ft;
    ft.text        = "PORTAL FECHADO";
    ft.position    = screenPos;
    ft.velocity    = {0.0f, -80.0f};
    ft.lifetime    = 1.8f;
    ft.maxLifetime = ft.lifetime;
    ft.color       = {0, 220, 255, 255};
    ft.fontSize    = 18;
    floatingTexts.push_back(ft);
}

void ScreenEffectsSystem::onLowHP() {
    if (!lowHPMode) {
        lowHPMode = true;
        triggerVignette({180,0,0,255}, 0.45f, 0.0f, true);
    }
}

void ScreenEffectsSystem::onLowHPEnd() {
    if (lowHPMode) {
        lowHPMode         = false;
        vignette.pulse    = false;
        vignette.timer    = 0.0f;
    }
}

void ScreenEffectsSystem::addKillFeedText(const std::string& text, Color col) {
    KillEntry ke;
    ke.text  = text;
    ke.timer = 3.5f;
    ke.col   = col;
    killFeed.push_back(ke);
    if (killFeed.size() > 8) killFeed.erase(killFeed.begin());
}
