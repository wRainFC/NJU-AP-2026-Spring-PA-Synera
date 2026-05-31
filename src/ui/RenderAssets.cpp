#include "ui/RenderAssets.hpp"

#include <array>
#include <cstddef>
#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace synera {

namespace {

struct SlotTexturePath {
    TextureSlot slot;
    std::string_view path;
};

struct EquipmentTexturePath {
    EquipmentType equipment;
    std::string_view path;
};

struct StateTexturePath {
    UnitState state;
    std::string_view name;
};

inline constexpr std::array SlotTexturePaths{
    SlotTexturePath{TextureSlot::PlayerBoardHex, "board/player_hex.png"},
    SlotTexturePath{TextureSlot::EnemyBoardHex, "board/enemy_hex.png"},
    SlotTexturePath{TextureSlot::Button, "ui/button.png"},
    SlotTexturePath{TextureSlot::ButtonDisabled, "ui/button_disabled.png"},
    SlotTexturePath{TextureSlot::TraitActive, "ui/trait_active.png"},
    SlotTexturePath{TextureSlot::TraitInactive, "ui/trait_inactive.png"},
    SlotTexturePath{TextureSlot::SellArea, "ui/sell_area.png"},
    SlotTexturePath{TextureSlot::Panel, "ui/panel.png"},
};

inline constexpr std::array EquipmentTexturePaths{
    EquipmentTexturePath{EquipmentType::IronSword, "equipment/iron_sword.png"},
    EquipmentTexturePath{EquipmentType::ChainVest, "equipment/chain_vest.png"},
    EquipmentTexturePath{EquipmentType::SwiftGlove, "equipment/swift_glove.png"},
    EquipmentTexturePath{EquipmentType::ManaCrystal, "equipment/mana_crystal.png"},
};

inline constexpr std::array StateTexturePaths{
    StateTexturePath{UnitState::Idle, "idle"},
    StateTexturePath{UnitState::Moving, "moving"},
    StateTexturePath{UnitState::Attacking, "attacking"},
    StateTexturePath{UnitState::Casting, "casting"},
    StateTexturePath{UnitState::Stunned, "stunned"},
    StateTexturePath{UnitState::Dead, "dead"},
};

[[nodiscard]] std::size_t slotIndex(TextureSlot slot) noexcept {
    return static_cast<std::size_t>(slot);
}

[[nodiscard]] std::size_t equipmentIndex(EquipmentType equipment) noexcept {
    return static_cast<std::size_t>(equipment);
}

[[nodiscard]] std::string_view ownerName(Owner owner) noexcept {
    return owner == Owner::PlayerCtrl ? "player" : "enemy";
}

[[nodiscard]] std::string_view stateName(UnitState state) noexcept {
    const auto iter = std::ranges::find(StateTexturePaths, state, &StateTexturePath::state);
    return iter == StateTexturePaths.end() ? "idle" : iter->name;
}

[[nodiscard]] std::string animationKey(std::string_view templateId, Owner owner, UnitState state) {
    std::string key{templateId};
    key += '/';
    key += ownerName(owner);
    key += '_';
    key += stateName(state);
    return key;
}

[[nodiscard]] std::optional<TextureResource> loadTextureIfPresent(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) {
        return std::nullopt;
    }

    const Texture2D texture = LoadTexture(path.string().c_str());
    if (texture.id == 0) {
        return std::nullopt;
    }
    return std::optional<TextureResource>{std::in_place, texture};
}

[[nodiscard]] std::optional<RenderAssets::SpriteAnimation> loadAnimationIfPresent(
    const std::filesystem::path& path) {
    auto texture = loadTextureIfPresent(path);
    if (!texture) {
        return std::nullopt;
    }

    const Texture2D* loaded = texture->get();
    const int frameCount = loaded == nullptr || loaded->height <= 0
                               ? 1
                               : std::max(1, loaded->width / loaded->height);
    return RenderAssets::SpriteAnimation{
        .texture = std::move(*texture),
        .frameCount = frameCount,
        .framesPerSecond = 8.0F,
    };
}

}  // namespace

TextureResource::TextureResource(Texture2D texture) noexcept : texture_(texture) {}

TextureResource::~TextureResource() {
    reset();
}

TextureResource::TextureResource(TextureResource&& other) noexcept
    : texture_(std::exchange(other.texture_, Texture2D{})) {}

TextureResource& TextureResource::operator=(TextureResource&& other) noexcept {
    if (this != &other) {
        reset();
        texture_ = std::exchange(other.texture_, Texture2D{});
    }
    return *this;
}

