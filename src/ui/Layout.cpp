#include "ui/Layout.hpp"

#include "app/GameConfig.hpp"
#include "board/HexGrid.hpp"
#include "ui/UiDrawing.hpp"

#include <cstddef>
#include <cmath>
#include <ranges>

namespace synera {

namespace {

constexpr float HexRadius = 42.0F;
constexpr float HexWidth = HexRadius * 1.73205080757F;
constexpr float BenchSlotSize = 76.0F;
constexpr float BoardLeft = 210.0F;
constexpr float BoardTop = 180.0F;
constexpr float BenchLeft = 150.0F;
constexpr float BenchTop = 780.0F;
constexpr float SlotGap = 12.0F;
constexpr float ShopLeft = 1040.0F;
constexpr float ShopTop = 178.0F;
constexpr float ShopOfferWidth = 300.0F;
constexpr float ShopOfferHeight = 70.0F;
constexpr float ShopGap = 10.0F;
constexpr float EquipmentSlotSize = 60.0F;
constexpr float EquipmentTop = 766.0F;
constexpr float TraitLeft = 150.0F;
constexpr float TraitTop = 82.0F;
constexpr float TraitWidth = 112.0F;
constexpr float TraitHeight = 34.0F;
constexpr float TraitGap = 10.0F;
constexpr float ButtonTop = 82.0F;
constexpr float SellLeft = 1370.0F;
constexpr float SellTop = 178.0F;
constexpr float Pi = 3.14159265359F;

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
        BenchLeft + static_cast<float>(slot) * (BenchSlotSize + SlotGap),
        BenchTop,
        BenchSlotSize,
        BenchSlotSize,
    };
}

Rectangle Layout::startButtonRect() const noexcept {
    return Rectangle{1040.0F, ButtonTop, 196.0F, 52.0F};
}

Rectangle Layout::saveButtonRect() const noexcept {
    return Rectangle{1250.0F, ButtonTop, 96.0F, 52.0F};
}

Rectangle Layout::loadButtonRect() const noexcept {
    return Rectangle{1360.0F, ButtonTop, 96.0F, 52.0F};
}

Rectangle Layout::outcomeRestartButtonRect() const noexcept {
    return Rectangle{static_cast<float>(config::WindowWidth) / 2.0F - 240.0F,
                     static_cast<float>(config::WindowHeight) / 2.0F + 54.0F, 220.0F, 52.0F};
}

Rectangle Layout::outcomeLoadButtonRect() const noexcept {
    return Rectangle{static_cast<float>(config::WindowWidth) / 2.0F + 20.0F,
                     static_cast<float>(config::WindowHeight) / 2.0F + 54.0F, 220.0F, 52.0F};
}

Rectangle Layout::traitSlotRect(Trait trait) const noexcept {
    return Rectangle{
        TraitLeft + static_cast<float>(static_cast<int>(trait)) * (TraitWidth + TraitGap),
        TraitTop,
        TraitWidth,
        TraitHeight,
    };
}

Rectangle Layout::sellAreaRect() const noexcept {
    return Rectangle{SellLeft, SellTop, 170.0F, 110.0F};
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
    return Rectangle{ShopLeft, ShopTop + 5.0F * (ShopOfferHeight + ShopGap) + 12.0F, 144.0F, 48.0F};
}

Rectangle Layout::shopLockButtonRect() const noexcept {
    return Rectangle{ShopLeft + 156.0F, ShopTop + 5.0F * (ShopOfferHeight + ShopGap) + 12.0F, 144.0F, 48.0F};
}

Rectangle Layout::populationUpgradeButtonRect() const noexcept {
    return Rectangle{ShopLeft, ShopTop + 5.0F * (ShopOfferHeight + ShopGap) + 72.0F, ShopOfferWidth, 48.0F};
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
            if (ui::contains(boardHexBounds(pos), mouse) && containsPolygon(boardHexCorners(pos), mouse)) {
                return pos;
            }
        }
    }
    return std::nullopt;
}

std::optional<int> Layout::benchSlotAt(Vector2 mouse) const noexcept {
    for (int slot : std::views::iota(0, config::BenchSize)) {
        if (ui::contains(benchSlotRect(slot), mouse)) {
            return slot;
        }
    }
    return std::nullopt;
}

std::optional<int> Layout::shopOfferAt(Vector2 mouse) const noexcept {
    for (int index : std::views::iota(0, config::ShopOfferCount)) {
        if (ui::contains(shopOfferRect(index), mouse)) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<Trait> Layout::traitAt(Vector2 mouse) const noexcept {
    for (int index : std::views::iota(0, 6)) {
        const auto trait = static_cast<Trait>(index);
        if (ui::contains(traitSlotRect(trait), mouse)) {
            return trait;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> Layout::equipmentSlotAt(Vector2 mouse, std::size_t itemCount) const noexcept {
    for (std::size_t index : std::views::iota(std::size_t{0}, itemCount)) {
        if (ui::contains(equipmentSlotRect(index), mouse)) {
            return index;
        }
    }
    return std::nullopt;
}

}  // namespace synera
