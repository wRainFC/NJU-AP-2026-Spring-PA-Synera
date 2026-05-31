#include "app/GameApp.hpp"

#include "app/GameConfig.hpp"
#include "raylib.h"

#include <string>
#include <string_view>
#include <utility>

namespace synera {

namespace {

constexpr std::string_view ManualSavePath = "saves/manual.json";
constexpr float StatusMessageSeconds = 3.0F;

}  // namespace

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
    (void)shopSystem_.refresh(state_, ShopRefreshMode::Initial);
}

void GameApp::update(float dt) {
    if (statusMessageTimer_ > 0.0F) {
        statusMessageTimer_ -= dt;
        if (statusMessageTimer_ <= 0.0F) {
            statusMessage_.clear();
        }
    }

    const InputResult inputResult = input_.update(state_, layout_, roundSystem_, shopSystem_, upgradeSystem_,
                                                  synergySystem_, equipmentSystem_);
    if (inputResult.saveRequested) {
        handleSave();
    }
    if (inputResult.loadRequested && handleLoad()) {
        return;
    }

    if (state_.phase() == Phase::Combat) {
        combatSystem_.update(state_, dt);
        if (state_.isCombatFinished()) {
            const bool playerWon = state_.playerWonCombat();
            roundSystem_.enterResolve(state_, playerWon);
            (void)equipmentSystem_.tryGrantRoundDrop(state_, playerWon);
            resolveTimer_ = 0.0F;
        }
    } else if (state_.phase() == Phase::Resolve) {
        resolveTimer_ += dt;
        if (resolveTimer_ >= 1.0F) {
            roundSystem_.finishResolve(state_);
            (void)shopSystem_.refresh(state_, ShopRefreshMode::RoundStart);
        }
    }
}

void GameApp::render() {
    BeginDrawing();
    ClearBackground(Color{24, 26, 27, 255});
    renderer_.draw(state_, layout_, statusMessage_);
    EndDrawing();
}

void GameApp::shutdown() {
    CloseWindow();
}

void GameApp::handleSave() {
    const auto result = saveSystem_.save(state_, std::string{ManualSavePath});
    if (result) {
        setStatusMessage("Saved to " + std::string{ManualSavePath});
        return;
    }
    setStatusMessage("Save failed: " + result.error());
}

bool GameApp::handleLoad() {
    auto loaded = saveSystem_.load(std::string{ManualSavePath});
    if (!loaded) {
        setStatusMessage("Load failed: " + loaded.error());
        return false;
    }

    state_ = std::move(*loaded);
    synergySystem_.recompute(state_);
    resolveTimer_ = 0.0F;
    setStatusMessage("Loaded from " + std::string{ManualSavePath});
    return true;
}

void GameApp::setStatusMessage(std::string message) {
    statusMessage_ = std::move(message);
    statusMessageTimer_ = StatusMessageSeconds;
}

}  // namespace synera
