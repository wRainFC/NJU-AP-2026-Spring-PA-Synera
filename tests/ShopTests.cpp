#include <catch2/catch_test_macros.hpp>

#include "core/GameState.hpp"
#include "core/Shop.hpp"
#include "systems/ShopPool.hpp"
#include "systems/ShopSystem.hpp"

#include <algorithm>
#include <random>
#include <ranges>

TEST_CASE("Shop stores offers and lock state", "[shop]") {
    synera::Shop shop;

    CHECK_FALSE(shop.locked());
    CHECK(shop.offers().size() == synera::config::ShopOfferCount);
    CHECK_FALSE(shop.offerAt(-1).has_value());

    shop.setOffer(0, synera::ShopOffer{.unitTemplateId = "iron_guard", .cost = 1, .tier = 1});
    const auto offer = shop.offerAt(0);
    REQUIRE(offer.has_value());
    CHECK(offer->get().unitTemplateId == "iron_guard");
    CHECK(offer->get().cost == 1);

    CHECK(shop.toggleLocked());
    CHECK(shop.locked());

    shop.clearOffer(0);
    REQUIRE(shop.offerAt(0).has_value());
    CHECK(shop.offerAt(0)->get().empty());
}

TEST_CASE("ShopPool rolls level-gated offers", "[shop]") {
    synera::ShopPool pool;
    std::mt19937 rng{123};

    const auto levelOneOdds = pool.tierWeightsForLevel(1);
    CHECK(levelOneOdds[0] == 100);
    CHECK(levelOneOdds[1] == 0);

    const auto levelFiveOdds = pool.tierWeightsForLevel(5);
    CHECK(levelFiveOdds[1] > 0);
    CHECK(levelFiveOdds[2] > 0);

    const synera::Shop::Offers offers = pool.rollOffers(synera::ShopRollContext{.playerLevel = 1}, rng);
    CHECK(std::ranges::all_of(
        offers, [](const synera::ShopOffer& offer) { return !offer.empty() && offer.tier == 1; }));
}

TEST_CASE("ShopSystem refreshes, locks, and buys through GameState", "[shop]") {
    synera::GameState state;
    synera::ShopSystem shopSystem{123};

    CHECK(shopSystem.refresh(state, synera::ShopRefreshMode::Initial) == synera::ShopRefreshResult::Ok);
    CHECK(std::ranges::all_of(state.shop().offers(),
                              [](const synera::ShopOffer& offer) { return !offer.empty(); }));

    const int goldBeforeManualRefresh = state.player().gold;
    CHECK(shopSystem.refresh(state, synera::ShopRefreshMode::Manual) == synera::ShopRefreshResult::Ok);
    CHECK(state.player().gold == goldBeforeManualRefresh - synera::config::ShopRefreshCost);

    state.shop().setLocked(true);
    synera::Shop::Offers lockedOffers{};
    std::ranges::copy(state.shop().offers(), lockedOffers.begin());
    CHECK(shopSystem.refresh(state, synera::ShopRefreshMode::RoundStart) ==
          synera::ShopRefreshResult::Locked);
    CHECK(std::ranges::equal(state.shop().offers(), lockedOffers, {}, &synera::ShopOffer::unitTemplateId,
                             &synera::ShopOffer::unitTemplateId));

    const synera::ShopBuyResult result = shopSystem.buy(state, 0);
    REQUIRE(result.ok());
    CHECK(result.gainedUnitId != synera::InvalidUnitId);
    CHECK(state.benchOccupant(0) == result.gainedUnitId);
    REQUIRE(state.shop().offerAt(0).has_value());
    CHECK(state.shop().offerAt(0)->get().empty());
}
