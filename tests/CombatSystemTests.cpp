#include <catch2/catch_test_macros.hpp>

#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "systems/CombatSystem.hpp"
#include "systems/SynergySystem.hpp"

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

TEST_CASE("CombatSystem applies Ranger double basic attack synergy", "[combat][synergy]") {
    synera::GameState state;
    synera::CombatSystem combat;
    synera::SynergySystem synergies;
    const synera::UnitId first = state.createUnit("storm_archer", synera::Owner::PlayerCtrl);
    const synera::UnitId second = state.createUnit("storm_archer", synera::Owner::PlayerCtrl);
    const synera::UnitId enemyId = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    REQUIRE(state.placeUnitOnBoard(first, pos(3, 4)));
    REQUIRE(state.placeUnitOnBoard(second, pos(5, 4)));
    REQUIRE(state.placeUnitOnBoard(enemyId, pos(3, 3)));
    synergies.recompute(state);

    auto* firstArcher = state.findUnit(first);
    auto* secondArcher = state.findUnit(second);
    auto* enemy = state.findUnit(enemyId);
    REQUIRE(firstArcher != nullptr);
    REQUIRE(secondArcher != nullptr);
    REQUIRE(enemy != nullptr);
    CHECK(firstArcher->mechanics.doubleBasicAttack);

    secondArcher->derivedStats.attackInterval = 999.0F;
    state.setPhase(synera::Phase::Combat);
    combat.update(state, 1.0F);

    CHECK(enemy->runtime.hp == enemy->derivedStats.maxHp - firstArcher->derivedStats.atk * 2);
    CHECK(firstArcher->runtime.mana == 10);
}

TEST_CASE("CombatSystem targets lowest hp before board-order ties", "[combat][targeting]") {
    synera::GameState state;
    synera::CombatSystem combat;
    const synera::UnitId playerId = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId highHpEnemy = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);
    const synera::UnitId lowHpEnemy = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    REQUIRE(state.placeUnitOnBoard(playerId, pos(0, 4)));
    REQUIRE(state.placeUnitOnBoard(highHpEnemy, pos(1, 2)));
    REQUIRE(state.placeUnitOnBoard(lowHpEnemy, pos(1, 3)));

    auto* lowHp = state.findUnit(lowHpEnemy);
    REQUIRE(lowHp != nullptr);
    lowHp->runtime.hp = 1;

    state.setPhase(synera::Phase::Combat);
    combat.update(state, 0.0F);

    const auto* player = state.findUnit(playerId);
    REQUIRE(player != nullptr);
    CHECK(player->runtime.targetId == lowHpEnemy);
}

TEST_CASE("CombatSystem targets left-to-right on equal hp", "[combat][targeting]") {
    synera::GameState state;
    synera::CombatSystem combat;
    const synera::UnitId playerId = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId leftEnemy = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);
    const synera::UnitId rightEnemy = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    REQUIRE(state.placeUnitOnBoard(playerId, pos(3, 4)));
    REQUIRE(state.placeUnitOnBoard(leftEnemy, pos(2, 3)));
    REQUIRE(state.placeUnitOnBoard(rightEnemy, pos(3, 3)));

    state.setPhase(synera::Phase::Combat);
    combat.update(state, 0.0F);

    const auto* player = state.findUnit(playerId);
    REQUIRE(player != nullptr);
    CHECK(player->runtime.targetId == leftEnemy);
}

TEST_CASE("CombatSystem targets lower board cells before upper cells on equal hp and column",
          "[combat][targeting]") {
    synera::GameState state;
    synera::CombatSystem combat;
    const synera::UnitId playerId = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId upperEnemy = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);
    const synera::UnitId lowerEnemy = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    REQUIRE(state.placeUnitOnBoard(playerId, pos(0, 4)));
    REQUIRE(state.placeUnitOnBoard(upperEnemy, pos(1, 2)));
    REQUIRE(state.placeUnitOnBoard(lowerEnemy, pos(1, 3)));

    state.setPhase(synera::Phase::Combat);
    combat.update(state, 0.0F);

    const auto* player = state.findUnit(playerId);
    REQUIRE(player != nullptr);
    CHECK(player->runtime.targetId == lowerEnemy);
}
