#include <catch2/catch_test_macros.hpp>

#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/SynergySystem.hpp"
#include "systems/UpgradeSystem.hpp"

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

}  // namespace

TEST_CASE("Player population upgrade spends gold and increases cap", "[economy]") {
    synera::GameState state;

    const int gold = state.player().gold;
    REQUIRE(state.player().upgradePopulation());

    CHECK(state.player().gold == gold - 4);
    CHECK(state.player().level == 2);
    CHECK(state.player().populationCap == synera::config::InitialPopulationCap + 1);
}

TEST_CASE("UpgradeSystem merges three matching player units into the gained unit", "[upgrade]") {
    synera::GameState state;
    synera::UpgradeSystem upgrades;
    const synera::UnitId first  = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId second = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId gained = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);

    REQUIRE(state.placeUnitOnBench(first, 0));
    REQUIRE(state.placeUnitOnBench(second, 1));
    REQUIRE(state.placeUnitOnBench(gained, 2));

    CHECK(upgrades.tryMergeAfterGain(state, gained));

    CHECK(state.findUnit(first) == nullptr);
    CHECK(state.findUnit(second) == nullptr);
    auto* merged = state.findUnit(gained);
    REQUIRE(merged != nullptr);
    CHECK(merged->star == 2);
    CHECK(merged->benchSlot == 2);
    CHECK(merged->derivedStats.maxHp == 612);
    CHECK(merged->runtime.hp == merged->derivedStats.maxHp);
}

TEST_CASE("SynergySystem counts only player board units", "[synergy]") {
    synera::GameState state;
    synera::SynergySystem synergies;
    const synera::UnitId guard = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId medic = state.createUnit("field_medic", synera::Owner::PlayerCtrl);
    const synera::UnitId mage  = state.createUnit("ember_mage", synera::Owner::PlayerCtrl);

    REQUIRE(state.placeUnitOnBoard(guard, pos(0, 4)));
    REQUIRE(state.placeUnitOnBoard(medic, pos(1, 4)));
    REQUIRE(state.placeUnitOnBench(mage, 0));

    synergies.recompute(state);

    const synera::TraitSummary guardian = synera::summarizeTrait(state, synera::Trait::Guardian);
    CHECK(guardian.count == 2);
    CHECK(guardian.activationThreshold == 2);
    CHECK(guardian.active);
    CHECK(synera::countPlayerBoardTrait(state, synera::Trait::Mage) == 0);

    const auto* guardUnit = state.findUnit(guard);
    const auto* medicUnit = state.findUnit(medic);
    const auto* mageUnit  = state.findUnit(mage);
    REQUIRE(guardUnit != nullptr);
    REQUIRE(medicUnit != nullptr);
    REQUIRE(mageUnit != nullptr);

    CHECK(guardUnit->derivedStats.maxHp == guardUnit->baseStats.maxHp + 80);
    CHECK(medicUnit->derivedStats.maxHp == medicUnit->baseStats.maxHp + 80);
    CHECK(guardUnit->runtime.hp == guardUnit->derivedStats.maxHp);
    CHECK(medicUnit->runtime.hp == medicUnit->derivedStats.maxHp);
    CHECK(mageUnit->derivedStats.maxHp == mageUnit->baseStats.maxHp);
    CHECK(mageUnit->derivedStats.maxMana == mageUnit->baseStats.maxMana);

    state.findUnit(guard)->runtime.hp -= 50;
    const int damagedHp = state.findUnit(guard)->runtime.hp;
    synergies.recompute(state);
    CHECK(state.findUnit(guard)->runtime.hp == damagedHp);
}

TEST_CASE("EquipmentSystem grants drops and equips from the pool", "[equipment]") {
    synera::GameState state;
    synera::EquipmentSystem equipment{123};
    const synera::UnitId unitId = state.createUnit("ember_mage", synera::Owner::PlayerCtrl);

    CHECK(equipment.tryGrantRoundDrop(state, true));
    CHECK(state.equipmentPool().size() == 1);
    CHECK_FALSE(equipment.tryGrantRoundDrop(state, false));
    CHECK(state.equipmentPool().size() == 1);

    state.addEquipment(synera::EquipmentType::IronSword);
    REQUIRE(equipment.equipFromPool(state, 1, unitId));
    CHECK(state.equipmentPool().size() == 1);

    auto* unit = state.findUnit(unitId);
    REQUIRE(unit != nullptr);
    CHECK(unit->equipment == synera::EquipmentType::IronSword);
    CHECK(unit->derivedStats.atk == unit->baseStats.atk + 15);
    CHECK_FALSE(equipment.equip(state, unitId, synera::EquipmentType::ChainVest));
}

TEST_CASE("Max hp equipment heals by the gained maximum hp", "[equipment]") {
    synera::GameState state;
    synera::EquipmentSystem equipment;
    const synera::UnitId unitId = state.createUnit("ember_mage", synera::Owner::PlayerCtrl);
    auto* unit                  = state.findUnit(unitId);
    REQUIRE(unit != nullptr);

    unit->runtime.hp -= 40;
    const int previousHp    = unit->runtime.hp;
    const int previousMaxHp = unit->derivedStats.maxHp;

    REQUIRE(equipment.equip(state, unitId, synera::EquipmentType::ChainVest));

    CHECK(unit->derivedStats.maxHp == previousMaxHp + 150);
    CHECK(unit->runtime.hp == previousHp + 150);
}
