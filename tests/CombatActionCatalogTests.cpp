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
    CHECK(ranged.projectileEnabled);

    const synera::CombatActionProfile& ability = catalog.defaultAbility();
    CHECK(ability.id == "default.ability_cast");
    CHECK(ability.kind == synera::CombatActionKind::Ability);
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
                "unitClip": {
                    "id": "test.unit",
                    "texturePath": "textures/actions/test.png",
                    "frameCount": 6,
                    "framesPerSecond": 12.0
                },
                "lungePixels": 0.0,
                "projectileEnabled": true,
                "projectileClip": {
                    "id": "test.projectile",
                    "texturePath": "textures/projectiles/test.png",
                    "frameCount": 2,
                    "framesPerSecond": 8.0
                },
                "projectilePixelsPerSecond": 640.0,
                "impactClip": {
                    "id": "test.impact",
                    "texturePath": "textures/effects/test.png",
                    "frameCount": 4,
                    "framesPerSecond": 16.0
                },
                "impactDurationSeconds": 0.3
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
    CHECK(profile.unitClip.id == "test.unit");
    CHECK(profile.unitClip.frameCount == 6);
    CHECK(profile.projectileEnabled);
    CHECK(profile.projectileClip.id == "test.projectile");
    CHECK(profile.impactClip.id == "test.impact");

    std::filesystem::remove(path);
}
