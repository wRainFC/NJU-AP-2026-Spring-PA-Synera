#pragma once

#include "core/GameState.hpp"

namespace synera {

class ShopSystem {
public:
    void refresh(GameState& state, bool payCost);
    bool buy(GameState& state, int offerIndex);
};

}  // namespace synera
