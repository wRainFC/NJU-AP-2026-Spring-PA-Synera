#pragma once

#include "board/Board.hpp"

#include <vector>

namespace synera {

class Pathfinder {
public:
    [[nodiscard]] std::vector<AxialPos> findPathToAttackRange(const Board& board, AxialPos start,
                                                              AxialPos target, int attackRange) const;
};

}  // namespace synera
