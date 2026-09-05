cmake_minimum_required(VERSION 3.14)
project(DarknetPrototype LANGUAGES CXX)

# Compat: raylib 5.0 declara um cmake_minimum_required antigo que o CMake >= 3.31
# rejeita. Mantém a populacao do FetchContent funcionando em toolchains novas.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(darknet
    src/main.cpp
    src/Game.cpp
    src/Game_Menus.cpp
    src/Game_Shaders.cpp
    src/Game_Network.cpp
    src/Game_PremiumStore.cpp
    src/Game_Resources.cpp
    src/Game_Evolution.cpp
    src/Game_QuestsNPC.cpp
    src/Game_Phases.cpp
    src/Game_Spawn.cpp
    src/Game_WorldGen.cpp
    src/Game_WorldRender.cpp
    src/Game_Gameplay.cpp
    src/Enemy_Render.cpp
    src/Game_HUD.cpp
    src/Game_Bot.cpp
    src/Game_SkillTree.cpp
    src/SkillTree.cpp
    src/SpriteExtrude.cpp
    src/Player.cpp
    src/Enemy.cpp
    src/Item.cpp
    src/Projectile.cpp
    src/EnemyProjectile.cpp
    src/Skill.cpp
    src/Tilemap.cpp
    src/NPC.cpp
    src/Quest.cpp
    src/XPOrb.cpp
    src/Particle.cpp
    src/AudioManager.cpp
    src/SaveManager.cpp
    src/Background.cpp
    src/BotController.cpp
    src/EnemyDirector.cpp
    src/Companion.cpp
    src/BuildingSystem.cpp
    src/ShopSystem.cpp
    src/DarkWorld.cpp
    src/AnomalyPortal.cpp
    src/LightSystem.cpp
    src/CraftingSystem.cpp
    src/InfernoZone.cpp
    src/ScreenEffects.cpp
    src/Achievement.cpp
    src/TutorialSystem.cpp
    src/SpriteGen.cpp
    src/NetClient.cpp
    src/HttpClient.cpp
    src/StoreClient.cpp
)

target_include_directories(darknet PRIVATE src third_party)
target_link_libraries(darknet PRIVATE raylib)

if (WIN32)
    target_link_libraries(darknet PRIVATE winmm ws2_32 winhttp shell32)
endif()

# ── Testes unitarios (doctest, vendored em third_party/doctest) ──────────────
# Compila APENAS os .cpp das classes testadas (regras puras) — nao o jogo
# inteiro. Linka raylib porque Enemy/CraftingSystem chamam funcoes da raylib
# nos renders (nunca executados nos testes; nenhuma janela e aberta).
enable_testing()
add_executable(darknet_tests
    tests/test_main.cpp
    tests/test_cases.cpp
    src/CraftingSystem.cpp
    src/Enemy.cpp
    src/SpriteGen.cpp     # SpriteBank, referenciado por Enemy::render (nao executado)
    src/SaveManager.cpp   # save/load roundtrip (V5 + legado V4) — regras puras de arquivo
    src/Player.cpp        # usado por SaveManager::load (equipItem/applyClass/loadSavedProgress)
    src/Item.cpp          # Item::createRandom no load de inventario
    src/Quest.cpp         # construcao/update de quests nos testes
    src/Skill.cpp         # Player::update/useSkill referenciam Skill
    src/SkillTree.cpp     # Player::refreshSkillVectors/getEffectiveDamage usam perks
)
target_include_directories(darknet_tests PRIVATE src third_party)
target_link_libraries(darknet_tests PRIVATE raylib)
if (WIN32)
    target_link_libraries(darknet_tests PRIVATE winmm)
endif()
add_test(NAME darknet_tests COMMAND darknet_tests)

# Copia os recursos para o diretório do executável (Release/Debug) — assim o jogo
# roda mesmo ao executar o .exe diretamente (não só do diretório do projeto).
# content/ = conteudo em DADO (fases etc). Vai junto do exe para poder ser
# editado sem recompilar - e a base do "motor que evolui sem tocar em C++".
if (EXISTS "${CMAKE_SOURCE_DIR}/content")
    add_custom_command(TARGET darknet POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/content" "$<TARGET_FILE_DIR:darknet>/content"
        COMMENT "Copiando content/ para o diretorio do executavel")
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/resources")
    add_custom_command(TARGET darknet POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:darknet>/resources"
        COMMENT "Copiando resources/ para o diretorio do executavel")
endif()
