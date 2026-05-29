#pragma once

namespace synera {

class GameState;

class ShopSystem {
public:
    void refresh(GameState& state, bool payCost);
    bool buy(GameState& state, int offerIndex);
};

}  // namespace synera
