#include "SteamIntegration.h"

// Defina USE_STEAMWORKS e adicione o SDK ao build quando tiver o App ID.
#ifdef USE_STEAMWORKS
  #include "steam/steam_api.h"
#endif

namespace SteamIntegration {

bool init() {
#ifdef USE_STEAMWORKS
    // steam_appid.txt no dir do exe em dev (ou via launcher em produção)
    return SteamAPI_Init();
#else
    return false; // sem SDK: roda normal, sem recursos Steam
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
    return "Operador";
}

bool overlayActive() {
#ifdef USE_STEAMWORKS
    return SteamUtils() && SteamUtils()->IsOverlayEnabled();
#else
    return false;
#endif
}

} // namespace SteamIntegration
