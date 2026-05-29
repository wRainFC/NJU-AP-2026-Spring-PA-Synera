#include "systems/EquipmentSystem.hpp"

namespace synera {

void EquipmentSystem::recomputeStats(GameState&) {}

bool EquipmentSystem::equip(GameState& state, UnitId unitId, EquipmentType equipment) {
    Unit* unit = state.findUnit(unitId);
    if (unit == nullptr || unit->equipment) {
        return false;
    }
    unit->equipment = equipment;
    return true;
}

}  // namespace synera
