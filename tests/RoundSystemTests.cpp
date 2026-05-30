#include <catch2/catch_test_macros.hpp>

#include "app/GameConfig.hpp"
#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "systems/RoundSystem.hpp"

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

}  // namespace

TEST_CASE("RoundSystem starts combat and later restores player formation", "[round]") {
    synera::GameState state;
    synera::RoundSystem rounds;
    const synera::UnitId playerId = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const auto startPos = pos(0, 4);

    REQUIRE(state.placeUnitOnBoard(playerId, startPos));

    rounds.startCombat(state);

    CHECK(state.phase() == synera::Phase::Combat);
    bool hasEnemy = false;
    state.forEachUnit([&](const synera::Unit& unit) {
        hasEnemy = hasEnemy || (unit.owner == synera::Owner::EnemyCtrl && unit.onBoard());
    });
    CHECK(hasEnemy);

    auto* player = state.findUnit(playerId);
    REQUIRE(player != nullptr);
    state.removeUnitFromBoard(*player);
    player->runtime.hp = 1;
    player->runtime.mana = 25;

    rounds.enterResolve(state, true);

    CHECK(state.phase() == synera::Phase::Resolve);
    CHECK(state.player().gold == synera::config::InitialGold + synera::config::WinGoldReward);
    CHECK(state.player().currentRound == 2);
    CHECK(state.boardOccupant(startPos) == playerId);
    CHECK(player->boardPos == startPos);
    CHECK(player->runtime.hp == player->derivedStats.maxHp);
    CHECK(player->runtime.mana == 0);

    bool enemiesRemain = false;
    state.forEachUnit([&](const synera::Unit& unit) {
        enemiesRemain = enemiesRemain || unit.owner == synera::Owner::EnemyCtrl;
    });
    CHECK_FALSE(enemiesRemain);
}
