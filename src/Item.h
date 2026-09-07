#pragma once

#include <raylib.h>
#include <string>

enum class ItemType {
    EnergyCore,     // Grants shield burst
    ScrapMetal,     // Grants small HP
    WeaponPart,     // Grants attack damage boost
    HealthPack,     // Large HP restore
    TechChip,       // XP multiplier boost
    NanoCore,       // Rare: permanent +max HP
    PlasmaCell,     // Skill cooldown reduction
    Credits,        // Money pickup
    // ── Crafting Materials ──────────────────────────────────────────────────────
    MetalScrap,     // Scrap of metal
    AlienCarapace,  // Carapaca alien (of the Zerglings/Hydras)
    PlasmaCore,     // Core of plasma
    NanoFiber,      // Fibra nano
    OmegaEssence,   // Essencia of the OmegaBoss (rare)
    // ── Weapons ──────────────────────────────────────────────────────────────
    PlasmaRifle,    // rifle of plasma +35 dmg +50 range
    VoidBlade,      // espada of the void +55 dmg
    CrystalStaff,   // staff of crystal +40 dmg +80 range
    NanoBow,        // arco nano +30 dmg +70 range
    FrostHammer,    // martelo of ice +50 dmg (slow AoE)
    AcidGun,        // pistola of acid +28 dmg
    SoulScythe,     // foice of alma +60 dmg (drena HP)
    ChainBlade,     // lamina of corrente +45 dmg combo
    // ── Armor ────────────────────────────────────────────────────────────────
    NanoSuit,       // +80 HP +10 def
    CrystalArmor,   // +120 HP +15 def
    VoidPlating,    // +100 HP +20 def
    DragonScale,    // +150 HP +25 def
    PhaseCloak,     // +60 HP +speed 20
    IronBastionArmor,// +200 HP +30 def
    BioRegenSuit,   // +90 HP +regen
    // ── Accessories ──────────────────────────────────────────────────────────
    QuantumCore,    // XP x1.5 +speed 15
    SoulCrystal,    // HP regen +0.5/s
    VoidFragment,   // CD reduction -15%
    NanoChip,       // +dmg 20 +range 30
    TimePiece,      // time-slow aura
    FrostRune,      // cold aura slows enemies
    PlasmaCell2,    // all skills +20% damage
    // ── Consumables ──────────────────────────────────────────────────────────
    MedKit,         // restore 150 HP
    EnergyDrink,    // +speed 40% for 10s
    NanoPatch,      // +regen for 15s
    VoidEssence2,   // +dmg 50% for 8s
    FrostCrystal2,  // freeze area 3s
    PlasmaVial,     // +shield 5s
    SoulFragment2,  // +XP x2 for 60s
    // ── Legendaries ──────────────────────────────────────────────────────────
    OmegaWeapon,    // weapon maxima — +120 dmg +100 range (0.01% drop)
    VoidCrown,      // capacete legendary — +300 HP +40 def
    InfinityCore,   // acessorio — XP x2 permanent
    DragonSlayer    // espada epica — +100 dmg, boss +50% dmg
};

// Rarity tier for visual distinction — 6 tiers (Diablo-style)
enum class ItemRarity {
    Common   = 0,  // gray    — 60%
    Uncommon = 1,  // green    — 25%
    Rare     = 2,  // blue     — 10%
    Epic     = 3,  // purple     —  4%   (era "Elite")
    Legendary= 4,  // orange  —  0.9%
    Omega    = 5,  // red —  0.1% (only OmegaBoss)
};

struct Item {
    Vector2     position;
    ItemType    type;
    std::string name;
    Color       color;
    float       radius    = 8.0f;
    float       lifetime  = 30.0f;
    bool        pickedUp  = false;
    ItemRarity  rarity    = ItemRarity::Common;
    int         value       = 0;
    float       pulseTimer  = 0.0f;
    float       pickupDelay = 0.35f;

    // Rarity drop-beam fields
    Color       rarityColor  = {180,180,180,255};
    float       dropBeamTimer = 0.0f;   // >0 while beam is showing
    bool        isNew         = false;  // true when freshly dropped

    // ── Afixos (bonus that the Player can read to the equipar) ─────────────────────
    // O parent can somar estes in applyEquipmentStats (ex: equippedWeapon.bonusDamage).
    float       bonusDamage    = 0.0f;  // +damage plano
    float       bonusHealth    = 0.0f;  // +HP maximum
    float       bonusSpeed     = 0.0f;  // +speed
    float       bonusDefense   = 0.0f;  // +defense
    float       bonusCrit      = 0.0f;  // +% chance of critical
    float       bonusVampirism = 0.0f;  // +% of the damage converted in healing
    std::string affixPrefix;            // ex: "Flamejante"
    std::string affixSuffix;            // ex: "of the Tita"
    std::string baseName;               // nome base without afixos
    int         affixCount     = 0;     // quantos afixos rolaram

    void rollAffixes();                 // generates afixos by raridade and compoe the nome

    static Item createRandom(Vector2 pos);
    static Item createCredits(Vector2 pos, int amount);
    static Item createTech(Vector2 pos);
    static Item createEliteDrop(Vector2 pos);
    static Item createWeaponDrop(Vector2 pos);
    void render() const;
    void render3D() const;              // draws the low-poly 3D model floating over the ground
    void drawDropEffect() const;        // call from world render, in camera space
    void update(float dt);

    // Static helpers
    static ItemRarity  rollRarity(bool fromOmegaBoss = false);
    static Color       rarityToColor(ItemRarity r);
    static const char* rarityToName(ItemRarity r);
    void applyRarityBonus();            // boosts value/stats by tier
};
