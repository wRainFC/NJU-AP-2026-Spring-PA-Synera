#pragma once

#include "core/Types.hpp"

#include <cstdint>
#include <string>

namespace synera {

enum class CombatEventType { UnitMoved, AttackStarted, DamageDealt, AbilityCast, UnitDied };

struct CombatEvent {
    CombatEventType type = CombatEventType::AttackStarted;
    std::uint64_t actionId = 0;
    std::string actionProfileId;
    UnitId sourceId      = InvalidUnitId;
    UnitId targetId      = InvalidUnitId;
    AxialPos from{};
    AxialPos to{};
    int amount = 0;
    AttackVisualKind attackKind = AttackVisualKind::Melee;
    float actionDurationSeconds = 0.0F;
    float hitDelaySeconds = 0.0F;
};

}  // namespace synera
