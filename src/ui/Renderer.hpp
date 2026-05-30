#pragma once

#include "raylib.h"

namespace synera {

class GameState;
class Layout;
class Unit;

class Renderer {
public:
    void draw(const GameState& state, const Layout& layout);

private:
    void drawTopBar(const GameState& state);
    void drawBoard(const GameState& state, const Layout& layout);
    void drawBench(const GameState& state, const Layout& layout);
    void drawShop(const GameState& state, const Layout& layout);
    void drawUnits(const GameState& state, const Layout& layout);
    void drawUnit(const Unit& unit, Rectangle rect);
    void drawStartButton(const GameState& state, const Layout& layout);
};

}  // namespace synera
