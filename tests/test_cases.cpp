// Casos de teste das regras PURAS do jogo — sem janela raylib.
// Roda em: cmake --build build --config Release --target darknet_tests
//          ./build/Release/darknet_tests.exe   (ou: ctest --test-dir build -C Release)
//
// Neste TU entra a raylib (via headers do jogo) MAS NAO a implementacao do
// doctest — por isso nao ha conflito com <windows.h> (ver tests/test_main.cpp).
#include "CraftingSystem.h"
#include "Enemy.h"
#include "Equipment.h"
#include "SaveManager.h"
#include "Player.h"
#include "Projectile.h"
#include "Tilemap.h"

#include <doctest/doctest.h>

// Global definido em Game.cpp no jogo; Enemy.cpp referencia via `extern`.
// Nos testes nao capturamos voxel, entao fica false.
bool g_voxelCapture = false;

// ── Helpers ──────────────────────────────────────────────────────────────────
static std::vector<Item> makeBag(ItemType t, int n) {
    std::vector<Item> bag;
    for (int i = 0; i < n; ++i) { Item it{}; it.type = t; bag.push_back(it); }
    return bag;
}

static void appendTo(std::vector<Item>& bag, ItemType t, int n) {
    for (int i = 0; i < n; ++i) { Item it{}; it.type = t; bag.push_back(it); }
}

// ── CraftingSystem ───────────────────────────────────────────────────────────

TEST_CASE("CraftingSystem::canCraft - materiais suficientes e insuficientes") {
    CraftingSystem cs;
    cs.buildRecipes();
    REQUIRE(!cs.recipes.empty());

    // "Faca de Combate": 3x MetalScrap
    int idx = -1;
    for (int i = 0; i < (int)cs.recipes.size(); ++i)
        if (cs.recipes[i].name == "Faca de Combate") { idx = i; break; }
    REQUIRE(idx >= 0);

    std::vector<Item> bag;
    CHECK_FALSE(cs.canCraft(bag, idx));                    // bag vazia
    bag = makeBag(ItemType::MetalScrap, 2);
    CHECK_FALSE(cs.canCraft(bag, idx));                    // 2 < 3
    appendTo(bag, ItemType::MetalScrap, 1);
    CHECK(cs.canCraft(bag, idx));                          // exato: 3
    appendTo(bag, ItemType::MetalScrap, 5);
    CHECK(cs.canCraft(bag, idx));                          // sobra nao atrapalha
}

TEST_CASE("CraftingSystem::canCraft - receita multi-ingrediente e indice invalido") {
    CraftingSystem cs;
    cs.buildRecipes();

    // "Lancador Acido": 3x AlienCarapace + 1x PlasmaCore
    int idx = -1;
    for (int i = 0; i < (int)cs.recipes.size(); ++i)
        if (cs.recipes[i].name == "Lancador Acido") { idx = i; break; }
    REQUIRE(idx >= 0);

    std::vector<Item> bag = makeBag(ItemType::AlienCarapace, 3);
    CHECK_FALSE(cs.canCraft(bag, idx));                    // falta PlasmaCore
    appendTo(bag, ItemType::PlasmaCore, 1);
    CHECK(cs.canCraft(bag, idx));                          // completo

    CHECK_FALSE(cs.canCraft(bag, -1));                     // indices invalidos
    CHECK_FALSE(cs.canCraft(bag, (int)cs.recipes.size()));
}

TEST_CASE("CraftingSystem::countMaterial conta so o tipo pedido") {
    std::vector<Item> bag = makeBag(ItemType::MetalScrap, 4);
    appendTo(bag, ItemType::NanoFiber, 2);

    CHECK(CraftingSystem::countMaterial(bag, ItemType::MetalScrap) == 4);
    CHECK(CraftingSystem::countMaterial(bag, ItemType::NanoFiber) == 2);
    CHECK(CraftingSystem::countMaterial(bag, ItemType::OmegaEssence) == 0);
}

TEST_CASE("CraftingSystem::getFilteredIndices filtra por categoria") {
    CraftingSystem cs;
    cs.buildRecipes();

    cs.selectedCategory = CraftCategory::All;
    CHECK((int)cs.getFilteredIndices().size() == (int)cs.recipes.size());

    cs.selectedCategory = CraftCategory::Weapons;
    auto weapons = cs.getFilteredIndices();
    REQUIRE(!weapons.empty());
    CHECK(weapons.size() < cs.recipes.size());
    for (int i : weapons)
        CHECK(cs.recipes[i].category == CraftCategory::Weapons);
}

