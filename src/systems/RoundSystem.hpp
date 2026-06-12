#pragma once

#include "systems/EconomySystem.hpp"

namespace synera {

class GameState;

struct RoundResult {
    bool applied       = false;
    bool playerWon     = false;
    int resolvedRound  = 1;
    int goldBefore     = 0;
    int goldAfter      = 0;
    int hpBefore       = 0;
    int hpAfter        = 0;
    int nextRound      = 1;
    bool advancedRound = false;
    EconomySettlement economy{};
};

class RoundSystem {
public:
    void startCombat(GameState& state);
    void spawnEnemies(GameState& state);
    [[nodiscard]] RoundResult enterResolve(GameState& state, bool playerWon);
    void finishResolve(GameState& state);
};

}  // namespace synera
