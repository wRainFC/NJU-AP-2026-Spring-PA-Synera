#pragma once

#include "ui/UiState.hpp"

#include <filesystem>
#include <memory>
#include <string_view>

namespace synera {

class GameState;
class Layout;

// Per-frame inputs required by the renderer. The context borrows state only and
// keeps rendering concerns separate from the persistent GameState model.
struct RenderContext {
    const GameState& state;
    const Layout& layout;
    const DragState& dragState;
    PointerInput pointer;
    std::string_view statusMessage;
    std::string_view outcomeMessage;
    float animationTimeSeconds = 0.0F;
    bool interactionsEnabled = true;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept;
    Renderer& operator=(Renderer&&) noexcept;

    void loadAssets(const std::filesystem::path& root = "assets");
    void unloadAssets() noexcept;
    void draw(const RenderContext& context);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace synera
