#include "ui/InputController.hpp"

#include "core/GameState.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/RoundSystem.hpp"
#include "systems/ShopSystem.hpp"
#include "systems/SynergySystem.hpp"
#include "systems/UpgradeSystem.hpp"
#include "ui/Layout.hpp"
#include "ui/UiDrawing.hpp"

#include <optional>
#include <string>

namespace synera {

namespace {

[[nodiscard]] std::optional<DragState> dragFromBench(GameState& state, const Layout& layout, Vector2 mouse) {
    return layout.benchSlotAt(mouse).and_then([&](int slot) -> std::optional<DragState> {
        return state.benchOccupant(slot).transform([&](UnitId unitId) {
            return DragState{
                .kind = DragKind::UnitFromBench,
                .unitId = unitId,
                .sourceBenchSlot = slot,
                .sourceBoardPos = std::nullopt,
                .sourceEquipmentIndex = std::nullopt,
            };
        });
    });
}

[[nodiscard]] std::optional<DragState> dragFromBoard(GameState& state, const Layout& layout, Vector2 mouse) {
    return layout.boardPosAt(mouse).and_then([&](AxialPos pos) -> std::optional<DragState> {
        return state.boardOccupant(pos).and_then([&](UnitId unitId) -> std::optional<DragState> {
            const Unit* unit = state.findUnit(unitId);
            if (unit == nullptr || unit->owner != Owner::PlayerCtrl) {
                return std::nullopt;
            }
            return DragState{
                .kind = DragKind::UnitFromBoard,
                .unitId = unitId,
                .sourceBenchSlot = std::nullopt,
                .sourceBoardPos = pos,
                .sourceEquipmentIndex = std::nullopt,
            };
        });
    });
}

[[nodiscard]] std::optional<DragState> dragFromEquipment(GameState& state, const Layout& layout,
                                                         Vector2 mouse) {
    return layout.equipmentSlotAt(mouse, state.equipmentPool().size()).transform([](std::size_t index) {
        return DragState{
            .kind = DragKind::EquipmentFromPool,
            .unitId = InvalidUnitId,
            .sourceBenchSlot = std::nullopt,
            .sourceBoardPos = std::nullopt,
            .sourceEquipmentIndex = index,
        };
    });
}

[[nodiscard]] std::optional<UnitId> playerUnitAt(GameState& state, const Layout& layout, Vector2 mouse) {
    const auto fromBoard = layout.boardPosAt(mouse).and_then([&](AxialPos pos) -> std::optional<UnitId> {
        return state.boardOccupant(pos).and_then([&](UnitId unitId) -> std::optional<UnitId> {
            const Unit* unit = state.findUnit(unitId);
            return unit != nullptr && unit->owner == Owner::PlayerCtrl ? std::optional<UnitId>{unitId}
                                                                       : std::nullopt;
        });
    });
    if (fromBoard) {
        return fromBoard;
    }
    return layout.benchSlotAt(mouse).and_then([&](int slot) { return state.benchOccupant(slot); });
}

}  // namespace

InputResult InputController::update(GameState& state, const Layout& layout, RoundSystem& roundSystem,
                                    ShopSystem& shopSystem, UpgradeSystem& upgradeSystem,
                                    SynergySystem& synergySystem, EquipmentSystem& equipmentSystem,
                                    const PointerInput& pointer, bool interactionsEnabled) {
    const Vector2 mouse = pointer.position;
    InputResult result;

    if (pointer.leftPressed && ui::contains(layout.loadButtonRect(), mouse)) {
        result.loadRequested = true;
        return result;
    }

    if (!interactionsEnabled) {
        drag_ = DragState{};
        return result;
    }

    if (pointer.leftPressed && state.phase() == Phase::Prep && ui::contains(layout.startButtonRect(), mouse)) {
        synergySystem.recompute(state);
        roundSystem.startCombat(state);
        return result;
    }
    if (pointer.leftPressed && state.phase() == Phase::Prep && ui::contains(layout.saveButtonRect(), mouse)) {
        result.saveRequested = true;
        return result;
    }

    if (state.phase() != Phase::Prep) {
        return result;
    }

    if (pointer.leftPressed) {
        if (handlePrepClick(state, layout, shopSystem, upgradeSystem, synergySystem, mouse)) {
            return result;
        }
        beginDrag(state, layout, mouse);
    }
    if (pointer.leftReleased) {
        endDrag(state, layout, shopSystem, synergySystem, equipmentSystem, mouse, result);
    }
    return result;
}

const DragState& InputController::dragState() const noexcept {
    return drag_;
}

void InputController::beginDrag(GameState& state, const Layout& layout, Vector2 mouse) {
    drag_ = dragFromEquipment(state, layout, mouse)
                .or_else([&] { return dragFromBench(state, layout, mouse); })
                .or_else([&] { return dragFromBoard(state, layout, mouse); })
                .value_or(DragState{});
}

void InputController::endDrag(GameState& state, const Layout& layout, ShopSystem& shopSystem,
                              SynergySystem& synergySystem, EquipmentSystem& equipmentSystem,
                              Vector2 mouse, InputResult& result) {
    if (drag_.kind == DragKind::None) {
        return;
    }

    if (drag_.kind == DragKind::EquipmentFromPool) {
        const bool equipped = drag_.sourceEquipmentIndex
                                  .and_then([&](std::size_t index) {
                                      return playerUnitAt(state, layout, mouse).transform([&](UnitId unitId) {
                                          return equipmentSystem.equipFromPool(state, index, unitId);
                                      });
                                  })
                                  .value_or(false);
        if (equipped) {
            synergySystem.recompute(state);
        }
    } else if (ui::contains(layout.sellAreaRect(), mouse)) {
        const Unit* unit = state.findUnit(drag_.unitId);
        const std::string unitName = unit == nullptr ? "Unit" : unit->name;
        const ShopSellResult sellResult = shopSystem.sellUnit(state, drag_.unitId);
        if (sellResult.ok()) {
            synergySystem.recompute(state);
            result.statusMessage = "Sold " + unitName + " for " + std::to_string(sellResult.goldGained) + "g";
        } else {
            result.statusMessage = "Sell failed";
        }
    } else if (const auto pos = layout.boardPosAt(mouse)) {
        if (state.placeUnitOnBoardResult(drag_.unitId, *pos) == PlacementResult::Ok) {
            synergySystem.recompute(state);
        }
    } else if (const auto slot = layout.benchSlotAt(mouse)) {
        if (state.placeUnitOnBenchResult(drag_.unitId, *slot) == PlacementResult::Ok) {
            synergySystem.recompute(state);
        }
    }

    drag_ = DragState{};
}

bool InputController::handlePrepClick(GameState& state, const Layout& layout, ShopSystem& shopSystem,
                                      UpgradeSystem& upgradeSystem, SynergySystem& synergySystem,
                                      Vector2 mouse) {
    if (ui::contains(layout.populationUpgradeButtonRect(), mouse)) {
        if (state.player().upgradePopulation()) {
            synergySystem.recompute(state);
        }
        return true;
    }
    if (ui::contains(layout.shopRefreshButtonRect(), mouse)) {
        (void)shopSystem.refresh(state, ShopRefreshMode::Manual);
        return true;
    }
    if (ui::contains(layout.shopLockButtonRect(), mouse)) {
        (void)shopSystem.toggleLocked(state);
        return true;
    }
    if (const auto offer = layout.shopOfferAt(mouse)) {
        const ShopBuyResult result = shopSystem.buy(state, *offer);
        if (result.ok()) {
            (void)upgradeSystem.tryMergeAfterGain(state, result.gainedUnitId);
            synergySystem.recompute(state);
        }
        return true;
    }
    return false;
}

}  // namespace synera
