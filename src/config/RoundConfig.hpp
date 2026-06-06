#pragma once

#include "config/GameConfig.hpp"
#include "core/Types.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

namespace synera::config {

struct EnemySpec {
    std::string_view templateId;
    OffsetPos offsetPos;
};

struct RoundRewardRule {
    bool playerWon;
    int goldReward;
    int hpDelta;
    bool advancesRound;
};

struct EnemyWaveTuning {
    float hpMultiplier;
    float atkMultiplier;
    float attackIntervalMultiplier;
};

inline constexpr std::array RoundRewardRules{
    RoundRewardRule{
        .playerWon     = true,
        .goldReward    = WinGoldReward,
        .hpDelta       = 0,
        .advancesRound = true,
    },
    RoundRewardRule{
        .playerWon     = false,
        .goldReward    = LossGoldReward,
        .hpDelta       = -LossHpPenalty,
        .advancesRound = false,
    },
};

inline constexpr std::array RoundOneEnemies{
    EnemySpec{.templateId = "training_dummy", .offsetPos = OffsetPos{3, 1}},
};

inline constexpr std::array RoundTwoEnemies{
    EnemySpec{.templateId = "training_dummy", .offsetPos = OffsetPos{2, 1}},
    EnemySpec{.templateId = "training_dummy", .offsetPos = OffsetPos{4, 1}},
};

inline constexpr std::array RoundThreeEnemies{
    EnemySpec{.templateId = "iron_guard", .offsetPos = OffsetPos{2, 1}},
    EnemySpec{.templateId = "ember_mage", .offsetPos = OffsetPos{5, 1}},
};

inline constexpr std::array ScalingEnemies{
    EnemySpec{.templateId = "iron_guard", .offsetPos = OffsetPos{2, 1}},
    EnemySpec{.templateId = "storm_archer", .offsetPos = OffsetPos{4, 1}},
    EnemySpec{.templateId = "field_medic", .offsetPos = OffsetPos{6, 2}},
};

[[nodiscard]] inline std::span<const EnemySpec> enemiesForRound(int round) noexcept {
    if (round <= 1) {
        return RoundOneEnemies;
    }
    if (round == 2) {
        return RoundTwoEnemies;
    }
    if (round == 3) {
        return RoundThreeEnemies;
    }
    return ScalingEnemies;
}

[[nodiscard]] inline int enemyStarForRound(int round) noexcept {
    if (round < 4) {
        return 1;
    }
    return std::clamp(1 + (round - 4) / 3, 1, 3);
}

[[nodiscard]] inline EnemyWaveTuning enemyTuningForRound(int round) noexcept {
    if (round <= 1) {
        return EnemyWaveTuning{.hpMultiplier = 1.35F, .atkMultiplier = 1.10F,
                               .attackIntervalMultiplier = 1.00F};
    }
    if (round == 2) {
        return EnemyWaveTuning{.hpMultiplier = 1.30F, .atkMultiplier = 1.10F,
                               .attackIntervalMultiplier = 1.00F};
    }
    if (round == 3) {
        return EnemyWaveTuning{.hpMultiplier = 1.20F, .atkMultiplier = 1.15F,
                               .attackIntervalMultiplier = 0.98F};
    }
    if (round == 4) {
        return EnemyWaveTuning{.hpMultiplier = 1.30F, .atkMultiplier = 1.15F,
                               .attackIntervalMultiplier = 0.95F};
    }
    if (round == 5) {
        return EnemyWaveTuning{.hpMultiplier = 1.45F, .atkMultiplier = 1.25F,
                               .attackIntervalMultiplier = 0.92F};
    }
    return EnemyWaveTuning{.hpMultiplier = 1.65F, .atkMultiplier = 1.35F,
                           .attackIntervalMultiplier = 0.90F};
}

[[nodiscard]] inline const RoundRewardRule& rewardRuleFor(bool playerWon) noexcept {
    const auto iter = std::ranges::find(RoundRewardRules, playerWon, &RoundRewardRule::playerWon);
    return iter == RoundRewardRules.end() ? RoundRewardRules.back() : *iter;
}

}  // namespace synera::config
