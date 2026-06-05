#include <catch2/catch_test_macros.hpp>

#include "board/HexGrid.hpp"
#include "config/AnimationConfig.hpp"
#include "systems/CombatEvents.hpp"
#include "ui/CombatAnimationController.hpp"
#include "ui/Layout.hpp"

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

}  // namespace

TEST_CASE("CombatAnimationController exposes movement offsets from move events", "[ui][animation]") {
    synera::CombatAnimationController controller;
    synera::Layout layout;
    const synera::UnitId unitId = 42;

    const synera::CombatEvent event{
        .type     = synera::CombatEventType::UnitMoved,
        .actionId = 0,
        .actionProfileId = {},
        .sourceId = unitId,
        .from     = pos(0, 4),
        .to       = pos(1, 4),
    };
    controller.addEvents(std::span<const synera::CombatEvent>{&event, 1}, layout);

    auto model = controller.readModel();
    REQUIRE(model.units.size() == 1);
    CHECK(model.units.front().unitId == unitId);
    CHECK(model.units.front().state == synera::UnitState::Moving);
    CHECK(model.units.front().overridesState);
    CHECK(model.units.front().offset.x != 0.0F);

    controller.update(synera::config::UnitMoveVisualDurationSeconds);
    CHECK(controller.readModel().units.empty());
}

TEST_CASE("CombatAnimationController turns ranged attacks into delayed projectiles and impacts",
          "[ui][animation]") {
    synera::CombatAnimationController controller;
    synera::Layout layout;
    const synera::CombatEvent event{
        .type                  = synera::CombatEventType::AttackStarted,
        .actionId              = 1,
        .actionProfileId       = "storm_archer.basic_arrow",
        .sourceId              = 1,
        .targetId              = 2,
        .from                  = pos(0, 4),
        .to                    = pos(3, 3),
        .attackKind            = synera::AttackVisualKind::Ranged,
        .actionDurationSeconds = 0.44F,
        .hitDelaySeconds       = 0.22F,
    };

    controller.addEvents(std::span<const synera::CombatEvent>{&event, 1}, layout);
    CHECK_FALSE(controller.readModel().projectiles.empty());

    const synera::CombatEvent damage{
        .type                  = synera::CombatEventType::DamageDealt,
        .actionId              = 1,
        .actionProfileId       = "storm_archer.basic_arrow",
        .sourceId              = 1,
        .targetId              = 2,
        .from                  = pos(0, 4),
        .to                    = pos(3, 3),
        .amount                = 48,
        .attackKind            = synera::AttackVisualKind::Ranged,
        .actionDurationSeconds = 0.44F,
        .hitDelaySeconds       = 0.22F,
    };

    controller.addEvents(std::span<const synera::CombatEvent>{&damage, 1}, layout);
    CHECK_FALSE(controller.readModel().impacts.empty());
}

TEST_CASE("CombatAnimationController adds melee lunge and hit impact", "[ui][animation]") {
    synera::CombatAnimationController controller;
    synera::Layout layout;
    const synera::CombatEvent event{
        .type                  = synera::CombatEventType::AttackStarted,
        .actionId              = 3,
        .actionProfileId       = "iron_guard.basic_slash",
        .sourceId              = 7,
        .targetId              = 8,
        .from                  = pos(3, 4),
        .to                    = pos(3, 3),
        .attackKind            = synera::AttackVisualKind::Melee,
        .actionDurationSeconds = 0.36F,
        .hitDelaySeconds       = 0.14F,
    };

    controller.addEvents(std::span<const synera::CombatEvent>{&event, 1}, layout);
    controller.update(synera::config::BasicAttackVisualDurationSeconds * 0.25F);

    auto model = controller.readModel();
    REQUIRE(model.units.size() == 1);
    CHECK(model.units.front().unitId == 7);
    CHECK(model.units.front().state == synera::UnitState::Attacking);
    CHECK(model.units.front().offset.y != 0.0F);

    const synera::CombatEvent damage{
        .type                  = synera::CombatEventType::DamageDealt,
        .actionId              = 3,
        .actionProfileId       = "iron_guard.basic_slash",
        .sourceId              = 7,
        .targetId              = 8,
        .from                  = pos(3, 4),
        .to                    = pos(3, 3),
        .amount                = 32,
        .attackKind            = synera::AttackVisualKind::Melee,
        .actionDurationSeconds = 0.36F,
        .hitDelaySeconds       = 0.14F,
    };
    controller.addEvents(std::span<const synera::CombatEvent>{&damage, 1}, layout);
    CHECK_FALSE(controller.readModel().impacts.empty());
}
