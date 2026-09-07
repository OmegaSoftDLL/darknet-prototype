#include "SaveManager.h"
#include "SkillTree.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <cstdlib>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#undef DrawText
#undef DrawTextEx
#endif
#include <raylib.h>

// ─── Helpers ─────────────────────────────────────────────────────────────────

static bool isValidSlot(int slot) { return slot >= 0 && slot < SAVE_SLOTS; }

static uint32_t checksumPayload(const std::string& s) {
    // FNV-1a 32-bit — suficiente to detectar edicao/corrosao accidental.
    uint32_t h = 0x811c9dc5u;
    for (char c : s) {
        h ^= static_cast<uint8_t>(c);
        h *= 0x01000193u;
    }
    return h;
}

static int parseVersionHeader(const char* header) {
    int v = 0;
    if (std::sscanf(header, "DARKNET_SAVE_V%d", &v) == 1) return v;
    return 0;
}

static ZoneID clampZone(int v) {
    const int max = static_cast<int>(ZoneID::InfernoZone);
    if (v < 0) return ZoneID::LARuins;
    if (v > max) return ZoneID::InfernoZone;
    return static_cast<ZoneID>(v);
}
static EvolutionPath clampEvolutionPath(int v) {
    const int max = static_cast<int>(EvolutionPath::ExecutorOmega);
    if (v < 0) return EvolutionPath::None;
    if (v > max) return EvolutionPath::ExecutorOmega;
    return static_cast<EvolutionPath>(v);
}
static CharacterClass clampCharacterClass(int v) {
    const int max = static_cast<int>(CharacterClass::COUNT) - 1;
    if (v < 0) return CharacterClass::Soldado;
    if (v > max) return static_cast<CharacterClass>(max);
    return static_cast<CharacterClass>(v);
}

void SaveManager::ensureSavesDir() {
#ifdef _WIN32
    CreateDirectoryA("saves", NULL);
#endif
}

std::string SaveManager::slotPath(int slot) {
    if (!isValidSlot(slot)) return std::string("saves/darknet_slot_INVALID.txt");
    return std::string("saves/darknet_slot") + std::to_string(slot) + ".txt";
}

// Consome until the end of the line ATUAL (inclusive the \n) without tocar in the next.
// O old `fscanf(f," %255[^\n]",val)` had um ESPACO in the formato: ele pulava the
// \n and engolia the ROW SEGUINTE whole all vez that caia numa chave desconhecida.
static void skipRestOfLine(FILE* f) {
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n') {}
}

// ─── Equipment resolution ─────────────────────────────────────────────────────

// LEGADO (saves V4 and anteriores): equipment era saved pelo NOME DE EXIBICAO —
// renomear um item quebrava saves antigos. Mantido only as fallback of load.
static Equipment resolveEquipByName(const char* name) {
    if (strcmp(name,"none")==0) return {};
    if (strcmp(name,"Pistola Plasma")==0)     return EDB::pistolaPlas();
    if (strcmp(name,"Rifle of Energia")==0)   return EDB::rifleEnergia();
    if (strcmp(name,"Canhao EMP")==0)         return EDB::canhaoEMP();
    if (strcmp(name,"Colete Militar")==0)     return EDB::coleteMilitar();
    if (strcmp(name,"Armor Avancada")==0)  return EDB::armaduraAvan();
    if (strcmp(name,"Exoesqueleto Titan")==0) return EDB::exoesqueleto();
    if (strcmp(name,"Chip of Speed")==0) return EDB::chipVel();
    if (strcmp(name,"Neural Link")==0)        return EDB::neuralLink();
    if (strcmp(name,"Quantum Core")==0)       return EDB::quantumCore();
    return {};
}

// Formato current (V5): resolve pelo ID stable. If the ID not consta in the catalog
// (save old editado, ou equipment craftado saved pelo nome), tenta the nome.
static Equipment resolveEquipById(const char* id) {
    if (strcmp(id,"none")==0) return {};
    Equipment eq = EDB::byId(id);
    if (!eq.isEmpty()) return eq;
    return resolveEquipByName(id);
}

