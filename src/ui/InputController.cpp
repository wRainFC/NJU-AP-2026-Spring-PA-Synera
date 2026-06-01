#include "ui/InputController.hpp"

#include "app/GameConfig.hpp"
#include "core/GameState.hpp"
#include "ui/Layout.hpp"
#include "ui/UiDrawing.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace synera {

namespace {

template <class... Visitors>
struct Overloaded : Visitors... {
    using Visitors::operator()...;
};

template <class... Visitors>
Overloaded(Visitors...) -> Overloaded<Visitors...>;

struct UnitPressTarget {
    UnitId unitId = InvalidUnitId;
};

struct EquipmentPressTarget {
    std::size_t poolIndex = 0;
    EquipmentType equipment = EquipmentType::IronSword;
};

struct EmptyPressTarget {};

using PressTarget = std::variant<UnitPressTarget, EquipmentPressTarget, EmptyPressTarget>;

struct UnitDragPayload {
    UnitId unitId = InvalidUnitId;
};

struct EquipmentDragPayload {
    std::size_t poolIndex = 0;
    EquipmentType equipment = EquipmentType::IronSword;
};

using DragPayload = std::variant<UnitDragPayload, EquipmentDragPayload>;

struct SelectUnitClick {
    UnitId unitId = InvalidUnitId;
};

struct ClearSelectionClick {};
struct NoClickAction {};

using ClickAction = std::variant<SelectUnitClick, ClearSelectionClick, NoClickAction>;

[[nodiscard]] float distanceSquared(Vector2 left, Vector2 right) noexcept {
    const float dx = left.x - right.x;
    const float dy = left.y - right.y;
    return dx * dx + dy * dy;
}

[[nodiscard]] bool reachedDragThreshold(Vector2 pressPosition, Vector2 mouse) noexcept {
    const float threshold = config::DragStartThresholdVirtualPixels;
    return distanceSquared(pressPosition, mouse) >= threshold * threshold;
}

[[nodiscard]] bool isPlayerUnit(const GameState& state, UnitId unitId) {
    const Unit* unit = state.findUnit(unitId);
    return unit != nullptr && unit->owner == Owner::PlayerCtrl;
}

[[nodiscard]] std::optional<UnitId> playerUnitAt(const GameState& state, const Layout& layout,
                                                 Vector2 mouse) {
    const auto fromBoard = layout.boardPosAt(mouse).and_then([&](AxialPos pos) -> std::optional<UnitId> {
        return state.boardOccupant(pos).and_then([&](UnitId unitId) -> std::optional<UnitId> {
            return isPlayerUnit(state, unitId) ? std::optional<UnitId>{unitId} : std::nullopt;
        });
    });
    if (fromBoard) {
        return fromBoard;
    }
    return layout.benchSlotAt(mouse).and_then([&](int slot) -> std::optional<UnitId> {
        return state.benchOccupant(slot).and_then([&](UnitId unitId) -> std::optional<UnitId> {
            return isPlayerUnit(state, unitId) ? std::optional<UnitId>{unitId} : std::nullopt;
        });
    });
}

[[nodiscard]] PressTarget pressTargetAt(const GameState& state, const Layout& layout, Vector2 mouse) {
    const auto pool = state.equipmentPool();
    if (const auto equipmentIndex = layout.equipmentSlotAt(mouse, pool.size())) {
        return EquipmentPressTarget{
            .poolIndex = *equipmentIndex,
            .equipment = pool[*equipmentIndex],
        };
    }

    if (const auto unitId = playerUnitAt(state, layout, mouse)) {
        return UnitPressTarget{.unitId = *unitId};
    }

    return EmptyPressTarget{};
}

class PressController {
public:
    void begin(PressTarget target, Vector2 mouse) {
        session_ = PressSession{.target = std::move(target), .pressPosition = mouse};
    }

    [[nodiscard]] std::optional<DragPayload> updateHold(Vector2 mouse) const {
        if (!session_ || !reachedDragThreshold(session_->pressPosition, mouse)) {
            return std::nullopt;
        }

        return std::visit(Overloaded{
                              [](UnitPressTarget target) -> std::optional<DragPayload> {
                                  return DragPayload{UnitDragPayload{.unitId = target.unitId}};
                              },
                              [](EquipmentPressTarget target) -> std::optional<DragPayload> {
                                  return DragPayload{EquipmentDragPayload{
                                      .poolIndex = target.poolIndex,
                                      .equipment = target.equipment,
                                  }};
                              },
                              [](EmptyPressTarget) -> std::optional<DragPayload> { return std::nullopt; },
                          },
                          session_->target);
    }

