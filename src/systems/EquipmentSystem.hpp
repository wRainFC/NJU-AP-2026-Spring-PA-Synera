#pragma once

#include "core/Types.hpp"

namespace synera {

class GameState;

class EquipmentSystem {
public:
    void recomputeStats(GameState& state);
    bool equip(GameState& state, UnitId unitId, EquipmentType equipment);
};

}  // namespace synera
