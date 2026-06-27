#pragma once
#include "Equipment.h"
#include "Item.h"
#include <vector>
#include <string>

struct ShopItem {
    std::string   name;
    std::string   description;
    int           price;        // creditos
    bool          isEquipment;
    Equipment     equip;        // se isEquipment
    Item          item;         // se !isEquipment
    Color         color;
    bool          isCosmetic   = false;  // sem efeito mecanico
    Color         cosmeticColor = WHITE; // nova cor do personagem
};

struct ShopSystem {
    bool          open       = false;
    int           npcIndex   = -1;
    int           selected   = 0;
    std::vector<ShopItem> items;

    // Feedback de compra
    float         buyMsgTimer = 0.0f;
    std::string   buyMsg;

    // Cor cosmetica aplicada ao player (0 = nenhuma)
    bool          hasCosmeticColor = false;
    Color         playerColor      = WHITE;

    // Mouse (coords ja virtualizadas pelo Game). Usado para hover/botoes no render.
    Vector2 mousePos    = {-1, -1};
    double  lastClickT  = -1.0;
    int     lastClickIdx = -1;

    void buildShop(int npcIdx, const std::string& npcName);
    void render(int playerCredits, int screenW, int screenH) const;
    // Retorna true se comprou algo. gotEquip indica tipo de retorno.
    bool tryBuy(int& playerCredits, Equipment& outEquip, Item& outItem,
                bool& gotEquip, bool& gotCosmetic, Color& cosmeticOut);
    void handleInput();   // UP/DOWN navigation
    // Mouse-aware: hover seleciona; duplo-clique no item OU botao COMPRAR compra;
    // botao FECHAR fecha (outClosed=true). Mesmos out-params do tryBuy.
    bool handleMouse(Vector2 vmouse, bool clicked, int screenW, int screenH,
                     int& playerCredits, Equipment& outEquip, Item& outItem,
                     bool& gotEquip, bool& gotCosmetic, Color& cosmeticOut, bool& outClosed);
    void close();
    void update(float dt);
};
