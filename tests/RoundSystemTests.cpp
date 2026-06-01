#include <catch2/catch_test_macros.hpp>

#include "app/GameConfig.hpp"
#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "systems/RoundSystem.hpp"

#include <algorithm>
#include <ranges>
#include <string>
#include <vector>

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

[[nodiscard]] std::vector<const synera::Unit*> enemyUnits(const synera::GameState& state) {
    std::vector<const synera::Unit*> enemies;
    state.forEachUnit([&](const synera::Unit& unit) {
        if (unit.owner == synera::Owner::EnemyCtrl) {
            enemies.push_back(&unit);
        }
    });
    std::ranges::sort(enemies, {}, &synera::Unit::templateId);
    return enemies;
}

}  // namespace

TEST_CASE("RoundSystem starts combat and later restores player formation", "[round]") {
    synera::GameState state;
    synera::RoundSystem rounds;
    const synera::UnitId playerId = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const auto startPos           = pos(0, 4);

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
    player->runtime.hp   = 1;
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

TEST_CASE("RoundSystem spawns round-specific enemy waves", "[round]") {
    synera::GameState state;
    synera::RoundSystem rounds;

    state.player().currentRound = 2;
    rounds.spawnEnemies(state);
    auto roundTwo = enemyUnits(state);
    REQUIRE(roundTwo.size() == 2);
    CHECK(std::ranges::all_of(roundTwo,
                              [](const synera::Unit* unit) { return unit->templateId == "training_dummy"; }));

    state.player().currentRound = 3;
    rounds.spawnEnemies(state);
    auto roundThree = enemyUnits(state);
    REQUIRE(roundThree.size() == 2);
    CHECK(roundThree[0]->templateId == "ember_mage");
    CHECK(roundThree[1]->templateId == "iron_guard");
}

TEST_CASE("RoundSystem scales later enemy waves by round", "[round]") {
    synera::GameState state;
    synera::RoundSystem rounds;

    state.player().currentRound = 10;
    rounds.spawnEnemies(state);

    const auto enemies = enemyUnits(state);
    REQUIRE(enemies.size() == 3);
    CHECK(std::ranges::all_of(enemies, [](const synera::Unit* unit) {
        return unit->star == 3 && unit->runtime.hp == unit->derivedStats.maxHp;
    }));
}