// ── Enemy: classificacao de tipo (pura, inline no header) ────────────────────

TEST_CASE("Enemy::isBoss classifica todos os bosses") {
    CHECK(Enemy({0,0}, EnemyType::Boss).isBoss());
    CHECK(Enemy({0,0}, EnemyType::AlienBoss).isBoss());
    CHECK(Enemy({0,0}, EnemyType::OmegaBoss).isBoss());
    CHECK(Enemy({0,0}, EnemyType::PoltergeistBoss).isBoss());
    CHECK(Enemy({0,0}, EnemyType::ZombieLord).isBoss());
    CHECK(Enemy({0,0}, EnemyType::VoidColossus).isBoss());
    CHECK(Enemy({0,0}, EnemyType::FrostWyrm).isBoss());
    CHECK(Enemy({0,0}, EnemyType::InfernoHerald).isBoss());
    CHECK(Enemy({0,0}, EnemyType::VolcanicTitan).isBoss());
    CHECK(Enemy({0,0}, EnemyType::Leviathan).isBoss());

    CHECK_FALSE(Enemy({0,0}, EnemyType::Scout).isBoss());
    CHECK_FALSE(Enemy({0,0}, EnemyType::Tank).isBoss());
    CHECK_FALSE(Enemy({0,0}, EnemyType::Kamikaze).isBoss());
    CHECK_FALSE(Enemy({0,0}, EnemyType::GhostElite).isBoss()); // elite != boss
}

TEST_CASE("Enemy sobrenatural vs flutuante (zumbis NAO flutuam)") {
    Enemy ghost({0,0}, EnemyType::Ghost);
    CHECK(ghost.isSupernatural());
    CHECK(ghost.isFloating());

    Enemy wraith({0,0}, EnemyType::ShadowWraith);
    CHECK(wraith.isSupernatural());
    CHECK(wraith.isFloating());

    Enemy zombie({0,0}, EnemyType::Zombie);
    CHECK(zombie.isSupernatural());
    CHECK_FALSE(zombie.isFloating());   // corporeo: anda no chao

    Enemy scout({0,0}, EnemyType::Scout);
    CHECK_FALSE(scout.isSupernatural());
    CHECK_FALSE(scout.isFloating());
}

TEST_CASE("Enemy::getGlobalScaling - base, bonus e cap de +300%") {
    CHECK(Enemy::getGlobalScaling(0, 0) == doctest::Approx(1.0));
    CHECK(Enemy::getGlobalScaling(50, 0) == doctest::Approx(1.05));  // 1 grupo de 50 kills
    CHECK(Enemy::getGlobalScaling(0, 10) == doctest::Approx(1.8));   // 10 * 0.08
    CHECK(Enemy::getGlobalScaling(100, 5) == doctest::Approx(1.5));  // 0.10 + 0.40
    CHECK(Enemy::getGlobalScaling(100000, 1000) == doctest::Approx(4.0)); // cap 4.0
}

TEST_CASE("Enemy::takeDamage - morte, loot e clamp de HP") {
    Enemy e({0,0}, EnemyType::Scout);
    CHECK_FALSE(e.isDead());

    e.takeDamage(e.maxHealth + 50.0f);   // overkill
    CHECK(e.health == doctest::Approx(0.0)); // clamp em 0
    CHECK(e.isDead());
    CHECK(e.shouldDropLoot());

    e.markLootDropped();
    CHECK_FALSE(e.shouldDropLoot());     // loot so dropa uma vez
}

// ── Equipment: matematica de upgrade (inline no header) ──────────────────────

TEST_CASE("Equipment - stats efetivos escalam +30% por nivel de upgrade") {
    Equipment w = EDB::rifleEnergia();   // primary 35, secondary 40
    CHECK(w.getEffectivePrimary()   == doctest::Approx(35.0));
    CHECK(w.getEffectiveSecondary() == doctest::Approx(40.0));

    w.upgradeLevel = 1;
    CHECK(w.getEffectivePrimary()   == doctest::Approx(45.5));
    w.upgradeLevel = 3;
    CHECK(w.getEffectivePrimary()   == doctest::Approx(66.5));
    CHECK(w.getEffectiveSecondary() == doctest::Approx(76.0));
}