    [[nodiscard]] std::optional<ClickAction> endClick() {
        if (!session_) {
            return std::nullopt;
        }

        ClickAction action = std::visit(Overloaded{
                                            [](UnitPressTarget target) -> ClickAction {
                                                return SelectUnitClick{.unitId = target.unitId};
                                            },
                                            [](EquipmentPressTarget) -> ClickAction { return NoClickAction{}; },
                                            [](EmptyPressTarget) -> ClickAction { return ClearSelectionClick{}; },
                                        },
                                        session_->target);
        session_.reset();
        return action;
    }

    void cancel() noexcept { session_.reset(); }

private:
    struct PressSession {
        PressTarget target;
        Vector2 pressPosition{};
    };

    std::optional<PressSession> session_;
};

class DragDropController {
public:
    void begin(DragPayload payload) { activeDrag_ = std::move(payload); }

    [[nodiscard]] bool active() const noexcept { return activeDrag_.has_value(); }

    [[nodiscard]] DragDropReadModel readModel() const {
        DragDropReadModel model;
        if (!activeDrag_) {
            return model;
        }

        std::visit(Overloaded{
                       [&](UnitDragPayload payload) {
                           model.ghost = UnitDragGhost{.unitId = payload.unitId};
                       },
                       [&](EquipmentDragPayload payload) {
                           model.ghost = EquipmentDragGhost{.equipment = payload.equipment};
                       },
                   },
                   *activeDrag_);
        return model;
    }

    [[nodiscard]] std::optional<InputCommand> drop(const GameState& state, const Layout& layout,
                                                   Vector2 mouse) {
        if (!activeDrag_) {
            return std::nullopt;
        }

        std::optional<InputCommand> command = std::visit(Overloaded{
            [&](UnitDragPayload payload) -> std::optional<InputCommand> {
                if (ui::contains(layout.sellAreaRect(), mouse)) {
                    return InputCommand{SellUnit{.unitId = payload.unitId}};
                }
                if (const auto pos = layout.boardPosAt(mouse)) {
                    return InputCommand{PlaceUnitOnBoard{.unitId = payload.unitId, .pos = *pos}};
                }
                if (const auto slot = layout.benchSlotAt(mouse)) {
                    return InputCommand{PlaceUnitOnBench{.unitId = payload.unitId, .slot = *slot}};
                }
                return std::nullopt;
            },
            [&](EquipmentDragPayload payload) -> std::optional<InputCommand> {
                return playerUnitAt(state, layout, mouse)
                    .transform([&](UnitId unitId) {
                        return InputCommand{EquipFromPool{.poolIndex = payload.poolIndex, .unitId = unitId}};
                    });
            },
        }, *activeDrag_);
        activeDrag_.reset();
        return command;
    }

    void cancel() noexcept { activeDrag_.reset(); }

private:
    std::optional<DragPayload> activeDrag_;
};

void appendIf(InputFrameResult& result, std::optional<InputCommand> command) {
    if (!command) {
        return;
    }
    result.commands.push_back(*command);
}

}  // namespace

class InputController::Impl {
public:
    [[nodiscard]] InputFrameResult update(const GameState& state, const Layout& layout,
                                          const PointerInput& pointer, bool interactionsEnabled) {
        InputFrameResult result;
        const Vector2 mouse = pointer.position;

        if (pointer.leftPressed && ui::contains(layout.loadButtonRect(), mouse)) {
            clearInteraction();
            result.commands.push_back(RequestLoad{});
            return result;
        }

        if (!interactionsEnabled) {
            clearInteraction();
            selectedUnitId_.reset();
            if (pointer.leftPressed && ui::contains(layout.outcomeRestartButtonRect(), mouse)) {
                result.commands.push_back(RequestRestart{});
                return result;
            }
            if (pointer.leftPressed && ui::contains(layout.outcomeLoadButtonRect(), mouse)) {
                result.commands.push_back(RequestLoad{});
                return result;
            }
            return result;
        }

        if (state.phase() != Phase::Prep) {
            clearInteraction();
            selectedUnitId_.reset();
            return result;
        }

        if (pointer.leftPressed) {
            if (handleImmediateCommand(state, layout, mouse, result)) {
                clearInteraction();
                return result;
            }
            press_.begin(pressTargetAt(state, layout, mouse), mouse);
        }

        if (pointer.leftDown) {
            if (std::optional<DragPayload> dragStart = press_.updateHold(mouse)) {
                selectDraggedUnit(*dragStart);
                dragDrop_.begin(*dragStart);
                press_.cancel();
            }
        }

        if (pointer.leftReleased) {
            if (dragDrop_.active()) {
                appendIf(result, dragDrop_.drop(state, layout, mouse));
            } else if (std::optional<ClickAction> click = press_.endClick()) {
                applyClick(*click);
            }
        }

        return result;
    }

