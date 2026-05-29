#include "core/AbilityContext.hpp"

#include "core/GameState.hpp"
#include "core/Unit.hpp"

namespace synera {

AbilityContext::AbilityContext(GameState& state) noexcept : state_(state) {}

void AbilityContext::dealDamage(Unit& target, int amount) const noexcept {
    target.receiveDamage(amount);
}

void AbilityContext::heal(Unit& target, int amount) const noexcept {
    target.heal(amount);
}

}  // namespace synera
