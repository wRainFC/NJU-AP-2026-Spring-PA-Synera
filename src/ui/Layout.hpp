#pragma once

#include "core/Types.hpp"
#include "raylib.h"

#include <array>
#include <cstddef>
#include <optional>

namespace synera {

// Converts between axial board positions and pointy-top hex screen geometry.
class Layout {
public:
    [[nodiscard]] Vector2 boardHexCenter(AxialPos pos) const noexcept;
    [[nodiscard]] std::array<Vector2, 6> boardHexCorners(AxialPos pos) const noexcept;
    [[nodiscard]] Rectangle boardHexBounds(AxialPos pos) const noexcept;
    [[nodiscard]] Rectangle benchSlotRect(int slot) const noexcept;
    [[nodiscard]] Rectangle shopOfferRect(int index) const noexcept;
    [[nodiscard]] Rectangle shopRefreshButtonRect() const noexcept;
    [[nodiscard]] Rectangle shopLockButtonRect() const noexcept;
    [[nodiscard]] Rectangle populationUpgradeButtonRect() const noexcept;
    [[nodiscard]] Rectangle equipmentSlotRect(std::size_t index) const noexcept;
    [[nodiscard]] Rectangle startButtonRect() const noexcept;
    [[nodiscard]] Rectangle saveButtonRect() const noexcept;
    [[nodiscard]] Rectangle loadButtonRect() const noexcept;
    // Returns nullopt when the pointer is in the visual gap between hexes or outside the board.
    [[nodiscard]] std::optional<AxialPos> boardPosAt(Vector2 mouse) const noexcept;
    [[nodiscard]] std::optional<int> benchSlotAt(Vector2 mouse) const noexcept;
    [[nodiscard]] std::optional<int> shopOfferAt(Vector2 mouse) const noexcept;
    [[nodiscard]] std::optional<std::size_t> equipmentSlotAt(Vector2 mouse,
                                                             std::size_t itemCount) const noexcept;
};

}  // namespace synera
