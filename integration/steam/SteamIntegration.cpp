#include "SteamIntegration.h"

// Define USE_STEAMWORKS and add the SDK to the build when you have the App ID.
#ifdef USE_STEAMWORKS
  #include "steam/steam_api.h"
#endif

namespace SteamIntegration {

bool init() {
#ifdef USE_STEAMWORKS
    // steam_appid.txt in the exe dir in dev (or via launcher in production)
    return SteamAPI_Init();
#else
    return false; // in the SDK: runs normally, without Steam features
#endif
}

void shutdown() {
#ifdef USE_STEAMWORKS
    SteamAPI_Shutdown();
#endif
}

void runCallbacks() {
#ifdef USE_STEAMWORKS
    SteamAPI_RunCallbacks();
#endif
}

void unlockAchievement(const std::string& id) {
#ifdef USE_STEAMWORKS
    if (SteamUserStats()) {
        SteamUserStats()->SetAchievement(id.c_str());
        SteamUserStats()->StoreStats();
    }
#else
    (void)id;
#endif
}

std::string playerName() {
#ifdef USE_STEAMWORKS
    if (SteamFriends()) return SteamFriends()->GetPersonaName();
#endif
    return "Operator";
}

bool overlayActive() {
#ifdef USE_STEAMWORKS
    return SteamUtils() && SteamUtils()->IsOverlayEnabled();
#else
    return false;
#endif
}

} // namespace SteamIntegration
