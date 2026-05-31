#include "ui/UnitItem.hpp"

#include "core/Metadata.hpp"
#include "core/Unit.hpp"
#include "ui/RenderAssets.hpp"
#include "ui/UiDrawing.hpp"
#include "ui/UiTheme.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace synera {

namespace {

[[nodiscard]] Rectangle spriteRect(Rectangle rect) noexcept {
    return Rectangle{rect.x + 6.0F, rect.y + 13.0F, rect.width - 12.0F,
                     std::max(24.0F, rect.height - 32.0F)};
}

void drawFallbackBody(const Unit& unit, Rectangle rect) {
    const Color body = unit.owner == Owner::PlayerCtrl ? ui::theme::UnitPlayer : ui::theme::UnitEnemy;
    DrawCircle(static_cast<int>(rect.x + rect.width / 2.0F),
               static_cast<int>(rect.y + rect.height / 2.0F), rect.width * 0.32F, body);
}

void drawUnitSprite(const RenderAssets& assets, const Unit& unit, Rectangle rect,
                    float animationTimeSeconds) {
    const Rectangle destination = spriteRect(rect);
    const SpriteAnimationView animation = assets.unitAnimation(unit.templateId, unit.owner, unit.runtime.state);
    if (animation.loaded()) {
        const int frame = animation.frameCount <= 1
                              ? 0
                              : static_cast<int>(std::floor(animationTimeSeconds * animation.framesPerSecond)) %
                                    animation.frameCount;
        const float frameWidth = static_cast<float>(animation.texture->width) /
                                 static_cast<float>(animation.frameCount);
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
    drawUnitSprite(assets, unit, rect, animationTimeSeconds);

    ui::drawBar(Rectangle{rect.x + 6.0F, rect.y + 4.0F, rect.width - 12.0F, 5.0F},
                ui::clampedRatio(unit.runtime.hp, unit.derivedStats.maxHp), GREEN,
                ui::theme::BarTrack, BLACK);
    ui::drawBar(Rectangle{rect.x + 6.0F, rect.y + 10.0F, rect.width - 12.0F, 4.0F},
                ui::clampedRatio(unit.runtime.mana, unit.derivedStats.maxMana), BLUE,
                ui::theme::BarTrack, BLACK);

    const std::string label = unit.name + " *" + std::to_string(unit.star);
    ui::drawTextInRect(label, Rectangle{rect.x + 3.0F, rect.y + 37.0F, rect.width - 6.0F, 11.0F},
                       9, RAYWHITE, ui::HorizontalAlign::Left, ui::VerticalAlign::Top);

    if (!unit.equipment) {
        return;
    }

    const std::string name{equipmentName(*unit.equipment)};
    if (const Texture2D* texture = assets.equipmentTexture(*unit.equipment); texture != nullptr) {
        const Rectangle icon{rect.x + 4.0F, rect.y + 48.0F, 12.0F, 12.0F};
        (void)ui::drawTextureToRect(texture, icon);
        ui::drawTextInRect(name, Rectangle{rect.x + 18.0F, rect.y + 49.0F, rect.width - 20.0F, 10.0F},
                           8, GOLD, ui::HorizontalAlign::Left, ui::VerticalAlign::Top);
        return;
    }

    ui::drawTextInRect(name, Rectangle{rect.x + 4.0F, rect.y + 48.0F, rect.width - 8.0F, 10.0F},
                       8, GOLD, ui::HorizontalAlign::Left, ui::VerticalAlign::Top);
}

void UnitItem::drawEquipmentIcon(const RenderAssets& assets, EquipmentType equipment, Rectangle rect) {
    DrawRectangleRec(rect, ui::theme::Surface);
    if (const Texture2D* texture = assets.equipmentTexture(equipment); texture != nullptr) {
        (void)ui::drawTextureToRect(texture, ui::inset(rect, 5.0F));
    } else {
        const std::string name{equipmentName(equipment)};
        ui::drawTextInRect(name, ui::inset(rect, 4.0F), 10, RAYWHITE,
                           ui::HorizontalAlign::Center, ui::VerticalAlign::Middle);
    }
    DrawRectangleLinesEx(rect, 1.0F, ui::theme::SurfaceBorder);
}

}  // namespace synera
