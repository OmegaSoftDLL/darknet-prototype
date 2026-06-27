#pragma once
#include <raylib.h>
#include <vector>
#include <string>

struct FloatingText {
    std::string text;
    Vector2     position;   // screen-space
    Vector2     velocity;   // screen-space px/s
    float       lifetime;
    float       maxLifetime;
    Color       color;
    int         fontSize;
    bool        isCrit     = false;
    bool        isLoot     = false;
    bool        isHeal     = false;
};

struct ScreenShake {
    float intensity = 0.0f;
    float duration  = 0.0f;
    float timer     = 0.0f;
};

struct FlashEffect {
    Color color    = {0,0,0,255};
    float alpha    = 0.0f;
    float duration = 0.0f;
    float timer    = 0.0f;
};

struct VignetteEffect {
    Color color    = {0,0,0,255};
    float alpha    = 0.0f;
    float duration = 0.0f;
    float timer    = 0.0f;
    bool  pulse    = false;   // repeating pulse vs one-shot
};

class ScreenEffectsSystem {
public:
    std::vector<FloatingText> floatingTexts;
    ScreenShake    shake;
    FlashEffect    flash;
    VignetteEffect vignette;
    Vector2        shakeOffset = {0, 0};

    // Kill feed entries (top-left area)
    struct KillEntry { std::string text; float timer; Color col; };
    std::vector<KillEntry> killFeed;

    // Boss warning
    bool  bossWarning      = false;
    float bossWarningTimer = 0.0f;
    std::string bossWarningName;

    void update(float dt);

    // render() must be called in SCREEN-SPACE (outside BeginMode2D / BeginTextureMode)
    // Pass screen dimensions of the virtual render target (1280x720)
    void render(int screenW, int screenH) const;

    // ── Floating text ──────────────────────────────────────────────────────────
    // All positions in screen-space (convert world→screen before calling)
    void addDamageText(Vector2 screenPos, int damage, bool isCrit = false);
    void addHealText(Vector2 screenPos, int amount);
    void addLootText(Vector2 screenPos, const std::string& itemName, Color rarityColor);
    void addXPText(Vector2 screenPos, int xp);

    // ── Screen effects ─────────────────────────────────────────────────────────
    void triggerShake(float intensity, float duration);
    void triggerFlash(Color col, float alpha, float duration);
    void triggerVignette(Color col, float alpha, float duration, bool pulse = false);

    // ── Convenience helpers ────────────────────────────────────────────────────
    void onPlayerHit(Vector2 screenPos, int damage);
    void onEnemyKill(Vector2 screenPos, const std::string& enemyName, int xp);
    void onLevelUp(int newLevel);
    void onBossSpawn(const std::string& name);
    void onItemPickup(Vector2 screenPos, const std::string& name, Color rarityColor);
    void onPortalClosed(Vector2 screenPos);
    void onLowHP();       // call when player enters low HP (< 25%)
    void onLowHPEnd();    // call when player is healed above threshold
    void addKillFeedText(const std::string& text, Color col = {255,80,80,255});

private:
    void renderFloatingTexts() const;
    void renderFlash(int w, int h) const;
    void renderVignette(int w, int h) const;
    void renderKillFeed(int w, int h) const;
    void renderBossWarning(int w, int h) const;

    bool lowHPMode = false;
};
