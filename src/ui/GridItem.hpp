#pragma once

#include "core/Types.hpp"
#include "raylib.h"

#include <array>

namespace synera {

class Layout;
class RenderAssets;

class GridItem {
public:
    static void drawBoardHex(const RenderAssets &assets, const Layout &layout, AxialPos pos, bool enemyHalf);
    static void drawBenchSlot(Rectangle rect);
};

}  // namespace synera
