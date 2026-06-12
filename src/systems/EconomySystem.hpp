#pragma once

#include "core/Player.hpp"

namespace synera {

struct EconomySettlement {
    bool playerWon   = false;
    int baseGold     = 0;
    int interestGold = 0;
    int streakGold   = 0;
    int totalGold    = 0;
    int winStreak    = 0;
    int lossStreak   = 0;
};

// Computes round income and updates player streak state. Spending rules remain in Player/ShopSystem.
class EconomySystem {
public:
    [[nodiscard]] static int interestForGold(int gold) noexcept;
    [[nodiscard]] static int streakBonusFor(int streak) noexcept;
    [[nodiscard]] EconomySettlement settleRound(Player& player, bool playerWon, int baseGold) const noexcept;
};

}  // namespace synera
