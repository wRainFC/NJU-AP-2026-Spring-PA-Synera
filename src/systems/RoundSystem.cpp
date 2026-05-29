#include "systems/RoundSystem.hpp"

#include "app/GameConfig.hpp"

namespace synera {

void RoundSystem::startCombat(GameState& state) {
    if (state.phase() != Phase::Prep) {
        return;
    }
    spawnEnemies(state);
    state.forEachUnit([](Unit& unit) {
        if (unit.onBoard()) {
            unit.resetForCombat();
        }
    });
    state.setPhase(Phase::Combat);
}

void RoundSystem::spawnEnemies(GameState& state) {
    state.removeEnemyUnits();
    const UnitId enemy = state.createUnit("training_dummy", Owner::EnemyCtrl);
    state.placeUnitOnBoard(enemy, GridPos{3, 1});
}

void RoundSystem::enterResolve(GameState& state, bool playerWon) {
    if (state.phase() != Phase::Combat) {
        return;
    }
    state.setPhase(Phase::Resolve);
    if (playerWon) {
        state.player().addGold(config::WinGoldReward);
        ++state.player().currentRound;
    } else {
        state.player().hp -= config::LossHpPenalty;
        state.player().addGold(config::LossGoldReward);
    }
    state.removeEnemyUnits();
}

void RoundSystem::finishResolve(GameState& state) {
    if (state.phase() != Phase::Resolve) {
        return;
    }
    state.setPhase(Phase::Prep);
}

} // namespace synera
