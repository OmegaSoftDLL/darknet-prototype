#pragma once
#include <raylib.h>
#include <vector>
#include <string>
#include <cstdint>
#include "Item.h"
#include "Skill.h"
#include "Equipment.h"

struct PlayerSpeech {
    std::string text;
    float       duration  = 3.0f;
    float       timer     = 0.0f;
    bool        active    = false;
    Color       color     = {0, 220, 255, 255};
};

struct KillStreak {
    int   count      = 0;
    float resetTimer = 0.0f;
};

enum class EvolutionPath {
    None = 0,
    CyborgSoldier,
    HackerFantasma,
    ExecutorOmega
};

struct LevelUpChoice {
    std::string title;
    std::string description;
    int         statType;    // 0=maxHP 1=dmg 2=speed 3=defense 4=range 5=cdReduction
    float       bonusAmount;
};

// Classes jogaveis — cada uma has aparencia, formato of body and stats own.
enum class CharacterClass {
    Soldado = 0,   // homem soldier — equilibrado, armor, rifle
    Guerreira,     // mulher guerreira — fast, agil, pistolas duplas
    Robo,          // robo of combat — very HP/defense, slow, heavy
    Mago,          // mage — little health, MUITO damage of skill, staff, tunica
    Bruxa,         // bruxa — equilibrada, chapeu, magic, varinha
    HomemFera,     // homem-fera — high damage body the body, garras, pelos, fast
    COUNT
};

class Player {
public:
    Vector2 position     = {0, 0};
    Vector2 velocity     = {0, 0};
    float   speed        = 250.0f;

    // Corrida and pulo (publicos — Game controla)
    float jumpZ      = 0.0f;   // height current of the pulo (z)
    float jumpVel    = 0.0f;   // speed vertical of the pulo
    bool  isJumping  = false;
    bool  sprinting  = false;  // Game seta when segura correr
    void  startJump() { if (!isJumping) { isJumping = true; jumpVel = 360.0f; } }

    float   health       = 100.0f;
    float   maxHealth    = 100.0f;
    float   attackDamage = 25.0f;
    float   attackRange  = 90.0f;
    float   radius       = 16.0f;
    float   defense      = 0.0f;
    float   xpMultiplier = 1.0f;

    int level        = 1;
    int xp           = 0;
    int xpToNextLevel= 100;
    int credits      = 0;

    std::vector<Item>      inventory;
    std::vector<Skill>     skills;

    Equipment equippedWeapon;
    Equipment equippedArmor;
    Equipment equippedImplant;

    // Equipment backpack guardados (gear not equipado) — estilo Diablo:
    // the player escolhe the that equipar of uma list instead of descartar the old.
    std::vector<Equipment> equipBag;
    int  selectedBagEquip = 0;
    void equipFromBag(int idx);   // troca the item of the bolsa pelo equipado of the slot

    // Assinatura of the APARENCIA (weapon/armor/implant/tinta). O model 3D and generated
    // the partir of the sprite 2D and stays CACHEADO: without incluir isto in the chave of the cache, the
    // character continuava with the visual old after swap of equipment.
    int visualSignature() const;

    // ── Cosmeticos (atualizados pelo Game: shop common + premium skins of Gems) ──
    bool  hasCosmeticTint = false;          // tinta comprada with credits
    Color cosmeticTint    = {255,255,255,255};
    bool  skinNeon        = false;          // skin_neon  (glow neon aditivo)
    bool  skinDragon      = false;          // skin_dragon (tonalidade hot + brasas)
    bool  petDrone        = false;          // pet_drone  (drone float to the lado)

    // ── Power of healing (estilo Diablo 3: potion with cooldown + regen buff) ───────
    float healCooldown = 0.0f;   // time restante until power usar of new
    float regenTimer   = 0.0f;   // duration of the buff of regeneracao active
    void  usePotion();           // healing instantanea + ativa regeneracao (key Q)
    bool  potionReady() const { return healCooldown <= 0.0f; }

    // Buff timers
    float overloadTimer = 0.0f;
    float shieldTimer   = 0.0f;
    bool  inSafeRefuge  = false;   // setado pelo Game: invulneravel inside the zone segura
    bool  reviveReady   = true;    // Protocolo Imortal (perk) — 1 uso by 60s
    float reviveTimer   = 0.0f;

    bool isOverloaded() const { return overloadTimer > 0.0f; }
    bool isShielded()   const { return shieldTimer   > 0.0f; }

    // ── Sistema of classes of character ─────────────────────────────────────
    CharacterClass charClass = CharacterClass::Soldado;
    float          skillPower = 1.0f;   // multiplicador of damage of skill of the class
    // Paleta visual of the class (definida in applyClass, usada in the render)
    Color classPrimary   = {35, 55, 120, 255};
    Color classSecondary = {55, 90, 180, 255};
    Color classAccent    = {0, 210, 255, 255};
    Color classSkin      = {180, 135, 105, 255};
    Color classTrim      = {160, 170, 185, 255};

    void applyClass(CharacterClass c);                      // aplica stats/cores base
    static const char* className(CharacterClass c);        // nome p/ the screen of selecao
    static const char* classDescription(CharacterClass c); // descricao short
    static const char* classFantasy(CharacterClass c);     // 1 frase of "sabor"

