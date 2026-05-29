#pragma once

namespace synera::config {

inline constexpr int WindowWidth = 1280;
inline constexpr int WindowHeight = 720;
inline constexpr int TargetFps = 60;

inline constexpr int BoardWidth = 8;
inline constexpr int BoardHeight = 8;
inline constexpr int BenchSize = 8;
inline constexpr int ShopOfferCount = 5;

inline constexpr int InitialPlayerHp = 100;
inline constexpr int InitialGold = 8;
inline constexpr int InitialPopulationCap = 3;

inline constexpr int WinGoldReward = 6;
inline constexpr int LossGoldReward = 3;
inline constexpr int LossHpPenalty = 10;
inline constexpr int ShopRefreshCost = 2;

}  // namespace synera::config
