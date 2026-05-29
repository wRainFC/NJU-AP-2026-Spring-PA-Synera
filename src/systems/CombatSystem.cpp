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

void CombatSystem::performAttack(Unit& attacker, Unit& target) {
    attacker.runtime.state = UnitState::Attacking;
    target.receiveDamage(attacker.derivedStats.atk);
    attacker.gainMana(10);
}

}  // namespace synera
