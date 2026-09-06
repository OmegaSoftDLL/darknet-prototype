#include "SaveManager.h"
#include "SkillTree.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>
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

// Consome ate o fim da linha ATUAL (inclusive o \n) sem tocar na proxima.
// O antigo `fscanf(f," %255[^\n]",val)` tinha um ESPACO no formato: ele pulava o
// \n e engolia a LINHA SEGUINTE inteira toda vez que caia numa chave desconhecida.
static void skipRestOfLine(FILE* f) {
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n') {}
}

// ─── Equipment resolution ─────────────────────────────────────────────────────

// LEGADO (saves V4 e anteriores): equipamento era salvo pelo NOME DE EXIBICAO —
// renomear um item quebrava saves antigos. Mantido so como fallback de load.
static Equipment resolveEquipByName(const char* name) {
    if (strcmp(name,"none")==0) return {};
    if (strcmp(name,"Pistola Plasma")==0)     return EDB::pistolaPlas();
    if (strcmp(name,"Rifle de Energia")==0)   return EDB::rifleEnergia();
    if (strcmp(name,"Canhao EMP")==0)         return EDB::canhaoEMP();
    if (strcmp(name,"Colete Militar")==0)     return EDB::coleteMilitar();
    if (strcmp(name,"Armadura Avancada")==0)  return EDB::armaduraAvan();
    if (strcmp(name,"Exoesqueleto Titan")==0) return EDB::exoesqueleto();
    if (strcmp(name,"Chip de Velocidade")==0) return EDB::chipVel();
    if (strcmp(name,"Neural Link")==0)        return EDB::neuralLink();
    if (strcmp(name,"Quantum Core")==0)       return EDB::quantumCore();
    return {};
}

// Formato atual (V5): resolve pelo ID estavel. Se o ID nao consta no catalogo
// (save antigo editado, ou equipamento craftado salvo pelo nome), tenta o nome.
static Equipment resolveEquipById(const char* id) {
    if (strcmp(id,"none")==0) return {};
    Equipment eq = EDB::byId(id);
    if (!eq.isEmpty()) return eq;
    return resolveEquipByName(id);
}

// O que vai para o save: o ID estavel; se o equipamento nao tem ID (craftado
// fora do catalogo EDB), salva o nome — o load cai no fallback legado.
static const char* equipSaveToken(const Equipment& eq) {
    if (eq.isEmpty()) return "none";
    return eq.id.empty() ? eq.name.c_str() : eq.id.c_str();
}

// ─── Save ────────────────────────────────────────────────────────────────────

