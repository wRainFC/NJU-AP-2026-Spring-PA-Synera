#pragma once

namespace synera {

class GameState;

class RoundSystem {
public:
    void startCombat(GameState& state);
    void spawnEnemies(GameState& state);
    void enterResolve(GameState& state, bool playerWon);
    void finishResolve(GameState& state);
};

}  // namespace synera
