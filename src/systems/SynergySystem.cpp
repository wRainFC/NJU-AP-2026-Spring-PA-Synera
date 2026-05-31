#include "systems/SynergySystem.hpp"

#include "core/GameState.hpp"
#include "core/Metadata.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>

namespace synera {

namespace {

using TraitCounts = std::array<int, static_cast<std::size_t>(Trait::Assassin) + 1>;

[[nodiscard]] std::size_t traitIndex(Trait trait) noexcept {
    return static_cast<std::size_t>(trait);
}

[[nodiscard]] bool hasTrait(const Unit& unit, Trait trait) {
    return std::ranges::find(unit.traits, trait) != unit.traits.end();
}

[[nodiscard]] TraitCounts countPlayerBoardTraits(const GameState& state) {
    TraitCounts counts{};
    state.forEachPlayerBoardUnit([&](const Unit& unit) {
        for (Trait trait : unit.traits) {
            ++counts[traitIndex(trait)];
        }
    });
    return counts;
}

void clampRuntime(Unit& unit) noexcept {
    unit.runtime.hp = std::clamp(unit.runtime.hp, 0, unit.derivedStats.maxHp);
    unit.runtime.mana = std::clamp(unit.runtime.mana, 0, unit.derivedStats.maxMana);
}

}  // namespace

int countPlayerBoardTrait(const GameState& state, Trait trait) {
    int count = 0;
    state.forEachPlayerBoardUnit([&](const Unit& unit) {
        if (hasTrait(unit, trait)) {
            ++count;
        }
    });
    return count;
}

bool traitIsActive(Trait trait, int count) noexcept {
    const int threshold = traitActivationThreshold(trait);
    return threshold > 0 && count >= threshold;
}

TraitSummary summarizeTrait(const GameState& state, Trait trait) {
    const int count = countPlayerBoardTrait(state, trait);
    const int threshold = traitActivationThreshold(trait);
    return TraitSummary{
        .trait = trait,
        .count = count,
        .activationThreshold = threshold,
        .active = threshold > 0 && count >= threshold,
        .name = traitName(trait),
        .effectDescription = traitEffectDescription(trait),
    };
}

void SynergySystem::recompute(GameState& state) {
    state.forEachUnit([](Unit& unit) { unit.recomputeDerivedStats(); });

    const TraitCounts counts = countPlayerBoardTraits(state);
    state.forEachPlayerBoardUnit([&](Unit& unit) {
        if (traitIsActive(Trait::Guardian, counts[traitIndex(Trait::Guardian)])) {
            unit.derivedStats.maxHp += 80;
        }
        if (traitIsActive(Trait::Mystic, counts[traitIndex(Trait::Mystic)])) {
            unit.derivedStats.maxMana = std::max(20, unit.derivedStats.maxMana - 10);
        }
        if (traitIsActive(Trait::Warrior, counts[traitIndex(Trait::Warrior)]) &&
            hasTrait(unit, Trait::Warrior)) {
            unit.derivedStats.atk += 15;
        }
        if (traitIsActive(Trait::Ranger, counts[traitIndex(Trait::Ranger)]) &&
            hasTrait(unit, Trait::Ranger)) {
            unit.mechanics.doubleBasicAttack = true;
        }
        clampRuntime(unit);
    });
}

}  // namespace synera
