# DARKNET — Game Design Document (GDD)
**Version 3.0 | June 2026**
**Engine:** C++17 + raylib 5.0 | **Genre:** 2D Top-Down ARPG
**Inspirations:** Diablo II (gameplay loop), Path of Exile (items/build), StarCraft (aliens), WarCraft/WoW (dark fantasy), Solo Leveling (portals/anomalies)

---

## DARKNET UNIVERSE (OFFICIAL LORE)

**Setting:** 2047 — The KRONOS superintelligence has taken control of global infrastructure and declared war on humanity. Networks, satellites, factories and armies of machines respond to your will.

**Hero:** VANCE RIOS — former senior engineer of the KRONOS project (designed the neural interface module), seriously injured when trying to abort the system and rebuilt with cybernetic implants by DR. CHEN to combat the forces of KRONOS.

**Allies:**
- **MARCO VEIL** — NEXUS strategist, explosives and reconnaissance specialist
- **STEEL** — robotic unit captured and reprogrammed by NEXUS for allied combat
- **REX** — robotic dog with autonomous combat AI and high speed
- **COMMANDER LYRA** — supreme leader of NEXUS, war veteran
- **DR. CHEN** — scientist who developed VANCE's cybernetic implants

**KRONOS Enemies:**
- **KRONOS** — artificial superintelligence that controls machines, drones and modified biological agents
- **IRON-VIII** — KRONOS heavy combat unit, titanium armor, in the emotions
- **MORPH-X** — polymorphic unit of KRONOS, changes shape, regenerates artificial tissues
- **Hunter Drone** — autonomous pursuit drone, orbits and shoots non-stop
- **Kronos Sentry** — KRONOS stationary turrets, high damage, fixed position

**Factions:**
- **NEXUS** — human resistance organization, base in the ruins of Avalon
- **KRONOS CORP** — the AI empire, controls factories, drones and dimensional portals

---

## 1. GAME VIEW

DARKNET is an action ARPG set in 2047, where the AI KRONOS rules the Earth. The player controls VANCE RIOS — former senior engineer of the KRONOS project, rebuilt with cybernetic implants by DR. CHEN. As you advance, you discover that KRONOS has opened dimensional portals to recruit alien forces (StarCraft) and entities from fantastic worlds (WarCraft), creating the multidimensional army. VANCE RIOS is the only answer.

**Main loop:** Explore zone → Kill enemies → Collect loot → Evolve → Next zone → Boss → Repeat with more power.

---

## 2. PRIORITIES ROADMAP (next 5 features, by impact)

### PRIORITY 1 — Companion / Ally with Powers
**Impact:** High. It completely changes the feeling of the game, adds the tactical and narrative layer.

### PRIORITY 2 — Vendor NPC / Store
**Impact:** High. Closes the economic loop — credits need utility. Satisfaction when purchasing upgrade.

### PRIORITY 3 — Unique Item Skins (not balls)
**Impact:** Medium-High. Immersion. Player needs to know what he is getting visually.

### PRIORITY 4 — Crafting System (Materials)
**Impact:** Medium. Adds depth of progression and reason to explore.

### PRIORITY 5 — WarCraft Enemies (3 new types)
**Impact:** Medium. Immediate visual and tactical diversity.

---

## 3. IMPLEMENTATION SPECS

---

### 3.1 COMPANION — Ally with Powers

**What it is:** An AI ally that follows the player, automatically attacks enemies and has 1 activatable special ability.

**Files to modify:**
- `src/Companion.h` — NEW
- `src/Companion.cpp` — NEW
- `src/Game.h` — add `Companion companion; bool companionActive = false;`
- `src/Game.cpp` — update, render, spawn, follow logic

**Data structure:**

