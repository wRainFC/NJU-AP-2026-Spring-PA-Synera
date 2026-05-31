#pragma once

#include "core/Types.hpp"

#include <string_view>

namespace synera {

class GameState;

// Display-ready trait state derived from the current player board.
struct TraitSummary {
    Trait trait;
    int count = 0;
    int activationThreshold = 0;
    bool active = false;
    std::string_view name;
    std::string_view effectDescription;
};

// Counts only player-controlled units currently placed on the board.
[[nodiscard]] int countPlayerBoardTrait(const GameState& state, Trait trait);
[[nodiscard]] bool traitIsActive(Trait trait, int count) noexcept;
[[nodiscard]] TraitSummary summarizeTrait(const GameState& state, Trait trait);

class SynergySystem {
public:
    // Rebuilds derived stats and mechanics from base unit state, equipment, and active traits.
    void recompute(GameState& state);
};

}  // namespace synera