TEST_CASE("Equipment - custo de upgrade e canUpgrade") {
    Equipment w = EDB::pistolaPlas();
    CHECK(w.canUpgrade());
    CHECK(w.upgradeCost() == 100);
    w.upgradeLevel = 1;
    CHECK(w.upgradeCost() == 300);
    w.upgradeLevel = 2;
    CHECK(w.upgradeCost() == 600);
    w.upgradeLevel = 3;
    CHECK(w.upgradeCost() == 0);
    CHECK_FALSE(w.canUpgrade());         // maximo atingido

    Equipment vazio;
    CHECK(vazio.isEmpty());
    CHECK_FALSE(vazio.canUpgrade());     // slot vazio nao upa
}

// ── SaveManager: roundtrip V5 (slot) ─────────────────────────────────────────

TEST_CASE("SaveManager - save/load roundtrip preserva estado completo (V5)") {
    const int slot = 2;
    SaveManager::deleteSave(slot);

    Player p;
    p.position = {123.0f, 456.0f};
    p.applyClass(CharacterClass::Mago);
    p.health = p.maxHealth;
    p.level = 7;
    p.xp = 2500;
    p.xpToNextLevel = 3200;
    p.credits = 9876;
    p.totalKills = 123;
    p.equipItem(EDB::rifleEnergia());
    p.equipItem(EDB::armaduraAvan());
    p.equipItem(EDB::neuralLink());
    p.inventory.clear();
    for (int i = 0; i < 5; ++i) { Item it{}; it.type = ItemType::MetalScrap; p.inventory.push_back(it); }
    p.inventory.push_back(p.inventory[0]); // 6 no total para o DELETE do slot

    std::vector<Quest> quests;
    Quest qa("q_teste_a", "A", "d", "npc", QuestType::Kill, 10);
    qa.current = 6; qa.completed = false; qa.rewardGiven = false;
    Quest qb("q_teste_b", "B", "d", "npc", QuestType::KillBoss, 1);
    qb.current = 1; qb.completed = true; qb.rewardGiven = true;
    quests.push_back(qa); quests.push_back(qb);

    SaveManager::save(p, quests, ZoneID::Cemetery, slot, 45.5f, 123);

    Player loaded;
    std::vector<Quest> loadedQuests;
    loadedQuests.push_back(Quest("q_teste_a", "A", "d", "npc", QuestType::Kill, 10));
    loadedQuests.push_back(Quest("q_teste_b", "B", "d", "npc", QuestType::KillBoss, 1));
    ZoneID loadedZone = ZoneID::LARuins;

    REQUIRE(SaveManager::load(loaded, loadedQuests, loadedZone, slot));
    REQUIRE(SaveManager::hasSave(slot));

    SaveSlotInfo info = SaveManager::getSlotInfo(slot);
    CHECK(info.exists);
    CHECK(info.playerLevel == 7);

    // Player core
    CHECK(loaded.position.x == doctest::Approx(123.0f));
    CHECK(loaded.position.y == doctest::Approx(456.0f));
    CHECK(loaded.level == 7);
    CHECK(loaded.xp == 2500);
    CHECK(loaded.credits == 9876);
    CHECK(loaded.getCharClass() == CharacterClass::Mago);
    CHECK(loaded.totalKills == 123);
    CHECK(static_cast<int>(loadedZone) == static_cast<int>(ZoneID::Cemetery));

    // Equipment resolvido por ID estavel (nao por nome)
    CHECK(loaded.equippedWeapon.id == EDB::rifleEnergia().id);
    CHECK(loaded.equippedArmor.id   == EDB::armaduraAvan().id);
    CHECK(loaded.equippedImplant.id == EDB::neuralLink().id);

    // Inventory
    REQUIRE(loaded.inventory.size() == 6);
    for (const auto& item : loaded.inventory)
        CHECK(item.type == ItemType::MetalScrap);

    // Quests (match por id, estado restaurado)
    REQUIRE(loadedQuests.size() == 2);
    CHECK(loadedQuests[0].current    == 6);
    CHECK_FALSE(loadedQuests[0].completed);
    CHECK(loadedQuests[1].current    == 1);
    CHECK(loadedQuests[1].completed);
    CHECK(loadedQuests[1].rewardGiven);

    SaveManager::deleteSave(slot);
    CHECK_FALSE(SaveManager::hasSave(slot));
}

