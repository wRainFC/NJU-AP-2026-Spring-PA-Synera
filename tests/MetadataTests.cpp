#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/Metadata.hpp"
#include "core/UnitCatalog.hpp"

#include <ranges>
#include <string_view>

TEST_CASE("Metadata exposes display names and equipment effects", "[metadata]") {
    const auto traits = synera::allTraits();

    CHECK(traits.size() == 6);
    CHECK(std::ranges::find(traits, synera::Trait::Warrior) != traits.end());
    CHECK(std::ranges::find(traits, synera::Trait::Assassin) != traits.end());
    CHECK(synera::phaseName(synera::Phase::Prep) == "Prep");
    CHECK(synera::traitName(synera::Trait::Mystic) == "Mystic");
    CHECK(synera::equipmentName(synera::EquipmentType::ManaCrystal) == "Crystal");

    const auto sword = synera::equipmentEffect(synera::EquipmentType::IronSword);
    REQUIRE(sword.has_value());
    CHECK(sword->atkBonus == 15);

    const auto vest = synera::equipmentEffect(synera::EquipmentType::ChainVest);
    REQUIRE(vest.has_value());
    CHECK(vest->maxHpBonus == 150);

    const auto glove = synera::equipmentEffect(synera::EquipmentType::SwiftGlove);
    REQUIRE(glove.has_value());
    CHECK(glove->attackIntervalMultiplier == Catch::Approx(0.8F));

    const auto crystal = synera::equipmentEffect(synera::EquipmentType::ManaCrystal);
    REQUIRE(crystal.has_value());
    CHECK(crystal->maxManaDelta == -30);
    CHECK(crystal->minMaxMana == 20);
}

TEST_CASE("Unit derived stats applies table-driven equipment effects", "[metadata][unit]") {
    auto equipped = [](synera::EquipmentType equipment) {
        auto unit = synera::UnitCatalog::createUnit(1, "ember_mage", synera::Owner::PlayerCtrl);
        unit.equipment = equipment;
        unit.recomputeDerivedStats();
        return unit;
    };

    const auto sword = equipped(synera::EquipmentType::IronSword);
    CHECK(sword.derivedStats.atk == sword.baseStats.atk + 15);

    const auto vest = equipped(synera::EquipmentType::ChainVest);
    CHECK(vest.derivedStats.maxHp == vest.baseStats.maxHp + 150);

    const auto glove = equipped(synera::EquipmentType::SwiftGlove);
    CHECK(glove.derivedStats.attackInterval == Catch::Approx(glove.baseStats.attackInterval * 0.8F));

    const auto crystal = equipped(synera::EquipmentType::ManaCrystal);
    CHECK(crystal.derivedStats.maxMana == crystal.baseStats.maxMana - 30);
}
