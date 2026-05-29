#pragma once

#include "core/Types.hpp"

namespace synera {

class GameState;

class UpgradeSystem {
public:
    bool tryMergeAfterGain(GameState& state, UnitId gainedUnitId);
};

}  // namespace synera
