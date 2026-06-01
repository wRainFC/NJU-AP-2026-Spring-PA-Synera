#include "app/GameApp.hpp"

#include "app/GameConfig.hpp"
#include "ui/UiTheme.hpp"

#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace synera {

namespace {

constexpr std::string_view ManualSavePath = "saves/manual.json";
constexpr float StatusMessageSeconds      = 3.0F;
constexpr int FinalRound                  = 6;

template <class... Visitors>
struct Overloaded : Visitors... {
    using Visitors::operator()...;
};

template <class... Visitors>
Overloaded(Visitors...) -> Overloaded<Visitors...>;

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
    startNewRun();
}

void GameApp::startNewRun() {
    state_           = GameState{};
    input_           = InputController{};
    shopSystem_      = ShopSystem{};
    equipmentSystem_ = EquipmentSystem{};
    outcome_         = GameOutcome::Playing;
    statusMessage_.clear();
    statusMessageTimer_   = 0.0F;
    resolveTimer_         = 0.0F;
    animationTimeSeconds_ = 0.0F;

    const UnitId first = state_.createUnit("iron_guard", Owner::PlayerCtrl);
    (void)state_.placeUnitOnBench(first, 0);
    const UnitId second = state_.createUnit("ember_mage", Owner::PlayerCtrl);
    (void)state_.placeUnitOnBench(second, 1);
    const UnitId third = state_.createUnit("field_medic", Owner::PlayerCtrl);
    (void)state_.placeUnitOnBench(third, 2);
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

    const PointerInput pointer        = window_.pointerInput();
    const InputFrameResult inputFrame = input_.update(state_, layout_, pointer, interactionsEnabled());
    if (applyInputCommands(inputFrame.commands)) {
        return;
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
        .state                = state_,
        .layout               = layout_,
        .input                = input_.readModel(state_),
        .pointer              = pointer,
        .statusMessage        = statusMessage_,
        .outcomeMessage       = outcomeMessage(),
        .animationTimeSeconds = animationTimeSeconds_,
        .interactionsEnabled  = interactionsEnabled(),
    };
    renderer_.draw(context);
    window_.endFrame();
}

void GameApp::shutdown() {
    renderer_.unloadAssets();
    window_.shutdown();
}

bool GameApp::applyInputCommands(std::span<const InputCommand> commands) {
    for (const InputCommand& command : commands) {
        const bool stopFrame = std::visit(
            Overloaded{
                [&](RequestSave) {
                    handleSave();
                    return false;
                },
                [&](RequestLoad) { return handleLoad(); },
                [&](RequestRestart) {
                    startNewRun();
                    setStatusMessage("Started a new run");
                    return true;
                },
                [&](StartCombat) {
                    synergySystem_.recompute(state_);
                    roundSystem_.startCombat(state_);
                    if (state_.phase() != Phase::Prep) {
                        input_.clearInteraction();
                        input_.clearSelection();
                    }
                    return false;
                },
                [&](RefreshShop) {
                    (void)shopSystem_.refresh(state_, ShopRefreshMode::Manual);
                    return false;
                },
                [&](ToggleShopLock) {
                    (void)shopSystem_.toggleLocked(state_);
                    return false;
                },
                [&](UpgradePopulation) {
                    if (state_.player().upgradePopulation()) {
                        synergySystem_.recompute(state_);
                    }
                    return false;
                },
                [&](BuyOffer buy) {
                    const ShopBuyResult result = shopSystem_.buy(state_, buy.offerIndex);
                    if (result.ok()) {
                        (void)upgradeSystem_.tryMergeAfterGain(state_, result.gainedUnitId);
                        synergySystem_.recompute(state_);
                    }
                    return false;
                },
                [&](PlaceUnitOnBoard place) {
                    if (state_.placeUnitOnBoardResult(place.unitId, place.pos) == PlacementResult::Ok) {
                        synergySystem_.recompute(state_);
                    }
                    return false;
                },
                [&](PlaceUnitOnBench place) {
                    if (state_.placeUnitOnBenchResult(place.unitId, place.slot) == PlacementResult::Ok) {
                        synergySystem_.recompute(state_);
                    }
                    return false;
                },
                [&](SellUnit sell) {
                    const Unit* unit            = state_.findUnit(sell.unitId);
                    const std::string unitName  = unit == nullptr ? "Unit" : unit->name;
                    const ShopSellResult result = shopSystem_.sellUnit(state_, sell.unitId);
                    if (result.ok()) {
                        input_.clearSelection();
                        synergySystem_.recompute(state_);
                        setStatusMessage("Sold " + unitName + " for " + std::to_string(result.goldGained) +
                                         "g");
                    } else {
                        setStatusMessage("Sell failed");
                    }
                    return false;
                },
                [&](EquipFromPool equip) {
                    if (equipmentSystem_.equipFromPool(state_, equip.poolIndex, equip.unitId)) {
                        synergySystem_.recompute(state_);
                    }
                    return false;
                },
            },
            command);
        if (stopFrame) {
            return true;
        }
    }
    return false;
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
    input_ = InputController{};
    synergySystem_.recompute(state_);
    refreshOutcomeFromState();
    resolveTimer_ = 0.0F;
    setStatusMessage("Loaded from " + std::string{ManualSavePath});
    return true;
}

void GameApp::setStatusMessage(std::string message) {
    statusMessage_      = std::move(message);
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
