#include <catch2/catch_test_macros.hpp>

#include "config/CombatAnimationCatalog.hpp"

#include <filesystem>
#include <fstream>

TEST_CASE("CombatAnimationCatalog exposes default animation profiles", "[combat-animations]") {
    synera::CombatAnimationCatalog catalog;

    const synera::CombatAnimationProfile& melee =
        catalog.defaultBasicAttack(synera::AttackVisualKind::Melee);
    CHECK(melee.id == "default.basic_melee");
    CHECK(melee.attackKind == synera::AttackVisualKind::Melee);
    CHECK(melee.unitState == synera::UnitState::Attacking);
    CHECK(melee.lungePixels > 0.0F);

    const synera::CombatAnimationProfile& ranged =
        catalog.defaultBasicAttack(synera::AttackVisualKind::Ranged);
    CHECK(ranged.projectileEnabled);
    CHECK(ranged.projectileClip.id == "projectile.basic");

    const synera::CombatAnimationProfile& fireLine = catalog.profile("fire_line.cast");
    CHECK(fireLine.projectileEnabled);
    CHECK(fireLine.projectileClip.id == "projectile.fire_line");
    CHECK(fireLine.damageImpactClip.id == "impact.fire");
}

TEST_CASE("CombatAnimationCatalog loads manifest profiles over defaults", "[combat-animations]") {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "synera_combat_animations_test.json";
    {
        std::ofstream out{path};
        out << R"json({
    "combatAnimations": {
        "profiles": [
            {
                "id": "test.animation",
                "unitState": "casting",
                "attackKind": "ranged",
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
                "castImpactClip": {
                    "id": "test.cast",
                    "texturePath": "textures/effects/cast.png",
                    "frameCount": 3,
                    "framesPerSecond": 10.0
                },
                "damageImpactClip": {
                    "id": "test.impact",
                    "texturePath": "textures/effects/test.png",
                    "frameCount": 4,
                    "framesPerSecond": 16.0
                },
                "healImpactClip": {
                    "id": "test.heal",
                    "texturePath": "textures/effects/heal.png",
                    "frameCount": 4,
                    "framesPerSecond": 16.0
                },
                "statusImpactClip": {
                    "id": "test.status",
                    "texturePath": "textures/effects/status.png",
                    "frameCount": 4,
                    "framesPerSecond": 16.0
                },
                "impactDurationSeconds": 0.3
            }
        ]
    }
})json";
    }

    synera::CombatAnimationCatalog catalog;
    REQUIRE(catalog.loadFromFile(path));

    const synera::CombatAnimationProfile& profile = catalog.profile("test.animation");
    CHECK(profile.id == "test.animation");
    CHECK(profile.unitState == synera::UnitState::Casting);
    CHECK(profile.attackKind == synera::AttackVisualKind::Ranged);
    CHECK(profile.unitClip.id == "test.unit");
    CHECK(profile.unitClip.frameCount == 6);
    CHECK(profile.projectileEnabled);
    CHECK(profile.projectileClip.id == "test.projectile");
    CHECK(profile.castImpactClip.id == "test.cast");
    CHECK(profile.damageImpactClip.id == "test.impact");
    CHECK(profile.healImpactClip.id == "test.heal");
    CHECK(profile.statusImpactClip.id == "test.status");

    std::filesystem::remove(path);
}
