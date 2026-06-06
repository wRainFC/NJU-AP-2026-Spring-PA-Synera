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
    synera::UnitId player      = synera::InvalidUnitId;
    synera::UnitId enemy       = synera::InvalidUnitId;
    synera::AxialPos playerPos = pos(3, 4);
    synera::AxialPos enemyPos  = pos(3, 3);

    CombatFixture() {
        player = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
        enemy  = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);
        REQUIRE(state.placeUnitOnBoard(player, playerPos));
        REQUIRE(state.placeUnitOnBoard(enemy, enemyPos));
        state.setPhase(synera::Phase::Combat);
    }
};

}  // namespace

TEST_CASE("CombatSystem casts full mana ability and clears mana", "[combat]") {
    CombatFixture fixture;
    auto* player = fixture.state.findUnit(fixture.player);
    auto* enemy  = fixture.state.findUnit(fixture.enemy);
    REQUIRE(player != nullptr);
    REQUIRE(enemy != nullptr);

    player->runtime.mana = player->derivedStats.maxMana;
    fixture.combat.update(fixture.state, 0.0F);

    CHECK(player->runtime.mana == 0);
    CHECK(enemy->runtime.hp == enemy->derivedStats.maxHp);

    fixture.combat.update(fixture.state, 0.24F);

    CHECK(enemy->runtime.hp == enemy->derivedStats.maxHp - 55);
    CHECK(enemy->runtime.state == synera::UnitState::Stunned);

    bool sawDamage = false;
    bool sawStatus = false;
    for (const synera::CombatEvent& event : fixture.combat.events()) {
        sawDamage = sawDamage || (event.type == synera::CombatEventType::DamageDealt &&
                                  event.actionProfileId == "stun_strike.cast" &&
                                  event.animationProfileId == "stun_strike.cast" &&
                                  event.targetId == fixture.enemy && event.amount == 55);
        sawStatus = sawStatus || (event.type == synera::CombatEventType::StatusApplied &&
                                  event.actionProfileId == "stun_strike.cast" &&
                                  event.animationProfileId == "stun_strike.cast" &&
                                  event.targetId == fixture.enemy &&
                                  event.statusDurationSeconds > 0.0F);
    }
    CHECK(sawDamage);
    CHECK(sawStatus);
}

TEST_CASE("CombatSystem emits damage events for Fire Line ability targets", "[combat][events]") {
    synera::GameState state;
    synera::CombatSystem combat;
    const synera::UnitId mageId = state.createUnit("ember_mage", synera::Owner::PlayerCtrl);
    const synera::UnitId firstEnemyId = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);
    const synera::UnitId secondEnemyId = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    REQUIRE(state.placeUnitOnBoard(mageId, pos(3, 4)));
    REQUIRE(state.placeUnitOnBoard(firstEnemyId, pos(3, 3)));
    REQUIRE(state.placeUnitOnBoard(secondEnemyId, pos(4, 2)));
    state.setPhase(synera::Phase::Combat);

    auto* mage = state.findUnit(mageId);
    auto* firstEnemy = state.findUnit(firstEnemyId);
    auto* secondEnemy = state.findUnit(secondEnemyId);
    REQUIRE(mage != nullptr);
    REQUIRE(firstEnemy != nullptr);
    REQUIRE(secondEnemy != nullptr);
    mage->runtime.mana = mage->derivedStats.maxMana;
    firstEnemy->derivedStats.attackInterval = 999.0F;
    secondEnemy->derivedStats.attackInterval = 999.0F;

    combat.update(state, 0.0F);
    combat.update(state, 0.31F);

    int fireLineDamageEvents = 0;
    for (const synera::CombatEvent& event : combat.events()) {
        if (event.type == synera::CombatEventType::DamageDealt &&
            event.actionProfileId == "fire_line.cast") {
            ++fireLineDamageEvents;
            CHECK(event.amount == 85);
            CHECK(event.animationProfileId == "fire_line.cast");
        }
    }
    CHECK(fireLineDamageEvents == 2);
    CHECK(firstEnemy->runtime.hp == firstEnemy->derivedStats.maxHp - 85);
    CHECK(secondEnemy->runtime.hp == secondEnemy->derivedStats.maxHp - 85);
}

TEST_CASE("CombatSystem emits heal events for Healing Aura ability targets", "[combat][events]") {
    synera::GameState state;
    synera::CombatSystem combat;
    const synera::UnitId medicId = state.createUnit("field_medic", synera::Owner::PlayerCtrl);
    const synera::UnitId allyId = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId enemyId = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    REQUIRE(state.placeUnitOnBoard(medicId, pos(3, 4)));
    REQUIRE(state.placeUnitOnBoard(allyId, pos(4, 4)));
    REQUIRE(state.placeUnitOnBoard(enemyId, pos(3, 3)));
    state.setPhase(synera::Phase::Combat);

    auto* medic = state.findUnit(medicId);
    auto* ally = state.findUnit(allyId);
    auto* enemy = state.findUnit(enemyId);
    REQUIRE(medic != nullptr);
    REQUIRE(ally != nullptr);
    REQUIRE(enemy != nullptr);
    medic->runtime.mana = medic->derivedStats.maxMana;
    ally->runtime.hp -= 90;
    enemy->derivedStats.attackInterval = 999.0F;

    combat.update(state, 0.0F);
    combat.update(state, 0.33F);

    bool sawHeal = false;
    for (const synera::CombatEvent& event : combat.events()) {
        sawHeal = sawHeal || (event.type == synera::CombatEventType::HealReceived &&
                              event.actionProfileId == "healing_aura.cast" &&
                              event.animationProfileId == "healing_aura.cast" &&
                              event.targetId == allyId && event.amount == 70);
    }
    CHECK(sawHeal);
    CHECK(ally->runtime.hp == ally->derivedStats.maxHp - 20);
}

