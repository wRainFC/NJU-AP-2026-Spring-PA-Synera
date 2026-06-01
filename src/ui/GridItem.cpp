#include "ui/GridItem.hpp"

#include "ui/Layout.hpp"
#include "ui/RenderAssets.hpp"
#include "ui/UiDrawing.hpp"
#include "ui/UiTheme.hpp"

#include <cstddef>
#include <ranges>

namespace synera {

namespace {

void drawHexOutline(const std::array<Vector2, 6> &corners, Color color) {
    for (int index : std::views::iota(0, 6)) {
        const Vector2 start = corners[static_cast<std::size_t>(index)];
        const Vector2 end   = corners[static_cast<std::size_t>((index + 1) % 6)];
        DrawLineV(start, end, color);
    }
}

void drawHexGeometry(Vector2 center, const std::array<Vector2, 6> &corners, Color fill, Color outline) {
    for (int index : std::views::iota(0, 6)) {
        const Vector2 start = corners[static_cast<std::size_t>(index)];
        const Vector2 end   = corners[static_cast<std::size_t>((index + 1) % 6)];
        DrawTriangle(center, end, start, fill);
    }
    drawHexOutline(corners, outline);
}

}  // namespace

void GridItem::drawBoardHex(const RenderAssets &assets, const Layout &layout, AxialPos pos, bool enemyHalf) {
    const TextureSlot textureSlot = enemyHalf ? TextureSlot::EnemyBoardHex : TextureSlot::PlayerBoardHex;
    const auto corners            = layout.boardHexCorners(pos);
    if (ui::drawTextureToRect(assets.texture(textureSlot), layout.boardHexBounds(pos))) {
        drawHexOutline(corners, ui::theme::BoardBorder);
        return;
    }

    drawHexGeometry(layout.boardHexCenter(pos), corners,
                    enemyHalf ? ui::theme::BoardEnemy : ui::theme::BoardPlayer, ui::theme::BoardBorder);
}

void GridItem::drawBenchSlot(Rectangle rect) {
    DrawRectangleRec(rect, ui::theme::SurfaceMuted);
    DrawRectangleLinesEx(rect, 1.0F, ui::theme::SlotBorder);
}

}  // namespace synera
