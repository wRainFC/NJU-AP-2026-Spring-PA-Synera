#pragma once

#include "core/Types.hpp"

#include <array>
#include <cstddef>

namespace synera {

namespace hex {

inline constexpr std::array<AxialPos, 6> Directions{{
    AxialPos{1, 0},
    AxialPos{1, -1},
    AxialPos{0, -1},
    AxialPos{-1, 0},
    AxialPos{-1, 1},
    AxialPos{0, 1},
}};

[[nodiscard]] constexpr OffsetPos axialToOddR(AxialPos pos) noexcept {
    return OffsetPos{
        .col = pos.q + (pos.r - (pos.r & 1)) / 2,
        .row = pos.r,
    };
}

[[nodiscard]] constexpr AxialPos oddRToAxial(OffsetPos pos) noexcept {
    return AxialPos{
        .q = pos.col - (pos.row - (pos.row & 1)) / 2,
        .r = pos.row,
    };
}

[[nodiscard]] constexpr int absInt(int value) noexcept {
    return value < 0 ? -value : value;
}

[[nodiscard]] constexpr int hexDistance(AxialPos lhs, AxialPos rhs) noexcept {
    const int dq = lhs.q - rhs.q;
    const int dr = lhs.r - rhs.r;
    const int ds = -dq - dr;
    return (absInt(dq) + absInt(dr) + absInt(ds)) / 2;
}

[[nodiscard]] constexpr AxialPos neighbor(AxialPos pos, int direction) noexcept {
    const AxialPos delta = Directions[static_cast<std::size_t>(direction)];
    return AxialPos{pos.q + delta.q, pos.r + delta.r};
}

}  // namespace hex

}  // namespace synera
