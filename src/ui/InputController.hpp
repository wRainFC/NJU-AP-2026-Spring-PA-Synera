#pragma once

#include "core/Types.hpp"

#include <cstddef>
#include <optional>

namespace synera {

class GameState;
class Layout;
class EquipmentSystem;
class RoundSystem;
class ShopSystem;
class SynergySystem;
class UpgradeSystem;

enum class DragKind { None, UnitFromBench, UnitFromBoard, EquipmentFromPool };

struct DragState {
    DragKind kind = DragKind::None;
    UnitId unitId = InvalidUnitId;
    std::optional<int> sourceBenchSlot;
    std::optional<AxialPos> sourceBoardPos;
    std::optional<std::size_t> sourceEquipmentIndex;
};

class InputController {
public:
    void update(GameState& state, const Layout& layout, RoundSystem& roundSystem, ShopSystem& shopSystem,
                UpgradeSystem& upgradeSystem, SynergySystem& synergySystem, EquipmentSystem& equipmentSystem);

private:
    DragState drag_;

    void beginDrag(GameState& state, const Layout& layout);
    void endDrag(GameState& state, const Layout& layout, SynergySystem& synergySystem,
                 EquipmentSystem& equipmentSystem);
    [[nodiscard]] bool handlePrepClick(GameState& state, const Layout& layout, ShopSystem& shopSystem,
                                       UpgradeSystem& upgradeSystem, SynergySystem& synergySystem);
};

}  // namespace synera
