#include "core/Unit.hpp"

#include "board/HexGrid.hpp"
#include "core/Contract.hpp"
#include "core/Metadata.hpp"

#include <algorithm>

namespace synera {

namespace {

[[nodiscard]] int scaledInt(int value, float multiplier) noexcept {
    return static_cast<int>(static_cast<float>(value) * multiplier);
}

void applyStatModifier(UnitStats& stats, EquipmentStatModifier modifier) noexcept {
    stats.atk += modifier.atkBonus;
    stats.maxHp += modifier.maxHpBonus;
    stats.attackInterval *= modifier.attackIntervalMultiplier;
    stats.maxMana = modifier.minMaxMana > 0
                        ? std::max(modifier.minMaxMana, stats.maxMana + modifier.maxManaDelta)
                        : stats.maxMana + modifier.maxManaDelta;
}

}  // namespace

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
    if (!alive() || !target.alive()) {
        return false;
    }

    return boardPos
        .and_then([&](AxialPos source) {
            return target.boardPos.transform([&](AxialPos targetPos) {
                return hex::hexDistance(source, targetPos) <= derivedStats.range;
            });
        })
        .value_or(false);
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

void Unit::recomputeDerivedStats() noexcept {
    const int previousMaxHp     = derivedStats.maxHp;
    const int previousHp        = runtime.hp;
    const int previousMissingHp = std::max(0, previousMaxHp - previousHp);

    derivedStats = baseStats;
    mechanics    = {};

    const float starMultiplier = 1.0F + 0.7F * static_cast<float>(star - 1);
    derivedStats.maxHp         = scaledInt(derivedStats.maxHp, starMultiplier);
    derivedStats.atk           = scaledInt(derivedStats.atk, starMultiplier);

    if (const auto modifier = equipment.and_then(equipmentStatModifier)) {
        applyStatModifier(derivedStats, *modifier);
    }

    runtime.hp   = runtime.state == UnitState::Dead
                       ? 0
                       : std::clamp(derivedStats.maxHp - previousMissingHp, 0, derivedStats.maxHp);
    runtime.mana = std::clamp(runtime.mana, 0, derivedStats.maxMana);
}

void Unit::resetForCombat() noexcept {
    runtime.hp          = derivedStats.maxHp;
    runtime.mana        = 0;
    runtime.state       = UnitState::Idle;
    runtime.targetId    = InvalidUnitId;
    runtime.attackTimer = 0.0F;
    runtime.moveTimer   = 0.0F;
    runtime.stunTimer   = 0.0F;
    runtime.combatStartPos.reset();
}

void Unit::checkInvariants() const {
    SYNERA_INVARIANT(!(boardPos.has_value() && benchSlot.has_value()));
    SYNERA_INVARIANT(star >= 1);
    SYNERA_INVARIANT(derivedStats.maxHp > 0);
    SYNERA_INVARIANT(runtime.hp >= 0);
    SYNERA_INVARIANT(runtime.hp <= derivedStats.maxHp);
}

}  // namespace synera
