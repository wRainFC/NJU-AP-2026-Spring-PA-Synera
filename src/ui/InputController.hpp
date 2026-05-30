#pragma once

#include "core/Types.hpp"

#include <optional>

namespace synera {

class GameState;
class Layout;
class RoundSystem;
class ShopSystem;

enum class DragKind { None, UnitFromBench, UnitFromBoard };

struct DragState {
    DragKind kind = DragKind::None;
    UnitId unitId = InvalidUnitId;
    std::optional<int> sourceBenchSlot;
    std::optional<AxialPos> sourceBoardPos;
};

class InputController {
public:
    void update(GameState& state, const Layout& layout, RoundSystem& roundSystem, ShopSystem& shopSystem);

private:
    DragState drag_;

    void beginDrag(GameState& state, const Layout& layout);
    void endDrag(GameState& state, const Layout& layout);
    [[nodiscard]] bool handleShopClick(GameState& state, const Layout& layout, ShopSystem& shopSystem);
};

}  // namespace synera
