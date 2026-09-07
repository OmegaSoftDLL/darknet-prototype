#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <sstream>

// Mantido to compatibilidade with Game.cpp existente
enum class NPCRole {
    Soldier, Engineer, Leader, Scientist, Merchant, WeaponDealer, ArmorSmith
};

// Novos types expandidos
enum class NPCType {
    Merchant,
    QuestGiver,
    Lorekeeper,
    Blacksmith,
    HackerContact,
    Survivor,
    RebellionLeader,
    GhostInformer,
    AncientAI,
    BountyBoard,
    ArmorSmith,
    RuneMaster,
};

struct DialogueLine {
    std::string speaker;
    std::string text;
    Color       color        = {220, 220, 220, 255};
    float       displayTime  = 0.0f; // 0 = aguarda input
};

struct DialogueTree {
    std::string                id;
    std::string                triggerCondition; // "first_talk", "quest_active", "quest_done", "always"
    std::vector<DialogueLine>  lines;
    std::string                nextTreeId;
    bool                       givesQuest    = false;
    std::string                questId;
    int                        rewardCredits = 0;
    int                        rewardXP      = 0;
};

class NPC {
public:
    // ── Campos originais (compatibilidade) ───────────────────────────────
    Vector2                  position;
    float                    radius      = 20.0f;
    std::string              name;
    std::vector<std::string> dialogLines; // system legacy
    std::string              questId;
    NPCRole                  role        = NPCRole::Soldier;
    Color                    color       = DARKGREEN;
    bool                     hasQuest    = false;

    // ── Campos novos ─────────────────────────────────────────────────────
    NPCType                  npcType     = NPCType::Merchant;
    std::string              title;
    Color                    nameColor   = {0, 200, 255, 255};
    float                    interactRadius = 80.0f;

    std::vector<DialogueTree> dialogues;
    int                       currentDialogueTree = 0;
    int                       currentLine         = 0;
    bool                      isInDialogue        = false;
    float                     dialogueTimer       = 0.0f;
    bool                      hasNewDialogue      = true;
    std::string               activatedTree;

    // Visual
    float   bobTimer    = 0.0f;
    float   bobOffset   = 0.0f;
    bool    facingRight = true;
    Color   bodyColor   = {80, 80, 180, 255};
    Color   accentColor = {0, 200, 255, 255};
    float   glowPulse   = 0.0f;

    bool        talked_before = false;
    std::string currentMood   = "neutral";

    // ── Construtor legacy ─────────────────────────────────────────────────
    NPC() = default;
    NPC(Vector2 pos, const std::string& n, NPCRole r,
        const std::vector<std::string>& lines,
        const std::string& qid = "");

    // ── API legacy ────────────────────────────────────────────────────────
    void render() const;
    bool isPlayerNear(Vector2 playerPos) const;
    void showDialog(int line = 0) const;

    // ── API new ──────────────────────────────────────────────────────────
    void setup(NPCType t, const std::string& n, Vector2 pos);
    void update(float dt, Vector2 playerPos, bool playerInteract);
    void renderFull() const;                        // render new detalhado
    // Phase of the passo (0..2pi) and if is andando. O 2D desenhava the legs in
    // position FIXA, entao the model 3D (that and generated the partir of the 2D) also
    // ficava congelado: the NPC deslizava pelo scenario instead of caminhar.
    float walkPhase = 0.0f;
    bool  walking   = false;
    void drawLegsAnim(Color c, float w, float h, float yTop) const;
    void renderDialogue(int screenW, int screenH) const;
    void advanceDialogue();
    void startDialogue(const std::string& treeId = "");
    void endDialogue();

    void setupMerchantDialogues();
    void setupBlacksmithDialogues();
    void setupSurvivorDialogues();
    void setupRebellionLeaderDialogues();
    void setupHackerContactDialogues();
    void setupGhostInformerDialogues();
    void setupAncientAIDialogues();

private:
    void renderSoldier()    const;
    void renderEngineer()   const;
    void renderLeader()     const;
    void renderScientist()  const;
    void renderMerchant()   const;
    void drawBody()         const;
    void drawNameplate()    const;
    void drawInteractPrompt(Vector2 playerPos) const;

    static std::vector<std::string> wrapText(const std::string& text, int maxWidth, int fontSize);
};

// ── NPCManager ────────────────────────────────────────────────────────────────
class NPCManager {
public:
    std::vector<NPC> npcs;
    bool             anyInDialogue = false;

    void init();
    void spawnNPCsForZone(int zoneId);
    void update(float dt, Vector2 playerPos, bool playerInteract);
    void render() const;
    void renderDialogues(int screenW, int screenH) const;
    void clearZoneNPCs();
};
