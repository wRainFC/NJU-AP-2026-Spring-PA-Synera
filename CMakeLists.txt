cmake_minimum_required(VERSION 3.28)
project(Synera LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(raylib QUIET)

if (NOT raylib_FOUND)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

add_library(synera_logic
    src/core/AbilityContext.cpp
    src/core/abilities/BasicAbilities.cpp
    src/core/GameState.cpp
    src/core/Player.cpp
    src/core/Shop.cpp
    src/core/UnitCatalog.cpp
    src/core/Unit.cpp
    src/board/Bench.cpp
    src/board/Board.cpp
    src/board/Pathfinder.cpp
    src/systems/CombatSystem.cpp
    src/systems/EquipmentSystem.cpp
    src/systems/RoundSystem.cpp
    src/systems/SaveSystem.cpp
    src/systems/ShopPool.cpp
    src/systems/ShopSystem.cpp
    src/systems/SynergySystem.cpp
    src/systems/UpgradeSystem.cpp
)

target_include_directories(synera_logic PUBLIC src)

add_executable(synera
    src/main.cpp
    src/app/GameApp.cpp
    src/ui/InputController.cpp
    src/ui/Layout.cpp
    src/ui/Renderer.cpp
)

target_link_libraries(synera PRIVATE synera_logic raylib)

if (MSVC)
    target_compile_options(synera_logic PRIVATE /W4 /permissive-)
    target_compile_options(synera PRIVATE /W4 /permissive-)
else()
    target_compile_options(synera_logic PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(synera PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if (BUILD_TESTING)
    find_package(Catch2 3 CONFIG QUIET)

    if (NOT Catch2_FOUND)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.15.0
        )
        set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(Catch2)
        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    endif()

    add_executable(synera_tests
        tests/BoardTests.cpp
        tests/GameStatePlacementTests.cpp
        tests/CombatSystemTests.cpp
        tests/RoundSystemTests.cpp
        tests/ShopTests.cpp
        tests/UnitCatalogTests.cpp
    )

    target_link_libraries(synera_tests PRIVATE synera_logic Catch2::Catch2WithMain)

    if (MSVC)
        target_compile_options(synera_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(synera_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    include(Catch)
    catch_discover_tests(synera_tests)
endif()
