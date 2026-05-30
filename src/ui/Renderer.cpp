#include "ui/Renderer.hpp"

#include "app/GameConfig.hpp"
#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "ui/Layout.hpp"

#include <cstddef>
#include <optional>
#include <ranges>
#include <string>

namespace synera {

namespace {

[[nodiscard]] const char* phaseName(Phase phase) noexcept {
    switch (phase) {
        case Phase::Prep:
            return "Prep";
        case Phase::Combat:
            return "Combat";
        case Phase::Resolve:
            return "Resolve";
    }
    return "Unknown";
}

[[nodiscard]] std::optional<Rectangle> unitRect(const Unit& unit, const Layout& layout) {
    return unit.boardPos.transform([&](AxialPos pos) { return layout.boardHexBounds(pos); })
        .or_else([&]() -> std::optional<Rectangle> {
            return unit.benchSlot.transform([&](int slot) { return layout.benchSlotRect(slot); });
        });
}

}  // namespace

void Renderer::draw(const GameState& state, const Layout& layout) {
    drawTopBar(state);
    drawBoard(state, layout);
    drawBench(state, layout);
    drawUnits(state, layout);
    drawStartButton(state, layout);
}

void Renderer::drawTopBar(const GameState& state) {
    const std::string text =
        "HP: " + std::to_string(state.player().hp) + "  Gold: " + std::to_string(state.player().gold) +
        "  Pop: " + std::to_string(state.playerBoardUnitCount()) + "/" +
        std::to_string(state.player().populationCap) +
        "  Round: " + std::to_string(state.player().currentRound) + "  Phase: " + phaseName(state.phase());
    DrawText(text.c_str(), 32, 24, 20, RAYWHITE);
}

void Renderer::drawBoard(const GameState& state, const Layout& layout) {
    (void)state;
    for (int y : std::views::iota(0, config::BoardHeight)) {
        for (int x : std::views::iota(0, config::BoardWidth)) {
            const AxialPos pos = hex::oddRToAxial(OffsetPos{x, y});
            const Vector2 center = layout.boardHexCenter(pos);
            const auto corners = layout.boardHexCorners(pos);
            const Color color = y < config::BoardHeight / 2 ? Color{58, 64, 72, 255} : Color{48, 78, 62, 255};
            for (int index : std::views::iota(0, 6)) {
                const Vector2 start = corners[static_cast<std::size_t>(index)];
                const Vector2 end = corners[static_cast<std::size_t>((index + 1) % 6)];
                DrawTriangle(center, end, start, color);
                DrawLineV(start, end, Color{95, 103, 112, 255});
            }
        }
    }
}

void Renderer::drawBench(const GameState& state, const Layout& layout) {
    (void)state;
    DrawText("Bench", 80, 548, 18, RAYWHITE);
    for (int slot : std::views::iota(0, config::BenchSize)) {
        const Rectangle rect = layout.benchSlotRect(slot);
        DrawRectangleRec(rect, Color{50, 50, 55, 255});
        DrawRectangleLinesEx(rect, 1.0F, Color{120, 120, 128, 255});
    }
}

void Renderer::drawUnits(const GameState& state, const Layout& layout) {
    state.forEachUnit([&](const Unit& unit) {
        if (const auto rect = unitRect(unit, layout)) {
            drawUnit(unit, *rect);
        }
    });
}

void Renderer::drawUnit(const Unit& unit, Rectangle rect) {
    const Color body = unit.owner == Owner::PlayerCtrl ? Color{74, 144, 226, 255} : Color{212, 91, 91, 255};
    DrawCircle(static_cast<int>(rect.x + rect.width / 2.0F), static_cast<int>(rect.y + rect.height / 2.0F),
               rect.width * 0.32F, body);

    const float hpRatio = static_cast<float>(unit.runtime.hp) / static_cast<float>(unit.derivedStats.maxHp);
    const float manaRatio =
        static_cast<float>(unit.runtime.mana) / static_cast<float>(unit.derivedStats.maxMana);
    DrawRectangle(static_cast<int>(rect.x + 6.0F), static_cast<int>(rect.y + 4.0F),
                  static_cast<int>((rect.width - 12.0F) * hpRatio), 5, GREEN);
    DrawRectangle(static_cast<int>(rect.x + 6.0F), static_cast<int>(rect.y + 10.0F),
                  static_cast<int>((rect.width - 12.0F) * manaRatio), 4, BLUE);
    DrawText(unit.name.c_str(), static_cast<int>(rect.x + 4.0F), static_cast<int>(rect.y + 40.0F), 9,
             RAYWHITE);
}

void Renderer::drawStartButton(const GameState& state, const Layout& layout) {
    const Rectangle rect = layout.startButtonRect();
    const bool enabled = state.phase() == Phase::Prep;
    DrawRectangleRec(rect, enabled ? Color{66, 132, 92, 255} : Color{70, 70, 70, 255});
    DrawRectangleLinesEx(rect, 1.0F, RAYWHITE);
    DrawText("Start Combat", static_cast<int>(rect.x + 22.0F), static_cast<int>(rect.y + 13.0F), 18,
             RAYWHITE);
}

}  // namespace synera
