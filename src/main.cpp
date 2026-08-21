#include "Game.h"
#include <raylib.h>
#include <string>
#include <cstdlib>

int main(int argc, char* argv[]) {
    printf("DARKNET v1.0\n");
    printf("Uso: darknet.exe [--autobot|--autotest] [--test-seconds=N] [--seed=N]\n");
    printf("  --autotest        bot de teste automatico\n");
    printf("  --test-seconds=N  encerra o teste em N segundos e grava o relatorio\n");
    printf("  --seed=N          mundo REPRODUTIVEL (mesmo seed = mesmo mapa)\n\n");

    bool     autoTest = false;
    float    testSecs = 0.0f;   // 0 = usa o padrao (2h)
    unsigned seed     = 0;      // 0 = aleatorio
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--autotest" || arg == "--autobot") autoTest = true;
        // Sem --test-seconds o autoteste ia ate 7200s: so dava pra medir matando o
        // processo, e ai o relatorio nunca era escrito.
        else if (arg.rfind("--test-seconds=", 0) == 0)
            testSecs = (float)atof(arg.substr(15).c_str());
        // Sem seed fixa todo relatorio de bug virava anedota irreproduzivel.
        else if (arg.rfind("--seed=", 0) == 0)
            seed = (unsigned)strtoul(arg.substr(7).c_str(), nullptr, 10);
    }

    if (seed != 0) {
        SetRandomSeed(seed);
        printf("SEED FIXA: %u (mundo reprodutivel)\n", seed);
    }

    Game game;
    game.worldSeed       = seed;
    game.autoTestSeconds = testSecs;
    game.runAutoTest(autoTest);

    // Exit code = veredito do portao de validacao (so no autoteste): qualquer
    // script ou agente sabe se a build ficou jogavel sem precisar abrir o jogo.
    return (autoTest && !game.autoTestPassed) ? 1 : 0;
}
