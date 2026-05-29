#pragma once

#include "board/HexGrid.hpp"
#include "core/Types.hpp"

#include <concepts>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace synera {

// Owns board occupancy only. Unit position fields are synchronized by GameState.
class Board {
public:
    Board(int width, int height);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] bool inBounds(AxialPos pos) const noexcept;
    [[nodiscard]] bool isPlayerHalf(AxialPos pos) const noexcept;
    [[nodiscard]] bool isEnemyHalf(AxialPos pos) const noexcept;
    [[nodiscard]] std::optional<UnitId> occupant(AxialPos pos) const;
    [[nodiscard]] bool empty(AxialPos pos) const;

    // Return false for invalid writes, occupied destinations, or missing sources.
    bool place(UnitId unitId, AxialPos pos);
    void remove(AxialPos pos);
    bool move(AxialPos from, AxialPos to);
    bool swapCells(AxialPos left, AxialPos right);
    void clear();

    // Visits only in-bounds axial neighbors; callers decide passability and ownership rules.
    template <std::invocable<AxialPos> Visitor>
    void forEachNeighbor(AxialPos pos, Visitor&& visitor) const {
        for (const AxialPos direction : hex::Directions) {
            const AxialPos neighbor{pos.q + direction.q, pos.r + direction.r};
            if (inBounds(neighbor)) {
                std::invoke(std::forward<Visitor>(visitor), neighbor);
            }
        }
    }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<std::optional<UnitId>> cells_;

    [[nodiscard]] int index(AxialPos pos) const;
};

}  // namespace synera