// O that goes for the save: the ID stable; if the equipment not has ID (craftado
// outside the catalog EDB), saves the nome — the load falls in the fallback legacy.
static const char* equipSaveToken(const Equipment& eq) {
    if (eq.isEmpty()) return "none";
    return eq.id.empty() ? eq.name.c_str() : eq.id.c_str();
}

// ─── Save ────────────────────────────────────────────────────────────────────

void SaveManager::save(const Player& player, const std::vector<Quest>& quests, ZoneID zone,
                       int slot, float playMinutes, int totalKills,
                       int totalDeaths, int bossesKilled, int portalsSealed,
                       int difficultyLevel, int gameTotalKills,
                       const std::vector<std::string>* buildingLines) {
    ensureSavesDir();
    std::string path = slotPath(slot);

    // Build payload in memory only we can checksum it and write atomically.
    std::ostringstream out;

    out << "slot " << slot << "\n";

    // Timestamp
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M:%S", tm_info);
    out << "saveDate " << dateBuf << "\n";

    // Player core
    out << "posX "         << player.position.x   << "\n";
    out << "posY "         << player.position.y   << "\n";
    out << "health "       << player.health       << "\n";
    out << "maxHealth "    << player.maxHealth    << "\n";
    out << "attackDamage " << player.attackDamage << "\n";
    out << "attackRange "  << player.attackRange  << "\n";
    out << "speed "        << player.speed        << "\n";
    out << "defense "      << player.defense      << "\n";
    // Class + stats BASE (efetivos sao recalculados; without isso the "continue" quebra)
    out << "charClass "        << (int)player.getCharClass()        << "\n";
    out << "baseMaxHealth "    << player.getBaseMaxHealth()         << "\n";
    out << "baseAttackDamage " << player.getBaseAttackDamage()      << "\n";
    out << "baseSpeed "        << player.getBaseSpeed()             << "\n";
    out << "baseAttackRange "  << player.getBaseAttackRange()       << "\n";
    out << "baseDefense "      << player.getBaseDefense()           << "\n";
    out << "level "        << player.level        << "\n";
    out << "xp "           << player.xp           << "\n";
    out << "xpToNext "     << player.xpToNextLevel << "\n";
    out << "credits "      << player.credits      << "\n";
    out << "zone "         << static_cast<int>(zone) << "\n";

    // Evolution
    out << "evolutionPath " << static_cast<int>(player.evolutionPath) << "\n";
    out << "evolutionTier " << player.evolutionTier << "\n";

    // Hack Tree (skill tree of perks)
    out << "skillPoints " << player.skillPoints << "\n";
    out << "perkMask "    << player.perkMask    << "\n";

    // Stats
    out << "totalKills "    << (totalKills > 0 ? totalKills : player.totalKills) << "\n";
    out << "gameTotalKills "<< gameTotalKills << "\n";
    out << "totalDeaths "   << totalDeaths    << "\n";
    out << "bossesKilled "  << bossesKilled   << "\n";
    out << "portalsSealed " << portalsSealed  << "\n";
    out << "difficultyLevel " << difficultyLevel << "\n";
    out << "playMinutes "   << playMinutes    << "\n";

    // Equipment (V7: ID + upgrade + primary/secondary; V5/V6 liam only the ID)
    auto writeEquip = [&](const char* idKey, const char* upKey, const char* priKey, const char* secKey, const Equipment& eq) {
        out << idKey << " " << equipSaveToken(eq) << "\n";
        out << upKey << " " << eq.upgradeLevel    << "\n";
        out << priKey << " " << eq.primary        << "\n";
        out << secKey << " " << eq.secondary      << "\n";
    };
    writeEquip("weaponId", "weaponUpgrade", "weaponPrimary", "weaponSecondary", player.equippedWeapon);
    writeEquip("armorId",  "armorUpgrade",  "armorPrimary",  "armorSecondary",  player.equippedArmor);
    writeEquip("implantId","implantUpgrade","implantPrimary","implantSecondary",player.equippedImplant);

    // EquipBag (V7)
    out << "equipBagCount " << player.equipBag.size() << "\n";
    for (const auto& eq : player.equipBag)
        out << "equipBag " << equipSaveToken(eq) << " " << eq.upgradeLevel << " "
            << eq.primary << " " << eq.secondary << " " << (int)eq.slot << "\n";

    // Quests
    out << "questCount " << quests.size() << "\n";
    for (const auto& q : quests)
        out << "quest " << q.id << " " << q.current << " "
            << (q.completed?1:0) << " " << (q.rewardGiven?1:0) << "\n";

    // Inventory (V5/V6: only type; V7: full state)
    out << "inventoryV2 " << player.inventory.size() << "\n";
    for (const auto& item : player.inventory) {
        out << "invItm " << (int)item.type << " " << (int)item.rarity << " " << item.value << " "
            << item.bonusDamage << " " << item.bonusHealth << " " << item.bonusSpeed << " "
            << item.bonusDefense << " " << item.bonusCrit << " " << item.bonusVampirism << "\n";
        out << "invPrefix "  << item.affixPrefix  << "\n";
        out << "invSuffix "  << item.affixSuffix  << "\n";
        out << "invBaseName "<< item.baseName     << "\n";
    }

    // BuildingSystem (V7+)
    if (buildingLines) {
        out << "buildingLineCount " << buildingLines->size() << "\n";
        for (const auto& line : *buildingLines)
            out << "bdg " << line << "\n";
    }

    std::string payload = out.str();
    uint32_t chk = checksumPayload(payload);

    // Atomic write: temp file then rename.
    std::string tmpPath = path + ".tmp";
    FILE* f = fopen(tmpPath.c_str(), "w");
    if (!f) return;
    fprintf(f, "DARKNET_SAVE_V%d\n", SAVE_VERSION);
    fprintf(f, "checksum %08x\n", chk);
    fwrite(payload.c_str(), 1, payload.size(), f);
    fclose(f);
    std::remove(path.c_str());
    std::rename(tmpPath.c_str(), path.c_str());
}

