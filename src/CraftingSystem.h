#pragma once
#include "Item.h"
#include "Equipment.h"
#include <vector>
#include <string>

enum class CraftCategory {
    All = 0,
    Weapons,
    Armor,
    Accessories,
    Consumables,
    COUNT
};

struct CraftingRecipe {
    std::string       name;
    std::string       description;
    std::vector<std::pair<ItemType,int>> ingredients;
    CraftCategory     category    = CraftCategory::Weapons;
    bool              makesEquipment = true;
    Equipment         resultEquip;
    Item              resultItem;
    bool              isLegendary = false;
};

struct CraftingSystem {
    bool open     = false;
    int  selected = 0;
    CraftCategory selectedCategory = CraftCategory::All;
    int  categoryScroll = 0;    // scroll offset for recipe list
    std::vector<CraftingRecipe> recipes;
    float msgTimer = 0.f;
    std::string lastMsg;

    // Crafting animation
    float craftTimer   = 0.f;
    bool  crafting     = false;
    int   craftingIdx  = -1;

    // Mouse (coords ja virtualizadas pelo Game)
    Vector2 mousePos    = {-1, -1};
    double  lastClickT  = -1.0;
    int     lastClickIdx = -1;

    void buildRecipes();
    void render(const std::vector<Item>& bag, int screenW, int screenH);
    bool tryCraft(std::vector<Item>& bag, Equipment& outEquip, Item& outItem, bool& gotEquip);
    void update(float dt);
    void handleInput();   // UP/DOWN/LEFT/RIGHT/ENTER/ESC
    // Mouse-aware: clique nas abas muda categoria; hover/clique seleciona receita;
    // duplo-clique OU botao CRAFTAR fabrica; botao FECHAR fecha (outClosed=true).
    bool handleMouse(Vector2 vmouse, bool clicked, std::vector<Item>& bag,
                     int screenW, int screenH, Equipment& outEquip, Item& outItem,
                     bool& gotEquip, bool& outClosed);

    // Returns indices of recipes matching current category
    std::vector<int> getFilteredIndices() const;

    static int countMaterial(const std::vector<Item>& bag, ItemType t);
    bool canCraft(const std::vector<Item>& bag, int recipeIdx) const;
};
