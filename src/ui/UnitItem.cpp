#include "ui/UnitItem.hpp"

#include "core/Metadata.hpp"
#include "core/Unit.hpp"
#include "ui/RenderAssets.hpp"
#include "ui/UiDrawing.hpp"
#include "ui/UiTheme.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace synera {

namespace {

[[nodiscard]] Rectangle spriteRect(Rectangle rect) noexcept {
    const float horizontalPadding = rect.width * 0.10F;
    const float top               = rect.y + rect.height * 0.22F;
    const float height            = std::max(28.0F, rect.height * 0.50F);
    return Rectangle{rect.x + horizontalPadding, top, rect.width - horizontalPadding * 2.0F, height};
}

void drawFallbackBody(const Unit& unit, Rectangle rect) {
    const Color body = unit.owner == Owner::PlayerCtrl ? ui::theme::UnitPlayer : ui::theme::UnitEnemy;
    DrawCircle(static_cast<int>(rect.x + rect.width / 2.0F), static_cast<int>(rect.y + rect.height / 2.0F),
               rect.width * 0.32F, body);
}

void drawUnitSprite(const RenderAssets& assets, const Unit& unit, Rectangle rect, UnitState visualState,
                    std::string_view clipId, float animationTimeSeconds) {
    const Rectangle destination = spriteRect(rect);
    SpriteAnimationView animation = assets.spriteAnimation(clipId);
    if (!animation.loaded()) {
        animation = assets.unitAnimation(unit.templateId, unit.owner, visualState);
    }
    if (animation.loaded()) {
        const int frame =
            animation.frameCount <= 1
                ? 0
                : static_cast<int>(std::floor(animationTimeSeconds * animation.framesPerSecond)) %
                      animation.frameCount;
        const float frameWidth =
            static_cast<float>(animation.texture->width) / static_cast<float>(animation.frameCount);
        const Rectangle source{frameWidth * static_cast<float>(frame), 0.0F, frameWidth,
                               static_cast<float>(animation.texture->height)};
        ui::drawTextureFrameToRect(*animation.texture, source, destination);
        return;
    }

    if (!ui::drawTextureToRect(assets.unitTexture(unit.templateId, unit.owner), destination)) {
        drawFallbackBody(unit, rect);
    }
}

}  // namespace

void UnitItem::drawUnit(const RenderAssets& assets, const Unit& unit, Rectangle rect,
                        float animationTimeSeconds) {
    drawUnit(assets, unit, rect, unit.runtime.state, animationTimeSeconds, Vector2{});
}

void UnitItem::drawUnit(const RenderAssets& assets, const Unit& unit, Rectangle rect, UnitState visualState,
                        float animationTimeSeconds, Vector2 offset) {
    drawUnit(assets, unit, rect, visualState, std::string_view{}, animationTimeSeconds, offset);
}

void UnitItem::drawUnit(const RenderAssets& assets, const Unit& unit, Rectangle rect, UnitState visualState,
                        std::string_view clipId, float animationTimeSeconds, Vector2 offset) {
    rect.x += offset.x;
    rect.y += offset.y;
    drawUnitSprite(assets, unit, rect, visualState, clipId, animationTimeSeconds);

    const float padding = rect.width * 0.10F;
    ui::drawBar(Rectangle{rect.x + padding, rect.y + rect.height * 0.07F, rect.width - padding * 2.0F,
                          std::max(5.0F, rect.height * 0.075F)},
                ui::clampedRatio(unit.runtime.hp, unit.derivedStats.maxHp), GREEN, ui::theme::BarTrack,
                BLACK);
    ui::drawBar(Rectangle{rect.x + padding, rect.y + rect.height * 0.15F, rect.width - padding * 2.0F,
                          std::max(4.0F, rect.height * 0.055F)},
                ui::clampedRatio(unit.runtime.mana, unit.derivedStats.maxMana), BLUE, ui::theme::BarTrack,
                BLACK);

    const std::string label = unit.name + " *" + std::to_string(unit.star);
    ui::drawTextInRect(label,
                       Rectangle{rect.x + rect.width * 0.05F, rect.y + rect.height * 0.61F,
                                 rect.width * 0.90F, rect.height * 0.16F},
                       12, RAYWHITE, ui::HorizontalAlign::Left, ui::VerticalAlign::Top);

    if (!unit.equipment) {
        return;
    }

    const std::string name{equipmentName(*unit.equipment)};
    if (const Texture2D* texture = assets.equipmentTexture(*unit.equipment); texture != nullptr) {
        const float iconSize = std::max(14.0F, rect.width * 0.20F);
        const Rectangle icon{rect.x + rect.width * 0.06F, rect.y + rect.height * 0.79F, iconSize, iconSize};
        (void)ui::drawTextureToRect(texture, icon);
        ui::drawTextInRect(name,
                           Rectangle{icon.x + icon.width + 4.0F, rect.y + rect.height * 0.80F,
                                     rect.x + rect.width - icon.x - icon.width - 6.0F, rect.height * 0.14F},
                           12, GOLD, ui::HorizontalAlign::Left, ui::VerticalAlign::Top);
        return;
    }

    ui::drawTextInRect(name,
                       Rectangle{rect.x + rect.width * 0.06F, rect.y + rect.height * 0.80F,
                                 rect.width * 0.88F, rect.height * 0.14F},
                       12, GOLD, ui::HorizontalAlign::Left, ui::VerticalAlign::Top);
}

void UnitItem::drawEquipmentIcon(const RenderAssets& assets, EquipmentType equipment, Rectangle rect) {
    DrawRectangleRec(rect, ui::theme::Surface);
    if (const Texture2D* texture = assets.equipmentTexture(equipment); texture != nullptr) {
        (void)ui::drawTextureToRect(texture, ui::inset(rect, rect.width * 0.12F));
    } else {
        const std::string name{equipmentName(equipment)};
        ui::drawTextInRect(name, ui::inset(rect, rect.width * 0.08F), 12, RAYWHITE,
                           ui::HorizontalAlign::Center, ui::VerticalAlign::Middle);
    }
    DrawRectangleLinesEx(rect, 1.0F, ui::theme::SurfaceBorder);
}

}  // namespace synera
