#include <catch2/catch_test_macros.hpp>

#include "board/Bench.hpp"
#include "board/Board.hpp"
#include "board/HexGrid.hpp"
#include "board/Pathfinder.hpp"

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

}  // namespace

TEST_CASE("Board tracks placement movement and swaps", "[board]") {
    synera::Board board{4, 4};
    const auto left = pos(0, 0);
    const auto right = pos(1, 0);
    const auto invalid = pos(9, 9);

    REQUIRE(board.place(synera::UnitId{1}, left));
    CHECK(board.occupant(left) == synera::UnitId{1});
    CHECK_FALSE(board.place(synera::UnitId{2}, left));
    CHECK_FALSE(board.place(synera::UnitId{2}, invalid));

    REQUIRE(board.move(left, right));
    CHECK_FALSE(board.occupant(left).has_value());
    CHECK(board.occupant(right) == synera::UnitId{1});

    REQUIRE(board.place(synera::UnitId{2}, left));
    REQUIRE(board.swapCells(left, right));
    CHECK(board.occupant(left) == synera::UnitId{1});
    CHECK(board.occupant(right) == synera::UnitId{2});
}

TEST_CASE("Bench tracks placement first empty slot and swaps", "[bench]") {
    synera::Bench bench{3};

    CHECK(bench.firstEmptySlot() == 0);
    REQUIRE(bench.place(synera::UnitId{1}, 0));
    REQUIRE(bench.place(synera::UnitId{2}, 1));
    CHECK(bench.firstEmptySlot() == 2);
    CHECK_FALSE(bench.place(synera::UnitId{3}, 1));
    CHECK_FALSE(bench.place(synera::UnitId{3}, 9));

    REQUIRE(bench.swapSlots(0, 1));
    CHECK(bench.occupant(0) == synera::UnitId{2});
    CHECK(bench.occupant(1) == synera::UnitId{1});

    bench.remove(0);
    CHECK_FALSE(bench.occupant(0).has_value());
}

TEST_CASE("Pathfinder reaches attack range through empty cells only", "[pathfinder]") {
    synera::Board board{8, 8};
    const auto start = pos(0, 4);
    const auto target = pos(4, 4);
    const auto blocker = pos(1, 4);

    REQUIRE(board.place(synera::UnitId{10}, blocker));

    const synera::Pathfinder pathfinder;
    const auto path = pathfinder.findPathToAttackRange(board, start, target, 1);

    REQUIRE_FALSE(path.empty());
    CHECK(path.front() != blocker);
    CHECK(path.back() != target);
    CHECK(synera::hex::hexDistance(path.back(), target) <= 1);
    for (const auto step : path) {
        CHECK(board.empty(step));
    }
}

TEST_CASE("Pathfinder returns no path when already in attack range", "[pathfinder]") {
    synera::Board board{4, 4};
    const synera::Pathfinder pathfinder;

    CHECK(pathfinder.findPathToAttackRange(board, pos(1, 2), pos(1, 1), 1).empty());
}
