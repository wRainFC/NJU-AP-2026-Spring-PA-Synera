#pragma once

#include "config/GameConfig.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string_view>

namespace synera::config {

using ShopTierWeights = std::array<int, ShopMaxTier>;

struct ShopPoolEntry {
    std::string_view templateId;
    int cost   = 1;
    int tier   = 1;
    int weight = 1;
};

inline constexpr std::array ShopUnitPool{
    ShopPoolEntry{.templateId = "iron_guard", .cost = 1, .tier = 1, .weight = 55},
    ShopPoolEntry{.templateId = "field_medic", .cost = 1, .tier = 1, .weight = 45},
    ShopPoolEntry{.templateId = "ember_mage", .cost = 2, .tier = 2, .weight = 35},
    ShopPoolEntry{.templateId = "storm_archer", .cost = 3, .tier = 3, .weight = 25},
};

inline constexpr std::array<ShopTierWeights, MaxPlayerLevel> ShopTierOddsByLevel{{
    ShopTierWeights{100, 0, 0},
    ShopTierWeights{80, 20, 0},
    ShopTierWeights{70, 30, 0},
    ShopTierWeights{55, 40, 5},
    ShopTierWeights{45, 45, 10},
    ShopTierWeights{30, 55, 15},
    ShopTierWeights{20, 55, 25},
    ShopTierWeights{15, 50, 35},
    ShopTierWeights{10, 45, 45},
}};

[[nodiscard]] inline std::span<const ShopPoolEntry> shopUnitPool() noexcept {
    return ShopUnitPool;
}

[[nodiscard]] inline ShopTierWeights shopTierWeightsForLevel(int playerLevel) noexcept {
    const int level = std::clamp(playerLevel, 1, MaxPlayerLevel);
    return ShopTierOddsByLevel[static_cast<std::size_t>(level - 1)];
}

[[nodiscard]] inline int shopBaseCostForTemplate(std::string_view templateId) noexcept {
    const auto iter = std::ranges::find(ShopUnitPool, templateId, &ShopPoolEntry::templateId);
    return iter == ShopUnitPool.end() ? 1 : iter->cost;
}

}  // namespace synera::config
