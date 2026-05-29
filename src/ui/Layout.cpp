#include "ui/Layout.hpp"

#include "app/GameConfig.hpp"

namespace synera {

namespace {

constexpr float TileSize = 58.0F;
constexpr float BoardLeft = 80.0F;
constexpr float BoardTop = 80.0F;
constexpr float BenchTop = 580.0F;
constexpr float SlotGap = 8.0F;

[[nodiscard]] bool contains(Rectangle rect, Vector2 point) noexcept {
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y &&
           point.y <= rect.y + rect.height;
}

} // namespace

Rectangle Layout::boardTileRect(GridPos pos) const noexcept {
    return Rectangle{
        BoardLeft + static_cast<float>(pos.x) * TileSize,
        BoardTop + static_cast<float>(pos.y) * TileSize,
        TileSize - 1.0F,
        TileSize - 1.0F,
    };
}

Rectangle Layout::benchSlotRect(int slot) const noexcept {
    return Rectangle{
        BoardLeft + static_cast<float>(slot) * (TileSize + SlotGap),
        BenchTop,
        TileSize,
        TileSize,
    };
}

Rectangle Layout::startButtonRect() const noexcept {
    return Rectangle{820.0F, 96.0F, 180.0F, 44.0F};
}

std::optional<GridPos> Layout::boardPosAt(Vector2 mouse) const noexcept {
    for (int y = 0; y < config::BoardHeight; ++y) {
        for (int x = 0; x < config::BoardWidth; ++x) {
            GridPos pos{x, y};
            if (contains(boardTileRect(pos), mouse)) {
                return pos;
            }
        }
    }
    return std::nullopt;
}

std::optional<int> Layout::benchSlotAt(Vector2 mouse) const noexcept {
    for (int slot = 0; slot < config::BenchSize; ++slot) {
        if (contains(benchSlotRect(slot), mouse)) {
            return slot;
        }
    }
    return std::nullopt;
}

} // namespace synera
