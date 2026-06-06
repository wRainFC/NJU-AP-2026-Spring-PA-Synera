#include <catch2/catch_test_macros.hpp>

#include "config/CombatActionCatalog.hpp"

#include <filesystem>
#include <fstream>

TEST_CASE("CombatActionCatalog exposes default action profiles", "[combat-actions]") {
    synera::CombatActionCatalog catalog;

    const synera::CombatActionProfile& melee =
        catalog.defaultBasicAttack(synera::AttackVisualKind::Melee);
    CHECK(melee.id == "default.basic_melee");
    CHECK(melee.kind == synera::CombatActionKind::BasicAttack);
    CHECK(melee.attackKind == synera::AttackVisualKind::Melee);
    REQUIRE(melee.hitTimes.size() == 1);
    CHECK(melee.hitTimes.front() > 0.0F);

    const synera::CombatActionProfile& ranged =
        catalog.defaultBasicAttack(synera::AttackVisualKind::Ranged);
    CHECK(ranged.id == "default.basic_ranged");
    CHECK(ranged.animationProfileId == "default.basic_ranged");

    const synera::CombatActionProfile& ability = catalog.defaultAbility();
    CHECK(ability.id == "default.ability_cast");
    CHECK(ability.kind == synera::CombatActionKind::Ability);

    const synera::CombatActionProfile& fireLine = catalog.profile("fire_line.cast");
    CHECK(fireLine.id == "fire_line.cast");
    CHECK(fireLine.animationProfileId == "fire_line.cast");
}

TEST_CASE("CombatActionCatalog loads manifest profiles over defaults", "[combat-actions]") {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "synera_combat_actions_test.json";
    {
        std::ofstream out{path};
        out << R"json({
    "combatActions": {
        "profiles": [
            {
                "id": "test.basic",
                "kind": "basic_attack",
                "attackKind": "ranged",
                "durationSeconds": 0.7,
                "hitTimes": [0.33],
                "animationProfileId": "test.animation"
            }
        ]
    }
})json";
    }

    synera::CombatActionCatalog catalog;
    REQUIRE(catalog.loadFromFile(path));

    const synera::CombatActionProfile& profile = catalog.profile("test.basic");
    CHECK(profile.id == "test.basic");
    CHECK(profile.attackKind == synera::AttackVisualKind::Ranged);
    CHECK(profile.durationSeconds == 0.7F);
    REQUIRE(profile.hitTimes.size() == 1);
    CHECK(profile.hitTimes.front() == 0.33F);
    CHECK(profile.animationProfileId == "test.animation");

    std::filesystem::remove(path);
}