    // Evolution system
    EvolutionPath evolutionPath  = EvolutionPath::None;
    int           evolutionTier  = 0;
    float         evolutionPulse = 0.0f;

    float walkAnimTimer = 0.0f;
    bool  isMoving      = false;
    bool  moveRequested = false;  // input received neste frame (game feel)
    int   facing        = 1;
    bool  leveledUp     = false;   // SO effect visual (stays true by levelUpTimer)
    float levelUpTimer  = 0.0f;
    // Niveis ganhos that the Game still not contabilizou. Contador (not flag): the
    // Game drena 1x by frame. Antes the Game usava `leveledUp` as gatilho of
    // edge — mas ela stays true by 2.5s, entao cada orb of XP colhido nessa
    // window dava um level-up/evolution DE GRACA. E go up 2 levels of uma vez
    // (addXP big) only dava 1 point.
    int   unclaimedLevels = 0;
    std::string lastPassive;

    // ── Hack Tree (skill tree of perks) ──
    int      skillPoints = 0;   // 1 by level-up; gasta in the tree (key X)
    uint32_t perkMask    = 0;   // bits of the perks comprados (SkillTree::bit)

    // Player speech / dialogue
    PlayerSpeech speech;
    KillStreak   killStreak;
    bool         wasLowHP     = false;  // track for speech trigger
    int          totalKills   = 0;

    void say(const std::string& text, float duration = 3.0f, Color col = {0,220,255,255});
    void onKill();
    void onBossFound();
    void onPortalClosed();
    void onEnterZone(int zoneId);

    Player();

    void  move(Vector2 direction, float dt);
    void  update(float dt);
    void  updateSpeech(float dt);
    void  renderSpeech() const;
    void  render() const;
    void  addItem(const Item& item);
    void  useSkill(int index, Vector2 target);
    void  drawInventory() const;
    void  absorbBossEssence(int kind);   // buff PERMANENTE of boss (mexe in the stat BASE)

    // ── Save/Load of progress (class + stats BASE — efetivos sao recalculados) ──
    CharacterClass getCharClass() const { return charClass; }
    float getBaseMaxHealth()    const { return baseMaxHealth; }
    float getBaseAttackDamage() const { return baseAttackDamage; }
    float getBaseSpeed()        const { return baseSpeed; }
    float getBaseAttackRange()  const { return baseAttackRange; }
    float getBaseDefense()      const { return baseDefense; }
    void  loadSavedProgress(CharacterClass c, float bMax, float bDmg, float bSpd, float bRng, float bDef) {
        applyClass(c);
        baseMaxHealth = bMax; baseAttackDamage = bDmg; baseSpeed = bSpd;
        baseAttackRange = bRng; baseDefense = bDef;
        applyEquipmentStats();
    }
    void  handleInventoryInput();
    // Mouse in the inventory (vmouse already virtualizado p/ 1280x720). Returns true if
    // the click went in the button FECHAR (Game must entao close the inventory).
    bool  handleInventoryMouse(Vector2 vmouse, bool leftClick, bool rightClick);
    bool  tryUpgradeEquip(int slot);
    void  heal(float amount);
    void  addXP(int amount);   // acumula in unclaimedLevels
    void  takeDamage(float amount);
    void  increaseBaseMaxHP(float amount);
    void  increaseBaseAttackDamage(float amount);
    void  increaseBaseSpeed(float amount);
    void  increaseBaseAttackRange(float amount);
    void  increaseBaseDefense(float amount);
    void  refreshSkillVectors();   // recalcula damage/range/cooldown of the skills (idempotent)
    void  equipItem(const Equipment& equip);
    void  drawEquipment() const;
    float getEffectiveDamage() const;
    bool  useInventoryItem(int index);
    bool  tryFuseItems();

    int   selectedEquipSlot = 0;
    int   selectedInvItem   = 0;

private:
    void  levelUp();
    void  applyEquipmentStats();
    float baseSpeed        = 250.0f;
    float baseMaxHealth    = 100.0f;
    float baseAttackDamage = 25.0f;
    float baseAttackRange  = 90.0f;
    float baseDefense      = 8.0f;   // defense base of the class (armor soma by up)

    std::vector<float> baseSkillDamage;  // damage base of the skills (p/ aplicar skillPower idempotent)
    std::vector<float> baseSkillRange;    // range base of the skills (perks recomputam of the originais)
    std::vector<float> baseSkillCool;     // cooldown base of the skills (idem)
    float cdEvoMult = 1.0f;               // reducoes of cooldown by evolution (lv 10/25/40/60)

    // Render of body by class (formatos distintos)
    Color accentNow() const;  // accent considerando overload/shield
    void  renderSoldado  (float px, float py, float f, float legL, float legR) const;
    void  renderGuerreira(float px, float py, float f, float legL, float legR) const;
    void  renderRobo     (float px, float py, float f, float legL, float legR) const;
    void  renderMago     (float px, float py, float f, float legL, float legR) const;
    void  renderBruxa    (float px, float py, float f, float legL, float legR) const;
    void  renderHomemFera(float px, float py, float f, float legL, float legR) const;
};
