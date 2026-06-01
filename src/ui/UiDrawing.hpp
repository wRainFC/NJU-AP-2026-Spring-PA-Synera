#pragma once

#include "raylib.h"

#include <span>
#include <string>
#include <string_view>

namespace synera::ui {

enum class HorizontalAlign { Left, Center, Right };
enum class VerticalAlign { Top, Middle, Bottom };

struct ButtonStyle {
    Color background;
    Color border;
    Color text;
    Color tint;
};

struct PanelStyle {
    int fontSize   = 18;
    int lineHeight = 28;
    float padding  = 14.0F;
    float minWidth = 240.0F;
    float maxWidth = 520.0F;
};

// Sets the font used by text helpers; nullptr selects Raylib's built-in fallback font.
void setFont(const Font* font) noexcept;
[[nodiscard]] bool contains(Rectangle rect, Vector2 point) noexcept;
[[nodiscard]] Rectangle inset(Rectangle rect, float amount) noexcept;
[[nodiscard]] Rectangle virtualCanvasRect() noexcept;
[[nodiscard]] float clampedRatio(int value, int maximum) noexcept;
[[nodiscard]] int measureText(std::string_view text, int fontSize);
[[nodiscard]] int fitFontSize(std::string_view text, float maxWidth, int preferredSize, int minimumSize = 8);

void drawText(std::string_view text, int x, int y, int fontSize, Color color);
void drawTextInRect(std::string_view text, Rectangle rect, int fontSize, Color color,
                    HorizontalAlign horizontal = HorizontalAlign::Left,
                    VerticalAlign vertical = VerticalAlign::Top, float padding = 0.0F);
[[nodiscard]] bool drawTextureToRect(const Texture2D* texture, Rectangle rect, Color tint = WHITE);
void drawTextureFrameToRect(const Texture2D& texture, Rectangle source, Rectangle rect, Color tint = WHITE);
void drawTexturedRect(const Texture2D* texture, Rectangle rect, Color fallback, Color tint = WHITE);
void drawButton(const Texture2D* texture, Rectangle rect, std::string_view label, ButtonStyle style,
                int fontSize = 16);
void drawBar(Rectangle rect, float ratio, Color fill, Color track, Color outline);
[[nodiscard]] Rectangle panelNear(Vector2 anchor, float width, float height) noexcept;
[[nodiscard]] Rectangle panelNear(Vector2 anchor, std::span<const std::string> lines, PanelStyle style = {});
void drawPanel(Rectangle rect, std::span<const std::string> lines, const Texture2D* texture,
               PanelStyle style = {});

}  // namespace synera::ui
