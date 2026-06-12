#include "systems/ShopPool.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <random>
#include <string>
#include <vector>

namespace synera {

namespace {

[[nodiscard]] int rollTier(config::ShopTierWeights weights, std::mt19937& rng) {
    std::discrete_distribution<int> distribution(weights.begin(), weights.end());
    return distribution(rng) + 1;
}

[[nodiscard]] int highestAvailableTier(config::ShopTierWeights weights) noexcept {
    int highestTier = 1;
    for (int index : std::views::iota(0, config::ShopMaxTier)) {
        if (weights[static_cast<std::size_t>(index)] > 0) {
            highestTier = index + 1;
        }
    }
    return highestTier;
}

[[nodiscard]] std::vector<config::ShopPoolEntry> candidatesForTier(int tier) {
    std::vector<config::ShopPoolEntry> candidates;
    for (const config::ShopPoolEntry& entry :
         config::shopUnitPool() |
             std::views::filter([&](const config::ShopPoolEntry& entry) { return entry.tier == tier; })) {
        candidates.push_back(entry);
    }
    return candidates;
}

[[nodiscard]] std::vector<config::ShopPoolEntry> fallbackCandidates(int highestTier) {
    std::vector<config::ShopPoolEntry> candidates;
    for (const config::ShopPoolEntry& entry :
         config::shopUnitPool() | std::views::filter([&](const config::ShopPoolEntry& entry) {
             return entry.tier <= highestTier;
         })) {
        candidates.push_back(entry);
    }
    return candidates;
}

[[nodiscard]] ShopOffer offerFrom(const config::ShopPoolEntry& entry) {
    return ShopOffer{
        .unitTemplateId = std::string(entry.templateId),
        .cost           = entry.cost,
        .tier           = entry.tier,
    };
}

}  // namespace

ShopOffers ShopPool::rollOffers(ShopRollContext context, std::mt19937& rng) const {
    ShopOffers offers{};
    for (int index : std::views::iota(0, config::ShopOfferCount)) {
        offers[static_cast<std::size_t>(index)] = rollOffer(context, rng);
    }
    return offers;
}

ShopOffer ShopPool::rollOffer(ShopRollContext context, std::mt19937& rng) const {
    const config::ShopTierWeights tierWeights = tierWeightsForLevel(context.playerLevel);
    const int tier                            = rollTier(tierWeights, rng);
    auto candidates                           = candidatesForTier(tier);
    if (candidates.empty()) {
        candidates = fallbackCandidates(highestAvailableTier(tierWeights));
    }
    if (candidates.empty()) {
        return ShopOffer{};
    }

    std::vector<int> weights;
    weights.reserve(candidates.size());
    for (const config::ShopPoolEntry& candidate : candidates) {
        weights.push_back(candidate.weight);
    }
    std::discrete_distribution<std::size_t> distribution(weights.begin(), weights.end());
    return offerFrom(candidates[distribution(rng)]);
}

config::ShopTierWeights ShopPool::tierWeightsForLevel(int playerLevel) const noexcept {
    return config::shopTierWeightsForLevel(playerLevel);
}

int ShopPool::costForTemplate(std::string_view templateId) const noexcept {
    return config::shopBaseCostForTemplate(templateId);
}

std::span<const config::ShopPoolEntry> ShopPool::entries() const noexcept {
    return config::shopUnitPool();
}

}  // namespace synera
