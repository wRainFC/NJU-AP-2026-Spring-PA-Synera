#pragma once

#include "core/GameState.hpp"

namespace synera {

class RoundSystem {
public:
    void startCombat(GameState& state);
    void spawnEnemies(GameState& state);
    void resolveRound(GameState& state, bool playerWon);
};

} // namespace synera
