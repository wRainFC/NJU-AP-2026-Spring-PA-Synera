#pragma once

#include "ui/UiState.hpp"

namespace synera {

class GameState;
class Layout;
class EquipmentSystem;
class RoundSystem;
class ShopSystem;
class SynergySystem;
class UpgradeSystem;

class InputController {
public:
    [[nodiscard]] InputResult update(GameState& state, const Layout& layout, RoundSystem& roundSystem,
                                     ShopSystem& shopSystem, UpgradeSystem& upgradeSystem,
                                     SynergySystem& synergySystem, EquipmentSystem& equipmentSystem,
                                     const PointerInput& pointer, bool interactionsEnabled);
    [[nodiscard]] const DragState& dragState() const noexcept;

private:
    DragState drag_;

    void beginDrag(GameState& state, const Layout& layout, Vector2 mouse);
    void endDrag(GameState& state, const Layout& layout, ShopSystem& shopSystem, SynergySystem& synergySystem,
                 EquipmentSystem& equipmentSystem, Vector2 mouse, InputResult& result);
    [[nodiscard]] bool handlePrepClick(GameState& state, const Layout& layout, ShopSystem& shopSystem,
                                       UpgradeSystem& upgradeSystem, SynergySystem& synergySystem,
                                       Vector2 mouse);
};

}  // namespace synera
