#include "core/Unit.hpp"

#include "core/Contract.hpp"

#include <algorithm>

namespace synera {

bool Unit::alive() const noexcept {
    return runtime.state != UnitState::Dead && runtime.hp > 0;
}

bool Unit::onBoard() const noexcept {
    return boardPos.has_value();
}

bool Unit::onBench() const noexcept {
    return benchSlot.has_value();
}

bool Unit::canAttackTarget(const Unit& target) const {
    if (!alive() || !target.alive() || !boardPos || !target.boardPos) {
        return false;
    }

    const int dx = boardPos->x - target.boardPos->x;
    const int dy = boardPos->y - target.boardPos->y;
    return dx * dx + dy * dy <= derivedStats.range * derivedStats.range;
}

void Unit::receiveDamage(int amount) noexcept {
    if (amount <= 0 || runtime.state == UnitState::Dead) {
        return;
    }
    runtime.hp = std::max(0, runtime.hp - amount);
    if (runtime.hp == 0) {
        runtime.state = UnitState::Dead;
    }
}

void Unit::heal(int amount) noexcept {
    if (amount <= 0 || runtime.state == UnitState::Dead) {
        return;
    }
    runtime.hp = std::min(derivedStats.maxHp, runtime.hp + amount);
}

void Unit::gainMana(int amount) noexcept {
    if (amount <= 0 || runtime.state == UnitState::Dead) {
        return;
    }
    runtime.mana = std::min(derivedStats.maxMana, runtime.mana + amount);
}

void Unit::resetForCombat() noexcept {
    runtime.hp = derivedStats.maxHp;
    runtime.mana = 0;
    runtime.state = UnitState::Idle;
    runtime.targetId = InvalidUnitId;
    runtime.attackTimer = 0.0F;
    runtime.moveTimer = 0.0F;
    runtime.stunTimer = 0.0F;
}

void Unit::checkInvariants() const {
    SYNERA_INVARIANT(!(boardPos.has_value() && benchSlot.has_value()));
    SYNERA_INVARIANT(star >= 1);
    SYNERA_INVARIANT(derivedStats.maxHp > 0);
    SYNERA_INVARIANT(runtime.hp >= 0);
    SYNERA_INVARIANT(runtime.hp <= derivedStats.maxHp);
}

} // namespace synera