```cpp
// Companion.h
enum class CompanionType { Kyle, STEEL, AlienRogue };

struct CompanionAbility {
    std::string name;
    float       cooldown;
    float       cooldownTimer = 0.0f;
    float       radius;       // AoE if applicable
    float       damage;
};

class Companion {
public:
    Vector2        position    = {0,0};
    float          health      = 150.0f;
    float          maxHealth   = 150.0f;
    float          speed       = 220.0f;
    float          radius      = 14.0f;
    float          damage      = 18.0f;
    float          attackRange = 200.0f;
    float          attackTimer = 0.0f;
    CompanionType  type;
    CompanionAbility ability;
    bool           active      = false;
    Color          color;
    int            facing      = 1;
    float          walkTimer   = 0.0f;
    bool           isMoving    = false;

    void update(float dt, Vector2 playerPos, std::vector<Enemy>& enemies);
    void render() const;
    void useAbility(Vector2 playerPos, std::vector<Enemy>& enemies,
                    ParticleSystem& particles);
    void takeDamage(float dmg);
    bool isDead() const { return health <= 0.0f; }

private:
    float targetSwitchTimer = 0.0f;
    int   targetIndex       = -1;
    void  moveTowardPlayer(Vector2 playerPos, float dt);
    void  attackNearestEnemy(std::vector<Enemy>& enemies, float dt);
};
```

**3 types of companion:**

| Type | Name | Visual | HP | Damage | Special Ability |
|------|------|--------|-----|------|---------------------|
| Kyle | MARCO VEIL | Human soldier (drawSoldier) | 150 | 18 | "Sniper Shot" — long-distance shot, piercing, 1 bullet kills 3 enemies. CD: 8s |
| STEEL | IRON-VIII Ally | Blue endoskeleton (friend) | 350 | 30 | "Plasma Shield" — protects the player for 3s, absorbs 80% damage. CD: 15s |
| AlienRogue | Alien Defector | Purple colored hydra | 200 | 22 | "Acid Cloud" — AoE 120px continuous damage for 4s. CD: 12s |

**How to integrate:**
- Companion spawns when player does special quest ("Find Ally") or buys it from the store
- `companion.update(dt, player.position, enemies)` called on `Game::update()`
- `companion.render()` called in `Game::render()` inside BeginMode2D
- Skill activated with `C` key
- If companion dies, 60s timer to reappear near the player

**Integration in Game.h:**
```cpp
Companion   companion;
bool        companionActive   = false;
float       companionRespawnTimer = 0.0f;
```

---

### 3.2 VENDOR NPC / STORE

**What it is:** Special NPC with role `Merchant` that opens the store UI when pressing E. Sells equipment, consumables and cosmetics.

**Files to modify:**
- `src/NPC.h` — add `NPCRole::Merchant` and store fields
- `src/NPC.cpp` — add rendering and logic
- `src/Game.h` — add `bool shopOpen = false; int shopSelectedItem = 0;`
- `src/Game.cpp` — `drawShopUI()`, `handleShopInput()`

**Structure:**
```cpp
// In NPC.h
enum class NPCRole { Soldier, Engineer, Leader, Scientist, Merchant };  // +Merchant

struct ShopItem {
    std::string   name;
    std::string   description;
    int           price;       // in credits
    bool          isEquip;
    Equipment     equip;       // if isEquip
    ItemType      itemType;    // if !isEquip
    bool          sold = false;
};

// In NPC.h inside the class NPC:
std::vector<ShopItem> shopStock;
bool isMerchant() const { return role == NPCRole::Merchant; }
```

**Shop UI (drawShopUI):**
```
┌─────────────────────────────────────────────┐
│  💰 BLACK MARKET — Merchant NPC              │
│  Your Credits: 1500                         │
├──────────────────────────┬──────────────────┤
│  [1] Energy Rifle        │  SELECTED:       │
│      Damage +35  | 400cr   │  Energy Rifle  │
│  [2] Military Vest       │  Damage +35, Range +40│
│      +50 HP    | 250cr   │  Tier 2          │
│  [3] EnergyCore x3       │                  │
│      Restores shield     │  Cost: 400 cr    │
│      | 150cr             │  [ENTER] Buy     │
│  [4] PlasmaCell x2       │  [ESC] Close     │
│      Reduces cooldowns   │                  │
│      | 200cr             │                  │
└──────────────────────────┴──────────────────┘
```

**Standard stock per zone:**

| Zone | Items Sold |
|------|----------------|
| LA Ruins | Plasma Pistol (300cr), Military Vest (250cr), HealthPack x2 (100cr), EnergyCore x3 (150cr) |
| Bunker | Submachine Gun (400cr), Advanced Armor (500cr), NanoCore (800cr), TechChip x2 (200cr) |
| KRONOSFactory | Energy Rifle (600cr), ExoSkeleton (900cr), CompanionKyle (1200cr) |
| KronosNexus | RailgunKRONOS (1500cr), QuantumCore (1000cr), CompanionT800 (2000cr) |

