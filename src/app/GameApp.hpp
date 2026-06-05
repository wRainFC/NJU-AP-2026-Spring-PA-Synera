#pragma once

#include "core/GameState.hpp"
#include "config/CombatActionCatalog.hpp"
#include "systems/CombatSystem.hpp"
#include "systems/EquipmentSystem.hpp"
#include "systems/RoundSystem.hpp"
#include "systems/SaveSystem.hpp"
#include "systems/ShopSystem.hpp"
#include "systems/SynergySystem.hpp"
#include "systems/UpgradeSystem.hpp"
#include "ui/CombatAnimationController.hpp"
#include "ui/GameWindow.hpp"
#include "ui/InputController.hpp"
#include "ui/Layout.hpp"
#include "ui/Renderer.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace synera {

enum class GameOutcome { Playing, Victory, Defeat };

class GameApp {
public:
    GameApp();
    void run();

private:
    void init();
    void update(float dt);
    void render();
    void shutdown();
    void startNewRun();
    [[nodiscard]] bool applyInputCommands(std::span<const InputCommand> commands);
    void handleSave();
    [[nodiscard]] bool handleLoad();
    void setStatusMessage(std::string message);
    [[nodiscard]] bool interactionsEnabled() const noexcept;
    void refreshOutcomeFromState() noexcept;
    void finishRoundSettlement();
    [[nodiscard]] bool handleModalButton(ModalButtonId id);

    GameWindow window_;
    GameState state_;
    Layout layout_;
    CombatActionCatalog combatActions_;
    Renderer renderer_;
    InputController input_;
    RoundSystem roundSystem_;
    CombatSystem combatSystem_;
    ShopSystem shopSystem_;
    SynergySystem synergySystem_;
    UpgradeSystem upgradeSystem_;
    EquipmentSystem equipmentSystem_;
    SaveSystem saveSystem_;
    CombatAnimationController combatAnimations_;
    GameOutcome outcome_ = GameOutcome::Playing;
    std::optional<ModalModel> activeModal_;
    std::string statusMessage_;
    float statusMessageTimer_   = 0.0F;
    float animationTimeSeconds_ = 0.0F;
};

}  // namespace synera