void SaveManager::save(const Player& player, const std::vector<Quest>& quests, ZoneID zone,
                       int slot, float playMinutes, int totalKills,
                       int totalDeaths, int bossesKilled, int portalsSealed,
                       int difficultyLevel, int gameTotalKills) {
    ensureSavesDir();
    std::string path = slotPath(slot);
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return;

    // Header
    fprintf(f, "DARKNET_SAVE_V%d\n", SAVE_VERSION);
    fprintf(f, "slot %d\n", slot);

    // Timestamp
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(f, "saveDate %s\n", dateBuf);

    // Player core
    fprintf(f, "posX %f\n",         player.position.x);
    fprintf(f, "posY %f\n",         player.position.y);
    fprintf(f, "health %f\n",       player.health);
    fprintf(f, "maxHealth %f\n",    player.maxHealth);
    fprintf(f, "attackDamage %f\n", player.attackDamage);
    fprintf(f, "attackRange %f\n",  player.attackRange);
    fprintf(f, "speed %f\n",        player.speed);
    fprintf(f, "defense %f\n",      player.defense);
    // Classe + stats BASE (efetivos sao recalculados; sem isso o "continuar" quebra)
    fprintf(f, "charClass %d\n",        (int)player.getCharClass());
    fprintf(f, "baseMaxHealth %f\n",    player.getBaseMaxHealth());
    fprintf(f, "baseAttackDamage %f\n", player.getBaseAttackDamage());
    fprintf(f, "baseSpeed %f\n",        player.getBaseSpeed());
    fprintf(f, "baseAttackRange %f\n",  player.getBaseAttackRange());
    fprintf(f, "baseDefense %f\n",      player.getBaseDefense());
    fprintf(f, "level %d\n",        player.level);
    fprintf(f, "xp %d\n",           player.xp);
    fprintf(f, "xpToNext %d\n",     player.xpToNextLevel);
    fprintf(f, "credits %d\n",      player.credits);
    fprintf(f, "zone %d\n",         static_cast<int>(zone));

    // Evolution
    fprintf(f, "evolutionPath %d\n", static_cast<int>(player.evolutionPath));
    fprintf(f, "evolutionTier %d\n", player.evolutionTier);

    // Hack Tree (skill tree de perks)
    fprintf(f, "skillPoints %d\n", player.skillPoints);
    fprintf(f, "perkMask %d\n",    player.perkMask);

    // Stats
    fprintf(f, "totalKills %d\n",    totalKills > 0 ? totalKills : player.totalKills);
    fprintf(f, "gameTotalKills %d\n", gameTotalKills);
    fprintf(f, "totalDeaths %d\n",   totalDeaths);
    fprintf(f, "bossesKilled %d\n",  bossesKilled);
    fprintf(f, "portalsSealed %d\n", portalsSealed);
    fprintf(f, "difficultyLevel %d\n", difficultyLevel);
    fprintf(f, "playMinutes %f\n",   playMinutes);

    // Equipment (V7: ID + upgrade + primary/secondary; V5/V6 liam so o ID)
    auto writeEquip = [&](const char* idKey, const char* upKey, const char* priKey, const char* secKey, const Equipment& eq) {
        fprintf(f, "%s %s\n", idKey, equipSaveToken(eq));
        fprintf(f, "%s %d\n", upKey, eq.upgradeLevel);
        fprintf(f, "%s %f\n", priKey, eq.primary);
        fprintf(f, "%s %f\n", secKey, eq.secondary);
    };
    writeEquip("weaponId", "weaponUpgrade", "weaponPrimary", "weaponSecondary", player.equippedWeapon);
    writeEquip("armorId",  "armorUpgrade",  "armorPrimary",  "armorSecondary",  player.equippedArmor);
    writeEquip("implantId","implantUpgrade","implantPrimary","implantSecondary",player.equippedImplant);

    // EquipBag (V7)
    fprintf(f, "equipBagCount %zu\n", player.equipBag.size());
    for (const auto& eq : player.equipBag)
        fprintf(f, "equipBag %s %d %f %f %d\n",
                equipSaveToken(eq), eq.upgradeLevel, eq.primary, eq.secondary, (int)eq.slot);

    // Quests
    fprintf(f, "questCount %d\n", (int)quests.size());
    for (const auto& q : quests)
        fprintf(f, "quest %s %d %d %d\n", q.id.c_str(), q.current, q.completed?1:0, q.rewardGiven?1:0);

    // Inventory (V5/V6: so type; V7: full state)
    fprintf(f, "inventoryV2 %zu\n", player.inventory.size());
    for (const auto& item : player.inventory) {
        fprintf(f, "invItm %d %d %d %f %f %f %f %f %f\n",
                (int)item.type, (int)item.rarity, item.value,
                item.bonusDamage, item.bonusHealth, item.bonusSpeed,
                item.bonusDefense, item.bonusCrit, item.bonusVampirism);
        fprintf(f, "invPrefix %s\n", item.affixPrefix.c_str());
        fprintf(f, "invSuffix %s\n", item.affixSuffix.c_str());
        fprintf(f, "invBaseName %s\n", item.baseName.c_str());
    }

    fclose(f);
}

// ─── Load ────────────────────────────────────────────────────────────────────