// ─── Load ────────────────────────────────────────────────────────────────────

bool SaveManager::load(Player& player, std::vector<Quest>& quests, ZoneID& zone, int slot,
                       int* gameTotalKillsOut,
                       std::vector<std::string>* buildingLinesOut) {
    std::string path = slotPath(slot);
    FILE* f = fopen(path.c_str(), "r");

    // Fallback to legacy single file for slot 0
    if (!f && slot == 0) {
        f = fopen(saveFile, "r");
    }
    if (!f) return false;

    // Read and validate header version.
    char header[64] = {};
    if (!fgets(header, sizeof(header), f)) { fclose(f); return false; }
    int version = parseVersionHeader(header);
    if (version == 0 || version > SAVE_VERSION || version < 4) {
        fclose(f);
        return false;   // unknown, future or too-old format
    }

    // Optional checksum line (V7+). If present, validate the payload.
    long payloadOffset = ftell(f);
    uint32_t expectedChecksum = 0;
    bool hasChecksum = false;
    char line[64] = {};
    if (fgets(line, sizeof(line), f)) {
        if (std::strncmp(line, "checksum ", 9) == 0) {
            expectedChecksum = static_cast<uint32_t>(std::strtoul(line + 9, nullptr, 16));
            hasChecksum = true;
            payloadOffset = ftell(f);
        }
    }

    int  slotRead = 0, zoneInt = 0;
    char dateBuf[256] = {};

    // Try to parse new format first
    char key[64];   // (the old `val` sumiu junto with the fscanf that engolia the line seguinte)
    bool hasCredits = false, hasEvolution = false;
    int   savedClass = -1;
    float bMax = 0, bDmg = 0, bSpd = 0, bRng = 0, bDef = 0;
    int   loadedGameTotalKills = 0;
    Equipment loadedWeapon, loadedArmor, loadedImplant;
    int   equipBagRemaining = 0;
    int   inventoryV2Remaining = 0;
    int   buildingLinesRemaining = 0;
    Item  inventoryV2Item;

    // Limpa listas to evitar contaminacao of state previous (P0)
    player.equipBag.clear();
    player.inventory.clear();
    if (buildingLinesOut) buildingLinesOut->clear();
    loadedWeapon = loadedArmor = loadedImplant = Equipment{};

    // Parse payload from after the header (and optional checksum line).
    std::fseek(f, payloadOffset, SEEK_SET);

    while (fscanf(f, " %63s", key) == 1) {
        if (strcmp(key,"slot")==0)          { fscanf(f," %d",&slotRead); }
        else if (strcmp(key,"saveDate")==0) { fscanf(f," %255[^\n]",dateBuf); }
        else if (strcmp(key,"posX")==0)     { fscanf(f," %f",&player.position.x); }
        else if (strcmp(key,"posY")==0)     { fscanf(f," %f",&player.position.y); }
        else if (strcmp(key,"health")==0)   { fscanf(f," %f",&player.health); }
        else if (strcmp(key,"maxHealth")==0){ fscanf(f," %f",&player.maxHealth); }
        else if (strcmp(key,"attackDamage")==0){ fscanf(f," %f",&player.attackDamage); }
        else if (strcmp(key,"attackRange")==0) { fscanf(f," %f",&player.attackRange); }
        else if (strcmp(key,"speed")==0)    { fscanf(f," %f",&player.speed); }
        else if (strcmp(key,"defense")==0)  { fscanf(f," %f",&player.defense); }
        else if (strcmp(key,"charClass")==0)       { fscanf(f," %d",&savedClass); }
        else if (strcmp(key,"baseMaxHealth")==0)   { fscanf(f," %f",&bMax); }
        else if (strcmp(key,"baseAttackDamage")==0){ fscanf(f," %f",&bDmg); }
        else if (strcmp(key,"baseSpeed")==0)       { fscanf(f," %f",&bSpd); }
        else if (strcmp(key,"baseAttackRange")==0) { fscanf(f," %f",&bRng); }
        else if (strcmp(key,"baseDefense")==0)     { fscanf(f," %f",&bDef); }
        else if (strcmp(key,"level")==0)    { fscanf(f," %d",&player.level); }
        else if (strcmp(key,"xp")==0)       { fscanf(f," %d",&player.xp); }
        else if (strcmp(key,"xpToNext")==0) { fscanf(f," %d",&player.xpToNextLevel); }
        else if (strcmp(key,"credits")==0)  { fscanf(f," %d",&player.credits); hasCredits=true; }
        else if (strcmp(key,"totalKills")==0){ fscanf(f," %d",&player.totalKills); }
        else if (strcmp(key,"gameTotalKills")==0){ fscanf(f," %d",&loadedGameTotalKills); }
        else if (strcmp(key,"zone")==0)     { fscanf(f," %d",&zoneInt); zone=clampZone(zoneInt); }
        else if (strcmp(key,"evolutionPath")==0){ int ep=0; fscanf(f," %d",&ep); player.evolutionPath=clampEvolutionPath(ep); hasEvolution=true; }
        else if (strcmp(key,"evolutionTier")==0){ fscanf(f," %d",&player.evolutionTier); }
        else if (strcmp(key,"skillPoints")==0){
            int sp = 0; fscanf(f," %d",&sp);
            if (sp < 0) sp = 0;
            if (sp > 4096) sp = 4096;
            player.skillPoints = sp;
        }
        else if (strcmp(key,"perkMask")==0){
            int pm = 0; fscanf(f," %d",&pm);
            if (pm < 0) pm = 0;
            if (pm > (1 << SkillTree::PERK_COUNT) - 1) pm = (1 << SkillTree::PERK_COUNT) - 1;
            player.perkMask = static_cast<uint32_t>(pm);
        }
        else if (strcmp(key,"weaponId")==0){
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);
            loadedWeapon = resolveEquipById(buf);
        }
        else if (strcmp(key,"armorId")==0){
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);
            loadedArmor = resolveEquipById(buf);
        }
        else if (strcmp(key,"implantId")==0){
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);
            loadedImplant = resolveEquipById(buf);
        }
        else if (strcmp(key,"weaponUpgrade")==0){ fscanf(f," %d",&loadedWeapon.upgradeLevel); }
        else if (strcmp(key,"weaponPrimary")==0){ fscanf(f," %f",&loadedWeapon.primary); }
        else if (strcmp(key,"weaponSecondary")==0){ fscanf(f," %f",&loadedWeapon.secondary); }
        else if (strcmp(key,"armorUpgrade")==0){ fscanf(f," %d",&loadedArmor.upgradeLevel); }
        else if (strcmp(key,"armorPrimary")==0){ fscanf(f," %f",&loadedArmor.primary); }
        else if (strcmp(key,"armorSecondary")==0){ fscanf(f," %f",&loadedArmor.secondary); }
        else if (strcmp(key,"implantUpgrade")==0){ fscanf(f," %d",&loadedImplant.upgradeLevel); }
        else if (strcmp(key,"implantPrimary")==0){ fscanf(f," %f",&loadedImplant.primary); }
        else if (strcmp(key,"implantSecondary")==0){ fscanf(f," %f",&loadedImplant.secondary); }
        else if (strcmp(key,"weaponName")==0){   // legacy V4: nome of display
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);   // nomes has ESPACO ("Pistola Plasma")
            loadedWeapon = resolveEquipByName(buf);
        }
        else if (strcmp(key,"armorName")==0){   // legacy V4
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);   // nomes has ESPACO ("Pistola Plasma")
            loadedArmor = resolveEquipByName(buf);
        }
        else if (strcmp(key,"implantName")==0){   // legacy V4
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);   // nomes has ESPACO ("Pistola Plasma")
            loadedImplant = resolveEquipByName(buf);
        }
        else if (strcmp(key,"equipBagCount")==0){
            fscanf(f," %d",&equipBagRemaining);
            if (equipBagRemaining < 0) equipBagRemaining = 0;
            if (equipBagRemaining > 4096) equipBagRemaining = 4096;
            player.equipBag.clear();
        }
        else if (strcmp(key,"equipBag")==0 && equipBagRemaining > 0){
            char buf[128] = {}; int up=0, slotInt=0; float pri=0, sec=0;
            fscanf(f," %127s %d %f %f %d",buf,&up,&pri,&sec,&slotInt);
            Equipment eq = resolveEquipById(buf);
            if (eq.isEmpty()) eq = resolveEquipByName(buf);
            if (!eq.isEmpty()) {
                eq.upgradeLevel = up;
                eq.primary = pri;
                eq.secondary = sec;
                eq.slot = static_cast<EquipSlot>(slotInt);
                player.equipBag.push_back(eq);
            }
            --equipBagRemaining;
        }
        else if (strcmp(key,"questCount")==0){
            int qc=0; fscanf(f," %d",&qc);
            // Save and text puro and editavel: without ceiling, um `questCount 2000000000`
            // travava the game num loop of bilhoes of iteracoes.
            if (qc < 0) qc = 0;
            if (qc > 4096) qc = 4096;
            for (int i=0;i<qc;++i) {
                char qid[64]; int cur=0,comp=0,rew=0;
                fscanf(f," quest %63s %d %d %d",qid,&cur,&comp,&rew);
                for (auto& q : quests) {
                    if (q.id==std::string(qid)) {
                        q.current=cur; q.completed=comp!=0; q.rewardGiven=rew!=0; break;
                    }
                }
            }
        }
        else if (strcmp(key,"inventory")==0){
            int invSz=0; fscanf(f," %d",&invSz);
            if (invSz < 0) invSz = 0;
            if (invSz > 4096) invSz = 4096;   // idem: ceiling contra save corrompido
            player.inventory.clear();
            for (int i=0;i<invSz;++i) {
                int type=0; fscanf(f," %d",&type);
                // enum outside of range vira lixo in the switch of render/uso
                if (type < 0 || type > (int)ItemType::DragonSlayer) continue;
                Item item = Item::createRandom(player.position);
                item.type = static_cast<ItemType>(type);
                item.pickedUp = true;
                player.inventory.push_back(item);
            }
        }
        else if (strcmp(key,"inventoryV2")==0){
            fscanf(f," %d",&inventoryV2Remaining);
            if (inventoryV2Remaining < 0) inventoryV2Remaining = 0;
            if (inventoryV2Remaining > 4096) inventoryV2Remaining = 4096;
            player.inventory.clear();
        }
        else if (strcmp(key,"invItm")==0 && inventoryV2Remaining > 0){
            int type=0, rarity=0, value=0;
            float bd=0,bh=0,bs=0,bdef=0,bc=0,bv=0;
            fscanf(f," %d %d %d %f %f %f %f %f %f",
                   &type,&rarity,&value,&bd,&bh,&bs,&bdef,&bc,&bv);
            inventoryV2Item = Item::createRandom(player.position);
            if (type >= 0 && type <= (int)ItemType::DragonSlayer)
                inventoryV2Item.type = static_cast<ItemType>(type);
            if (rarity >= 0 && rarity <= (int)ItemRarity::Omega)
                inventoryV2Item.rarity = static_cast<ItemRarity>(rarity);
            inventoryV2Item.value = value;
            inventoryV2Item.bonusDamage = bd;
            inventoryV2Item.bonusHealth = bh;
            inventoryV2Item.bonusSpeed = bs;
            inventoryV2Item.bonusDefense = bdef;
            inventoryV2Item.bonusCrit = bc;
            inventoryV2Item.bonusVampirism = bv;
            inventoryV2Item.pickedUp = true;
        }
        else if (strcmp(key,"invPrefix")==0 && inventoryV2Remaining > 0){
            char buf[128] = {}; fscanf(f,"%127[^\n]",buf);
            if (buf[0]==' ') inventoryV2Item.affixPrefix = buf+1;
            else inventoryV2Item.affixPrefix = buf;
        }
        else if (strcmp(key,"invSuffix")==0 && inventoryV2Remaining > 0){
            char buf[128] = {}; fscanf(f,"%127[^\n]",buf);
            if (buf[0]==' ') inventoryV2Item.affixSuffix = buf+1;
            else inventoryV2Item.affixSuffix = buf;
        }
        else if (strcmp(key,"invBaseName")==0 && inventoryV2Remaining > 0){
            char buf[128] = {}; fscanf(f,"%127[^\n]",buf);
            if (buf[0]==' ') inventoryV2Item.baseName = buf+1;
            else inventoryV2Item.baseName = buf;
            player.inventory.push_back(inventoryV2Item);
            --inventoryV2Remaining;
        }
        else if (strcmp(key,"buildingLineCount")==0){
            fscanf(f," %d",&buildingLinesRemaining);
            if (buildingLinesRemaining < 0) buildingLinesRemaining = 0;
            if (buildingLinesRemaining > 8192) buildingLinesRemaining = 8192;
            if (buildingLinesOut) buildingLinesOut->clear();
        }
        else if (strcmp(key,"bdg")==0 && buildingLinesRemaining > 0){
            char buf[1024] = {}; fscanf(f," %1023[^\n]",buf);
            if (buildingLinesOut) {
                if (buf[0]==' ') buildingLinesOut->push_back(buf+1);
                else buildingLinesOut->push_back(buf);
            }
            --buildingLinesRemaining;
        }
        else {
            skipRestOfLine(f);   // pula only the resto DESTA line (the formato old with espaco engolia the PROXIMA)
        }
    }

    // Validate checksum if present (V7+). Corrupted/tampered payload is rejected.
    if (hasChecksum) {
        std::fseek(f, payloadOffset, SEEK_SET);
        uint32_t computed = 0x811c9dc5u;
        int c;
        while ((c = std::fgetc(f)) != EOF) {
            computed ^= static_cast<uint32_t>(static_cast<uint8_t>(c));
            computed *= 0x01000193u;
        }
        if (computed != expectedChecksum) {
            fclose(f);
            return false;
        }
    }

    fclose(f);

    // Restaura CLASS + stats BASE (with the gear already equipado above), recalculando the
    // efetivos corretamente — without isso the "continue" voltava as Soldier level-base.
    if (savedClass >= 0) {
        float keepHealth = player.health;
        player.loadSavedProgress(clampCharacterClass(savedClass), bMax, bDmg, bSpd, bRng, bDef);
        player.health = (keepHealth > 0.0f && keepHealth <= player.maxHealth) ? keepHealth : player.maxHealth;
    }
    // Equipa gear loaded (V7: with upgrade/primary/secondary; V5/V6: only ID)
    if (!loadedWeapon.isEmpty())  player.equipItem(loadedWeapon);
    if (!loadedArmor.isEmpty())   player.equipItem(loadedArmor);
    if (!loadedImplant.isEmpty()) player.equipItem(loadedImplant);
    player.refreshSkillVectors();   // perks carregados: reaplica mods of skill

    if (gameTotalKillsOut) *gameTotalKillsOut = loadedGameTotalKills;
    return true;
}

