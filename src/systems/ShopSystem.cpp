#include "systems/ShopSystem.hpp"

#include "app/GameConfig.hpp"

namespace synera {

void ShopSystem::refresh(GameState& state, bool payCost) {
    if (payCost && !state.player.spendGold(config::ShopRefreshCost)) {
        return;
    }

    for (auto& offer : state.shopOffers) {
        offer = ShopOffer{.unitTemplateId = "iron_guard", .cost = 1};
    }
}

bool ShopSystem::buy(GameState& state, int offerIndex) {
    if (offerIndex < 0 || offerIndex >= static_cast<int>(state.shopOffers.size())) {
        return false;
    }
    const ShopOffer& offer = state.shopOffers[offerIndex];
    const auto slot = state.bench.firstEmptySlot();
    if (!slot || !state.player.spendGold(offer.cost)) {
        return false;
    }

    const UnitId unit = state.createUnit(offer.unitTemplateId, Owner::PlayerCtrl);
    return state.placeUnitOnBench(unit, *slot);
}

} // namespace synera
