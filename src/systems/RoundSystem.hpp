#pragma once

#include "core/GameState.hpp"

namespace synera {

class RoundSystem {
public:
    void startCombat(GameState& state);
    void spawnEnemies(GameState& state);
    void enterResolve(GameState& state, bool playerWon);
    void finishResolve(GameState& state);
};

}  // namespace synera
