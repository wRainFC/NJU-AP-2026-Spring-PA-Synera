#pragma once

#include "core/Types.hpp"
#include "raylib.h"

#include <string_view>

namespace synera {

class RenderAssets;
class Unit;

class UnitItem {
public:
    static void drawUnit(const RenderAssets& assets, const Unit& unit, Rectangle rect,
                         float animationTimeSeconds);
    static void drawUnit(const RenderAssets& assets, const Unit& unit, Rectangle rect,
                         UnitState visualState, float animationTimeSeconds, Vector2 offset);
    static void drawUnit(const RenderAssets& assets, const Unit& unit, Rectangle rect,
                         UnitState visualState, std::string_view clipId, float animationTimeSeconds,
                         Vector2 offset);
    static void drawEquipmentIcon(const RenderAssets& assets, EquipmentType equipment, Rectangle rect);
};

}  // namespace synera
