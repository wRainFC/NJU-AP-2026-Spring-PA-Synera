#include <catch2/catch_test_macros.hpp>

#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "ui/InputController.hpp"
#include "ui/Layout.hpp"

#include <variant>

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

[[nodiscard]] Vector2 center(Rectangle rect) noexcept {
    return Vector2{rect.x + rect.width / 2.0F, rect.y + rect.height / 2.0F};
}

[[nodiscard]] synera::PointerInput pointer(Vector2 position, bool pressed = false, bool down = false,
                                           bool released = false) noexcept {
    return synera::PointerInput{
        .position            = position,
        .insideVirtualCanvas = true,
        .leftPressed         = pressed,
        .leftReleased        = released,
        .leftDown            = down,
    };
}

[[nodiscard]] Vector2 dragged(Vector2 position) noexcept {
    return Vector2{position.x + synera::config::DragStartThresholdVirtualPixels + 2.0F, position.y};
}

}  // namespace

TEST_CASE("InputController toggles a clicked player unit without producing commands", "[ui][input]") {
    synera::GameState state;
    synera::Layout layout;
    synera::InputController input;
    const synera::UnitId unit = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    REQUIRE(state.placeUnitOnBench(unit, 0));

    const Vector2 mouse = center(layout.benchSlotRect(0));
    CHECK(input.update(state, layout, pointer(mouse, true, true), true).commands.empty());
    CHECK(input.update(state, layout, pointer(mouse, false, false, true), true).commands.empty());

    const synera::InputReadModel model = input.readModel(state);
    CHECK(model.selectedUnitId == unit);
    CHECK(std::holds_alternative<std::monostate>(model.dragDrop.ghost));
    CHECK(state.benchOccupant(0) == unit);

    CHECK(input.update(state, layout, pointer(mouse, true, true), true).commands.empty());
    CHECK(input.update(state, layout, pointer(mouse, false, false, true), true).commands.empty());
    CHECK_FALSE(input.readModel(state).selectedUnitId.has_value());
}

TEST_CASE("InputController starts a unit drag only after the configured threshold", "[ui][input]") {
    synera::GameState state;
    synera::Layout layout;
    synera::InputController input;
    const synera::UnitId unit = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    REQUIRE(state.placeUnitOnBench(unit, 0));

    const Vector2 mouse = center(layout.benchSlotRect(0));
    CHECK(input.update(state, layout, pointer(mouse, true, true), true).commands.empty());

    const synera::InputFrameResult holdResult =
        input.update(state, layout, pointer(dragged(mouse), false, true), true);
    CHECK(holdResult.commands.empty());

    const synera::InputReadModel model = input.readModel(state);
    const auto* ghost                  = std::get_if<synera::UnitDragGhost>(&model.dragDrop.ghost);
    REQUIRE(ghost != nullptr);
    CHECK(ghost->unitId == unit);
    CHECK(model.selectedUnitId == unit);
    CHECK(state.benchOccupant(0) == unit);
}

TEST_CASE("InputController emits placement and sell commands for unit drops", "[ui][input]") {
    synera::GameState state;
    synera::Layout layout;
    synera::InputController input;
    const synera::UnitId unit = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    REQUIRE(state.placeUnitOnBench(unit, 0));

    const Vector2 mouse = center(layout.benchSlotRect(0));
    CHECK(input.update(state, layout, pointer(mouse, true, true), true).commands.empty());
    CHECK(input.update(state, layout, pointer(dragged(mouse), false, true), true).commands.empty());

    const synera::AxialPos boardPos = pos(0, 4);
    const synera::InputFrameResult placeResult =
        input.update(state, layout, pointer(layout.boardHexCenter(boardPos), false, false, true), true);
    REQUIRE(placeResult.commands.size() == 1);
    const auto* place = std::get_if<synera::PlaceUnitOnBoard>(&placeResult.commands.front());
    REQUIRE(place != nullptr);
    CHECK(place->unitId == unit);
    CHECK(place->pos == boardPos);
    CHECK(state.benchOccupant(0) == unit);

    CHECK(input.update(state, layout, pointer(mouse, true, true), true).commands.empty());
    CHECK(input.update(state, layout, pointer(dragged(mouse), false, true), true).commands.empty());
    const synera::InputFrameResult sellResult =
        input.update(state, layout, pointer(center(layout.sellAreaRect()), false, false, true), true);
    REQUIRE(sellResult.commands.size() == 1);
    const auto* sell = std::get_if<synera::SellUnit>(&sellResult.commands.front());
    REQUIRE(sell != nullptr);
    CHECK(sell->unitId == unit);
}

