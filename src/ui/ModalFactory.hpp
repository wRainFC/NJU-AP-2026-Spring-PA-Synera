#pragma once

#include "systems/EquipmentSystem.hpp"
#include "systems/RoundSystem.hpp"
#include "ui/UiState.hpp"

#include "raylib.h"

#include <string_view>

namespace synera {

[[nodiscard]] ModalModel roundSettlementModal(const RoundResult& result, const EquipmentDropResult& drop);
[[nodiscard]] ModalModel terminalOutcomeModal(std::string_view title, Color accent);

}  // namespace synera
