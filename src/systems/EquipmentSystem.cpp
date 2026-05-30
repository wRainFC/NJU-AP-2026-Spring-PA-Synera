#include "systems/EquipmentSystem.hpp"

#include "app/GameConfig.hpp"
#include "core/GameState.hpp"

#include <array>
#include <cstddef>
#include <random>

namespace synera {

namespace {

inline constexpr std::array EquipmentPool{
    EquipmentType::IronSword,
    EquipmentType::ChainVest,
    EquipmentType::SwiftGlove,
    EquipmentType::ManaCrystal,
};

}  // namespace

EquipmentSystem::EquipmentSystem() : EquipmentSystem(config::EquipmentRandomSeed) {}

EquipmentSystem::EquipmentSystem(std::uint32_t seed) : rng_(seed) {}

void EquipmentSystem::recomputeStats(GameState& state) {
    state.forEachUnit([](Unit& unit) { unit.recomputeDerivedStats(); });
}

bool EquipmentSystem::tryGrantRoundDrop(GameState& state, bool playerWon) {
    if (!playerWon) {
        return false;
    }

    std::uniform_int_distribution<int> chance(1, 100);
    if (chance(rng_) > config::EquipmentDropChancePercent) {
        return false;
    }

    std::uniform_int_distribution<std::size_t> equipment(0, EquipmentPool.size() - 1);
    state.addEquipment(EquipmentPool[equipment(rng_)]);
    return true;
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
