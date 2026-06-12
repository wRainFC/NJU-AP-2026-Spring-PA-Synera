#pragma once

#include <cstdint>

namespace synera::config {

inline constexpr int WindowWidth                       = 1600;
inline constexpr int WindowHeight                      = 900;
inline constexpr int TargetFps                         = 60;
inline constexpr float DragStartThresholdVirtualPixels = 6.0F;
inline constexpr bool EnableRandom                     = true;

inline constexpr int BoardWidth     = 8;
inline constexpr int BoardHeight    = 8;
inline constexpr int BenchSize      = 8;
inline constexpr int ShopOfferCount = 5;
inline constexpr int ShopMaxTier    = 3;
inline constexpr int MaxPlayerLevel = 9;

inline constexpr int InitialPlayerHp      = 100;
inline constexpr int InitialGold          = 8;
inline constexpr int InitialPopulationCap = 3;

inline constexpr int WinGoldReward                 = 6;
inline constexpr int LossGoldReward                = 3;
inline constexpr int LossHpPenalty                 = 10;
inline constexpr int ShopRefreshCost               = 2;
inline constexpr std::uint32_t ShopRandomSeed      = 0x53594E41U;
inline constexpr int EquipmentDropChancePercent    = 100;
inline constexpr std::uint32_t EquipmentRandomSeed = 0x45515550U;

inline constexpr int InterestGoldStep     = 10;
inline constexpr int MaxInterestGold      = 5;
inline constexpr int StreakTierOneCount   = 2;
inline constexpr int StreakTierTwoCount   = 4;
inline constexpr int StreakTierThreeCount = 6;
inline constexpr int StreakTierOneGold    = 1;
inline constexpr int StreakTierTwoGold    = 2;
inline constexpr int StreakTierThreeGold  = 3;

}  // namespace synera::config
