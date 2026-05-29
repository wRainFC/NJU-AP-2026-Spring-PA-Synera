#include "systems/RoundSystem.hpp"

#include "app/GameConfig.hpp"

namespace synera {

void RoundSystem::startCombat(GameState& state) {
    if (state.phase != Phase::Prep) {
        return;
    }
    spawnEnemies(state);
    for (auto& [id, unit] : state.units) {
        (void)id;
        if (unit->onBoard()) {
            unit->resetForCombat();
        }
    }
    state.phase = Phase::Combat;
}

void RoundSystem::spawnEnemies(GameState& state) {
    state.removeEnemyUnits();
    const UnitId enemy = state.createUnit("training_dummy", Owner::EnemyCtrl);
    state.placeUnitOnBoard(enemy, GridPos{3, 1});
}

void RoundSystem::resolveRound(GameState& state, bool playerWon) {
    state.phase = Phase::Resolve;
    if (playerWon) {
        state.player.addGold(config::WinGoldReward);
        ++state.player.currentRound;
    } else {
        state.player.hp -= config::LossHpPenalty;
        state.player.addGold(config::LossGoldReward);
    }
    state.removeEnemyUnits();
    state.phase = Phase::Prep;
}

} // namespace synera
