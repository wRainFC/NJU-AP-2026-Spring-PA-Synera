#include "systems/RoundSystem.hpp"

#include "config/RoundConfig.hpp"
#include "board/HexGrid.hpp"
#include "core/GameState.hpp"

namespace synera {

namespace {

void applyEnemyTuning(Unit& unit, config::EnemyWaveTuning tuning) noexcept {
    unit.derivedStats.maxHp =
        static_cast<int>(static_cast<float>(unit.derivedStats.maxHp) * tuning.hpMultiplier);
    unit.derivedStats.atk =
        static_cast<int>(static_cast<float>(unit.derivedStats.atk) * tuning.atkMultiplier);
    unit.derivedStats.attackInterval *= tuning.attackIntervalMultiplier;
    unit.runtime.hp = unit.derivedStats.maxHp;
}

}  // namespace

void RoundSystem::startCombat(GameState& state) {
    if (state.phase() != Phase::Prep || state.playerBoardUnitCount() == 0) {
        return;
    }
    spawnEnemies(state);
    state.forEachUnit([](Unit& unit) {
        if (unit.onBoard()) {
            unit.resetForCombat();
            if (unit.owner == Owner::PlayerCtrl) {
                unit.runtime.combatStartPos = unit.boardPos;
            }
        }
    });
    state.setPhase(Phase::Combat);
}

void RoundSystem::spawnEnemies(GameState& state) {
    state.removeEnemyUnits();
    const int star = config::enemyStarForRound(state.player().currentRound);
    const config::EnemyWaveTuning tuning = config::enemyTuningForRound(state.player().currentRound);
    for (const config::EnemySpec& spec : config::enemiesForRound(state.player().currentRound)) {
        const UnitId enemy = state.createUnit(spec.templateId, Owner::EnemyCtrl);
        if (Unit* unit = state.findUnit(enemy); unit != nullptr) {
            unit->star = star;
            unit->recomputeDerivedStats();
            applyEnemyTuning(*unit, tuning);
            unit->resetForCombat();
        }
        state.placeUnitOnBoard(enemy, hex::oddRToAxial(spec.offsetPos));
    }
}

RoundResult RoundSystem::enterResolve(GameState& state, bool playerWon) {
    RoundResult result{
        .applied       = false,
        .playerWon     = playerWon,
        .resolvedRound = state.player().currentRound,
        .goldBefore    = state.player().gold,
        .goldAfter     = state.player().gold,
        .hpBefore      = state.player().hp,
        .hpAfter       = state.player().hp,
        .nextRound     = state.player().currentRound,
        .advancedRound = false,
    };
    if (state.phase() != Phase::Combat) {
        return result;
    }

    const config::RoundRewardRule& rule = config::rewardRuleFor(playerWon);
    state.setPhase(Phase::Resolve);

    state.player().addGold(rule.goldReward);
    state.player().hp += rule.hpDelta;
    if (rule.advancesRound) {
        ++state.player().currentRound;
    }

    state.removeEnemyUnits();
    state.restorePlayerUnitsAfterCombat();

    result.applied       = true;
    result.goldAfter     = state.player().gold;
    result.hpAfter       = state.player().hp;
    result.nextRound     = state.player().currentRound;
    result.advancedRound = rule.advancesRound;
    return result;
}

void RoundSystem::finishResolve(GameState& state) {
    if (state.phase() != Phase::Resolve) {
        return;
    }
    state.setPhase(Phase::Prep);
}

}  // namespace synera
