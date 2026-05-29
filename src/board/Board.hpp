#pragma once

#include "core/Types.hpp"

#include <optional>
#include <vector>

namespace synera {

class Board {
public:
    Board(int width, int height);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] bool inBounds(GridPos pos) const noexcept;
    [[nodiscard]] bool isPlayerHalf(GridPos pos) const noexcept;
    [[nodiscard]] bool isEnemyHalf(GridPos pos) const noexcept;
    [[nodiscard]] std::optional<UnitId> occupant(GridPos pos) const;
    [[nodiscard]] bool empty(GridPos pos) const;

    bool place(UnitId unitId, GridPos pos);
    void remove(GridPos pos);
    bool move(GridPos from, GridPos to);
    void clear();

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<std::optional<UnitId>> cells_;

    [[nodiscard]] int index(GridPos pos) const;
};

}  // namespace synera
