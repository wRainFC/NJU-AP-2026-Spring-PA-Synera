#pragma once

#include "core/Types.hpp"
#include "raylib.h"

namespace synera {

class RenderAssets;
class Unit;

class UnitItem {
public:
    static void drawUnit(const RenderAssets& assets, const Unit& unit, Rectangle rect,
                         float animationTimeSeconds);
    static void drawEquipmentIcon(const RenderAssets& assets, EquipmentType equipment, Rectangle rect);
};

}  // namespace synera
