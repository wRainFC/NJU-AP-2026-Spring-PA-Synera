#include "systems/ShopPool.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <random>
#include <string>
#include <vector>

namespace synera {

namespace {

inline constexpr std::array<ShopPoolEntry, 4> UnitPool{{
    ShopPoolEntry{.templateId = "iron_guard", .cost = 1, .tier = 1, .weight = 55},
    ShopPoolEntry{.templateId = "field_medic", .cost = 1, .tier = 1, .weight = 45},
    ShopPoolEntry{.templateId = "ember_mage", .cost = 2, .tier = 2, .weight = 35},
    ShopPoolEntry{.templateId = "storm_archer", .cost = 3, .tier = 3, .weight = 25},
}};

inline constexpr std::array<ShopTierWeights, config::MaxPlayerLevel> TierOddsByLevel{{
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

[[nodiscard]] int rollTier(ShopTierWeights weights, std::mt19937& rng) {
    std::discrete_distribution<int> distribution(weights.begin(), weights.end());
    return distribution(rng) + 1;
}

[[nodiscard]] int highestAvailableTier(ShopTierWeights weights) noexcept {
    int highestTier = 1;
    for (int index : std::views::iota(0, config::ShopMaxTier)) {
        if (weights[static_cast<std::size_t>(index)] > 0) {
            highestTier = index + 1;
        }
    }
    return highestTier;
}

[[nodiscard]] std::vector<ShopPoolEntry> candidatesForTier(int tier) {
    std::vector<ShopPoolEntry> candidates;
    for (const ShopPoolEntry& entry :
         UnitPool | std::views::filter([&](const ShopPoolEntry& entry) { return entry.tier == tier; })) {
        candidates.push_back(entry);
    }
    return candidates;
}

[[nodiscard]] std::vector<ShopPoolEntry> fallbackCandidates(int highestTier) {
    std::vector<ShopPoolEntry> candidates;
    for (const ShopPoolEntry& entry : UnitPool | std::views::filter([&](const ShopPoolEntry& entry) {
                                          return entry.tier <= highestTier;
                                      })) {
        candidates.push_back(entry);
    }
    return candidates;
}

[[nodiscard]] ShopOffer offerFrom(const ShopPoolEntry& entry) {
    return ShopOffer{
        .unitTemplateId = std::string(entry.templateId),
        .cost           = entry.cost,
        .tier           = entry.tier,
    };
}

}  // namespace

Shop::Offers ShopPool::rollOffers(ShopRollContext context, std::mt19937& rng) const {
    Shop::Offers offers{};
    for (int index : std::views::iota(0, config::ShopOfferCount)) {
        offers[static_cast<std::size_t>(index)] = rollOffer(context, rng);
    }
    return offers;
}

ShopOffer ShopPool::rollOffer(ShopRollContext context, std::mt19937& rng) const {
    const ShopTierWeights tierWeights = tierWeightsForLevel(context.playerLevel);
    const int tier                    = rollTier(tierWeights, rng);
    auto candidates                   = candidatesForTier(tier);
    if (candidates.empty()) {
        candidates = fallbackCandidates(highestAvailableTier(tierWeights));
    }
    if (candidates.empty()) {
        return ShopOffer{};
    }

    std::vector<int> weights;
    weights.reserve(candidates.size());
    for (const ShopPoolEntry& candidate : candidates) {
        weights.push_back(candidate.weight);
    }
    std::discrete_distribution<std::size_t> distribution(weights.begin(), weights.end());
    return offerFrom(candidates[distribution(rng)]);
}

ShopTierWeights ShopPool::tierWeightsForLevel(int playerLevel) const noexcept {
    const int level = std::clamp(playerLevel, 1, config::MaxPlayerLevel);
    return TierOddsByLevel[static_cast<std::size_t>(level - 1)];
}

int ShopPool::costForTemplate(std::string_view templateId) const noexcept {
    const auto iter = std::ranges::find(UnitPool, templateId, &ShopPoolEntry::templateId);
    return iter == UnitPool.end() ? 1 : iter->cost;
}

std::span<const ShopPoolEntry> ShopPool::entries() const noexcept {
    return UnitPool;
}

}  // namespace synera