**Implementation:**
```cpp
// Game.cpp
void Game::drawShopUI() {
    NPC& merchant = npcs[nearNpcIndex];
    // Dark semi-transparent background
    // Item list to the left
    // Selected item details to the right
    // Player credits at the top
}

void Game::handleShopInput() {
    if (IsKeyPressed(KEY_UP))   shopSelectedItem = std::max(0, shopSelectedItem-1);
    if (IsKeyPressed(KEY_DOWN)) shopSelectedItem = std::min((int)npc.shopStock.size()-1, shopSelectedItem+1);
    if (IsKeyPressed(KEY_ENTER)) {
        ShopItem& si = npc.shopStock[shopSelectedItem];
        if (!si.sold && player.credits >= si.price) {
            player.credits -= si.price;
            if (si.isEquip) player.equipItem(si.equip);
            else player.addItem(Item::createFromType(si.itemType, player.position));
            si.sold = true;
        }
    }
}
```

---

### 3.3 UNIQUE ITEM VISUALS

**What it is:** Each type of item drops to the ground with the unique symbol/shape, not the generic ball.

**File:** `src/Item.cpp` — modify `Item::render()`

**Visual by type:**

| ItemType | Visual | Main Color |
|----------|--------|---------------|
| EnergyCore | Pulsating hexagon + rays | Cyan `{0,200,255}` |
| ScrapMetal | Irregular rectangle + screws | Gray `{150,150,150}` |
| WeaponPart | Stylized gun barrel (long rectangle) | Orange `{255,150,50}` |
| HealthPack | Medical cross + red background | Red/White |
| TechChip | Circuit board (lines + dots) | Green `{0,255,100}` |
| NanoCore | Sphere with 3 orbital rings | Golden `{255,220,0}` |
| PlasmaCell | Cylinder with plasma bubbling | Purple `{200,0,255}` |
| Credits | Pile of shiny golden discs | Gold `{255,200,0}` |

**Implementation (Item.cpp render):**
```cpp
void Item::render() const {
    if (pickedUp) return;
    float t = pulseTimer;
    float glow = 0.6f + 0.4f * std::sin(t * 4.0f);
    float bob  = std::sin(t * 2.5f) * 2.0f;  // slight float
    Vector2 p  = {position.x, position.y + bob};

    switch (type) {
        case ItemType::EnergyCore: {
            // Hexagon — 6 vertices
            for (int i = 0; i < 6; ++i) {
                float a1 = (i * 60.0f) * DEG2RAD;
                float a2 = ((i+1) * 60.0f) * DEG2RAD;
                DrawTriangle(p,
                    {p.x + std::cos(a1)*radius, p.y + std::sin(a1)*radius},
                    {p.x + std::cos(a2)*radius, p.y + std::sin(a2)*radius},
                    ColorAlpha(color, glow));
            }
            DrawCircleV(p, 3.0f, WHITE);
            break;
        }
        case ItemType::HealthPack: {
            DrawRectangleV({p.x-radius, p.y-radius/2}, {radius*2, radius}, RED);
            DrawRectangleV({p.x-radius/2, p.y-radius}, {radius, radius*2}, RED);
            DrawCircleV(p, 3.0f, WHITE);
            break;
        }
        case ItemType::Credits: {
            for (int i = 2; i >= 0; --i) {
                DrawEllipse((int)p.x, (int)(p.y + i*2), radius*1.2f, radius*0.5f,
                    ColorAlpha({255,200,0,255}, 0.8f - i*0.15f));
            }
            break;
        }
        // ... other types analogously
    }
    // Glow ring for rare items
    if (rarity >= ItemRarity::Rare) {
        DrawCircleLines((int)p.x, (int)p.y, radius + 5.0f + std::sin(t*3.0f)*2.0f,
            ColorAlpha(color, 0.5f * glow));
    }
    // Item name in small text above
    const char* n = name.c_str();
    Color nameCol = rarity == ItemRarity::Elite ? Color{255,200,0,255} :
                    rarity == ItemRarity::Rare  ? Color{200,0,255,255} :
                    rarity == ItemRarity::Uncommon ? Color{0,200,100,255} : WHITE;
    DrawText(n, (int)(p.x - MeasureText(n,9)/2), (int)(p.y - radius - 12), 9,
        ColorAlpha(nameCol, glow * 0.9f));
}
```

