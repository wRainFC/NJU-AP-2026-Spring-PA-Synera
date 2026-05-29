#pragma once

#include "board/Board.hpp"

#include <vector>

namespace synera {

class Pathfinder {
public:
    [[nodiscard]] std::vector<GridPos> findPathToAttackRange(const Board& board, GridPos start,
                                                             GridPos target, int attackRange) const;
};

} // namespace synera
