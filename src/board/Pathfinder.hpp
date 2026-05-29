#pragma once

#include "board/Board.hpp"

#include <vector>

namespace synera {

class Pathfinder {
public:
    // Finds a path from start to an empty cell within attackRange of target; target itself is blocked.
    [[nodiscard]] std::vector<AxialPos> findPathToAttackRange(const Board& board, AxialPos start,
                                                              AxialPos target, int attackRange) const;
};

}  // namespace synera
