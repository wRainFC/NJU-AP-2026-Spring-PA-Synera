#include "systems/ShopSystem.hpp"

#include "app/GameConfig.hpp"

#include <algorithm>

namespace synera {

void ShopSystem::refresh(GameState& state, bool payCost) {
    if (payCost && !state.player().spendGold(config::ShopRefreshCost)) {
        return;
    }

    std::ranges::fill(state.shopOffers(), ShopOffer{.unitTemplateId = "iron_guard", .cost = 1});
}

bool ShopSystem::buy(GameState& state, int offerIndex) {
    if (offerIndex < 0 || offerIndex >= static_cast<int>(state.shopOffers().size())) {
        return false;
    }
    const ShopOffer& offer = state.shopOffers()[offerIndex];
    const auto slot = state.firstEmptyBenchSlot();
    if (!slot || !state.player().spendGold(offer.cost)) {
        return false;
    }

    const UnitId unit = state.createUnit(offer.unitTemplateId, Owner::PlayerCtrl);
    return state.placeUnitOnBench(unit, *slot);
}

} // namespace synera