TEST_CASE("SaveManager - V4 legado carrega equipamento por nome de exibicao") {
    const int slot = 1;
    SaveManager::deleteSave(slot);

    // Grava um save V4 a mao: IDs antigos nao existiam, equipamento por nome.
    std::string path = std::string("saves/darknet_slot1.txt");
    {
        FILE* f = fopen(path.c_str(), "w");
        REQUIRE(f != nullptr);
        fprintf(f, "DARKNET_SAVE_V4\nslot 1\nsaveDate 2026-08-21 10:00:00\n");
        fprintf(f, "posX 10.0\nposY 20.0\nhealth 80.0\nmaxHealth 100.0\n");
        fprintf(f, "attackDamage 15.0\nattackRange 90.0\nspeed 250.0\ndefense 0.0\n");
        fprintf(f, "charClass 0\nlevel 3\nxp 150\nxpToNext 300\ncredits 500\nzone 4\n");
        fprintf(f, "weaponName Pistola Plasma\narmorName Colete Militar\nimplantName none\n");
        fprintf(f, "questCount 0\ninventory 0\n");
        fclose(f);
    }

    Player loaded;
    std::vector<Quest> quests;
    ZoneID zone = ZoneID::LARuins;
    REQUIRE(SaveManager::load(loaded, quests, zone, slot));

    CHECK(loaded.equippedWeapon.id == EDB::pistolaPlas().id);
    CHECK(loaded.equippedArmor.id   == EDB::coleteMilitar().id);
    CHECK(loaded.equippedImplant.isEmpty());
    CHECK(loaded.level == 3);
    CHECK(loaded.credits == 500);

    SaveManager::deleteSave(slot);
}

TEST_CASE("SaveManager - valores de enum fora da faixa sao clampados") {
    const int slot = 2;
    SaveManager::deleteSave(slot);

    {
        FILE* f = fopen("saves/darknet_slot2.txt", "w");
        REQUIRE(f != nullptr);
        fprintf(f, "DARKNET_SAVE_V6\nslot 2\nsaveDate 2026-09-05 10:00:00\n");
        fprintf(f, "posX 10.0\nposY 20.0\nhealth 80.0\nmaxHealth 100.0\n");
        fprintf(f, "attackDamage 15.0\nattackRange 90.0\nspeed 250.0\ndefense 0.0\n");
        fprintf(f, "charClass 99\nlevel 1\nxp 0\nxpToNext 100\ncredits 0\nzone 9999\n");
        fprintf(f, "evolutionPath -5\nevolutionTier 0\nskillPoints 0\nperkMask 0\n");
        fprintf(f, "weaponId none\narmorId none\nimplantId none\n");
        fprintf(f, "questCount 0\ninventory 0\n");
        fclose(f);
    }

    Player loaded;
    std::vector<Quest> quests;
    ZoneID zone = ZoneID::LARuins;
    REQUIRE(SaveManager::load(loaded, quests, zone, slot));

    CHECK(static_cast<int>(zone) <= static_cast<int>(ZoneID::InfernoZone));
    CHECK(loaded.evolutionPath == EvolutionPath::None);
    CHECK(static_cast<int>(loaded.getCharClass()) < static_cast<int>(CharacterClass::COUNT));

    SaveManager::deleteSave(slot);
    CHECK_FALSE(SaveManager::hasSave(slot));
}

TEST_CASE("SaveManager - slot invalido nao cria arquivo estranho") {
    SaveManager::deleteSave(-1);
    SaveManager::deleteSave(999);
    // Nao deve existir arquivo com slot -1 ou 999.
    CHECK_FALSE(SaveManager::hasSave(-1));
    CHECK_FALSE(SaveManager::hasSave(999));
}

// ── Projectile ───────────────────────────────────────────────────────────────

TEST_CASE("Projectile - move e atinge alcance maximo") {
    Projectile p({0, 0}, {1, 0}, 10.0f, 100.0f, 50.0f, RED, false);
    CHECK(p.active);
    CHECK(p.velocity.x == doctest::Approx(50.0f));
    CHECK(p.velocity.y == doctest::Approx(0.0f));

    p.update(1.0f);          // move 50 unidades
    CHECK(p.position.x == doctest::Approx(50.0f));
    CHECK_FALSE(p.isOutOfRange());

    p.update(1.0f);          // mais 50 = 100 = alcance maximo
    CHECK(p.position.x == doctest::Approx(100.0f));
    CHECK(p.isOutOfRange());
}

TEST_CASE("Projectile - direcao zero fica inativo") {
    Projectile p({0, 0}, {0, 0}, 10.0f, 100.0f, 50.0f, RED, false);
    CHECK_FALSE(p.active);
}

// ── Player: regras puras de stats ────────────────────────────────────────────

