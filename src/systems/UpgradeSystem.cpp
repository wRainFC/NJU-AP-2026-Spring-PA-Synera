#include "systems/UpgradeSystem.hpp"

#include "core/GameState.hpp"

namespace synera {

bool UpgradeSystem::tryMergeAfterGain(GameState&, UnitId) {
    return false;
}

}  // namespace synera
