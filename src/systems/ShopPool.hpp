#pragma once

#include "app/GameConfig.hpp"
#include "core/Shop.hpp"

#include <array>
#include <random>
#include <span>
#include <string_view>

namespace synera {

using ShopTierWeights = std::array<int, config::ShopMaxTier>;

// Player-facing economy state used by the roll table. Increasing level changes tier odds.
struct ShopRollContext {
    int playerLevel = 1;
};

struct ShopPoolEntry {
    std::string_view templateId;
    int cost = 1;
    int tier = 1;
    int weight = 1;
};

// Rolls shop offers from level-based tier odds and per-unit weights. This is the future extension point for
// rarity tuning, temporary bans, and larger hero pools.
class ShopPool {
public:
    [[nodiscard]] Shop::Offers rollOffers(ShopRollContext context, std::mt19937& rng) const;
    [[nodiscard]] ShopOffer rollOffer(ShopRollContext context, std::mt19937& rng) const;
    [[nodiscard]] ShopTierWeights tierWeightsForLevel(int playerLevel) const noexcept;
    [[nodiscard]] std::span<const ShopPoolEntry> entries() const noexcept;
};

}  // namespace synera
