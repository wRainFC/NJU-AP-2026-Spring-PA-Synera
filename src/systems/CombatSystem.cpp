#include "systems/CombatSystem.hpp"

#include "board/HexGrid.hpp"
#include "core/AbilityContext.hpp"
#include "core/GameState.hpp"

#include <limits>
#include <tuple>

namespace synera {

void CombatSystem::update(GameState& state, float dt) {
    if (state.phase() != Phase::Combat) {
        return;
    }
    state.forEachUnit([&](Unit& unit) {
        updateUnit(state, unit, dt);
        cleanupDeadBoardUnits(state);
    });
}

void CombatSystem::updateUnit(GameState& state, Unit& unit, float dt) {
    if (!unit.alive() || !unit.onBoard()) {
        return;
    }
    if (unit.runtime.state == UnitState::Stunned) {
        updateStun(unit, dt);
        return;
    }

    Unit* target = acquireTarget(state, unit);
    if (target == nullptr) {
        unit.runtime.state = UnitState::Idle;
        unit.runtime.targetId = InvalidUnitId;
        return;
    }
    unit.runtime.targetId = target->id;

    if (!unit.canAttackTarget(*target)) {
        moveTowardTarget(state, unit, *target, dt);
        return;
    }

    if (tryCastAbility(state, unit)) {
        return;
    }

    unit.runtime.attackTimer += dt;
    if (unit.runtime.attackTimer >= unit.derivedStats.attackInterval) {
        unit.runtime.attackTimer = 0.0F;
        performAttack(unit, *target);
    }
}

Unit* CombatSystem::acquireTarget(GameState& state, const Unit& unit) {
    Unit* best = nullptr;
    auto bestKey =
        std::tuple{std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
                   std::numeric_limits<int>::max(), std::numeric_limits<int>::min(), InvalidUnitId};

    state.forEachUnit([&](Unit& candidate) {
        if (!candidate.alive() || !candidate.onBoard() || candidate.owner == unit.owner) {
            return;
        }

        const OffsetPos offset = hex::axialToOddR(*candidate.boardPos);
        const auto candidateKey = std::tuple{hex::hexDistance(*unit.boardPos, *candidate.boardPos),
                                             candidate.runtime.hp, offset.col, -offset.row, candidate.id};
        if (candidateKey < bestKey) {
            best = &candidate;
            bestKey = candidateKey;
        }
    });

    return best;
}

void CombatSystem::updateStun(Unit& unit, float dt) {
    unit.runtime.stunTimer -= dt;
    if (unit.runtime.stunTimer <= 0.0F) {
        unit.runtime.stunTimer = 0.0F;
        unit.runtime.state = UnitState::Idle;
    }
}

bool CombatSystem::tryCastAbility(GameState& state, Unit& unit) {
    if (!unit.ability || unit.runtime.mana < unit.derivedStats.maxMana) {
        return false;
    }

    AbilityContext context{state};
    unit.runtime.state = UnitState::Casting;
    unit.ability->cast(unit, context);
    unit.runtime.mana = 0;
    return true;
}

void CombatSystem::moveTowardTarget(GameState& state, Unit& unit, const Unit& target, float dt) {
    unit.runtime.state = UnitState::Moving;
    unit.runtime.moveTimer += dt;
    if (unit.runtime.moveTimer < unit.derivedStats.moveInterval) {
        return;
    }
    unit.runtime.moveTimer = 0.0F;

    const auto path = pathfinder_.findPathToAttackRange(state.board(), *unit.boardPos, *target.boardPos,
                                                        unit.derivedStats.range);
    if (path.empty()) {
        unit.runtime.state = UnitState::Idle;
        return;
    }
    if (!state.moveBoardUnit(unit.id, path.front())) {
        unit.runtime.state = UnitState::Idle;
    }
}

void CombatSystem::performAttack(Unit& attacker, Unit& target) {
    attacker.runtime.state = UnitState::Attacking;
    target.receiveDamage(attacker.derivedStats.atk);
    if (attacker.mechanics.doubleBasicAttack && target.alive()) {
        target.receiveDamage(attacker.derivedStats.atk);
    }
    attacker.gainMana(10);
}

void CombatSystem::cleanupDeadBoardUnits(GameState& state) {
    state.forEachUnit([&](Unit& unit) {
        if (!unit.alive() && unit.onBoard()) {
            state.removeUnitFromBoard(unit);
        }
    });
}

}  // namespace synera
