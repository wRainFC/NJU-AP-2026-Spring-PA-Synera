#include "board/Board.hpp"

#include "core/Contract.hpp"

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

bool Board::inBounds(GridPos pos) const noexcept {
    return pos.x >= 0 && pos.y >= 0 && pos.x < width_ && pos.y < height_;
}

bool Board::isPlayerHalf(GridPos pos) const noexcept {
    return inBounds(pos) && pos.y >= height_ / 2;
}

bool Board::isEnemyHalf(GridPos pos) const noexcept {
    return inBounds(pos) && pos.y < height_ / 2;
}

std::optional<UnitId> Board::occupant(GridPos pos) const {
    if (!inBounds(pos)) {
        return std::nullopt;
    }
    return cells_[index(pos)];
}

bool Board::empty(GridPos pos) const {
    return inBounds(pos) && !cells_[index(pos)].has_value();
}

bool Board::place(UnitId unitId, GridPos pos) {
    SYNERA_EXPECTS(unitId != InvalidUnitId);
    if (!empty(pos)) {
        return false;
    }

    cells_[index(pos)] = unitId;
    SYNERA_ENSURES(occupant(pos) == unitId);
    return true;
}

void Board::remove(GridPos pos) {
    if (!inBounds(pos)) {
        return;
    }
    cells_[index(pos)].reset();
}

bool Board::move(GridPos from, GridPos to) {
    if (!inBounds(from) || !empty(to) || !cells_[index(from)].has_value()) {
        return false;
    }

    cells_[index(to)] = cells_[index(from)];
    cells_[index(from)].reset();
    return true;
}

void Board::clear() {
    for (auto& cell : cells_) {
        cell.reset();
    }
}

int Board::index(GridPos pos) const {
    SYNERA_EXPECTS(inBounds(pos));
    return pos.y * width_ + pos.x;
}

} // namespace synera
