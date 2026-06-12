#pragma once

#include "config/ShopConfig.hpp"
#include "core/ShopOffer.hpp"

#include <random>
#include <span>
#include <string_view>

namespace synera {

// Player-facing economy state used by the roll table. Increasing level changes tier odds.
struct ShopRollContext {
    int playerLevel = 1;
};

// Rolls shop offers from level-based tier odds and per-unit weights. This is the future extension point for
// rarity tuning, temporary bans, and larger hero pools.
class ShopPool {
public:
    [[nodiscard]] ShopOffers rollOffers(ShopRollContext context, std::mt19937& rng) const;
    [[nodiscard]] ShopOffer rollOffer(ShopRollContext context, std::mt19937& rng) const;
    [[nodiscard]] config::ShopTierWeights tierWeightsForLevel(int playerLevel) const noexcept;
    [[nodiscard]] int costForTemplate(std::string_view templateId) const noexcept;
    [[nodiscard]] std::span<const config::ShopPoolEntry> entries() const noexcept;
};

}  // namespace synera
