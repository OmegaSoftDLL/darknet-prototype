#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Steamworks STUB wrapper. When you have the App ID and download the Steamworks SDK,
// define USE_STEAMWORKS and implement the bodies in SteamIntegration.cpp calling the SDK
// (steam_api.h). Without the SDK, everything becomes the in the-op only the game compiles normally.
// The same can be done for Epic (EOS SDK).
// ─────────────────────────────────────────────────────────────────────────────
#include <string>

namespace SteamIntegration {
    bool init();                      // SteamAPI_Init — false if Steam not running
    void shutdown();                  // SteamAPI_Shutdown
    void runCallbacks();              // call every frame
    void unlockAchievement(const std::string& id);
    std::string playerName();         // Steam user name
    bool overlayActive();
}
