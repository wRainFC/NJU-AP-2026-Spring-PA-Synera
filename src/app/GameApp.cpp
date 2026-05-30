#include "app/GameApp.hpp"

#include "app/GameConfig.hpp"
#include "raylib.h"

namespace synera {

GameApp::GameApp() = default;

void GameApp::run() {
    init();
    while (!WindowShouldClose()) {
        update(GetFrameTime());
        render();
    }
    shutdown();
}

void GameApp::init() {
    InitWindow(config::WindowWidth, config::WindowHeight, "Synera: Synergy Auto-Arena");
    SetTargetFPS(config::TargetFps);

    const UnitId first = state_.createUnit("iron_guard", Owner::PlayerCtrl);
    state_.placeUnitOnBench(first, 0);
    const UnitId second = state_.createUnit("ember_mage", Owner::PlayerCtrl);
    state_.placeUnitOnBench(second, 1);
    const UnitId third = state_.createUnit("field_medic", Owner::PlayerCtrl);
    state_.placeUnitOnBench(third, 2);
    shopSystem_.refresh(state_, false);
}

void GameApp::update(float dt) {
    input_.update(state_, layout_, roundSystem_, shopSystem_);

    if (state_.phase() == Phase::Combat) {
        combatSystem_.update(state_, dt);
        if (state_.isCombatFinished()) {
            roundSystem_.enterResolve(state_, state_.playerWonCombat());
            resolveTimer_ = 0.0F;
        }
    } else if (state_.phase() == Phase::Resolve) {
        resolveTimer_ += dt;
        if (resolveTimer_ >= 1.0F) {
            roundSystem_.finishResolve(state_);
        }
    }
}

void GameApp::render() {
    BeginDrawing();
    ClearBackground(Color{24, 26, 27, 255});
    renderer_.draw(state_, layout_);
    EndDrawing();
}

void GameApp::shutdown() {
    CloseWindow();
}

}  // namespace synera
