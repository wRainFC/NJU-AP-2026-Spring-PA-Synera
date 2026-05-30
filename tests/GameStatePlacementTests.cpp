#include <catch2/catch_test_macros.hpp>

#include "board/HexGrid.hpp"
#include "core/GameState.hpp"

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

}  // namespace

TEST_CASE("GameState places and swaps player units across bench and board", "[gamestate][placement]") {
    synera::GameState state;
    const synera::UnitId first = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId second = state.createUnit("ember_mage", synera::Owner::PlayerCtrl);
    const auto boardLeft = pos(0, 4);
    const auto boardRight = pos(1, 4);

    REQUIRE(state.placeUnitOnBench(first, 0));
    REQUIRE(state.placeUnitOnBench(second, 1));

    REQUIRE(state.placeUnitOnBenchResult(first, 1) == synera::PlacementResult::Ok);
    CHECK(state.benchOccupant(0) == second);
    CHECK(state.benchOccupant(1) == first);

    REQUIRE(state.placeUnitOnBoardResult(first, boardLeft) == synera::PlacementResult::Ok);
    CHECK(state.boardOccupant(boardLeft) == first);
    CHECK_FALSE(state.benchOccupant(1).has_value());

    REQUIRE(state.placeUnitOnBoardResult(second, boardLeft) == synera::PlacementResult::Ok);
    CHECK(state.boardOccupant(boardLeft) == second);
    CHECK(state.benchOccupant(0) == first);

    REQUIRE(state.placeUnitOnBoardResult(first, boardRight) == synera::PlacementResult::Ok);
    REQUIRE(state.placeUnitOnBoardResult(first, boardLeft) == synera::PlacementResult::Ok);
    CHECK(state.boardOccupant(boardLeft) == first);
    CHECK(state.boardOccupant(boardRight) == second);

    REQUIRE(state.placeUnitOnBenchResult(first, 0) == synera::PlacementResult::Ok);
    CHECK(state.benchOccupant(0) == first);
    CHECK_FALSE(state.boardOccupant(boardLeft).has_value());
}

TEST_CASE("GameState rejects invalid placement rules", "[gamestate][placement]") {
    synera::GameState state;
    const synera::UnitId unit = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);

    REQUIRE(state.placeUnitOnBench(unit, 0));
    CHECK(state.placeUnitOnBoardResult(unit, pos(0, 1)) == synera::PlacementResult::InvalidHalf);
    CHECK(state.placeUnitOnBenchResult(unit, 99) == synera::PlacementResult::InvalidPosition);
}

TEST_CASE("GameState enforces player population cap", "[gamestate][placement]") {
    synera::GameState state;
    const synera::UnitId first = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId second = state.createUnit("ember_mage", synera::Owner::PlayerCtrl);
    const synera::UnitId third = state.createUnit("field_medic", synera::Owner::PlayerCtrl);
    const synera::UnitId fourth = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);

    REQUIRE(state.placeUnitOnBench(first, 0));
    REQUIRE(state.placeUnitOnBench(second, 1));
    REQUIRE(state.placeUnitOnBench(third, 2));
    REQUIRE(state.placeUnitOnBench(fourth, 3));

    REQUIRE(state.placeUnitOnBoardResult(first, pos(0, 4)) == synera::PlacementResult::Ok);
    REQUIRE(state.placeUnitOnBoardResult(second, pos(1, 4)) == synera::PlacementResult::Ok);
    REQUIRE(state.placeUnitOnBoardResult(third, pos(2, 4)) == synera::PlacementResult::Ok);

    CHECK(state.playerBoardUnitCount() == state.player().populationCap);
    CHECK(state.placeUnitOnBoardResult(fourth, pos(3, 4)) == synera::PlacementResult::PopulationFull);
}