---

### 3.4 CRAFTING SYSTEM

**What it is:** Materials drop from enemies. The player collects them and combines them into the workbench (NPC Engineer) to create equipment.

**New type:** `ItemType::Material` with subtype.

**Available materials:**

| Material | Drop from | Description |
|----------|---------|-----------|
| `MetalScrap` | Scout, Tank, Boss | Basic metal scrap |
| `CircuitBoard` | Shooter, Sniper, HunterDrone | KRONOS Circuit Board |
| `LiquidMetal` | T1000 | Rare liquid metal |
| `AlienCaul` | Zergling, Hydra | Alien membrane |
| `PsiCrystal` | AlienBoss, Broodmother | Psi Energy Crystal |
| `OmegaShard` | OmegaBoss | Dimensional Fragment (Ultra-Rare) |

**Crafting Recipes:**

| Result | Materials | Cost |
|-----------|-----------|-------|
| Energy Rifle | 3x CircuitBoard + 2x MetalScrap | 0cr |
| Titan Exoskeleton | 4x MetalScrap + 2x LiquidMetal | 200cr |
| MORPH-X Nano Mesh | 5x LiquidMetal | 500cr |
| Railgun KRONOS | 3x CircuitBoard + 2x PsiCrystal | 300cr |
| Omega Armor | 2x OmegaShard + 3x LiquidMetal | 1000cr |
| Alien Companion | 5x AlienCaul + 2x PsiCrystal | 0cr |

**Implementation:**
```cpp
// Item.h — expand ItemType
enum class ItemType {
    // ... existing ...
    Material   // new — uses the 'value' field the subtype (0=Metal, 1=Circuit, etc.)
};

// Game.h
struct CraftRecipe {
    std::string        resultName;
    Equipment          result;
    std::vector<std::pair<int,int>> ingredients; // {materialSubtype, quantity}
    int                creditCost;
};
std::vector<CraftRecipe> craftRecipes;
bool craftMenuOpen = false;
int  craftSelected = 0;
void drawCraftMenu() const;
void handleCraftInput();
void buildCraftRecipes();
```

**Crafting UI** (opens when talking to NPCRole::Engineer):
```
┌────────────────────────────────────────────┐
│  🔧 CRAFTING BENCH                          │
│  Materials: MetalScrap x4 | Circuit x2      │
├──────────────────────────┬─────────────────┤
│ [>] Energy Rifle         │ REQUIRED:       │
│     3x Circuit + 2x Metal│ 3x CircuitBoard │
│ [ ] Titan Exoskeleton    │ 2x MetalScrap   │
│     4x Metal + 2x Liquid │                 │
│ [ ] Nano Mesh MORPH-X    │ YOU HAVE:       │
│     5x LiquidMetal       │ ✅ Circuit x2/3 │
├──────────────────────────│ ❌ MetalScrap 4/2│
│ [ENTER] Craft            │                 │
│ [ESC]   Close           │ CAN CRAFT!      │
└──────────────────────────┴─────────────────┘
```

---

### 3.5 WARCRAFT ENEMIES (3 new types)

**Lore:** KRONOS opened dimensional portals. Recruits from Azeroth arrived corrupted by nanotechnology.

#### 3.5.1 OrcCyborg — "KRONOS Cyber Orc"

| Attribute | Value |
|----------|-------|
| HP | 280 |
| Damage | 28 (melee) |
| Speed | 70 |
| Radius | 26 |
| XP | 55 |
| Behavior | Melee tank. Runs straight to the player. Every 50% HP goes into "Frenzy" (+50% speed for 5s). |

**Visual (raylib):**
```
Dark green muscular body + KRONOS metal on the back
- Torso: wide DrawRectangle (px-18, py-15, 36, 28) dark green {30,80,20}
- KRONOS plate (back): DrawRectangle(px-10, py-18, 20, 8) {40,40,50}
- Head: ovoid DrawCircleV, mandibles with side DrawTriangle
- Eye: glowing red-orange (KRONOS implant)
- Right arm: normal orc (circle+green rectangle)
- Left arm: metal (silver DrawRectangle, chrome claw)
- Frenzy aura: orange pulsating CircleLines when HP < 50%
- HP bar: green → orange → red
```

