#include "board/Board.hpp"

#include "core/Contract.hpp"

#include <algorithm>

namespace synera {

Board::Board(int width, int height) : width_(width), height_(height), cells_(width * height) {
    SYNERA_EXPECTS(width > 0);
    SYNERA_EXPECTS(height > 0);
}

int Board::width() const noexcept {
    return width_;
}

int Board::height() const noexcept {
    return height_;
}

bool Board::inBounds(AxialPos pos) const noexcept {
    const OffsetPos offset = hex::axialToOddR(pos);
    return offset.col >= 0 && offset.row >= 0 && offset.col < width_ && offset.row < height_;
}

bool Board::isPlayerHalf(AxialPos pos) const noexcept {
    const OffsetPos offset = hex::axialToOddR(pos);
    return inBounds(pos) && offset.row >= height_ / 2;
}

bool Board::isEnemyHalf(AxialPos pos) const noexcept {
    const OffsetPos offset = hex::axialToOddR(pos);
    return inBounds(pos) && offset.row < height_ / 2;
}

std::optional<UnitId> Board::occupant(AxialPos pos) const {
    if (!inBounds(pos)) {
        return std::nullopt;
    }
    return cells_[index(pos)];
}

bool Board::empty(AxialPos pos) const {
    return inBounds(pos) && !cells_[index(pos)].has_value();
}

bool Board::place(UnitId unitId, AxialPos pos) {
    SYNERA_EXPECTS(unitId != InvalidUnitId);
    if (!empty(pos)) {
        return false;
    }

    cells_[index(pos)] = unitId;
    SYNERA_ENSURES(occupant(pos) == unitId);
    return true;
}

void Board::remove(AxialPos pos) {
    if (!inBounds(pos)) {
        return;
    }
    cells_[index(pos)].reset();
}

bool Board::move(AxialPos from, AxialPos to) {
    if (!inBounds(from) || !empty(to) || !cells_[index(from)].has_value()) {
        return false;
    }

    cells_[index(to)] = cells_[index(from)];
    cells_[index(from)].reset();
    return true;
}

void Board::clear() {
    std::ranges::fill(cells_, std::nullopt);
}

int Board::index(AxialPos pos) const {
    SYNERA_EXPECTS(inBounds(pos));
    const OffsetPos offset = hex::axialToOddR(pos);
    return offset.row * width_ + offset.col;
}

}  // namespace synera
