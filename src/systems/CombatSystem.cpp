#include "systems/CombatSystem.hpp"

#include "board/HexGrid.hpp"

#include <limits>

namespace synera {

void CombatSystem::update(GameState& state, float dt) {
    if (state.phase() != Phase::Combat) {
        return;
    }
    state.forEachUnit([&](Unit& unit) { updateUnit(state, unit, dt); });
}

void CombatSystem::updateUnit(GameState& state, Unit& unit, float dt) {
    if (!unit.alive() || !unit.onBoard()) {
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

    unit.runtime.attackTimer += dt;
    if (unit.runtime.attackTimer >= unit.derivedStats.attackInterval) {
        unit.runtime.attackTimer = 0.0F;
        performAttack(unit, *target);
    }
}

Unit* CombatSystem::acquireTarget(GameState& state, const Unit& unit) {
    Unit* best = nullptr;
    int bestDist = std::numeric_limits<int>::max();

    state.forEachUnit([&](Unit& candidate) {
        if (!candidate.alive() || !candidate.onBoard() || candidate.owner == unit.owner) {
            return;
        }

        const int dist = hex::hexDistance(*unit.boardPos, *candidate.boardPos);
        const bool betterTie =
            best != nullptr && (candidate.runtime.hp < best->runtime.hp ||
                                (candidate.runtime.hp == best->runtime.hp && candidate.id < best->id));
        if (dist < bestDist || (dist == bestDist && betterTie)) {
            best = &candidate;
            bestDist = dist;
        }
    });

    return best;
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
    attacker.gainMana(10);
}

}  // namespace synera
