#include "systems/UpgradeSystem.hpp"

#include "core/GameState.hpp"

#include <algorithm>
#include <ranges>
#include <vector>

namespace synera {

namespace {

[[nodiscard]] bool sameMergeGroup(const Unit& unit, const Unit& gained) {
    return unit.owner == Owner::PlayerCtrl && unit.templateId == gained.templateId &&
           unit.star == gained.star && (unit.onBoard() || unit.onBench());
}

}  // namespace

bool UpgradeSystem::tryMergeAfterGain(GameState& state, UnitId gainedUnitId) {
    Unit* gained = state.findUnit(gainedUnitId);
    if (gained == nullptr || gained->owner != Owner::PlayerCtrl || state.phase() != Phase::Prep) {
        return false;
    }

    bool merged = false;
    while (true) {
        std::vector<UnitId> candidates;
        state.forEachUnit([&](const Unit& unit) {
            if (sameMergeGroup(unit, *gained)) {
                candidates.push_back(unit.id);
            }
        });

        if (candidates.size() < 3) {
            return merged;
        }

        std::ranges::sort(candidates, [&](UnitId left, UnitId right) {
            if (left == gainedUnitId) {
                return false;
            }
            if (right == gainedUnitId) {
                return true;
            }
            return left < right;
        });

        for (UnitId consumed : candidates | std::views::take(2)) {
            if (consumed != gainedUnitId) {
                (void)state.removeUnit(consumed);
            }
        }

        ++gained->star;
        gained->recomputeDerivedStats();
        gained->runtime.hp   = gained->derivedStats.maxHp;
        gained->runtime.mana = 0;
        merged               = true;
    }
}

}  // namespace synera
