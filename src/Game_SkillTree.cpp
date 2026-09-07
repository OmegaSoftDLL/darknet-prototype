#include "Game.h"
#include "SkillTree.h"

bool Game::buyPerk(int idx) {
    if (idx < 0 || idx >= SkillTree::PERK_COUNT) return false;
    if (player.skillPoints <= 0) return false;
    if (!SkillTree::canBuy(player.perkMask, idx)) return false;
    player.perkMask |= SkillTree::bit(idx);
    player.skillPoints--;
    if (idx == SkillTree::PERK_S2_PLACA) player.increaseBaseMaxHP(40.0f);
    if (idx == SkillTree::PERK_C2_MANTO) player.increaseBaseDefense(10.0f);
    player.refreshSkillVectors();
    audio.playSkillUnlock();
    return true;
}

void Game::autoSpendSkillPoints() {
    while (player.skillPoints > 0) {
        int next = SkillTree::bestNext(player.perkMask);
        if (next < 0) break;
        if (!buyPerk(next)) break;
    }
}

void Game::updateSkillTreePanel() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_X)) {
        showSkillTree = false;
        return;
    }
    const int n = SkillTree::PERK_COUNT;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) perkCursor = (perkCursor + 1) % n;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) perkCursor = (perkCursor - 1 + n) % n;
    if (IsKeyPressed(KEY_ENTER)) buyPerk(perkCursor);
}

void Game::drawSkillTreePanel() const {
    if (!showSkillTree) return;

    const int sw = 1280, sh = 720;
    const int pw = 1060, ph = 520;
    const int px = sw / 2 - pw / 2, py = sh / 2 - ph / 2;
    DrawPanel(px, py, pw, ph, Color{0, 235, 255, 255});
    DrawRectangle(px, py, pw, 3, ColorAlpha({0, 235, 255, 255}, 0.8f));

    DrawText("HACK TREE // INVASION SYSTEM", px + 24, py + 12, 22, Color{235, 245, 255, 255});
    DrawRectangle(px + 24, py + 40, (int)(pw * 0.62f), 2, ColorAlpha({0, 235, 255, 255}, 0.35f));
    DrawRectangle(px + 24 + (int)(pw * 0.62f), py + 38, 8, 6, ColorAlpha({255, 180, 40, 255}, 0.9f));
    DrawText(TextFormat("Points of hack: %d", player.skillPoints), px + pw - 300, py + 16, 16,
             player.skillPoints > 0 ? Color{0, 255, 200, 255} : Color{120, 130, 160, 255});

    const int colW = 320;
    const int gap  = 28;
    const int x0   = px + 28;
    const int yTop = py + 58;
    const int rowH = 96;

    for (int b = 0; b < 3; ++b) {
        int cx = x0 + b * (colW + gap);
        Color bc = SkillTree::branchColor(b);
        int spent = SkillTree::spentInBranch(player.perkMask, b);
        DrawText(SkillTree::branchName(b), cx, yTop - 26, 16, bc);
        DrawText(TextFormat("%d / 4", spent), cx + 170, yTop - 26, 13, ColorAlpha(bc, 0.8f));

        for (int r = 0; r < 4; ++r) {
            int i = b * 4 + r;
            const PerkInfo& pk = SkillTree::perk(i);
            bool owned = SkillTree::owns(player.perkMask, i);
            bool canGo = SkillTree::canBuy(player.perkMask, i);
            bool sel   = (perkCursor == i);
            int  ry    = yTop + r * (rowH + 8);

            DrawRectangle(cx, ry, colW, rowH, sel ? Color{28, 44, 74, 255}
                                                  : Color{15, 24, 44, 210});
            if (sel) {
                int sc = 6;
                DrawRectangle(cx, ry, 3, rowH, ColorAlpha({0, 235, 255, 255}, 0.9f));
                DrawLine(cx+sc, ry, cx+colW-sc, ry, ColorAlpha({0, 235, 255, 255}, 0.8f));
                DrawLine(cx+sc, ry+rowH, cx+colW-sc, ry+rowH, ColorAlpha({0, 235, 255, 255}, 0.8f));
                DrawLine(cx, ry+sc, cx+sc, ry, ColorAlpha({0, 235, 255, 255}, 0.9f));
                DrawLine(cx+colW-sc, ry, cx+colW, ry+sc, ColorAlpha({0, 235, 255, 255}, 0.9f));
                DrawLine(cx, ry+rowH-sc, cx+sc, ry+rowH, ColorAlpha({0, 235, 255, 255}, 0.9f));
                DrawLine(cx+colW-sc, ry+rowH, cx+colW, ry+rowH-sc, ColorAlpha({0, 235, 255, 255}, 0.9f));
            }

            DrawText(owned ? "COMPRADO" : (canGo ? "COMPRASPEED" : "BLOQUEADO"),
                     cx + 12, ry + 6, 13,
                     owned ? Color{0, 255, 200, 255}
                           : (canGo ? Color{255, 220, 120, 255} : Color{110, 120, 145, 255}));

            DrawText(pk.name, cx + 12, ry + 24, 15, sel ? Color{255, 255, 255, 255}
                                                        : ColorAlpha(WHITE, 0.9f));
            DrawText(pk.desc, cx + 12, ry + 44, 11,
                     owned ? ColorAlpha(WHITE, 0.65f) : ColorAlpha(WHITE, 0.55f));
            DrawText(TextFormat("COST: 1 point  (T%d)", pk.tier + 1), cx + 12, ry + 72, 10,
                     ColorAlpha(owned ? Color{0, 255, 200, 255} : bc, 0.8f));
        }
    }

    DrawText("W/S navegar   ENTER buy   X/ESC close", px + 28, py + ph - 26, 13,
             Color{200, 210, 230, 255});
}