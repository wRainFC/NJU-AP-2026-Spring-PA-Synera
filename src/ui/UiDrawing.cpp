#include "ui/UiDrawing.hpp"

#include "app/GameConfig.hpp"
#include "ui/UiTheme.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace synera::ui {

namespace {

constexpr float FontSpacing = 0.0F;
const Font* CurrentFont = nullptr;

[[nodiscard]] std::string owned(std::string_view text) {
    return std::string{text};
}

[[nodiscard]] int rounded(float value) noexcept {
    return static_cast<int>(std::lround(value));
}

[[nodiscard]] Font activeFont() noexcept {
    return CurrentFont == nullptr ? GetFontDefault() : *CurrentFont;
}

[[nodiscard]] int pixelFontSize(int requested) noexcept {
    if (requested <= 13) {
        return 12;
    }
    if (requested <= 20) {
        return 18;
    }
    if (requested <= 30) {
        return 24;
    }
    if (requested <= 42) {
        return 36;
    }
    return 48;
}

}  // namespace

void setFont(const Font* font) noexcept {
    CurrentFont = font;
}

bool contains(Rectangle rect, Vector2 point) noexcept {
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y &&
           point.y <= rect.y + rect.height;
}

Rectangle inset(Rectangle rect, float amount) noexcept {
    return Rectangle{rect.x + amount, rect.y + amount, rect.width - amount * 2.0F,
                     rect.height - amount * 2.0F};
}

Rectangle virtualCanvasRect() noexcept {
    return Rectangle{0.0F, 0.0F, static_cast<float>(config::WindowWidth),
                     static_cast<float>(config::WindowHeight)};
}

float clampedRatio(int value, int maximum) noexcept {
    if (maximum <= 0) {
        return 0.0F;
    }
    return std::clamp(static_cast<float>(value) / static_cast<float>(maximum), 0.0F, 1.0F);
}

int measureText(std::string_view text, int fontSize) {
    const std::string value = owned(text);
    return rounded(
        MeasureTextEx(activeFont(), value.c_str(), static_cast<float>(pixelFontSize(fontSize)), FontSpacing)
            .x);
}

int fitFontSize(std::string_view text, float maxWidth, int preferredSize, int minimumSize) {
    int fontSize = preferredSize;
    while (fontSize > minimumSize && static_cast<float>(measureText(text, fontSize)) > maxWidth) {
        --fontSize;
    }
    return fontSize;
}

void drawText(std::string_view text, int x, int y, int fontSize, Color color) {
    const std::string value = owned(text);
    DrawTextEx(activeFont(), value.c_str(), Vector2{static_cast<float>(x), static_cast<float>(y)},
               static_cast<float>(pixelFontSize(fontSize)), FontSpacing, color);
}

void drawTextInRect(std::string_view text, Rectangle rect, int fontSize, Color color,
                    HorizontalAlign horizontal, VerticalAlign vertical, float padding) {
    const float availableWidth = std::max(0.0F, rect.width - padding * 2.0F);
    const int fittedFontSize = fitFontSize(text, availableWidth, fontSize);
    const int width = measureText(text, fittedFontSize);

    float x = rect.x + padding;
    if (horizontal == HorizontalAlign::Center) {
        x = rect.x + (rect.width - static_cast<float>(width)) / 2.0F;
    } else if (horizontal == HorizontalAlign::Right) {
        x = rect.x + rect.width - static_cast<float>(width) - padding;
    }

    float y = rect.y + padding;
    if (vertical == VerticalAlign::Middle) {
        y = rect.y + (rect.height - static_cast<float>(fittedFontSize)) / 2.0F;
    } else if (vertical == VerticalAlign::Bottom) {
        y = rect.y + rect.height - static_cast<float>(fittedFontSize) - padding;
    }

    drawText(text, rounded(x), rounded(y), fittedFontSize, color);
}

bool drawTextureToRect(const Texture2D* texture, Rectangle rect, Color tint) {
    if (texture == nullptr) {
        return false;
    }

    const Rectangle source{0.0F, 0.0F, static_cast<float>(texture->width),
                           static_cast<float>(texture->height)};
    DrawTexturePro(*texture, source, rect, Vector2{}, 0.0F, tint);
    return true;
}

void drawTextureFrameToRect(const Texture2D& texture, Rectangle source, Rectangle rect, Color tint) {
    DrawTexturePro(texture, source, rect, Vector2{}, 0.0F, tint);
}

void drawTexturedRect(const Texture2D* texture, Rectangle rect, Color fallback, Color tint) {
    if (!drawTextureToRect(texture, rect, tint)) {
        DrawRectangleRec(rect, fallback);
    }
}

void drawButton(const Texture2D* texture, Rectangle rect, std::string_view label, ButtonStyle style,
                int fontSize) {
    drawTexturedRect(texture, rect, style.background, style.tint);
    DrawRectangleLinesEx(rect, 1.0F, style.border);
    drawTextInRect(label, rect, fontSize, style.text, HorizontalAlign::Center, VerticalAlign::Middle, 8.0F);
}

void drawBar(Rectangle rect, float ratio, Color fill, Color track, Color outline) {
    DrawRectangleRec(rect, track);
    const Rectangle filled{rect.x, rect.y, rect.width * std::clamp(ratio, 0.0F, 1.0F), rect.height};
    DrawRectangleRec(filled, fill);
    DrawRectangleLinesEx(rect, 1.0F, outline);
}

Rectangle panelNear(Vector2 anchor, float width, float height) noexcept {
    Rectangle rect{anchor.x + 18.0F, anchor.y + 18.0F, width, height};
    const Rectangle bounds = virtualCanvasRect();
    if (rect.x + rect.width > bounds.width - 12.0F) {
        rect.x = anchor.x - rect.width - 18.0F;
    }
    if (rect.y + rect.height > bounds.height - 12.0F) {
        rect.y = anchor.y - rect.height - 18.0F;
    }
    rect.x = std::max(12.0F, rect.x);
    rect.y = std::max(12.0F, rect.y);
    return rect;
}

Rectangle panelNear(Vector2 anchor, std::span<const std::string> lines, PanelStyle style) {
    const Rectangle bounds = virtualCanvasRect();
    const float availableWidth = std::max(1.0F, bounds.width - 48.0F);
    const float maxWidth = std::min(style.maxWidth, availableWidth);
    int widestLine = 0;
    for (const std::string& line : lines) {
        widestLine = std::max(widestLine, measureText(line, style.fontSize));
    }

    const float width =
        std::clamp(static_cast<float>(widestLine) + style.padding * 2.0F, style.minWidth, maxWidth);
    const float height =
        style.padding * 2.0F + static_cast<float>(lines.size()) * static_cast<float>(style.lineHeight);
    return panelNear(anchor, width, height);
}

void drawPanel(Rectangle rect, std::span<const std::string> lines, const Texture2D* texture,
               PanelStyle style) {
    drawTexturedRect(texture, rect, theme::Panel, Color{255, 255, 255, 238});
    DrawRectangleLinesEx(rect, 1.0F, theme::PanelBorder);

    const float textWidth = std::max(0.0F, rect.width - style.padding * 2.0F);
    int y = rounded(rect.y + style.padding);
    for (const std::string& line : lines) {
        drawTextInRect(line,
                       Rectangle{rect.x + style.padding, static_cast<float>(y), textWidth,
                                 static_cast<float>(style.lineHeight)},
                       style.fontSize, RAYWHITE);
        y += style.lineHeight;
    }
}

}  // namespace synera::ui
