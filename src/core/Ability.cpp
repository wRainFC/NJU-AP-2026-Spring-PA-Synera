#include "core/Ability.hpp"

#include "core/Unit.hpp"

namespace synera {

std::string_view NoopAbility::name() const noexcept {
    return "No Skill";
}

std::string_view NoopAbility::description() const noexcept {
    return "Placeholder ability.";
}

void NoopAbility::cast(Unit& caster, GameState&) {
    caster.currentStats.mana = 0;
}

std::string_view FireLineAbility::name() const noexcept {
    return "Fire Line";
}

std::string_view FireLineAbility::description() const noexcept {
    return "Deals line damage. Stub implementation for the interface stage.";
}

void FireLineAbility::cast(Unit& caster, GameState&) {
    caster.currentStats.mana = 0;
}

std::string_view HealingAuraAbility::name() const noexcept {
    return "Healing Aura";
}

std::string_view HealingAuraAbility::description() const noexcept {
    return "Heals nearby allies. Stub implementation for the interface stage.";
}

void HealingAuraAbility::cast(Unit& caster, GameState&) {
    caster.currentStats.mana = 0;
}

} // namespace synera