// ─── Slot utilities ───────────────────────────────────────────────────────────

bool SaveManager::hasSave(int slot) {
    std::string path = slotPath(slot);
    FILE* f = fopen(path.c_str(),"r");
    if (f) { fclose(f); return true; }
    if (slot == 0) return exists();
    return false;
}

void SaveManager::deleteSave(int slot) {
    if (!isValidSlot(slot)) return;
    std::string path = slotPath(slot);
    remove(path.c_str());
}

SaveSlotInfo SaveManager::getSlotInfo(int slot) {
    SaveSlotInfo info;
    info.slot = slot;
    std::string path = slotPath(slot);
    FILE* f = fopen(path.c_str(),"r");
    if (slot == 0 && !f) f = fopen(saveFile,"r");
    if (!f) { info.exists=false; return info; }
    info.exists = true;

    char header[64]={};
    fgets(header, sizeof(header), f);
    char key[64], val[256];
    while (fscanf(f," %63s",key)==1) {
        if      (strcmp(key,"level")==0)       fscanf(f," %d",&info.playerLevel);
        else if (strcmp(key,"totalKills")==0)  fscanf(f," %d",&info.totalKills);
        else if (strcmp(key,"zone")==0)        fscanf(f," %d",&info.currentZone);
        else if (strcmp(key,"playMinutes")==0) fscanf(f," %f",&info.playMinutes);
        else if (strcmp(key,"saveDate")==0)    { fscanf(f," %255[^\n]",val); info.saveDate=val; }
        else skipRestOfLine(f);
    }
    fclose(f);
    return info;
}

