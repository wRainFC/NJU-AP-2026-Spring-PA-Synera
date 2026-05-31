#include "systems/RoundSystem.hpp"

#include "app/GameConfig.hpp"
#include "board/HexGrid.hpp"
#include "core/GameState.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

namespace synera {

namespace {

struct EnemySpec {
    std::string_view templateId;
    OffsetPos offsetPos;
};

constexpr std::array RoundOneEnemies{
    EnemySpec{.templateId = "training_dummy", .offsetPos = OffsetPos{3, 1}},
};

constexpr std::array RoundTwoEnemies{
    EnemySpec{.templateId = "training_dummy", .offsetPos = OffsetPos{2, 1}},
    EnemySpec{.templateId = "training_dummy", .offsetPos = OffsetPos{4, 1}},
};

constexpr std::array RoundThreeEnemies{
    EnemySpec{.templateId = "iron_guard", .offsetPos = OffsetPos{2, 1}},
    EnemySpec{.templateId = "ember_mage", .offsetPos = OffsetPos{5, 1}},
};

constexpr std::array ScalingEnemies{
    EnemySpec{.templateId = "iron_guard", .offsetPos = OffsetPos{2, 1}},
    EnemySpec{.templateId = "storm_archer", .offsetPos = OffsetPos{4, 1}},
    EnemySpec{.templateId = "field_medic", .offsetPos = OffsetPos{6, 2}},
};

[[nodiscard]] std::span<const EnemySpec> enemiesForRound(int round) noexcept {
    if (round <= 1) {
        return RoundOneEnemies;
    }
    if (round == 2) {
        return RoundTwoEnemies;
    }
    if (round == 3) {
        return RoundThreeEnemies;
    }
    return ScalingEnemies;
}

[[nodiscard]] int enemyStarForRound(int round) noexcept {
    if (round < 4) {
        return 1;
    }
    return std::clamp(1 + (round - 4) / 3, 1, 3);
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
    const int star = enemyStarForRound(state.player().currentRound);
    for (const EnemySpec& spec : enemiesForRound(state.player().currentRound)) {
        const UnitId enemy = state.createUnit(spec.templateId, Owner::EnemyCtrl);
        if (Unit* unit = state.findUnit(enemy); unit != nullptr) {
            unit->star = star;
            unit->recomputeDerivedStats();
            unit->resetForCombat();
        }
        state.placeUnitOnBoard(enemy, hex::oddRToAxial(spec.offsetPos));
    }
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
    state.restorePlayerUnitsAfterCombat();
}

void RoundSystem::finishResolve(GameState& state) {
    if (state.phase() != Phase::Resolve) {
        return;
    }
    state.setPhase(Phase::Prep);
}

}  // namespace synera
