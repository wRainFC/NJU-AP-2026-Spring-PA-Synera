#include "ui/InputController.hpp"

#include "core/GameState.hpp"
#include "raylib.h"
#include "systems/RoundSystem.hpp"
#include "systems/ShopSystem.hpp"
#include "ui/Layout.hpp"

#include <optional>

namespace synera {

namespace {

[[nodiscard]] bool contains(Rectangle rect, Vector2 point) noexcept {
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y &&
           point.y <= rect.y + rect.height;
}

[[nodiscard]] std::optional<DragState> dragFromBench(GameState& state, const Layout& layout, Vector2 mouse) {
    return layout.benchSlotAt(mouse).and_then([&](int slot) -> std::optional<DragState> {
        return state.benchOccupant(slot).transform([&](UnitId unitId) {
            return DragState{
                .kind = DragKind::UnitFromBench,
                .unitId = unitId,
                .sourceBenchSlot = slot,
                .sourceBoardPos = std::nullopt,
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
            };
        });
    });
}

}  // namespace

void InputController::update(GameState& state, const Layout& layout, RoundSystem& roundSystem,
                             ShopSystem& shopSystem) {
    const Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && contains(layout.startButtonRect(), mouse)) {
        roundSystem.startCombat(state);
        return;
    }

    if (state.phase() != Phase::Prep) {
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (handleShopClick(state, layout, shopSystem)) {
            return;
        }
        beginDrag(state, layout);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        endDrag(state, layout);
    }
}

void InputController::beginDrag(GameState& state, const Layout& layout) {
    const Vector2 mouse = GetMousePosition();
    drag_ = dragFromBench(state, layout, mouse)
                .or_else([&] { return dragFromBoard(state, layout, mouse); })
                .value_or(DragState{});
}

void InputController::endDrag(GameState& state, const Layout& layout) {
    if (drag_.kind == DragKind::None) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    if (const auto pos = layout.boardPosAt(mouse)) {
        state.placeUnitOnBoardResult(drag_.unitId, *pos);
    } else if (const auto slot = layout.benchSlotAt(mouse)) {
        state.placeUnitOnBenchResult(drag_.unitId, *slot);
    }

    drag_ = DragState{};
}

bool InputController::handleShopClick(GameState& state, const Layout& layout, ShopSystem& shopSystem) {
    const Vector2 mouse = GetMousePosition();
    if (contains(layout.shopRefreshButtonRect(), mouse)) {
        shopSystem.refresh(state, true);
        return true;
    }
    if (const auto offer = layout.shopOfferAt(mouse)) {
        shopSystem.buy(state, *offer);
        return true;
    }
    return false;
}

}  // namespace synera
