#pragma once

#include "core/Types.hpp"
#include "raylib.h"

#include <cstddef>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace synera {

struct UnitDragGhost {
    UnitId unitId = InvalidUnitId;
};

struct EquipmentDragGhost {
    EquipmentType equipment = EquipmentType::IronSword;
};

using DragGhost = std::variant<std::monostate, UnitDragGhost, EquipmentDragGhost>;

struct DragDropReadModel {
    DragGhost ghost;
};

struct InputReadModel {
    std::optional<UnitId> selectedUnitId;
    DragDropReadModel dragDrop;
};

enum class ModalButtonId { ContinueResolve, NewRun, LoadSave };

struct ModalButton {
    ModalButtonId id = ModalButtonId::ContinueResolve;
    std::string label;
};

struct ModalModel {
    std::string title;
    std::vector<std::string> lines;
    std::vector<ModalButton> buttons;
    Color accent = GOLD;
};

struct RequestSave {};
struct RequestLoad {};
struct StartCombat {};
struct RefreshShop {};
struct ToggleShopLock {};
struct UpgradePopulation {};

struct SubmitModalButton {
    ModalButtonId id = ModalButtonId::ContinueResolve;
};

struct BuyOffer {
    int offerIndex = -1;
};

struct PlaceUnitOnBoard {
    UnitId unitId = InvalidUnitId;
    AxialPos pos{};
};

struct PlaceUnitOnBench {
    UnitId unitId = InvalidUnitId;
    int slot      = -1;
};

struct SellUnit {
    UnitId unitId = InvalidUnitId;
};

struct EquipFromPool {
    std::size_t poolIndex = 0;
    UnitId unitId         = InvalidUnitId;
};

using InputCommand =
    std::variant<RequestSave, RequestLoad, StartCombat, RefreshShop, ToggleShopLock, UpgradePopulation,
                 SubmitModalButton, BuyOffer, PlaceUnitOnBoard, PlaceUnitOnBench, SellUnit, EquipFromPool>;

struct InputFrameResult {
    std::vector<InputCommand> commands;
};

struct PointerInput {
    Vector2 position{};
    bool insideVirtualCanvas = false;
    bool leftPressed         = false;
    bool leftReleased        = false;
    bool leftDown            = false;
};

}  // namespace synera
