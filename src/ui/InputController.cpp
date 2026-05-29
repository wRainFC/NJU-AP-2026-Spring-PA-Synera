#include "ui/InputController.hpp"

#include "core/GameState.hpp"
#include "raylib.h"
#include "systems/RoundSystem.hpp"
#include "systems/ShopSystem.hpp"
#include "ui/Layout.hpp"

namespace synera {

namespace {

[[nodiscard]] bool contains(Rectangle rect, Vector2 point) noexcept {
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y &&
           point.y <= rect.y + rect.height;
}

} // namespace

void InputController::update(GameState& state, const Layout& layout, RoundSystem& roundSystem,
                             ShopSystem& shopSystem) {
    (void)shopSystem;
    const Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && contains(layout.startButtonRect(), mouse)) {
        roundSystem.startCombat(state);
        return;
    }

    if (state.phase != Phase::Prep) {
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        beginDrag(state, layout);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        endDrag(state, layout);
    }
}

void InputController::beginDrag(GameState& state, const Layout& layout) {
    const Vector2 mouse = GetMousePosition();
    if (const auto slot = layout.benchSlotAt(mouse)) {
        if (const auto unitId = state.bench.occupant(*slot)) {
            drag_ = DragState{
                .kind = DragKind::UnitFromBench,
                .unitId = *unitId,
                .sourceBenchSlot = slot,
                .sourceBoardPos = std::nullopt,
            };
        }
    }
}

void InputController::endDrag(GameState& state, const Layout& layout) {
    if (drag_.kind == DragKind::None) {
        return;
    }

    const Vector2 mouse = GetMousePosition();
    if (const auto pos = layout.boardPosAt(mouse)) {
        state.placeUnitOnBoard(drag_.unitId, *pos);
    } else if (const auto slot = layout.benchSlotAt(mouse)) {
        state.placeUnitOnBench(drag_.unitId, *slot);
    }

    drag_ = DragState{};
}

} // namespace synera
