#include "ui/Layout.hpp"

#include "app/GameConfig.hpp"
#include "board/HexGrid.hpp"

#include <cstddef>
#include <cmath>
#include <ranges>

namespace synera {

namespace {

constexpr float HexRadius = 34.0F;
constexpr float HexWidth = HexRadius * 1.73205080757F;
constexpr float BenchSlotSize = 58.0F;
constexpr float BoardLeft = 80.0F;
constexpr float BoardTop = 92.0F;
constexpr float BenchTop = 580.0F;
constexpr float SlotGap = 8.0F;
constexpr float ShopLeft = 820.0F;
constexpr float ShopTop = 160.0F;
constexpr float ShopOfferWidth = 220.0F;
constexpr float ShopOfferHeight = 52.0F;
constexpr float ShopGap = 8.0F;
constexpr float EquipmentSlotSize = 46.0F;
constexpr float EquipmentTop = 590.0F;
constexpr float Pi = 3.14159265359F;

[[nodiscard]] bool contains(Rectangle rect, Vector2 point) noexcept {
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y &&
           point.y <= rect.y + rect.height;
}

[[nodiscard]] bool containsPolygon(const std::array<Vector2, 6>& points, Vector2 point) noexcept {
    bool inside = false;
    // Ray casting keeps hit testing accurate in the empty gaps between staggered hex rows.
    for (std::size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
        const Vector2 a = points[i];
        const Vector2 b = points[j];
        const bool crosses = (a.y > point.y) != (b.y > point.y);
        if (crosses) {
            const float xAtY = (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x;
            inside = inside != (point.x < xAtY);
        }
    }
    return inside;
}

}  // namespace

Vector2 Layout::boardHexCenter(AxialPos pos) const noexcept {
    const OffsetPos offset = hex::axialToOddR(pos);
    // Pointy-top odd-r layout: odd rows are shifted by half a hex width.
    return Vector2{
        BoardLeft + HexWidth * (static_cast<float>(offset.col) + 0.5F * static_cast<float>(offset.row & 1)),
        BoardTop + HexRadius * 1.5F * static_cast<float>(offset.row),
    };
}

std::array<Vector2, 6> Layout::boardHexCorners(AxialPos pos) const noexcept {
    const Vector2 center = boardHexCenter(pos);
    std::array<Vector2, 6> corners{};
    for (int index : std::views::iota(0, 6)) {
        const float angle = (-90.0F + 60.0F * static_cast<float>(index)) * Pi / 180.0F;
        corners[static_cast<std::size_t>(index)] = Vector2{
            center.x + HexRadius * std::cos(angle),
            center.y + HexRadius * std::sin(angle),
        };
    }
    return corners;
}

Rectangle Layout::boardHexBounds(AxialPos pos) const noexcept {
    const Vector2 center = boardHexCenter(pos);
    return Rectangle{
        center.x - HexWidth / 2.0F,
        center.y - HexRadius,
        HexWidth,
        HexRadius * 2.0F,
    };
}

Rectangle Layout::benchSlotRect(int slot) const noexcept {
    return Rectangle{
        BoardLeft + static_cast<float>(slot) * (BenchSlotSize + SlotGap),
        BenchTop,
        BenchSlotSize,
        BenchSlotSize,
    };
}

Rectangle Layout::startButtonRect() const noexcept {
    return Rectangle{820.0F, 96.0F, 180.0F, 44.0F};
}

Rectangle Layout::shopOfferRect(int index) const noexcept {
    return Rectangle{
        ShopLeft,
        ShopTop + static_cast<float>(index) * (ShopOfferHeight + ShopGap),
        ShopOfferWidth,
        ShopOfferHeight,
    };
}

Rectangle Layout::shopRefreshButtonRect() const noexcept {
    return Rectangle{ShopLeft, ShopTop + 5.0F * (ShopOfferHeight + ShopGap) + 8.0F, 110.0F, 40.0F};
}

Rectangle Layout::shopLockButtonRect() const noexcept {
    return Rectangle{ShopLeft + 118.0F, ShopTop + 5.0F * (ShopOfferHeight + ShopGap) + 8.0F, 102.0F, 40.0F};
}

Rectangle Layout::populationUpgradeButtonRect() const noexcept {
    return Rectangle{ShopLeft, ShopTop + 5.0F * (ShopOfferHeight + ShopGap) + 56.0F, ShopOfferWidth, 40.0F};
}

Rectangle Layout::equipmentSlotRect(std::size_t index) const noexcept {
    return Rectangle{
        ShopLeft + static_cast<float>(index) * (EquipmentSlotSize + ShopGap),
        EquipmentTop,
        EquipmentSlotSize,
        EquipmentSlotSize,
    };
}

std::optional<AxialPos> Layout::boardPosAt(Vector2 mouse) const noexcept {
    for (int y : std::views::iota(0, config::BoardHeight)) {
        for (int x : std::views::iota(0, config::BoardWidth)) {
            const AxialPos pos = hex::oddRToAxial(OffsetPos{x, y});
            if (contains(boardHexBounds(pos), mouse) && containsPolygon(boardHexCorners(pos), mouse)) {
                return pos;
            }
        }
    }
    return std::nullopt;
}

std::optional<int> Layout::benchSlotAt(Vector2 mouse) const noexcept {
    for (int slot : std::views::iota(0, config::BenchSize)) {
        if (contains(benchSlotRect(slot), mouse)) {
            return slot;
        }
    }
    return std::nullopt;
}

std::optional<int> Layout::shopOfferAt(Vector2 mouse) const noexcept {
    for (int index : std::views::iota(0, config::ShopOfferCount)) {
        if (contains(shopOfferRect(index), mouse)) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> Layout::equipmentSlotAt(Vector2 mouse, std::size_t itemCount) const noexcept {
    for (std::size_t index : std::views::iota(std::size_t{0}, itemCount)) {
        if (contains(equipmentSlotRect(index), mouse)) {
            return index;
        }
    }
    return std::nullopt;
}

}  // namespace synera
