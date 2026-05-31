#include "app/GameApp.hpp"

#include "app/GameConfig.hpp"
#include "ui/UiTheme.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace synera {

namespace {

constexpr std::string_view ManualSavePath = "saves/manual.json";
constexpr float StatusMessageSeconds = 3.0F;
constexpr int FinalRound = 6;

}  // namespace

GameApp::GameApp() = default;

void GameApp::run() {
    init();
    while (!window_.shouldClose()) {
        update(window_.frameTime());
        render();
    }
    shutdown();
}

void GameApp::init() {
    window_.init(config::WindowWidth, config::WindowHeight, "Synera: Synergy Auto-Arena");
    window_.setTargetFps(config::TargetFps);
    renderer_.loadAssets();

    const UnitId first = state_.createUnit("iron_guard", Owner::PlayerCtrl);
    state_.placeUnitOnBench(first, 0);
    const UnitId second = state_.createUnit("ember_mage", Owner::PlayerCtrl);
    state_.placeUnitOnBench(second, 1);
    const UnitId third = state_.createUnit("field_medic", Owner::PlayerCtrl);
    state_.placeUnitOnBench(third, 2);
    (void)shopSystem_.refresh(state_, ShopRefreshMode::Initial);
}

void GameApp::update(float dt) {
    animationTimeSeconds_ += dt;

    if (statusMessageTimer_ > 0.0F) {
        statusMessageTimer_ -= dt;
        if (statusMessageTimer_ <= 0.0F) {
            statusMessage_.clear();
        }
    }

    const PointerInput pointer = window_.pointerInput();
    const InputResult inputResult =
        input_.update(state_, layout_, roundSystem_, shopSystem_, upgradeSystem_, synergySystem_,
                      equipmentSystem_, pointer, interactionsEnabled());
    if (inputResult.saveRequested) {
        handleSave();
    }
    if (inputResult.loadRequested && handleLoad()) {
        return;
    }
    if (!inputResult.statusMessage.empty()) {
        setStatusMessage(inputResult.statusMessage);
    }
    if (!interactionsEnabled()) {
        return;
    }

    if (state_.phase() == Phase::Combat) {
        combatSystem_.update(state_, dt);
        if (state_.isCombatFinished()) {
            const bool playerWon = state_.playerWonCombat();
            roundSystem_.enterResolve(state_, playerWon);
            (void)equipmentSystem_.tryGrantRoundDrop(state_, playerWon);
            refreshOutcomeFromState();
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
    window_.beginFrame(ui::theme::Background);
    const PointerInput pointer = window_.pointerInput();
    const RenderContext context{
        .state = state_,
        .layout = layout_,
        .dragState = input_.dragState(),
        .pointer = pointer,
        .statusMessage = statusMessage_,
        .outcomeMessage = outcomeMessage(),
        .animationTimeSeconds = animationTimeSeconds_,
        .interactionsEnabled = interactionsEnabled(),
    };
    renderer_.draw(context);
    window_.endFrame();
}

void GameApp::shutdown() {
    renderer_.unloadAssets();
    window_.shutdown();
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
    refreshOutcomeFromState();
    resolveTimer_ = 0.0F;
    setStatusMessage("Loaded from " + std::string{ManualSavePath});
    return true;
}

void GameApp::setStatusMessage(std::string message) {
    statusMessage_ = std::move(message);
    statusMessageTimer_ = StatusMessageSeconds;
}

bool GameApp::interactionsEnabled() const noexcept {
    return outcome_ == GameOutcome::Playing;
}

std::string_view GameApp::outcomeMessage() const noexcept {
    switch (outcome_) {
        case GameOutcome::Victory:
            return "Victory";
        case GameOutcome::Defeat:
            return "Defeat";
        case GameOutcome::Playing:
            return "";
    }
    return "";
}

void GameApp::refreshOutcomeFromState() noexcept {
    if (state_.player().isDead()) {
        outcome_ = GameOutcome::Defeat;
    } else if (state_.player().currentRound > FinalRound) {
        outcome_ = GameOutcome::Victory;
    } else {
        outcome_ = GameOutcome::Playing;
    }
}

}  // namespace synera
