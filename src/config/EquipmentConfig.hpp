#pragma once

#include "config/GameConfig.hpp"
#include "core/Types.hpp"

#include <array>

namespace synera::config {

struct EquipmentDropRule {
    bool requiresWin;
    int chancePercent;
};

inline constexpr std::array EquipmentPool{
    EquipmentType::IronSword,
    EquipmentType::ChainVest,
    EquipmentType::SwiftGlove,
    EquipmentType::ManaCrystal,
};

inline constexpr EquipmentDropRule RoundDropRule{
    .requiresWin   = true,
    .chancePercent = EquipmentDropChancePercent,
};

}  // namespace synera::config
