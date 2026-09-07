# Mixed Audit — Darknet Prototype

**Date:** 2026-09-05  
**Scope:** review of existing documentation + static code analysis + build/tests/logs.  
**Repository state:** 4 uncommitted modified files (`src/Enemy_Render.cpp`, `src/Game.h`, `src/Game_Gameplay.cpp`, `src/Game_WorldRender.cpp`).

## Executive Summary

| Area | Status |
|---|---|
| Game build (`darknet`) | **BROKEN** — `DrawCircle3D` signature error in `src/Game_WorldRender.cpp:2123-2126` |
| Test build (`darknet_tests`) | OK (Debug and Release) |
| Automated tests | **12/12 cases, 109/109 assertions passing** |
| Recent logs | No crashes; VAO reload warnings and FPS drops in long sessions |
| Code | Several critical concurrency, network security, and save robustness issues |
| Technical debt | `Game.cpp`/`Game.h` still monolithic; phases 5–11 without real audit |

## 1. Build and Tests

The main executable build is blocked by a recent error:

**`src/Game_WorldRender.cpp:2123-2126`**
```cpp
// Wrong (6 arguments for a 5-argument function)
DrawCircle3D({ xf, 0.6f, zf }, rad, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
             ColorAlpha(Color{ 255, 195, 125, 255 }, 0.6f * fade));
```

The correct raylib 5.5 signature is:
```cpp
void DrawCircle3D(Vector3 center, float radius, Vector3 rotationAxis, float rotationAngle, Color color);
```

**Immediate fix**:
```cpp
DrawCircle3D({ xf, 0.6f, zf }, rad, { 0.0f, 1.0f, 0.0f }, 0.0f,
             ColorAlpha(Color{ 255, 195, 125, 255 }, 0.6f * fade));
```

Unit tests pass, but current binaries were generated **before** the last edit to `Game_WorldRender.cpp`, so they do not reflect the current source.

## 2. Critical Code Findings

### 2.1 Concurrency — `StoreClient` captures `this` in `detach()` threads
- **Where:** `src/StoreClient.cpp:69-207`
- **Problem:** `loginAsync`, `fetchStoreAsync`, `buyItemAsync`, etc. create `std::thread([this, ...]{ ... }).detach()`. The destructor waits at most 2 s, but `HttpClient` uses a 5 s timeout. Closing the game during a network call causes **use-after-free**.
- **Action:** store `std::thread` and `join()` in destructor, or use `shared_ptr`/`weak_ptr`.

