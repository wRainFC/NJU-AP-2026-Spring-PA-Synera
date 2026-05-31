#pragma once

#include "core/GameState.hpp"
#include "systems/CombatSystem.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/RoundSystem.hpp"
#include "systems/SaveSystem.hpp"
#include "systems/ShopSystem.hpp"
#include "systems/SynergySystem.hpp"
#include "systems/UpgradeSystem.hpp"
#include "ui/InputController.hpp"
#include "ui/Layout.hpp"
#include "ui/Renderer.hpp"

#include <string>

namespace synera {

class GameApp {
public:
    GameApp();
    void run();

private:
    void init();
    void update(float dt);
    void render();
    void shutdown();
    void handleSave();
    [[nodiscard]] bool handleLoad();
    void setStatusMessage(std::string message);

    GameState state_;
    Layout layout_;
    Renderer renderer_;
    InputController input_;
    RoundSystem roundSystem_;
    CombatSystem combatSystem_;
    ShopSystem shopSystem_;
    SynergySystem synergySystem_;
    UpgradeSystem upgradeSystem_;
    EquipmentSystem equipmentSystem_;
    SaveSystem saveSystem_;
    std::string statusMessage_;
    float statusMessageTimer_ = 0.0F;
    float resolveTimer_ = 0.0F;
};

}  // namespace synera