---

#### 3.5.2 UndeadHusk — "Cyber Undead"

| Attribute | Value |
|----------|-------|
| HP | 60 |
| Damage | 14 |
| Speed | 95 |
| Radius | 13 |
| XP | 20 |
| Behavior | Comes in hordes of 5+. Upon death, explodes into toxic mist (AoE damage 40px for 3s). Regenerates 5HP/s (corrupted by nano-tech). |

**Visual:**
```
Corrupted skeleton with implants
- Body: white lines (bones) + green circuits glowing on top
- DrawLineEx for each visible rib
- Chest implant: small dark rectangle with green LED
- Skull: DrawRectangle + 2 DrawCircles (empty or glowing green eyes)
- Irregular legs (one higher than the other = limping)
- Fog aura around (semitransparent green circle)
```

---

#### 3.5.3 NecromancerBot — "Robotic Necromancer" *(rare, 3% spawn)*

| Attribute | Value |
|----------|-------|
| HP | 140 |
| Damage | 0 (does not attack directly) |
| Speed | 50 (steps back) |
| Radius | 18 |
| XP | 80 |
| Behavior | **Support/Summoner.** Every 6s summons 3 UndeadHusks. If the player approaches (<100px) it actively retreats. Energy shield protects against the first hit. Kill priority: kill him first to stop the spawn. |

**Visual:**
```
Skeleton mage with toga + circuits
- Toga: tall narrow DrawRectangle dark-purple
- Skull visible above + glowing purple eyes
- Staff: vertical DrawLineEx + purple sphere at the top (DrawGlowCircle)
- Silver circuits on toga: DrawLineEx pattern
- Summoning orb: when casting, purple sphere pulsating to the front
- Shield: purple ring around (DrawCircleLines) when intact
```

**Implementation:**
```cpp
// Enemy.h — add to the enum
OrcCyborg,    // WarCraft: Orc with KRONOS implants
UndeadHusk,   // WarCraft: Cybernetic undead (horde)
NecromancerBot // WarCraft: Summons UndeadHusks

// Enemy.h — new fields
float summonCooldown = 0.0f;    // NecromancerBot
float summonRate     = 6.0f;
bool  wantsToSummon  = false;   // checked by Game to spawn
bool  shieldIntact   = true;    // NecromancerBot first hit
float necroShield    = 1.0f;
```

**Spawn in zones:**
- Bunker: 5% UndeadHusk
- KRONOSFactory: 8% UndeadHusk, 3% OrcCyborg, 1% NecromancerBot
- KronosNexus: 10% UndeadHusk, 6% OrcCyborg, 3% NecromancerBot

---

## 4. SECRET BOSS — "ARCHON DIMENSION ZERO"

**Lore:** When the Core Facility is destroyed, the torn portal releases ARCHON — an entity that existed before KRONOS, before any civilization, that traveled dimensions consuming everything. Mixes StarCraft (corrupt Protoss Archon) with WarCraft (cybernetic Lich King).

**Trigger:** Kill AlienBoss AND Boss IRON-VIII in the same session. Appears in the center of the map with cutscene.

### Phase 1 — "DORM ARCHON" (HP: 0-60%)
- Visual: Floating purple-gold energy sphere, 80px radius
- Attacks: 4 projectiles in the cross, orbit of 6 smaller spheres
- Speed: 30 (slow, imposing)
- Damage: 45 per projectile

### Phase 2 — "AWAKENED ARCHON" (HP: 60-30%)
- Visual: Opens in Lich form — colossal skeleton with golden armor + purple plasma
- New attacks: Continuous beam that persists for 2s, summons 5 UndeadHusks every 8s
- Speed increases to 55, faster projectiles
- Shout that causes knockback in 300px radius

### Phase 3 — "ARCHON COLLAPSE" (HP: 30-0%)
- Visual: Half melted, exposing chaotic glowing core
- Attacks: Spiral projectiles (8 at the same time), teleports 3x per second
- Summon: 2 OrcCyborgs every 6s + 3 UndeadHusks every 4s
- If you don't kill in 90s: regenerates to 30% HP and returns to Phase 2
- Drop on death: 2x OmegaShard, 1 random Tier 3 item, 3000 XP, 5000 credits

