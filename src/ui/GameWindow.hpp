#pragma once

#include "raylib.h"
#include "ui/UiState.hpp"

#include <string_view>

namespace synera {

class GameWindow {
public:
    GameWindow() = default;
    ~GameWindow();

    GameWindow(const GameWindow&)            = delete;
    GameWindow& operator=(const GameWindow&) = delete;

    void init(int virtualWidth, int virtualHeight, std::string_view title);
    void shutdown() noexcept;
    void setTargetFps(int fps) noexcept;
    [[nodiscard]] bool shouldClose() const noexcept;
    [[nodiscard]] float frameTime() const noexcept;
    void beginFrame(Color clearColor);
    void endFrame();
    [[nodiscard]] PointerInput pointerInput() const noexcept;
    [[nodiscard]] Rectangle viewport() const noexcept;

private:
    RenderTexture2D target_{};
    int virtualWidth_  = 0;
    int virtualHeight_ = 0;
    bool initialized_  = false;

    [[nodiscard]] float viewportScale() const noexcept;
};

}  // namespace synera