    [[nodiscard]] InputReadModel readModel(const GameState& state) const {
        InputReadModel model{
            .selectedUnitId = std::nullopt,
            .dragDrop = dragDrop_.readModel(),
        };
        if (!selectedUnitId_) {
            return model;
        }

        const Unit* unit = state.findUnit(*selectedUnitId_);
        if (unit != nullptr && unit->owner == Owner::PlayerCtrl) {
            model.selectedUnitId = selectedUnitId_;
        }
        return model;
    }

    void clearInteraction() noexcept {
        press_.cancel();
        dragDrop_.cancel();
    }

    void clearSelection() noexcept { selectedUnitId_.reset(); }

    void selectUnit(UnitId unitId) noexcept { selectedUnitId_ = unitId; }

private:
    PressController press_;
    DragDropController dragDrop_;
    std::optional<UnitId> selectedUnitId_;

    [[nodiscard]] bool handleImmediateCommand(const GameState& state, const Layout& layout, Vector2 mouse,
                                              InputFrameResult& result) {
        if (ui::contains(layout.startButtonRect(), mouse)) {
            result.commands.push_back(StartCombat{});
            return true;
        }
        if (ui::contains(layout.saveButtonRect(), mouse)) {
            result.commands.push_back(RequestSave{});
            return true;
        }
        if (ui::contains(layout.populationUpgradeButtonRect(), mouse)) {
            result.commands.push_back(UpgradePopulation{});
            return true;
        }
        if (ui::contains(layout.shopRefreshButtonRect(), mouse)) {
            result.commands.push_back(RefreshShop{});
            return true;
        }
        if (ui::contains(layout.shopLockButtonRect(), mouse)) {
            result.commands.push_back(ToggleShopLock{});
            return true;
        }
        if (const auto offer = layout.shopOfferAt(mouse)) {
            (void)state;
            result.commands.push_back(BuyOffer{.offerIndex = *offer});
            return true;
        }
        return false;
    }

    void applyClick(ClickAction action) {
        std::visit(Overloaded{
                       [&](SelectUnitClick click) {
                           if (selectedUnitId_ == click.unitId) {
                               selectedUnitId_.reset();
                               return;
                           }
                           selectedUnitId_ = click.unitId;
                       },
                       [&](ClearSelectionClick) { selectedUnitId_.reset(); },
                       [](NoClickAction) {},
                   },
                   action);
    }

    void selectDraggedUnit(const DragPayload& payload) {
        if (const auto* unit = std::get_if<UnitDragPayload>(&payload)) {
            selectedUnitId_ = unit->unitId;
        }
    }
};

InputController::InputController() : impl_(std::make_unique<Impl>()) {}

InputController::~InputController() = default;

InputController::InputController(InputController&&) noexcept = default;

InputController& InputController::operator=(InputController&&) noexcept = default;

InputFrameResult InputController::update(const GameState& state, const Layout& layout,
                                         const PointerInput& pointer, bool interactionsEnabled) {
    return impl_->update(state, layout, pointer, interactionsEnabled);
}

InputReadModel InputController::readModel(const GameState& state) const {
    return impl_->readModel(state);
}

void InputController::clearInteraction() noexcept {
    impl_->clearInteraction();
}

void InputController::clearSelection() noexcept {
    impl_->clearSelection();
}

void InputController::selectUnit(UnitId unitId) noexcept {
    impl_->selectUnit(unitId);
}

}  // namespace synera
