#include "systems/ShopSystem.hpp"

#include "app/GameConfig.hpp"
#include "core/Contract.hpp"
#include "core/GameState.hpp"

#include <algorithm>

namespace synera {

namespace {

[[nodiscard]] bool costsGold(ShopRefreshMode mode) noexcept {
    return mode == ShopRefreshMode::Manual;
}

[[nodiscard]] bool respectsLock(ShopRefreshMode mode) noexcept {
    return mode == ShopRefreshMode::RoundStart;
}

}  // namespace

ShopSystem::ShopSystem() : ShopSystem(config::ShopRandomSeed) {}

ShopSystem::ShopSystem(std::uint32_t seed) : rng_(seed) {}

ShopRefreshResult ShopSystem::refresh(GameState& state, ShopRefreshMode mode) {
    if (state.phase() != Phase::Prep) {
        return ShopRefreshResult::InvalidPhase;
    }
    if (respectsLock(mode) && state.shop().locked()) {
        return ShopRefreshResult::Locked;
    }
    if (costsGold(mode) && !state.player().spendGold(config::ShopRefreshCost)) {
        return ShopRefreshResult::NotEnoughGold;
    }

    state.shop().replaceOffers(pool_.rollOffers(ShopRollContext{.playerLevel = state.player().level}, rng_));
    return ShopRefreshResult::Ok;
}

ShopBuyResult ShopSystem::buy(GameState& state, int offerIndex) {
    if (state.phase() != Phase::Prep) {
        return {.status = ShopBuyStatus::InvalidPhase};
    }
    const auto offer = state.shop().offerAt(offerIndex);
    if (!offer) {
        return {.status = ShopBuyStatus::InvalidOffer};
    }
    if (offer->get().empty()) {
        return {.status = ShopBuyStatus::EmptyOffer};
    }
    const auto slot = state.firstEmptyBenchSlot();
    if (!slot) {
        return {.status = ShopBuyStatus::BenchFull};
    }
    if (!state.player().spendGold(offer->get().cost)) {
        return {.status = ShopBuyStatus::NotEnoughGold};
    }

    const UnitId unit = state.createUnit(offer->get().unitTemplateId, Owner::PlayerCtrl);
    const bool placed = state.placeUnitOnBench(unit, *slot);
    SYNERA_ENSURES(placed);
    if (!placed) {
        state.player().addGold(offer->get().cost);
        return {.status = ShopBuyStatus::PlacementFailed};
    }

    state.shop().clearOffer(offerIndex);
    return {.status = ShopBuyStatus::Ok, .gainedUnitId = unit};
}

ShopSellResult ShopSystem::sellUnit(GameState& state, UnitId unitId) {
    if (state.phase() != Phase::Prep) {
        return {.status = ShopSellStatus::InvalidPhase};
    }

    const Unit* unit = state.findUnit(unitId);
    if (unit == nullptr) {
        return {.status = ShopSellStatus::InvalidUnit};
    }
    if (unit->owner != Owner::PlayerCtrl) {
        return {.status = ShopSellStatus::InvalidOwner};
    }

    const int gold = pool_.costForTemplate(unit->templateId) * std::max(1, unit->star);
    if (!state.removeUnit(unitId)) {
        return {.status = ShopSellStatus::InvalidUnit};
    }
    state.player().addGold(gold);
    return {.status = ShopSellStatus::Ok, .goldGained = gold};
}

void ShopSystem::setLocked(GameState& state, bool locked) const {
    state.shop().setLocked(locked);
}

bool ShopSystem::toggleLocked(GameState& state) const {
    return state.shop().toggleLocked();
}

}  // namespace synera
