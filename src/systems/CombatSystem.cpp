#include "systems/CombatSystem.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace synera {

void CombatSystem::update(GameState& state, float dt) {
    if (state.phase != Phase::Combat) {
        return;
    }
    for (auto& [id, unit] : state.units) {
        (void)id;
        updateUnit(state, *unit, dt);
    }
}

void CombatSystem::updateUnit(GameState& state, Unit& unit, float dt) {
    if (!unit.alive() || !unit.onBoard()) {
        return;
    }

    Unit* target = acquireTarget(state, unit);
    if (target == nullptr) {
        unit.state = UnitState::Idle;
        return;
    }

    unit.attackTimer += dt;
    if (unit.attackTimer >= unit.currentStats.attackInterval) {
        unit.attackTimer = 0.0F;
        performAttack(unit, *target);
    }
}

Unit* CombatSystem::acquireTarget(GameState& state, const Unit& unit) {
    Unit* best = nullptr;
    int bestDist = std::numeric_limits<int>::max();

    for (auto& [id, candidate] : state.units) {
        (void)id;
        if (!candidate->alive() || !candidate->onBoard() || candidate->owner == unit.owner) {
            continue;
        }

        const int dx = unit.boardPos->x - candidate->boardPos->x;
        const int dy = unit.boardPos->y - candidate->boardPos->y;
        const int dist = dx * dx + dy * dy;
        if (dist < bestDist) {
            best = candidate.get();
            bestDist = dist;
        }
    }

    return best;
}

void CombatSystem::performAttack(Unit& attacker, Unit& target) {
    attacker.state = UnitState::Attacking;
    target.receiveDamage(attacker.currentStats.atk);
    attacker.gainMana(10);
}

} // namespace synera
