#include "ui/GameWindow.hpp"

#include "ui/UiTheme.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace synera {

GameWindow::~GameWindow() {
    shutdown();
}

void GameWindow::init(int virtualWidth, int virtualHeight, std::string_view title) {
    virtualWidth_ = virtualWidth;
    virtualHeight_ = virtualHeight;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    const std::string windowTitle{title};
    InitWindow(virtualWidth_, virtualHeight_, windowTitle.c_str());
    target_ = LoadRenderTexture(virtualWidth_, virtualHeight_);
    SetTextureFilter(target_.texture, TEXTURE_FILTER_BILINEAR);
    initialized_ = true;
}

void GameWindow::shutdown() noexcept {
    if (target_.id != 0 && IsWindowReady()) {
        UnloadRenderTexture(target_);
    }
    target_ = RenderTexture2D{};

    if (initialized_ && IsWindowReady()) {
        CloseWindow();
    }
    initialized_ = false;
}

void GameWindow::setTargetFps(int fps) noexcept {
    SetTargetFPS(fps);
}

bool GameWindow::shouldClose() const noexcept {
    return WindowShouldClose();
}

float GameWindow::frameTime() const noexcept {
    return GetFrameTime();
}

void GameWindow::beginFrame(Color clearColor) {
    BeginTextureMode(target_);
    ClearBackground(clearColor);
}

void GameWindow::endFrame() {
    EndTextureMode();

    BeginDrawing();
    ClearBackground(ui::theme::Letterbox);
    const Rectangle destination = viewport();
    const Rectangle source{0.0F, 0.0F, static_cast<float>(virtualWidth_),
                           -static_cast<float>(virtualHeight_)};
    DrawTexturePro(target_.texture, source, destination, Vector2{}, 0.0F, WHITE);
    EndDrawing();
}

PointerInput GameWindow::pointerInput() const noexcept {
    const Rectangle view = viewport();
    const float scaleX = view.width / static_cast<float>(virtualWidth_);
    const float scaleY = view.height / static_cast<float>(virtualHeight_);
    const Vector2 mouse = GetMousePosition();
    const Vector2 virtualPosition{
        (mouse.x - view.x) / scaleX,
        (mouse.y - view.y) / scaleY,
    };

    return PointerInput{
        .position = virtualPosition,
        .insideVirtualCanvas = virtualPosition.x >= 0.0F &&
                               virtualPosition.x <= static_cast<float>(virtualWidth_) &&
                               virtualPosition.y >= 0.0F &&
                               virtualPosition.y <= static_cast<float>(virtualHeight_),
        .leftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        .leftReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT),
        .leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT),
    };
}

Rectangle GameWindow::viewport() const noexcept {
    const float scale = viewportScale();
    const float width = std::max(1.0F, std::round(static_cast<float>(virtualWidth_) * scale));
    const float height = std::max(1.0F, std::round(static_cast<float>(virtualHeight_) * scale));
    const float screenWidth = static_cast<float>(std::max(1, GetScreenWidth()));
    const float screenHeight = static_cast<float>(std::max(1, GetScreenHeight()));
    return Rectangle{
        std::round((screenWidth - width) / 2.0F),
        std::round((screenHeight - height) / 2.0F),
        width,
        height,
    };
}

float GameWindow::viewportScale() const noexcept {
    if (virtualWidth_ <= 0 || virtualHeight_ <= 0) {
        return 1.0F;
    }

    const float widthScale = static_cast<float>(std::max(1, GetScreenWidth())) /
                             static_cast<float>(virtualWidth_);
    const float heightScale = static_cast<float>(std::max(1, GetScreenHeight())) /
                              static_cast<float>(virtualHeight_);
    return std::min(widthScale, heightScale);
}

}  // namespace synera
