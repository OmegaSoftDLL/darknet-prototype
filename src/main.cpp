#include "Game.h"
#include <string>

int main(int argc, char* argv[]) {
    printf("DARKNET v1.0\n");
    printf("Uso: darknet.exe [--autobot] [--autotest]\n");
    printf("  --autobot / --autotest   Ativa bot de teste automaticamente\n\n");

    bool autoTest = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--autotest" || arg == "--autobot") autoTest = true;
    }

    Game game;
    game.runAutoTest(autoTest);
    return 0;
}