TEST_CASE("CombatSystem recovers stunned units after the stun timer", "[combat]") {
    CombatFixture fixture;
    auto* enemy = fixture.state.findUnit(fixture.enemy);
    REQUIRE(enemy != nullptr);

    enemy->runtime.state     = synera::UnitState::Stunned;
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
    const synera::UnitId first   = state.createUnit("storm_archer", synera::Owner::PlayerCtrl);
    const synera::UnitId second  = state.createUnit("storm_archer", synera::Owner::PlayerCtrl);
    const synera::UnitId enemyId = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    REQUIRE(state.placeUnitOnBoard(first, pos(3, 4)));
    REQUIRE(state.placeUnitOnBoard(second, pos(5, 4)));
    REQUIRE(state.placeUnitOnBoard(enemyId, pos(3, 3)));
    synergies.recompute(state);

    auto* firstArcher  = state.findUnit(first);
    auto* secondArcher = state.findUnit(second);
    auto* enemy        = state.findUnit(enemyId);
    REQUIRE(firstArcher != nullptr);
    REQUIRE(secondArcher != nullptr);
    REQUIRE(enemy != nullptr);
    CHECK(firstArcher->mechanics.doubleBasicAttack);

    secondArcher->derivedStats.attackInterval = 999.0F;
    enemy->derivedStats.attackInterval = 999.0F;
    state.setPhase(synera::Phase::Combat);
    combat.update(state, 1.0F);
    CHECK(enemy->runtime.hp == enemy->derivedStats.maxHp);

    combat.update(state, 0.30F);

    CHECK(enemy->runtime.hp == enemy->derivedStats.maxHp - firstArcher->derivedStats.atk * 2);
    CHECK(firstArcher->runtime.mana == 10);
}

TEST_CASE("CombatSystem emits combat events for attacks and damage", "[combat][events]") {
    CombatFixture fixture;
    auto* player = fixture.state.findUnit(fixture.player);
    REQUIRE(player != nullptr);

    fixture.combat.update(fixture.state, player->derivedStats.attackInterval);

    auto events = fixture.combat.events();
    REQUIRE(events.size() == 1);
    CHECK(events[0].type == synera::CombatEventType::AttackStarted);
    CHECK(events[0].sourceId == fixture.player);
    CHECK(events[0].targetId == fixture.enemy);
    CHECK(events[0].attackKind == synera::AttackVisualKind::Melee);
    CHECK(events[0].actionProfileId == "iron_guard.basic_slash");
    CHECK(events[0].animationProfileId == "iron_guard.basic_slash");
    CHECK(events[0].hitDelaySeconds > 0.0F);

    fixture.combat.update(fixture.state, events[0].hitDelaySeconds);

    events = fixture.combat.events();
    REQUIRE(events.size() >= 1);
    CHECK(events[0].type == synera::CombatEventType::DamageDealt);
    CHECK(events[0].actionProfileId == "iron_guard.basic_slash");
    CHECK(events[0].animationProfileId == "iron_guard.basic_slash");
    CHECK(events[0].amount == player->derivedStats.atk);
}

TEST_CASE("CombatSystem emits movement and ranged attack events", "[combat][events]") {
    synera::GameState state;
    synera::CombatSystem combat;
    const synera::UnitId archerId = state.createUnit("storm_archer", synera::Owner::PlayerCtrl);
    const synera::UnitId enemyId  = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    REQUIRE(state.placeUnitOnBoard(archerId, pos(0, 4)));
    REQUIRE(state.placeUnitOnBoard(enemyId, pos(7, 1)));
    state.setPhase(synera::Phase::Combat);

    combat.update(state, 1.0F);

    bool sawMove = false;
    bool sawRangedAttack = false;
    for (const synera::CombatEvent& event : combat.events()) {
        sawMove = sawMove || event.type == synera::CombatEventType::UnitMoved;
        sawRangedAttack = sawRangedAttack ||
                          (event.type == synera::CombatEventType::AttackStarted &&
                           event.attackKind == synera::AttackVisualKind::Ranged);
    }
    CHECK((sawMove || sawRangedAttack));
}

TEST_CASE("CombatSystem targets lowest hp before board-order ties", "[combat][targeting]") {
    synera::GameState state;
    synera::CombatSystem combat;
    const synera::UnitId playerId    = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId highHpEnemy = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);
    const synera::UnitId lowHpEnemy  = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

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
    const synera::UnitId playerId   = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId leftEnemy  = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);
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
    const synera::UnitId playerId   = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
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