bool SaveManager::load(Player& player, std::vector<Quest>& quests, ZoneID& zone, int slot,
                       int* gameTotalKillsOut) {
    std::string path = slotPath(slot);
    FILE* f = fopen(path.c_str(), "r");

    // Fallback to legacy single file for slot 0
    if (!f && slot == 0) {
        f = fopen(saveFile, "r");
    }
    if (!f) return false;

    // Read header line
    char header[64] = {};
    fgets(header, sizeof(header), f);
    // Accept any DARKNET_SAVE_V* version

    int  slotRead = 0, zoneInt = 0;
    char dateBuf[256] = {};

    // Try to parse new format first
    char key[64];   // (o antigo `val` sumiu junto com o fscanf que engolia a linha seguinte)
    bool hasCredits = false, hasEvolution = false;
    int   savedClass = -1;
    float bMax = 0, bDmg = 0, bSpd = 0, bRng = 0, bDef = 0;
    int   loadedGameTotalKills = 0;
    Equipment loadedWeapon, loadedArmor, loadedImplant;
    int   equipBagRemaining = 0;
    int   inventoryV2Remaining = 0;
    Item  inventoryV2Item;

    // Limpa listas para evitar contaminacao de estado anterior (P0)
    player.equipBag.clear();
    player.inventory.clear();
    loadedWeapon = loadedArmor = loadedImplant = Equipment{};

    // Re-read from start for simple line-by-line parsing
    rewind(f);
    fgets(header, sizeof(header), f); // skip version line

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
        else if (strcmp(key,"weaponName")==0){   // legado V4: nome de exibicao
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);   // nomes tem ESPACO ("Pistola Plasma")
            loadedWeapon = resolveEquipByName(buf);
        }
        else if (strcmp(key,"armorName")==0){   // legado V4
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);   // nomes tem ESPACO ("Pistola Plasma")
            loadedArmor = resolveEquipByName(buf);
        }
        else if (strcmp(key,"implantName")==0){   // legado V4
            char buf[128] = {}; fscanf(f," %127[^\n]",buf);   // nomes tem ESPACO ("Pistola Plasma")
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
            // Save e texto puro e editavel: sem teto, um `questCount 2000000000`
            // travava o jogo num loop de bilhoes de iteracoes.
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
            if (invSz > 4096) invSz = 4096;   // idem: teto contra save corrompido
            player.inventory.clear();
            for (int i=0;i<invSz;++i) {
                int type=0; fscanf(f," %d",&type);
                // enum fora de faixa vira lixo nos switch de render/uso
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
        else {
            skipRestOfLine(f);   // pula so o resto DESTA linha (o formato antigo com espaco engolia a PROXIMA)
        }
    }

    fclose(f);

    // Restaura CLASSE + stats BASE (com o gear ja equipado acima), recalculando os
    // efetivos corretamente — sem isso o "continuar" voltava como Soldado nivel-base.
    if (savedClass >= 0) {
        float keepHealth = player.health;
        player.loadSavedProgress(clampCharacterClass(savedClass), bMax, bDmg, bSpd, bRng, bDef);
        player.health = (keepHealth > 0.0f && keepHealth <= player.maxHealth) ? keepHealth : player.maxHealth;
    }
    // Equipa gear carregado (V7: com upgrade/primary/secondary; V5/V6: so ID)
    if (!loadedWeapon.isEmpty())  player.equipItem(loadedWeapon);
    if (!loadedArmor.isEmpty())   player.equipItem(loadedArmor);
    if (!loadedImplant.isEmpty()) player.equipItem(loadedImplant);
    player.refreshSkillVectors();   // perks carregados: reaplica mods de skill

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

    const char* t = "SELECIONAR SAVE";
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
            DrawText("--- vazio ---", panX+24, sy+30, 13, ColorAlpha({150,170,195,255},0.4f));
        }

        if (info.exists && sel)
            DrawText("[DEL] apagar", panX+panW-140, sy+48, 11, ColorAlpha({255,90,90,255},0.85f));
    }

    DrawText("[1/2/3] escolher   [ENTER] carregar   [ESC] voltar",
             panX+12, panY+panH-24, 12, ColorAlpha({170,190,215,255},0.55f));
}

// ─── Legacy ──────────────────────────────────────────────────────────────────

bool SaveManager::exists() {
    FILE* f = fopen(saveFile,"r");
    if (f) { fclose(f); return true; }
    return false;
}