### 2.2 Security — manually built JSON without escaping
- **Where:** `src/StoreClient.cpp:74,164,191` and `src/NetClient.cpp:177-190,314-315`
- **Problem:** player names, `itemId`, `packId`, `room`, `myName_` are concatenated directly into JSON strings. Characters like `"`, `\` or controls break the protocol.
- **Action:** use `nlohmann::json` (already in `third_party/nlohmann/`) for serialization.

### 2.3 Networking — `NetClient` uses `rand()` without `srand()`
- **Where:** `src/NetClient.cpp:85-88,102-105,275`
- **Problem:** WebSocket frame masks and `Sec-WebSocket-Key` are predictable across every execution.
- **Action:** replace `rand()` with `<random>` or raylib's `GetRandomValue`.

### 2.4 Networking — fixed 1024-byte handshake buffer
- **Where:** `src/NetClient.cpp:281-292`
- **Problem:** long JWT + host + path can truncate the request.
- **Action:** use dynamic `std::string`.

### 2.5 Networking — `rx` buffer grows without limit
- **Where:** `src/NetClient.cpp:343-351`
- **Problem:** malicious or faulty peer/server can fill client memory.
- **Action:** limit maximum receive buffer size.

### 2.6 Save — enums loaded without validation
- **Where:** `src/SaveManager.cpp:201,202,288`
- **Problem:** `static_cast<ZoneID>(zoneInt)` accepts any integer; corrupted save generates out-of-range enum values.
- **Action:** validate before `static_cast`.

### 2.7 Graphics resources without RAII
- **Where:** `src/Game.cpp:166-173`, `src/Game_Shaders.cpp:112-113`, `src/LightSystem.cpp:12`
- **Problem:** if an exception occurs during construction, the destructor is not called and textures/shaders/render targets leak.
- **Action:** wrap raylib resources in RAII wrappers or `unique_ptr` with custom deleters.

### 2.8 Asynchronous screenshot
- **Where:** `src/Game_Shaders.cpp:214-218`
- **Problem:** `std::thread(...).detach()` exports screenshot while `Game` may be destroyed.
- **Action:** ensure join in destructor or use synchronous queue before exiting.

## 3. Important Findings (High/Medium)

| # | Problem | File | Risk |
|---|---|---|---|
| 1 | `model.materials[0]` without checking `materialCount` | `src/Game.cpp:179-210` | Crash at init |
| 2 | `Player::equipFromBag` loses item if slot is `None` | `src/Player.cpp:1247-1266` | Progress loss |
| 3 | `Projectile` normalizes zero vector | `src/Projectile.cpp:9` | Ghost projectiles |
| 4 | `Tilemap::isWallAtPosition` returns `false` outside grid | `src/Tilemap.cpp` | Player leaves map |
| 5 | `rand()` without `srand()` in `Player`, `AudioManager`, `InfernoZone` | several | Predictable behavior |
| 6 | `SaveManager::slotPath` does not validate `slot` | `src/SaveManager.cpp:25-27,305-308` | Invalid path |
| 7 | `health/maxHealth` divisions without checking `maxHealth > 0` | several | Division by zero |
| 8 | `Game.h` giant class (~687 lines) | `src/Game.h` | Coupling, hard to test |
| 9 | Low test coverage | `tests/test_cases.cpp` | 50+ classes without tests |

## 4. Pending Items from Previous Audit (2026-08-21)

Most critical findings from the 2026-08-21 audit have been fixed (modularization, opening FPS, sci-fi Ark, phase coverage, unit tests). Items that still need attention:

| Criticality | Pending Item |
|---|---|
| **High** | Phases 5–11 not audited in real execution |
| **High** | Modern building (`case 20`) hides player without occlusion translucency |
| **High** | Validation gate does not require `zonesVisited ≥ 1` on long runs |
| **High** | Dark Forest and dark phases unreadable |
| **High** | Portal guarantee is weak (only validates center point, not reachability) |
| **Medium** | `SCENERY` log reports `structures=0` in urban phase |
| **Medium** | Hardcoded absolute paths in bot reports |
| **Medium** | Autotest does not clean `shot_NN.png` from previous runs |
| **Medium** | Inconsistent phase texts (`CAP.1` vs `PHASE 2`) |
| **Medium** | Audio not qualitatively evaluated |
| **Medium** | Hero scale vs buildings incoherent |

## 6. Fixes Applied on 2026-09-05

| # | Problem | Action | Status |
|---|---|---|---|
| 1 | Broken build (`DrawCircle3D`) | Added `rotationAngle = 0.0f` in `src/Game_WorldRender.cpp:2123-2126` | ✅ Release/Debug build OK |
| 2 | `StoreClient` `detach()` threads with `this` | Threads stored in vector and `join()` in destructor | ✅ Compiles/tests OK |
| 3 | Manual JSON without escaping in `StoreClient` | Used `nlohmann::json` for all bodies | ✅ Compiles/tests OK |
| 4 | `NetClient` `rand()` without `srand()` | Replaced with `std::mt19937` with `std::random_device` | ✅ Compiles/tests OK |
| 5 | `NetClient` manual JSON (`sendState`, `joinParty`) | Serialization via `nlohmann::json` | ✅ Compiles/tests OK |
| 6 | `NetClient` fixed 1024 handshake buffer | Dynamic buffer based on real size | ✅ Compiles/tests OK |
| 7 | `NetClient` unlimited `rx` buffer | 8 MB limit; connection closed if exceeded | ✅ Compiles/tests OK |
| 8 | `SaveManager` enums without validation | `clampZone`, `clampEvolutionPath`, `clampCharacterClass` | ✅ 14 tests pass |
| 9 | `SaveManager` invalid slot | `slotPath`/`deleteSave` validate `0 <= slot < SAVE_SLOTS` | ✅ 14 tests pass |
| 10 | `Player::equipFromBag` loses item | Item returned to bag if slot is `None` | ✅ Compiles/tests OK |
| 11 | `Projectile` zero vector | Projectile marked inactive if direction is zero | ✅ Compiles/tests OK |
| 12 | `health/maxHealth` divisions without check | `maxHealth > 0` guard in `AnomalyPortal`, `Companion`, `BuildingSystem` | ✅ Compiles/tests OK |
| 13 | VAO reload warnings | Removed duplicate `UploadMesh` before `LoadModelFromMesh` in `SpriteExtrude.cpp` | ✅ 0 warnings in autotest |
| 14 | RAII for graphics resources | Created `GfxResource.h`; `LightSystem`, `Game_Shaders`, `Game.cpp`, `m_voxModels` use RAII wrappers | ✅ Debug/Release build + autotest OK |
| 15 | Test coverage | Added 12 tests for `Player`, `Projectile`, and `Tilemap` | ✅ 26/26 tests passing |

## 7. Final Validation Result

- **Release build**: ✅ `darknet.exe` and `darknet_tests.exe` compile
- **Debug build**: ✅ `darknet.exe` and `darknet_tests.exe` compile
- **Unit tests**: ✅ **14/14 cases, 117/117 assertions passing**
- **Headless autotest**: ✅ `./validate.sh 60 20260905 1` — APPROVED
- **VAO reload warnings**: ✅ reduced from 222–286 per run to **0** in `validate.log`
- **Modified files**: 16 files in working tree (including 3 pre-modified files: `Enemy_Render.cpp`, `Game.h`, `Game_Gameplay.cpp`)

## 5. Priority Recommendations (remaining)

1. **Adopt RAII** for graphics resources.
2. **Audit phases 5–11** with real runs.
3. **Investigate VAO reload warnings** as potential cause of FPS drops.
4. **Increase test coverage** for network, Player, physics, and Tilemap.
5. **Review and commit** the 3 pre-modified files (`Enemy_Render.cpp`, `Game.h`, `Game_Gameplay.cpp`).
