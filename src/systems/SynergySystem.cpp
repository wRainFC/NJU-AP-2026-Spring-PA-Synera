#include "systems/SynergySystem.hpp"

#include "core/GameState.hpp"

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

void SynergySystem::recompute(GameState& state) {
    state.forEachUnit([](Unit& unit) { unit.recomputeDerivedStats(); });

    const TraitCounts counts = countPlayerBoardTraits(state);
    state.forEachPlayerBoardUnit([&](Unit& unit) {
        if (counts[traitIndex(Trait::Guardian)] >= 2) {
            unit.derivedStats.maxHp += 80;
        }
        if (counts[traitIndex(Trait::Mystic)] >= 2) {
            unit.derivedStats.maxMana = std::max(20, unit.derivedStats.maxMana - 10);
        }
        if (counts[traitIndex(Trait::Warrior)] >= 2 && hasTrait(unit, Trait::Warrior)) {
            unit.derivedStats.atk += 15;
        }
        if (counts[traitIndex(Trait::Ranger)] >= 2 && hasTrait(unit, Trait::Ranger)) {
            unit.derivedStats.attackInterval *= 0.85F;
        }
        clampRuntime(unit);
    });
}

}  // namespace synera
