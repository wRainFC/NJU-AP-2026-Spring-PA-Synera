#pragma once

#include "core/Types.hpp"
#include "raylib.h"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace synera {

enum class TextureSlot {
    PlayerBoardHex,
    EnemyBoardHex,
    Button,
    ButtonDisabled,
    TraitActive,
    TraitInactive,
    SellArea,
    Panel,
    Count,
};

struct SpriteAnimationView {
    const Texture2D* texture = nullptr;
    int frameCount = 1;
    float framesPerSecond = 8.0F;

    [[nodiscard]] bool loaded() const noexcept { return texture != nullptr; }
};

class TextureResource {
public:
    TextureResource() = default;
    explicit TextureResource(Texture2D texture) noexcept;
    ~TextureResource();

    TextureResource(const TextureResource&) = delete;
    TextureResource& operator=(const TextureResource&) = delete;
    TextureResource(TextureResource&& other) noexcept;
    TextureResource& operator=(TextureResource&& other) noexcept;

    [[nodiscard]] const Texture2D* get() const noexcept;
    [[nodiscard]] bool loaded() const noexcept;
    void reset() noexcept;

private:
    Texture2D texture_{};
};

// Owns a Raylib font loaded from optional UI assets.
class FontResource {
public:
    FontResource() = default;
    explicit FontResource(Font font) noexcept;
    ~FontResource();

    FontResource(const FontResource&) = delete;
    FontResource& operator=(const FontResource&) = delete;
    FontResource(FontResource&& other) noexcept;
    FontResource& operator=(FontResource&& other) noexcept;

    [[nodiscard]] const Font* get() const noexcept;
    [[nodiscard]] bool loaded() const noexcept;
    void reset() noexcept;

private:
    Font font_{};
};

class RenderAssets {
public:
    struct SpriteAnimation {
        TextureResource texture;
        int frameCount = 1;
        float framesPerSecond = 8.0F;
    };

    RenderAssets() = default;
    ~RenderAssets();

    RenderAssets(const RenderAssets&) = delete;
    RenderAssets& operator=(const RenderAssets&) = delete;
    RenderAssets(RenderAssets&&) noexcept = default;
    RenderAssets& operator=(RenderAssets&&) noexcept = default;

    void load(const std::filesystem::path& assetRoot);
    void unload() noexcept;

    [[nodiscard]] const Texture2D* texture(TextureSlot slot) const noexcept;
    [[nodiscard]] const Texture2D* unitTexture(std::string_view templateId, Owner owner) const;
    [[nodiscard]] SpriteAnimationView unitAnimation(std::string_view templateId, Owner owner,
                                                    UnitState state) const;
    [[nodiscard]] const Texture2D* equipmentTexture(EquipmentType equipment) const noexcept;
    // Returns nullptr when no UI font was loaded, letting drawing helpers use Raylib's default font.
    [[nodiscard]] const Font* font() const noexcept;

private:
    static constexpr std::size_t TextureSlotCount = static_cast<std::size_t>(TextureSlot::Count);
    static constexpr std::size_t EquipmentTextureCount =
        static_cast<std::size_t>(EquipmentType::ManaCrystal) + 1;

    std::array<std::optional<TextureResource>, TextureSlotCount> textures_;
    std::array<std::optional<TextureResource>, EquipmentTextureCount> equipmentTextures_;
    std::unordered_map<std::string, TextureResource> unitTextures_;
    std::unordered_map<std::string, SpriteAnimation> unitAnimations_;
    std::optional<TextureResource> playerDefaultUnitTexture_;
    std::optional<TextureResource> enemyDefaultUnitTexture_;
    std::optional<FontResource> font_;
};

}  // namespace synera
