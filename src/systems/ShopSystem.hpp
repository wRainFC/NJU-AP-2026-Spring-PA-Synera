#pragma once

#include "core/Types.hpp"
#include "systems/ShopPool.hpp"

#include <cstdint>
#include <random>

namespace synera {

class GameState;

// Initial ignores cost/lock; Manual costs gold; RoundStart is free but respects the shop lock.
enum class ShopRefreshMode { Initial, Manual, RoundStart };
enum class ShopRefreshResult { Ok, InvalidPhase, Locked, NotEnoughGold };
enum class ShopBuyStatus {
    Ok,
    InvalidPhase,
    InvalidOffer,
    EmptyOffer,
    BenchFull,
    NotEnoughGold,
    PlacementFailed
};

// Carries the gained unit id so UpgradeSystem can later run merge checks after a successful purchase.
struct ShopBuyResult {
    ShopBuyStatus status = ShopBuyStatus::Ok;
    UnitId gainedUnitId = InvalidUnitId;

    [[nodiscard]] bool ok() const noexcept { return status == ShopBuyStatus::Ok; }
};

// Applies shop economy rules; offer generation is delegated to ShopPool and state is stored by
// GameState::shop().
class ShopSystem {
public:
    ShopSystem();
    explicit ShopSystem(std::uint32_t seed);

    [[nodiscard]] ShopRefreshResult refresh(GameState& state, ShopRefreshMode mode);
    [[nodiscard]] ShopBuyResult buy(GameState& state, int offerIndex);
    void setLocked(GameState& state, bool locked) const;
    bool toggleLocked(GameState& state) const;

private:
    ShopPool pool_;
    std::mt19937 rng_;
};

}  // namespace synera
