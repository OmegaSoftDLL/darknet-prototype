#pragma once
#include "Equipment.h"
#include "Item.h"
#include <vector>
#include <string>

struct ShopItem {
    std::string   name;
    std::string   description;
    int           price;        // credits
    bool          isEquipment;
    Equipment     equip;        // if isEquipment
    Item          item;         // if !isEquipment
    Color         color;
    bool          isCosmetic   = false;  // without effect mechanical
    Color         cosmeticColor = WHITE; // new color of the character
};

struct ShopSystem {
    bool          open       = false;
    int           npcIndex   = -1;
    int           selected   = 0;
    std::vector<ShopItem> items;

    // Feedback of purchase
    float         buyMsgTimer = 0.0f;
    std::string   buyMsg;

    // Color cosmetica aplicada to the player (0 = nenhuma)
    bool          hasCosmeticColor = false;
    Color         playerColor      = WHITE;

    // Mouse (coords already virtualizadas pelo Game). Usado to hover/botoes in the render.
    Vector2 mousePos    = {-1, -1};
    double  lastClickT  = -1.0;
    int     lastClickIdx = -1;

    void buildShop(int npcIdx, const std::string& npcName);
    void render(int playerCredits, int screenW, int screenH) const;
    // Returns true if comprou algo. gotEquip indica type of return.
    bool tryBuy(int& playerCredits, Equipment& outEquip, Item& outItem,
                bool& gotEquip, bool& gotCosmetic, Color& cosmeticOut);
    void handleInput();   // UP/DOWN navigation
    // Mouse-aware: hover seleciona; double-click in the item OU button BUY purchase;
    // button FECHAR closes (outClosed=true). Same out-params of the tryBuy.
    bool handleMouse(Vector2 vmouse, bool clicked, int screenW, int screenH,
                     int& playerCredits, Equipment& outEquip, Item& outItem,
                     bool& gotEquip, bool& gotCosmetic, Color& cosmeticOut, bool& outClosed);
    void close();
    void update(float dt);
};
