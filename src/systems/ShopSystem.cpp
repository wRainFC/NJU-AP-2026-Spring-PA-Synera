#include "systems/ShopSystem.hpp"

#include "app/GameConfig.hpp"
#include "core/GameState.hpp"

#include <array>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>

namespace synera {

namespace {

struct ShopTemplate {
    std::string_view templateId;
    int cost = 1;
};

inline constexpr std::array<ShopTemplate, 3> UnitPool{{
    ShopTemplate{.templateId = "iron_guard", .cost = 1},
    ShopTemplate{.templateId = "ember_mage", .cost = 2},
    ShopTemplate{.templateId = "field_medic", .cost = 2},
}};

}  // namespace

void ShopSystem::refresh(GameState& state, bool payCost) {
    if (state.phase() != Phase::Prep) {
        return;
    }
    if (payCost && !state.player().spendGold(config::ShopRefreshCost)) {
        return;
    }

    for (int index : std::views::iota(0, config::ShopOfferCount)) {
        ShopOffer& offer = state.shopOffers()[static_cast<std::size_t>(index)];
        const auto& unit =
            UnitPool[static_cast<std::size_t>(index + state.player().currentRound) % UnitPool.size()];
        offer = ShopOffer{
            .unitTemplateId = std::string(unit.templateId),
            .cost = unit.cost,
        };
    }
}

bool ShopSystem::buy(GameState& state, int offerIndex) {
    if (state.phase() != Phase::Prep) {
        return false;
    }
    if (offerIndex < 0 || offerIndex >= static_cast<int>(state.shopOffers().size())) {
        return false;
    }
    const ShopOffer& offer = state.shopOffers()[offerIndex];
    if (offer.unitTemplateId.empty()) {
        return false;
    }
    const auto slot = state.firstEmptyBenchSlot();
    if (!slot || !state.player().spendGold(offer.cost)) {
        return false;
    }

    const UnitId unit = state.createUnit(offer.unitTemplateId, Owner::PlayerCtrl);
    return state.placeUnitOnBench(unit, *slot);
}

}  // namespace synera