TEST_CASE("InputController emits equipment drop commands without changing selection on click",
          "[ui][input]") {
    synera::GameState state;
    synera::Layout layout;
    synera::InputController input;
    const synera::UnitId unit = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    REQUIRE(state.placeUnitOnBench(unit, 0));
    state.addEquipment(synera::EquipmentType::IronSword);

    const Vector2 equipmentMouse = center(layout.equipmentSlotRect(0));
    CHECK(input.update(state, layout, pointer(equipmentMouse, true, true), true).commands.empty());
    CHECK(input.update(state, layout, pointer(equipmentMouse, false, false, true), true).commands.empty());
    CHECK_FALSE(input.readModel(state).selectedUnitId.has_value());

    CHECK(input.update(state, layout, pointer(equipmentMouse, true, true), true).commands.empty());
    CHECK(input.update(state, layout, pointer(dragged(equipmentMouse), false, true), true).commands.empty());

    const synera::InputReadModel dragModel = input.readModel(state);
    const auto* ghost = std::get_if<synera::EquipmentDragGhost>(&dragModel.dragDrop.ghost);
    REQUIRE(ghost != nullptr);
    CHECK(ghost->equipment == synera::EquipmentType::IronSword);

    const synera::InputFrameResult result =
        input.update(state, layout, pointer(center(layout.benchSlotRect(0)), false, false, true), true);
    REQUIRE(result.commands.size() == 1);
    const auto* equip = std::get_if<synera::EquipFromPool>(&result.commands.front());
    REQUIRE(equip != nullptr);
    CHECK(equip->poolIndex == 0);
    CHECK(equip->unitId == unit);
}

TEST_CASE("InputController emits immediate UI commands and clears selection from empty clicks",
          "[ui][input]") {
    synera::GameState state;
    synera::Layout layout;
    synera::InputController input;
    const synera::UnitId unit = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    REQUIRE(state.placeUnitOnBench(unit, 0));

    const Vector2 unitMouse = center(layout.benchSlotRect(0));
    CHECK(input.update(state, layout, pointer(unitMouse, true, true), true).commands.empty());
    CHECK(input.update(state, layout, pointer(unitMouse, false, false, true), true).commands.empty());
    REQUIRE(input.readModel(state).selectedUnitId == unit);

    const Vector2 emptyMouse{20.0F, 20.0F};
    CHECK(input.update(state, layout, pointer(emptyMouse, true, true), true).commands.empty());
    CHECK(input.update(state, layout, pointer(emptyMouse, false, false, true), true).commands.empty());
    CHECK_FALSE(input.readModel(state).selectedUnitId.has_value());

    const synera::InputFrameResult shopResult =
        input.update(state, layout, pointer(center(layout.shopOfferRect(0)), true, true), true);
    REQUIRE(shopResult.commands.size() == 1);
    CHECK(std::get_if<synera::BuyOffer>(&shopResult.commands.front()) != nullptr);

    const synera::InputFrameResult refreshResult =
        input.update(state, layout, pointer(center(layout.shopRefreshButtonRect()), true, true), true);
    REQUIRE(refreshResult.commands.size() == 1);
    CHECK(std::get_if<synera::RefreshShop>(&refreshResult.commands.front()) != nullptr);
}