const Texture2D* TextureResource::get() const noexcept {
    return loaded() ? &texture_ : nullptr;
}

bool TextureResource::loaded() const noexcept {
    return texture_.id != 0;
}

void TextureResource::reset() noexcept {
    if (loaded() && IsWindowReady()) {
        UnloadTexture(texture_);
    }
    texture_ = Texture2D{};
}

RenderAssets::~RenderAssets() {
    unload();
}

void RenderAssets::load(const std::filesystem::path& root) {
    unload();

    for (const SlotTexturePath& entry : SlotTexturePaths) {
        textures_[slotIndex(entry.slot)] = loadTextureIfPresent(root / entry.path);
    }

    std::error_code error;
    const std::filesystem::path unitsRoot = root / "units";
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator{unitsRoot, error}) {
        if (entry.is_directory(error)) {
            const std::string unitId = entry.path().filename().string();
            for (Owner owner : {Owner::PlayerCtrl, Owner::EnemyCtrl}) {
                for (const StateTexturePath& state : StateTexturePaths) {
                    const std::string filename = std::string{ownerName(owner)} + "_" +
                                                 std::string{state.name} + ".png";
                    auto animation = loadAnimationIfPresent(entry.path() / filename);
                    if (animation) {
                        unitAnimations_.emplace(animationKey(unitId, owner, state.state),
                                                std::move(*animation));
                    }
                }
            }
            continue;
        }

        if (!entry.is_regular_file(error) || entry.path().extension() != ".png") {
            continue;
        }

        const std::string unitId = entry.path().stem().string();
        if (unitId == "player_default" || unitId == "enemy_default") {
            continue;
        }
        auto texture = loadTextureIfPresent(entry.path());
        if (texture) {
            unitTextures_.emplace(unitId, std::move(*texture));
        }
    }

    playerDefaultUnitTexture_ = loadTextureIfPresent(root / "units/player_default.png");
    enemyDefaultUnitTexture_ = loadTextureIfPresent(root / "units/enemy_default.png");

    for (const EquipmentTexturePath& entry : EquipmentTexturePaths) {
        equipmentTextures_[equipmentIndex(entry.equipment)] = loadTextureIfPresent(root / entry.path);
    }
}

void RenderAssets::unload() noexcept {
    for (auto& texture : textures_) {
        texture.reset();
    }
    for (auto& texture : equipmentTextures_) {
        texture.reset();
    }
    unitTextures_.clear();
    unitAnimations_.clear();
    playerDefaultUnitTexture_.reset();
    enemyDefaultUnitTexture_.reset();
}

const Texture2D* RenderAssets::texture(TextureSlot slot) const noexcept {
    const std::size_t index = slotIndex(slot);
    if (index >= textures_.size() || !textures_[index]) {
        return nullptr;
    }
    return textures_[index]->get();
}

const Texture2D* RenderAssets::unitTexture(std::string_view templateId, Owner owner) const {
    if (const auto iter = unitTextures_.find(std::string{templateId}); iter != unitTextures_.end()) {
        return iter->second.get();
    }
    const auto& fallback = owner == Owner::PlayerCtrl ? playerDefaultUnitTexture_ : enemyDefaultUnitTexture_;
    return fallback ? fallback->get() : nullptr;
}

SpriteAnimationView RenderAssets::unitAnimation(std::string_view templateId, Owner owner,
                                                UnitState state) const {
    const auto findAnimation = [&](UnitState requested) -> const SpriteAnimation* {
        const auto iter = unitAnimations_.find(animationKey(templateId, owner, requested));
        return iter == unitAnimations_.end() ? nullptr : &iter->second;
    };

    const SpriteAnimation* animation = findAnimation(state);
    if (animation == nullptr && state != UnitState::Idle) {
        animation = findAnimation(UnitState::Idle);
    }
    if (animation == nullptr) {
        return {};
    }
    return SpriteAnimationView{
        .texture = animation->texture.get(),
        .frameCount = animation->frameCount,
        .framesPerSecond = animation->framesPerSecond,
    };
}

const Texture2D* RenderAssets::equipmentTexture(EquipmentType equipment) const noexcept {
    const std::size_t index = equipmentIndex(equipment);
    if (index >= equipmentTextures_.size() || !equipmentTextures_[index]) {
        return nullptr;
    }
    return equipmentTextures_[index]->get();
}

}  // namespace synera
