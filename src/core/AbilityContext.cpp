#include "core/AbilityContext.hpp"

#include "core/GameState.hpp"
#include "core/Unit.hpp"

namespace synera {

AbilityContext::AbilityContext(GameState& state) noexcept : state_(state) {}

std::vector<Unit*> AbilityContext::enemiesOf(const Unit& unit) {
    std::vector<Unit*> result;
    for (Unit* candidate : state_.allUnits()) {
        if (candidate->owner != unit.owner && candidate->alive()) {
            result.push_back(candidate);
        }
    }
    return result;
}

std::vector<Unit*> AbilityContext::alliesOf(const Unit& unit) {
    std::vector<Unit*> result;
    for (Unit* candidate : state_.allUnits()) {
        if (candidate->owner == unit.owner && candidate->alive()) {
            result.push_back(candidate);
        }
    }
    return result;
}

void AbilityContext::dealDamage(Unit& target, int amount) const noexcept {
    target.receiveDamage(amount);
}

void AbilityContext::heal(Unit& target, int amount) const noexcept {
    target.heal(amount);
}

} // namespace synera
