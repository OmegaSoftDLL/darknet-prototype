#include "Game.h"
#include <raylib.h>
#include <string>
#include <cstdlib>
#include <ctime>

int main(int argc, char* argv[]) {
    printf("DARKNET v1.0\n");
    printf("Uso: darknet.exe [--autobot|--autotest] [--test-seconds=N] [--seed=N] [--start-phase=N]\n");
    printf("  --autotest        bot of test automatic\n");
    printf("  --test-seconds=N  encerra the test in N seconds and grava the report\n");
    printf("  --seed=N          world REPRODUTISPEED (same seed = same map)\n");
    printf("  --start-phase=N   start directly at phase N (0..N-1; useful for audit)\n");
    printf("  --headless        without window/GPU (CI): only the simulacao of the bot\n\n");

    bool     autoTest = false;
    float    testSecs = 0.0f;   // 0 = usa the padrao (2h)
    unsigned seed     = 0;      // 0 = random
    bool     headless = false;
    int      startPhase = -1;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--autotest" || arg == "--autobot") autoTest = true;
        else if (arg == "--headless") headless = true;
        // Sem --test-seconds the autotest ia until 7200s: only dava to medir matando the
        // process, and ai the report never era written.
        else if (arg.rfind("--test-seconds=", 0) == 0)
            testSecs = (float)atof(arg.substr(15).c_str());
        // Sem seed fixa all report of bug virava anedota irreproduzivel.
        else if (arg.rfind("--seed=", 0) == 0)
            seed = (unsigned)strtoul(arg.substr(7).c_str(), nullptr, 10);
        // Jump to advanced phase (audit of phases 5-11).
        else if (arg.rfind("--start-phase=", 0) == 0)
            startPhase = atoi(arg.substr(14).c_str());
    }

    if (seed != 0) {
        SetRandomSeed(seed);
        srand(seed);
        printf("SEED FIXA: %u (world reprodutivel)\n", seed);
    } else {
        unsigned t = (unsigned)time(nullptr);
        SetRandomSeed(t);
        srand(t);
    }

    if (headless) printf("HEADLESS: without window/GPU (modo CI)\n");

    Game game(headless, startPhase);
    game.worldSeed          = seed;
    game.autoTestSeconds    = testSecs;
    game.runAutoTest(autoTest);

    // Exit code = veredito of the portao of validation (only in the autotest): qualquer
    // script ou agente sabe if the build stayed playable without precisar open the game.
    return (autoTest && !game.autoTestPassed) ? 1 : 0;
}
