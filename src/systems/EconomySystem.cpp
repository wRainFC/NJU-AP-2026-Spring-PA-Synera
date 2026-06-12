#include "systems/EconomySystem.hpp"

#include "config/GameConfig.hpp"

#include <algorithm>

namespace synera {

int EconomySystem::interestForGold(int gold) noexcept {
    return std::clamp(gold / config::InterestGoldStep, 0, config::MaxInterestGold);
}

int EconomySystem::streakBonusFor(int streak) noexcept {
    if (streak >= config::StreakTierThreeCount) {
        return config::StreakTierThreeGold;
    }
    if (streak >= config::StreakTierTwoCount) {
        return config::StreakTierTwoGold;
    }
    if (streak >= config::StreakTierOneCount) {
        return config::StreakTierOneGold;
    }
    return 0;
}

EconomySettlement EconomySystem::settleRound(Player& player, bool playerWon, int baseGold) const noexcept {
    const int interestGold = interestForGold(player.gold);

    if (playerWon) {
        ++player.winStreak;
        player.lossStreak = 0;
    } else {
        ++player.lossStreak;
        player.winStreak = 0;
    }

    const int streakGold = streakBonusFor(playerWon ? player.winStreak : player.lossStreak);
    const int totalGold  = std::max(0, baseGold) + interestGold + streakGold;
    player.addGold(totalGold);

    return EconomySettlement{
        .playerWon    = playerWon,
        .baseGold     = std::max(0, baseGold),
        .interestGold = interestGold,
        .streakGold   = streakGold,
        .totalGold    = totalGold,
        .winStreak    = player.winStreak,
        .lossStreak   = player.lossStreak,
    };
}

}  // namespace synera
