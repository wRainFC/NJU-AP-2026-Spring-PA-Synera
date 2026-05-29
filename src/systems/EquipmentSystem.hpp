#pragma once

#include "core/GameState.hpp"

namespace synera {

class EquipmentSystem {
public:
    void recomputeStats(GameState& state);
    bool equip(GameState& state, UnitId unitId, EquipmentType equipment);
};

} // namespace synera
