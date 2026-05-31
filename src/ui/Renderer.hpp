#pragma once

#include "raylib.h"

#include <string_view>

namespace synera {

class GameState;
class Layout;
class Unit;

class Renderer {
public:
    void draw(const GameState& state, const Layout& layout, std::string_view statusMessage);

private:
    void drawTopBar(const GameState& state, std::string_view statusMessage);
    void drawBoard(const GameState& state, const Layout& layout);
    void drawBench(const GameState& state, const Layout& layout);
    void drawShop(const GameState& state, const Layout& layout);
    void drawPopulationUpgrade(const GameState& state, const Layout& layout);
    void drawEquipmentPool(const GameState& state, const Layout& layout);
    void drawSynergies(const GameState& state);
    void drawUnits(const GameState& state, const Layout& layout);
    void drawUnit(const Unit& unit, Rectangle rect);
    void drawStartButton(const GameState& state, const Layout& layout);
    void drawSaveLoadButtons(const GameState& state, const Layout& layout);
};

}  // namespace synera
