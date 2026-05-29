#pragma once

#include "core/GameState.hpp"

namespace synera {

class UpgradeSystem {
public:
    bool tryMergeAfterGain(GameState& state, UnitId gainedUnitId);
};

}  // namespace synera
