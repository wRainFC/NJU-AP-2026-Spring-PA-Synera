#include "systems/EquipmentSystem.hpp"

#include "config/EquipmentConfig.hpp"
#include "core/GameState.hpp"

#include <cstddef>
#include <random>

namespace synera {

EquipmentSystem::EquipmentSystem() : EquipmentSystem(config::EquipmentRandomSeed) {}

EquipmentSystem::EquipmentSystem(std::uint32_t seed) : rng_(seed) {}

void EquipmentSystem::recomputeStats(GameState& state) {
    state.forEachUnit([](Unit& unit) { unit.recomputeDerivedStats(); });
}

EquipmentDropResult EquipmentSystem::tryGrantRoundDrop(GameState& state, bool playerWon) {
    if (config::RoundDropRule.requiresWin && !playerWon) {
        return {};
    }

    std::uniform_int_distribution<int> chance(1, 100);
    if (chance(rng_) > config::RoundDropRule.chancePercent) {
        return {};
    }

    std::uniform_int_distribution<std::size_t> equipment(0, config::EquipmentPool.size() - 1);
    const EquipmentType droppedEquipment = config::EquipmentPool[equipment(rng_)];
    state.addEquipment(droppedEquipment);
    return EquipmentDropResult{.dropped = true, .equipment = droppedEquipment};
}

bool EquipmentSystem::equip(GameState& state, UnitId unitId, EquipmentType equipment) {
    Unit* unit = state.findUnit(unitId);
    if (unit == nullptr || unit->equipment) {
        return false;
    }
    unit->equipment = equipment;
    unit->recomputeDerivedStats();
    return true;
}

bool EquipmentSystem::equipFromPool(GameState& state, std::size_t equipmentIndex, UnitId unitId) {
    const auto pool = state.equipmentPool();
    if (equipmentIndex >= pool.size()) {
        return false;
    }

    const EquipmentType equipment = pool[equipmentIndex];
    if (!equip(state, unitId, equipment)) {
        return false;
    }
    return state.removeEquipmentAt(equipmentIndex);
}

}  // namespace synera