TEST_CASE("Player - applyClass define stats base distintos") {
    Player soldado;
    soldado.applyClass(CharacterClass::Soldado);
    CHECK(soldado.getCharClass() == CharacterClass::Soldado);
    float soldadoHP = soldado.maxHealth;

    Player mago;
    mago.applyClass(CharacterClass::Mago);
    CHECK(mago.maxHealth < soldadoHP);          // mago tem menos vida
    CHECK(mago.attackRange > soldado.attackRange); // mago tem mais alcance
}

TEST_CASE("Player - takeDamage respeita defesa e clampa em zero") {
    Player p;
    p.applyClass(CharacterClass::Soldado);
    float hp = p.health;
    p.defense = 10.0f;
    p.takeDamage(50.0f);   // reducao de 10% -> 45 de dano efetivo
    CHECK(p.health == doctest::Approx(hp - 45.0f));

    p.takeDamage(9999.0f); // overkill
    CHECK(p.health == doctest::Approx(0.0f));
    CHECK(p.health <= 0.0f);
}

TEST_CASE("Player - heal nao ultrapassa maxHealth") {
    Player p;
    p.applyClass(CharacterClass::Soldado);
    p.health = 10.0f;
    p.heal(500.0f);
    CHECK(p.health == doctest::Approx(p.maxHealth));
}

TEST_CASE("Player - addXP sobe de nivel") {
    Player p;
    p.applyClass(CharacterClass::Soldado);
    int startLevel = p.level;
    p.addXP(150);          // suficiente para subir pelo menos 1 nivel
    CHECK(p.level > startLevel);
}

TEST_CASE("Player - equipItem aplica stats") {
    Player p;
    p.applyClass(CharacterClass::Soldado);
    float baseDmg = p.attackDamage;
    p.equipItem(EDB::rifleEnergia());
    CHECK(p.equippedWeapon.id == EDB::rifleEnergia().id);
    CHECK(p.attackDamage > baseDmg);
}

TEST_CASE("Player - usePotion cura e ativa cooldown") {
    Player p;
    p.applyClass(CharacterClass::Soldado);
    p.health = 10.0f;
    p.usePotion();
    CHECK(p.health > 10.0f);
    CHECK_FALSE(p.potionReady());
}

TEST_CASE("Player - equipFromBag devolve item com slot invalido") {
    Player p;
    p.applyClass(CharacterClass::Soldado);
    Equipment bug = EDB::pistolaPlas();
    bug.slot = EquipSlot::None;   // simula item corrompido/invalido
    p.equipBag.push_back(bug);
    size_t before = p.equipBag.size();
    p.equipFromBag(0);
    CHECK(p.equipBag.size() == before);   // nao perdeu o item
}

// ── Tilemap ──────────────────────────────────────────────────────────────────

TEST_CASE("Tilemap - generate cria dimensoes corretas e paredes") {
    Tilemap tm;
    tm.generate(ZoneID::LARuins);
    CHECK(tm.width == 40);
    CHECK(tm.height == 40);
    REQUIRE(tm.tiles.size() == static_cast<size_t>(tm.height));
    REQUIRE(tm.tiles[0].size() == static_cast<size_t>(tm.width));

    int walls = 0;
    for (int y = 0; y < tm.height; ++y)
        for (int x = 0; x < tm.width; ++x)
            if (tm.isWall(x, y)) ++walls;
    CHECK(walls > 0);
}

TEST_CASE("Tilemap - isWallAtPosition dentro e fora dos limites") {
    Tilemap tm;
    tm.generate(ZoneID::LARuins);
    Vector2 inside = { tm.tileSize * 2.0f, tm.tileSize * 2.0f };
    // nao garante que seja floor, mas nao deve crashar
    bool r = tm.isWallAtPosition(inside);
    (void)r;

    // Mapa fechado: fora dos limites e considerado parede (nao pode sair).
    Vector2 outside = { -1000.0f, -1000.0f };
    CHECK(tm.isWallAtPosition(outside));

    // Mundo aberto: fora dos limites e chao livre.
    Tilemap ow;
    ow.generateOpenWorld();
    CHECK_FALSE(ow.isWallAtPosition(outside));
}

TEST_CASE("Tilemap - generateOpenWorld cria layout 3x3") {
    Tilemap tm;
    tm.generateOpenWorld();
    CHECK(tm.openWorld);
    CHECK(tm.width == Tilemap::OW_ZONE_W * Tilemap::OW_COLS);
    CHECK(tm.height == Tilemap::OW_ZONE_H * Tilemap::OW_ROWS);
}
