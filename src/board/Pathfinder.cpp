#include "board/Pathfinder.hpp"

#include "board/HexGrid.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <queue>
#include <vector>

namespace synera {

namespace {

struct SearchNode {
    AxialPos pos;
    int cost     = 0;
    int priority = 0;
};

struct SearchNodeGreater {
    [[nodiscard]] bool operator()(const SearchNode& lhs, const SearchNode& rhs) const noexcept {
        return lhs.priority > rhs.priority;
    }
};

[[nodiscard]] int cellIndex(const Board& board, AxialPos pos) noexcept {
    const OffsetPos offset = hex::axialToOddR(pos);
    return offset.row * board.width() + offset.col;
}

[[nodiscard]] std::vector<AxialPos> reconstructPath(const Board& board, AxialPos start, AxialPos goal,
                                                    const std::vector<std::optional<AxialPos>>& cameFrom) {
    std::vector<AxialPos> path;
    AxialPos current = goal;
    while (current != start) {
        path.push_back(current);
        const auto previous = cameFrom[static_cast<std::size_t>(cellIndex(board, current))];
        if (!previous) {
            return {};
        }
        current = *previous;
    }
    std::ranges::reverse(path);
    return path;
}

}  // namespace

std::vector<AxialPos> Pathfinder::findPathToAttackRange(const Board& board, AxialPos start, AxialPos target,
                                                        int attackRange) const {
    if (!board.inBounds(start) || !board.inBounds(target) || attackRange < 1) {
        return {};
    }
    if (hex::hexDistance(start, target) <= attackRange) {
        return {};
    }

    const std::size_t cellCount = static_cast<std::size_t>(board.width() * board.height());
    std::vector<int> bestCost(cellCount, std::numeric_limits<int>::max());
    std::vector<std::optional<AxialPos>> cameFrom(cellCount);
    std::priority_queue<SearchNode, std::vector<SearchNode>, SearchNodeGreater> frontier;

    bestCost[static_cast<std::size_t>(cellIndex(board, start))] = 0;
    frontier.push(SearchNode{
        .pos      = start,
        .cost     = 0,
        .priority = hex::hexDistance(start, target),
    });

    while (!frontier.empty()) {
        const SearchNode current = frontier.top();
        frontier.pop();

        if (current.cost != bestCost[static_cast<std::size_t>(cellIndex(board, current.pos))]) {
            continue;
        }

        if (board.empty(current.pos) && hex::hexDistance(current.pos, target) <= attackRange) {
            return reconstructPath(board, start, current.pos, cameFrom);
        }

        board.forEachNeighbor(current.pos, [&](AxialPos next) {
            // Units stand on cells, so the target cell and occupied blockers are not traversable.
            if (next == target || !board.empty(next)) {
                return;
            }

            const int nextCost          = current.cost + 1;
            const std::size_t nextIndex = static_cast<std::size_t>(cellIndex(board, next));
            if (nextCost >= bestCost[nextIndex]) {
                return;
            }

            bestCost[nextIndex] = nextCost;
            cameFrom[nextIndex] = current.pos;
            frontier.push(SearchNode{
                .pos      = next,
                .cost     = nextCost,
                .priority = nextCost + hex::hexDistance(next, target),
            });
        });
    }

    return {};
}

}  // namespace synera
