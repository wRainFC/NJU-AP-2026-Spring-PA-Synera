#include <catch2/catch_test_macros.hpp>

#include "core/UnitCatalog.hpp"
#include "core/abilities/BasicAbilities.hpp"

TEST_CASE("UnitCatalog creates configured hero templates", "[unit-catalog]") {
    const auto ironGuard = synera::UnitCatalog::createUnit(1, "iron_guard", synera::Owner::PlayerCtrl);
    CHECK(ironGuard.name == "Iron Guard");
    CHECK(ironGuard.basicAttackProfileId == "iron_guard.basic_slash");
    CHECK(ironGuard.derivedStats.maxHp == 360);
    REQUIRE(dynamic_cast<synera::StunStrikeAbility *>(ironGuard.ability.get()) != nullptr);

    const auto emberMage = synera::UnitCatalog::createUnit(2, "ember_mage", synera::Owner::PlayerCtrl);
    CHECK(emberMage.name == "Ember Mage");
    CHECK(emberMage.basicAttackProfileId == "ember_mage.basic_bolt");
    CHECK(emberMage.derivedStats.range == 3);
    REQUIRE(dynamic_cast<synera::FireLineAbility *>(emberMage.ability.get()) != nullptr);

    const auto fieldMedic = synera::UnitCatalog::createUnit(3, "field_medic", synera::Owner::PlayerCtrl);
    CHECK(fieldMedic.name == "Field Medic");
    CHECK(fieldMedic.basicAttackProfileId == "field_medic.basic_pulse");
    CHECK(fieldMedic.derivedStats.maxMana == 55);
    REQUIRE(dynamic_cast<synera::HealingAuraAbility *>(fieldMedic.ability.get()) != nullptr);

    const auto stormArcher = synera::UnitCatalog::createUnit(4, "storm_archer", synera::Owner::PlayerCtrl);
    CHECK(stormArcher.name == "Storm Archer");
    CHECK(stormArcher.basicAttackProfileId == "storm_archer.basic_arrow");
    CHECK(stormArcher.derivedStats.range == 4);
    REQUIRE(dynamic_cast<synera::StunStrikeAbility *>(stormArcher.ability.get()) != nullptr);

    const auto dummy = synera::UnitCatalog::createUnit(5, "training_dummy", synera::Owner::EnemyCtrl);
    CHECK(dummy.name == "Training Dummy");
    CHECK(dummy.basicAttackProfileId == "training_dummy.basic_slam");
    CHECK(dummy.derivedStats.atk == 18);
    REQUIRE(dynamic_cast<synera::NoopAbility *>(dummy.ability.get()) != nullptr);
}

TEST_CASE("UnitCatalog falls back for unknown templates", "[unit-catalog]") {
    const auto unknown = synera::UnitCatalog::createUnit(1, "unknown_template", synera::Owner::PlayerCtrl);

    CHECK(unknown.name == "Unknown");
    CHECK(unknown.basicAttackProfileId == "default.basic_melee");
    CHECK(unknown.derivedStats.maxHp == 360);
    REQUIRE(dynamic_cast<synera::NoopAbility *>(unknown.ability.get()) != nullptr);
}
