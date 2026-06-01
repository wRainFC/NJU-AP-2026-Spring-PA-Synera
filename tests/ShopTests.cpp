#include <catch2/catch_test_macros.hpp>

#include "core/GameState.hpp"
#include "core/Shop.hpp"
#include "board/HexGrid.hpp"
#include "systems/ShopPool.hpp"
#include "systems/ShopSystem.hpp"

#include <algorithm>
#include <random>
#include <ranges>
#include <set>
#include <string>

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

}  // namespace

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
    CHECK(std::ranges::distance(pool.entries() | std::views::filter([](const synera::ShopPoolEntry& entry) {
                                    return entry.tier == 1;
                                })) >= 2);

    const auto levelFiveOdds = pool.tierWeightsForLevel(5);
    CHECK(levelFiveOdds[1] > 0);
    CHECK(levelFiveOdds[2] > 0);

    const synera::Shop::Offers offers = pool.rollOffers(synera::ShopRollContext{.playerLevel = 1}, rng);
    CHECK(std::ranges::all_of(
        offers, [](const synera::ShopOffer& offer) { return !offer.empty() && offer.tier == 1; }));
    std::set<std::string> templates;
    for (const synera::ShopOffer& offer : offers) {
        templates.insert(offer.unitTemplateId);
    }
    CHECK(templates.size() > 1);

    CHECK(pool.costForTemplate("storm_archer") == 3);
    CHECK(pool.costForTemplate("unknown_template") == 1);
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

TEST_CASE("ShopSystem sells player units and clears placement", "[shop]") {
    synera::GameState state;
    synera::ShopSystem shopSystem{123};
    const synera::UnitId unitId = state.createUnit("storm_archer", synera::Owner::PlayerCtrl);
    REQUIRE(state.placeUnitOnBoard(unitId, pos(0, 4)));

    auto* unit = state.findUnit(unitId);
    REQUIRE(unit != nullptr);
    unit->star           = 2;
    const int goldBefore = state.player().gold;

    const synera::ShopSellResult result = shopSystem.sellUnit(state, unitId);

    REQUIRE(result.ok());
    CHECK(result.goldGained == 6);
    CHECK(state.player().gold == goldBefore + 6);
    CHECK(state.findUnit(unitId) == nullptr);
    CHECK_FALSE(state.boardOccupant(pos(0, 4)).has_value());
}

TEST_CASE("ShopSystem rejects invalid unit sales", "[shop]") {
    synera::GameState state;
    synera::ShopSystem shopSystem{123};
    const synera::UnitId enemyId = state.createUnit("training_dummy", synera::Owner::EnemyCtrl);

    CHECK(shopSystem.sellUnit(state, synera::InvalidUnitId).status == synera::ShopSellStatus::InvalidUnit);
    CHECK(shopSystem.sellUnit(state, enemyId).status == synera::ShopSellStatus::InvalidOwner);

    const synera::UnitId playerId = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    state.setPhase(synera::Phase::Combat);
    CHECK(shopSystem.sellUnit(state, playerId).status == synera::ShopSellStatus::InvalidPhase);
}
