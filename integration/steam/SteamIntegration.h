#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Wrapper STUB do Steamworks. Quando você tiver o App ID e baixar o Steamworks SDK,
// defina USE_STEAMWORKS e implemente os corpos em SteamIntegration.cpp chamando o SDK
// (steam_api.h). Sem o SDK, tudo vira no-op para o jogo compilar normalmente.
// Idêntico pode ser feito para Epic (EOS SDK).
// ─────────────────────────────────────────────────────────────────────────────
#include <string>

namespace SteamIntegration {
    bool init();                      // SteamAPI_Init — false se Steam não rodando
    void shutdown();                  // SteamAPI_Shutdown
    void runCallbacks();              // chamar a cada frame
    void unlockAchievement(const std::string& id);
    std::string playerName();         // nome do usuário Steam
    bool overlayActive();
}
