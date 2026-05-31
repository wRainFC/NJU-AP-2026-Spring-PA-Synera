#pragma once

#include <cstdint>

namespace synera::config {

inline constexpr int WindowWidth = 1600;
inline constexpr int WindowHeight = 900;
inline constexpr int TargetFps = 60;

inline constexpr int BoardWidth = 8;
inline constexpr int BoardHeight = 8;
inline constexpr int BenchSize = 8;
inline constexpr int ShopOfferCount = 5;
inline constexpr int ShopMaxTier = 3;
inline constexpr int MaxPlayerLevel = 9;

inline constexpr int InitialPlayerHp = 100;
inline constexpr int InitialGold = 8;
inline constexpr int InitialPopulationCap = 3;

inline constexpr int WinGoldReward = 6;
inline constexpr int LossGoldReward = 3;
inline constexpr int LossHpPenalty = 10;
inline constexpr int ShopRefreshCost = 2;
inline constexpr std::uint32_t ShopRandomSeed = 0x53594E41U;
inline constexpr int EquipmentDropChancePercent = 100;
inline constexpr std::uint32_t EquipmentRandomSeed = 0x45515550U;

}  // namespace synera::config
