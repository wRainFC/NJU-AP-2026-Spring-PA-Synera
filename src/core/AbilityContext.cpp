#include "core/AbilityContext.hpp"

#include "core/GameState.hpp"
#include "core/Unit.hpp"

namespace synera {

AbilityContext::AbilityContext(GameState& state) noexcept : state_(state) {}

void AbilityContext::dealDamage(Unit& target, int amount) const noexcept {
    const int before = target.runtime.hp;
    target.receiveDamage(amount);
    const int damage = before - target.runtime.hp;
    if (damage > 0) {
        results_.push_back(AbilityResult{
            .type = AbilityResultType::Damage,
            .targetId = target.id,
            .targetPos = target.boardPos.value_or(AxialPos{}),
            .amount = damage,
        });
    }
}

void AbilityContext::heal(Unit& target, int amount) const noexcept {
    const int before = target.runtime.hp;
    target.heal(amount);
    const int healed = target.runtime.hp - before;
    if (healed > 0) {
        results_.push_back(AbilityResult{
            .type = AbilityResultType::Heal,
            .targetId = target.id,
            .targetPos = target.boardPos.value_or(AxialPos{}),
            .amount = healed,
        });
    }
}

void AbilityContext::applyStun(Unit& target, float durationSeconds) const noexcept {
    if (!target.alive() || durationSeconds <= 0.0F) {
        return;
    }
    target.runtime.state = UnitState::Stunned;
    target.runtime.stunTimer = durationSeconds;
    results_.push_back(AbilityResult{
        .type = AbilityResultType::Stun,
        .targetId = target.id,
        .targetPos = target.boardPos.value_or(AxialPos{}),
        .durationSeconds = durationSeconds,
    });
}

Unit* AbilityContext::findUnit(UnitId id) const {
    return state_.findUnit(id);
}

std::span<const AbilityResult> AbilityContext::results() const noexcept {
    return results_;
}

}  // namespace synera
