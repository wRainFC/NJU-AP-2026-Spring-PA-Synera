#pragma once

#include "core/Types.hpp"
#include "raylib.h"

#include <cstddef>
#include <optional>
#include <string>

namespace synera {

enum class DragKind { None, UnitFromBench, UnitFromBoard, EquipmentFromPool };

struct DragState {
    DragKind kind = DragKind::None;
    UnitId unitId = InvalidUnitId;
    std::optional<int> sourceBenchSlot;
    std::optional<AxialPos> sourceBoardPos;
    std::optional<std::size_t> sourceEquipmentIndex;
};

struct InputResult {
    bool saveRequested = false;
    bool loadRequested = false;
    std::string statusMessage;
};

struct PointerInput {
    Vector2 position{};
    bool insideVirtualCanvas = false;
    bool leftPressed = false;
    bool leftReleased = false;
    bool leftDown = false;
};

}  // namespace synera
