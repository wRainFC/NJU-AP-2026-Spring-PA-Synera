#pragma once

#include "ui/UiState.hpp"

#include <memory>

namespace synera {

class GameState;
class Layout;

class InputController {
public:
    InputController();
    ~InputController();

    InputController(const InputController&)            = delete;
    InputController& operator=(const InputController&) = delete;
    InputController(InputController&&) noexcept;
    InputController& operator=(InputController&&) noexcept;

    [[nodiscard]] InputFrameResult update(const GameState& state, const Layout& layout,
                                          const PointerInput& pointer, bool interactionsEnabled);
    [[nodiscard]] InputReadModel readModel(const GameState& state) const;
    void clearInteraction() noexcept;
    void clearSelection() noexcept;
    void selectUnit(UnitId unitId) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace synera
