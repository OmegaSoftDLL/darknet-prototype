#pragma once
#include <raylib.h>
#include <vector>
#include <string>

enum class TutorialStep {
    None,
    Welcome,
    Movement,
    Attack,
    UseSkill,
    PickupItem,
    CheckMap,
    TalkToNPC,
    OpenShop,
    LevelUp,
    Portal,
    Completed,
};

struct TutorialHint {
    TutorialStep step;
    std::string  title;
    std::string  text;
    std::string  key;
    Color        keyColor   = {0, 220, 255, 255};
    float        displayTime = 0.0f;
    bool         completed  = false;
    Vector2      arrowTarget = {0, 0};
    bool         hasArrow   = false;
};

class TutorialSystem {
public:
    bool                      active         = true;
    TutorialStep              currentStep    = TutorialStep::Welcome;
    std::vector<TutorialHint> hints;
    float                     stepTimer      = 0.0f;
    float                     fadeAlpha      = 0.0f;
    bool                      fadingIn       = true;
    int                       completedCount = 0;
    float                     completeBadgeTimer = 0.0f;

    void init();
    void update(float dt);
    void render(int screenW, int screenH) const;

    void onPlayerMoved();
    void onPlayerAttacked();
    void onSkillUsed();
    void onItemPickedUp();
    void onMapOpened();
    void onNPCTalked();
    void onShopOpened();
    void onLeveledUp();
    void onPortalFound();

    void completeStep(TutorialStep step);
    void skipTutorial();
    bool isBlockingInput() const;

private:
    void renderCurrentHint(int screenW, int screenH) const;
    void renderArrow(Vector2 target, int screenW, int screenH) const;
    void renderCompletionBadge(int screenW, int screenH) const;

    TutorialHint* getCurrentHint();
    int           stepIndex(TutorialStep s) const;
};