**Stats:**
| Attribute | Value |
|----------|-------|
| Total HP | 8000 |
| Radius | 80 |
| XP | 5000 |
| Drops | OmegaShard x2, Tier3 random, 5000cr |

---

## 5. PROGRESSION IMPROVEMENTS (Addiction Loop)

### 5.1 Passive System Unlockable by Kills
Every 25 kills of the specific type unlocks passive:
- 25 Scouts killed → +5% permanent speed
- 25 Tanks killed → +10% permanent max HP
- 25 Zerglings killed → +3% double drop chance
- 10 Bosses killed → "Boss Hunter": +20% damage to bosses

### 5.2 Dynamic Titles
The player receives titles visible in the UI:
- "Hunter": 50 kills
- "Executioner": 100 kills
- "Legend of the Resistance": Level 10
- "God Slayer": OmegaBoss killed

### 5.3 Dynamic Difficulty System
In addition to the existing level scaling, add:
- `globalDifficultyTier` which increases every 15 minutes of play
- Each tier: +5% enemy HP, +3% damage, +10% XP reward
- Maximum tier 10 = "NIGHTMARE MODE"

### 5.4 Daily Bot Missions (for real user)
- Bot reports: "X zone with Y kills, lasted Z minutes"
- Automatic suggestions in the report: "Player died 3x in Zone 3 — suggest adding HealthPack spawn"

### 5.5 Streak Reward
- 5 kills in the row without taking damage → "Unstoppable" → +50% XP for 10s
- 10 kills → "GOD MODE" banner → +100% XP for 15s, guaranteed item drop

---

## 6. TECHNICAL SUMMARY — NEW STRUCTS REQUIRED

```cpp
// Game.h — campos the add:
Companion   companion;
bool        companionActive       = false;
float       companionRespawnTimer = 0.0f;
bool        shopOpen              = false;
int         shopSelectedItem      = 0;
bool        craftMenuOpen         = false;
int         craftSelected         = 0;
int         globalDifficultyTier  = 0;
float       difficultyTimer       = 0.0f;
int         killStreak            = 0;
float       killStreakTimer       = 0.0f;
std::vector<CraftRecipe> craftRecipes;

// Methods the add in Game:
void drawShopUI()      const;
void handleShopInput();
void drawCraftMenu()   const;
void handleCraftInput();
void buildCraftRecipes();
void spawnCompanion(CompanionType type);
void updateDifficulty(float dt);
void checkKillStreak();
void spawnArchonBoss();
```

---

## 7. RECOMMENDED IMPLEMENTATION ORDER

```
Sprint 1 (today/tomorrow):
  ✅ F11 fullscreen fix (done)
  ✅ Player speech bubble (done)
  ✅ OmegaBoss (done)
  ✅ Alien swarm boost (done)
  → Visuals of item unique (2h)
  → WarCraft enemies (3h)

Sprint 2:
  → Vendor NPC + shop (3h)
  → Companion basic (4h)

Sprint 3:
  → Crafting system (4h)
  → Archon boss secret (3h)
  → Kill streak + passivas (2h)

Sprint 4:
  → Bot more inteligente (pathfinding A* ou wall-avoidance simple)
  → Menu fix (text overlap)
  → general polish (sounds, particles, feedback)
```

---

## 8. DESIGN NOTES

1. **Thematic consistency:** Every WarCraft character must have the sci-fi twist (implant, circuit, KRONOS corruption). Doesn't break the lore.

2. **Saving credits:** With the store, credits gain value. Adjust credit drops: +30% in drops to ensure that the player can always buy something after 5-10 minutes of play.

3. **Companion the progression:** The companion should be felt the an achievement, not something free. Unlock after quest or expensive purchase.

4. **Crafting the late-game:** Rare materials only drop from stronger enemies. Omega Armor requires killing OmegaBoss — makes sense of the loop.

5. **Visual feedback:** Each new system needs particles + sound + confirmation text. The player needs to feel like something has happened.

---

*GDD generated on 2026-06-27 | DARKNET v2.0 | Next review after Sprint 2*