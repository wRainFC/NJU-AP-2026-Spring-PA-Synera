#include <catch2/catch_test_macros.hpp>

#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "systems/CombatSystem.hpp"

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

struct CombatFixture {
    synera::GameState state;
    synera::CombatSystem combat;
    synera::UnitId player = synera::InvalidUnitId;
    synera::UnitId enemy = synera::InvalidUnitId;
    synera::AxialPos playerPos = pos(3, 4);
    synera::AxialPos enemyPos = pos(3, 3);

    CombatFixture() {
        player = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
        enemy = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);
        REQUIRE(state.placeUnitOnBoard(player, playerPos));
        REQUIRE(state.placeUnitOnBoard(enemy, enemyPos));
        state.setPhase(synera::Phase::Combat);
    }
};

}  // namespace

TEST_CASE("CombatSystem casts full mana ability and clears mana", "[combat]") {
    CombatFixture fixture;
    auto* player = fixture.state.findUnit(fixture.player);
    auto* enemy = fixture.state.findUnit(fixture.enemy);
    REQUIRE(player != nullptr);
    REQUIRE(enemy != nullptr);

    player->runtime.mana = player->derivedStats.maxMana;
    fixture.combat.update(fixture.state, 0.0F);

    CHECK(player->runtime.mana == 0);
    CHECK(enemy->runtime.hp == enemy->derivedStats.maxHp - 55);
    CHECK(enemy->runtime.state == synera::UnitState::Stunned);
}

TEST_CASE("CombatSystem recovers stunned units after the stun timer", "[combat]") {
    CombatFixture fixture;
    auto* enemy = fixture.state.findUnit(fixture.enemy);
    REQUIRE(enemy != nullptr);

    enemy->runtime.state = synera::UnitState::Stunned;
    enemy->runtime.stunTimer = 0.1F;

    fixture.combat.update(fixture.state, 0.2F);

    CHECK(enemy->runtime.state == synera::UnitState::Idle);
    CHECK(enemy->runtime.stunTimer == 0.0F);
}

TEST_CASE("CombatSystem removes dead units from board occupancy", "[combat]") {
    CombatFixture fixture;
    auto* enemy = fixture.state.findUnit(fixture.enemy);
    REQUIRE(enemy != nullptr);

    enemy->receiveDamage(999);
    REQUIRE_FALSE(enemy->alive());

    fixture.combat.update(fixture.state, 0.0F);

    CHECK_FALSE(fixture.state.boardOccupant(fixture.enemyPos).has_value());
    CHECK_FALSE(enemy->boardPos.has_value());
}