void SaveManager::renderSaveSlots(int screenW, int screenH, int highlightSlot) {
    Color neon = {0,235,255,255};
    Color amber = {255,180,40,255};
    int panW=540, panH=300;
    int panX=(screenW-panW)/2, panY=(screenH-panH)/2;

    // Painel chanfrado (cyberpunk)
    DrawRectangle(panX,panY,panW,panH,ColorAlpha(Color{7,13,28,255},0.97f));
    int cc=10;
    DrawLine(panX+cc,panY,     panX+panW-cc,panY,     neon);
    DrawLine(panX,   panY+cc,  panX,   panY+panH-cc,  neon);
    DrawLine(panX+cc,panY+panH,panX+panW-cc,panY+panH, neon);
    DrawLine(panX+panW,panY+cc,panX+panW,panY+panH-cc,neon);
    DrawLine(panX,   panY+cc,  panX+cc,panY,     neon);
    DrawLine(panX+panW-cc,panY,panX+panW,panY+cc,neon);
    DrawLine(panX,   panY+panH-cc,panX+cc,panY+panH, neon);
    DrawLine(panX+panW-cc,panY+panH,panX+panW,panY+panH-cc, neon);
    // Topo pulsante
    DrawRectangle(panX,panY,panW,2,ColorAlpha(neon,0.6f+0.4f*std::sin((float)GetTime()*1.7f)));

    const char* t = "SELECT SAVE";
    int tFont=20, tw=MeasureText(t,tFont);
    DrawText(t, panX+(panW-tw)/2, panY+12, tFont, ColorAlpha(neon,0.95f));
    DrawLine(panX+14,panY+40,panX+panW-14,panY+40,ColorAlpha(neon,0.35f));
    DrawRectangle(panX+panW/2-3,panY+37,6,6,ColorAlpha(amber,0.9f));

    for (int s=0;s<SAVE_SLOTS;++s) {
        SaveSlotInfo info = getSlotInfo(s);
        int sy = panY + 50 + s*74;
        bool sel = (s==highlightSlot);

        Color bg = sel ? Color{10,24,46,255} : Color{9,15,32,255};
        DrawRectangle(panX+12, sy, panW-24, 66, ColorAlpha(bg, sel?0.94f:0.72f));
        DrawRectangle(panX+12, sy, 3, 66, ColorAlpha(sel?amber:neon, sel?1.0f:0.4f));
        int c2=6;
        Color brd = sel ? neon : ColorAlpha(neon,0.28f);
        DrawLine(panX+12+c2,sy,     panX+panW-12-c2,sy,     brd);
        DrawLine(panX+12+c2,sy+66,  panX+panW-12-c2,sy+66,  brd);
        if (sel) {
            float p=0.5f+0.5f*std::sin((float)GetTime()*3.5f);
            DrawLine(panX+12,sy,  panX+12+c2,sy,    ColorAlpha(neon,p));
            DrawLine(panX+12,sy,  panX+12,sy+c2,    ColorAlpha(neon,p));
            DrawLine(panX+panW-12-c2,sy, panX+panW-12,sy, ColorAlpha(neon,p));
            DrawLine(panX+panW-12,sy,   panX+panW-12,sy+c2, ColorAlpha(neon,p));
            DrawLine(panX+12,sy+66-c2, panX+12,sy+66, ColorAlpha(neon,p));
            DrawLine(panX+12,sy+66, panX+12+c2,sy+66, ColorAlpha(neon,p));
            DrawLine(panX+panW-12,sy+66-c2, panX+panW-12,sy+66, ColorAlpha(neon,p));
            DrawLine(panX+panW-12-c2,sy+66, panX+panW-12,sy+66, ColorAlpha(neon,p));
        }

        DrawText(TextFormat("SLOT %d", s+1), panX+24, sy+7, 15,
                 sel ? Color{235,245,255,255} : ColorAlpha({170,190,215,255},0.85f));

        if (info.exists) {
            DrawText(TextFormat("Level %d  |  %d kills  |  %.0f min",
                     info.playerLevel, info.totalKills, info.playMinutes),
                     panX+24, sy+28, 12, ColorAlpha(neon,0.92f));
            DrawText(info.saveDate.c_str(), panX+24, sy+48, 11, ColorAlpha({140,165,195,255},0.6f));
        } else {
            DrawText("--- empty ---", panX+24, sy+30, 13, ColorAlpha({150,170,195,255},0.4f));
        }

        if (info.exists && sel)
            DrawText("[DEL] apagar", panX+panW-140, sy+48, 11, ColorAlpha({255,90,90,255},0.85f));
    }

    DrawText("[1/2/3] choose   [ENTER] load   [ESC] return",
             panX+12, panY+panH-24, 12, ColorAlpha({170,190,215,255},0.55f));
}

// ─── Legacy ──────────────────────────────────────────────────────────────────

bool SaveManager::exists() {
    FILE* f = fopen(saveFile,"r");
    if (f) { fclose(f); return true; }
    return false;
}
