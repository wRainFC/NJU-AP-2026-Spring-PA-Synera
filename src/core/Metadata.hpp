#pragma once

#include "core/Types.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace synera {

struct EquipmentEffect {
    int atkBonus = 0;
    int maxHpBonus = 0;
    int maxManaDelta = 0;
    int minMaxMana = 0;
    float attackIntervalMultiplier = 1.0F;
};

[[nodiscard]] std::string_view phaseName(Phase phase) noexcept;
[[nodiscard]] std::span<const Trait> allTraits() noexcept;
[[nodiscard]] std::string_view traitName(Trait trait) noexcept;
[[nodiscard]] std::string_view equipmentName(EquipmentType equipment) noexcept;
[[nodiscard]] std::optional<EquipmentEffect> equipmentEffect(EquipmentType equipment) noexcept;

}  // namespace synera
