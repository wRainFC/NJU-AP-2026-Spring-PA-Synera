#pragma once

#include "core/Types.hpp"
#include "raylib.h"

#include <optional>

namespace synera {

class Layout {
public:
    [[nodiscard]] Rectangle boardTileRect(GridPos pos) const noexcept;
    [[nodiscard]] Rectangle benchSlotRect(int slot) const noexcept;
    [[nodiscard]] Rectangle startButtonRect() const noexcept;
    [[nodiscard]] std::optional<GridPos> boardPosAt(Vector2 mouse) const noexcept;
    [[nodiscard]] std::optional<int> benchSlotAt(Vector2 mouse) const noexcept;
};

} // namespace synera
