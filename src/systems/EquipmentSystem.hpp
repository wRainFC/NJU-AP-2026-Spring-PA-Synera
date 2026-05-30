#pragma once

#include "core/Types.hpp"

#include <cstddef>
#include <cstdint>
#include <random>

namespace synera {

class GameState;

class EquipmentSystem {
public:
    EquipmentSystem();
    explicit EquipmentSystem(std::uint32_t seed);

    void recomputeStats(GameState& state);
    bool tryGrantRoundDrop(GameState& state, bool playerWon);
    bool equip(GameState& state, UnitId unitId, EquipmentType equipment);
    bool equipFromPool(GameState& state, std::size_t equipmentIndex, UnitId unitId);

private:
    std::mt19937 rng_;
};

}  // namespace synera
