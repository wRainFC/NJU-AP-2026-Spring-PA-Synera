cmake_minimum_required(VERSION 3.28)
project(Synera LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib QUIET)

if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(synera
    src/main.cpp
    src/app/GameApp.cpp
    src/core/Ability.cpp
    src/core/GameState.cpp
    src/core/Player.cpp
    src/core/Unit.cpp
    src/board/Bench.cpp
    src/board/Board.cpp
    src/board/Pathfinder.cpp
    src/systems/CombatSystem.cpp
    src/systems/EquipmentSystem.cpp
    src/systems/RoundSystem.cpp
    src/systems/SaveSystem.cpp
    src/systems/ShopSystem.cpp
    src/systems/SynergySystem.cpp
    src/systems/UpgradeSystem.cpp
    src/ui/InputController.cpp
    src/ui/Layout.cpp
    src/ui/Renderer.cpp
)

target_include_directories(synera PRIVATE src)
target_link_libraries(synera PRIVATE raylib)

if (MSVC)
    target_compile_options(synera PRIVATE /W4 /permissive-)
else()
    target_compile_options(synera PRIVATE -Wall -Wextra -Wpedantic)
endif()
