#include "core/Unit.hpp"

#include "core/Contract.hpp"

#include <algorithm>

namespace synera {

bool Unit::alive() const noexcept {
    return state != UnitState::Dead && currentStats.hp > 0;
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
    return dx * dx + dy * dy <= currentStats.range * currentStats.range;
}

void Unit::receiveDamage(int amount) noexcept {
    if (amount <= 0 || state == UnitState::Dead) {
        return;
    }
    currentStats.hp = std::max(0, currentStats.hp - amount);
    if (currentStats.hp == 0) {
        state = UnitState::Dead;
    }
}

void Unit::heal(int amount) noexcept {
    if (amount <= 0 || state == UnitState::Dead) {
        return;
    }
    currentStats.hp = std::min(currentStats.maxHp, currentStats.hp + amount);
}

void Unit::gainMana(int amount) noexcept {
    if (amount <= 0 || state == UnitState::Dead) {
        return;
    }
    currentStats.mana = std::min(currentStats.maxMana, currentStats.mana + amount);
}

void Unit::resetForCombat() noexcept {
    currentStats.hp = currentStats.maxHp;
    currentStats.mana = 0;
    state = UnitState::Idle;
    targetId = InvalidUnitId;
    attackTimer = 0.0F;
    moveTimer = 0.0F;
    stunTimer = 0.0F;
}

void Unit::checkInvariants() const {
    SYNERA_INVARIANT(!(boardPos.has_value() && benchSlot.has_value()));
    SYNERA_INVARIANT(star >= 1);
    SYNERA_INVARIANT(currentStats.maxHp > 0);
    SYNERA_INVARIANT(currentStats.hp >= 0);
}

} // namespace synera
