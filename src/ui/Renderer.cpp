#include "ui/Renderer.hpp"

#include "app/GameConfig.hpp"
#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "core/Metadata.hpp"
#include "systems/SynergySystem.hpp"
#include "ui/GridItem.hpp"
#include "ui/Layout.hpp"
#include "ui/RenderAssets.hpp"
#include "ui/UiDrawing.hpp"
#include "ui/UiTheme.hpp"
#include "ui/UnitItem.hpp"

#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace synera {

namespace {

[[nodiscard]] std::optional<Rectangle> unitRect(const Unit& unit, const Layout& layout) {
    return unit.boardPos.transform([&](AxialPos pos) { return layout.boardHexBounds(pos); })
        .or_else([&]() -> std::optional<Rectangle> {
            return unit.benchSlot.transform([&](int slot) { return layout.benchSlotRect(slot); });
        });
}

[[nodiscard]] const Unit* hoveredUnit(const GameState& state, const Layout& layout, Vector2 mouse) {
    if (const auto pos = layout.boardPosAt(mouse)) {
        return state.boardOccupant(*pos)
            .transform([&](UnitId unitId) { return state.findUnit(unitId); })
            .value_or(nullptr);
    }
    if (const auto slot = layout.benchSlotAt(mouse)) {
        return state.benchOccupant(*slot)
            .transform([&](UnitId unitId) { return state.findUnit(unitId); })
            .value_or(nullptr);
    }
    return nullptr;
}

[[nodiscard]] const UnitDragGhost* unitDragGhost(const InputReadModel& input) noexcept {
    return std::get_if<UnitDragGhost>(&input.dragDrop.ghost);
}

[[nodiscard]] const EquipmentDragGhost* equipmentDragGhost(const InputReadModel& input) noexcept {
    return std::get_if<EquipmentDragGhost>(&input.dragDrop.ghost);
}

[[nodiscard]] bool hasDragGhost(const InputReadModel& input) noexcept {
    return !std::holds_alternative<std::monostate>(input.dragDrop.ghost);
}

[[nodiscard]] ui::ButtonStyle regularButtonStyle(bool enabled = true) noexcept {
    return ui::ButtonStyle{
        .background = enabled ? ui::theme::Button : ui::theme::ButtonDisabled,
        .border = RAYWHITE,
        .text = enabled ? RAYWHITE : GRAY,
        .tint = enabled ? WHITE : ui::theme::DisabledTint,
    };
}

void drawSectionTitle(std::string_view title, Rectangle anchor) {
    ui::drawText(title, static_cast<int>(anchor.x), static_cast<int>(anchor.y - 36.0F), 24, RAYWHITE);
}

}  // namespace

class Renderer::Impl {
public:
    void loadAssets(const std::filesystem::path& root) { assets_.load(root); }
    void unloadAssets() noexcept {
        ui::setFont(nullptr);
        assets_.unload();
    }

    void draw(const RenderContext& context) {
        ui::setFont(assets_.font());

        // Base scene and persistent UI regions.
        drawTopBar(context);
        drawBoard(context.state, context.layout);
        drawBench(context.layout);
        drawShop(context.state, context.layout);
        drawPopulationUpgrade(context.state, context.layout);
        drawEquipmentPool(context.state, context.layout);
        drawSynergies(context.state, context.layout);

        // Dynamic board content and interaction feedback.
        drawSellArea(context);
        drawUnits(context);
        drawDragPreview(context);
        drawHoverPanel(context);
        drawOutcomeOverlay(context);

        // Controls drawn last so they stay readable over the base scene.
        drawStartButton(context);
        drawSaveLoadButtons(context);
    }

private:
    RenderAssets assets_;

    // Base scene, economy panels, and inventory panels.
    void drawTopBar(const RenderContext& context) {
        const GameState& state = context.state;
        const std::string text = "HP: " + std::to_string(state.player().hp) +
                                 "  Gold: " + std::to_string(state.player().gold) +
                                 "  Pop: " + std::to_string(state.playerBoardUnitCount()) + "/" +
                                 std::to_string(state.player().populationCap) +
                                 "  Round: " + std::to_string(state.player().currentRound) +
                                 "  Phase: " + std::string(phaseName(state.phase()));
        ui::drawText(text, 150, 24, 24, RAYWHITE);
        if (!context.statusMessage.empty()) {
            ui::drawText(context.statusMessage, 150, 58, 18, GOLD);
        }
    }

    void drawBoard(const GameState& state, const Layout& layout) {
        (void)state;
        for (int y : std::views::iota(0, config::BoardHeight)) {
            for (int x : std::views::iota(0, config::BoardWidth)) {
                const AxialPos pos = hex::oddRToAxial(OffsetPos{x, y});
                GridItem::drawBoardHex(assets_, layout, pos, y < config::BoardHeight / 2);
            }
        }
    }

    void drawBench(const Layout& layout) {
        drawSectionTitle("Bench", layout.benchSlotRect(0));
        for (int slot : std::views::iota(0, config::BenchSize)) {
            GridItem::drawBenchSlot(layout.benchSlotRect(slot));
        }
    }

    void drawShop(const GameState& state, const Layout& layout) {
        drawSectionTitle("Shop", layout.shopOfferRect(0));
        for (int index : std::views::iota(0, config::ShopOfferCount)) {
            const Rectangle rect = layout.shopOfferRect(index);
            const ShopOffer& offer = state.shop().offers()[static_cast<std::size_t>(index)];
            DrawRectangleRec(rect, ui::theme::Surface);
            DrawRectangleLinesEx(rect, 1.0F, ui::theme::SurfaceBorder);

            const std::string name = offer.empty() ? "-" : offer.unitTemplateId;
            const std::string cost =
                offer.empty() ? ""
                              : ("T" + std::to_string(offer.tier) + "  " + std::to_string(offer.cost) + "g");
            float textLeft = rect.x + 14.0F;
            if (!offer.empty()) {
                const Rectangle preview{rect.x + 12.0F, rect.y + 10.0F, 50.0F, 50.0F};
                if (ui::drawTextureToRect(assets_.unitTexture(offer.unitTemplateId, Owner::PlayerCtrl),
                                          preview)) {
                    DrawRectangleLinesEx(preview, 1.0F, ui::theme::SurfaceBorder);
                    textLeft = rect.x + 72.0F;
                }
            }

            ui::drawTextInRect(
                name, Rectangle{textLeft, rect.y + 10.0F, rect.x + rect.width - textLeft - 72.0F, 26.0F}, 24,
                RAYWHITE);
            ui::drawTextInRect(cost, Rectangle{rect.x + rect.width - 88.0F, rect.y + 38.0F, 76.0F, 24.0F}, 18,
                               GOLD, ui::HorizontalAlign::Right);
        }

        ui::drawButton(assets_.texture(TextureSlot::Button), layout.shopRefreshButtonRect(), "Refresh",
                       regularButtonStyle(), 24);

        const bool locked = state.shop().locked();
        ui::drawButton(assets_.texture(TextureSlot::Button), layout.shopLockButtonRect(),
                       locked ? "Locked" : "Lock",
                       ui::ButtonStyle{
                           .background = locked ? ui::theme::ButtonLocked : ui::theme::Button,
                           .border = RAYWHITE,
                           .text = RAYWHITE,
                           .tint = locked ? ui::theme::ButtonLockedTint : WHITE,
                       },
                       24);
    }

    void drawPopulationUpgrade(const GameState& state, const Layout& layout) {
        const std::string label = "Level Up  " + std::to_string(2 + state.player().level * 2) + "g";
        ui::drawButton(assets_.texture(TextureSlot::Button), layout.populationUpgradeButtonRect(), label,
                       regularButtonStyle(), 24);
    }

    void drawEquipmentPool(const GameState& state, const Layout& layout) {
        drawSectionTitle("Equipment", layout.equipmentSlotRect(0));
        const auto pool = state.equipmentPool();
        for (std::size_t index : std::views::iota(std::size_t{0}, pool.size())) {
            UnitItem::drawEquipmentIcon(assets_, pool[index], layout.equipmentSlotRect(index));
        }
    }

    // Dynamic board state and prep controls.
    void drawSynergies(const GameState& state, const Layout& layout) {
        for (Trait trait : allTraits()) {
            const TraitSummary summary = summarizeTrait(state, trait);
            const Rectangle rect = layout.traitSlotRect(trait);
            const TextureSlot textureSlot =
                summary.active ? TextureSlot::TraitActive : TextureSlot::TraitInactive;
            ui::drawTexturedRect(assets_.texture(textureSlot), rect,
                                 summary.active ? Color{93, 82, 42, 255} : ui::theme::Surface);
            DrawRectangleLinesEx(rect, 1.0F, summary.active ? GOLD : ui::theme::SurfaceBorder);
            const std::string label = std::string(summary.name) + " " + std::to_string(summary.count);
            ui::drawTextInRect(label, rect, 18, summary.active ? GOLD : RAYWHITE, ui::HorizontalAlign::Center,
                               ui::VerticalAlign::Middle, 6.0F);
        }
    }

    void drawUnits(const RenderContext& context) {
        const UnitDragGhost* dragGhost = unitDragGhost(context.input);
        context.state.forEachUnit([&](const Unit& unit) {
            if (dragGhost != nullptr && unit.id == dragGhost->unitId) {
                return;
            }
            if (const auto rect = unitRect(unit, context.layout)) {
                UnitItem::drawUnit(assets_, unit, *rect, context.animationTimeSeconds);
                if (context.input.selectedUnitId == unit.id) {
                    DrawRectangleLinesEx(*rect, 3.0F, GOLD);
                }
            }
        });
    }

    void drawStartButton(const RenderContext& context) {
        const bool enabled = context.interactionsEnabled && context.state.phase() == Phase::Prep;
        ui::drawButton(
            assets_.texture(enabled ? TextureSlot::Button : TextureSlot::ButtonDisabled),
            context.layout.startButtonRect(), "Start Combat",
            ui::ButtonStyle{
                .background = enabled ? ui::theme::ButtonPrimary : ui::theme::ButtonPrimaryDisabled,
                .border = RAYWHITE,
                .text = enabled ? RAYWHITE : GRAY,
                .tint = enabled ? WHITE : ui::theme::DisabledTint,
            },
            24);
    }

    void drawSaveLoadButtons(const RenderContext& context) {
        const bool saveEnabled = context.interactionsEnabled && context.state.phase() == Phase::Prep;
        ui::drawButton(assets_.texture(saveEnabled ? TextureSlot::Button : TextureSlot::ButtonDisabled),
                       context.layout.saveButtonRect(), "Save", regularButtonStyle(saveEnabled), 24);
        ui::drawButton(assets_.texture(TextureSlot::Button), context.layout.loadButtonRect(), "Load",
                       regularButtonStyle(), 24);
    }

    void drawSellArea(const RenderContext& context) {
        const Rectangle rect = context.layout.sellAreaRect();
        ui::drawTexturedRect(assets_.texture(TextureSlot::SellArea), rect,
                             context.interactionsEnabled ? ui::theme::SellArea : ui::theme::SellAreaDisabled,
                             context.interactionsEnabled ? WHITE : ui::theme::DisabledTint);
        DrawRectangleLinesEx(rect, 1.0F, context.interactionsEnabled ? ui::theme::SellAreaBorder : GRAY);
        ui::drawTextInRect("Sell", Rectangle{rect.x, rect.y + 22.0F, rect.width, 34.0F}, 24,
                           context.interactionsEnabled ? RAYWHITE : GRAY, ui::HorizontalAlign::Center,
                           ui::VerticalAlign::Middle);
        ui::drawTextInRect("Drop unit", Rectangle{rect.x, rect.y + 62.0F, rect.width, 26.0F}, 18,
                           context.interactionsEnabled ? RAYWHITE : GRAY, ui::HorizontalAlign::Center,
                           ui::VerticalAlign::Middle);
    }

    // Pointer-driven feedback.
    void drawDragPreview(const RenderContext& context) {
        const Vector2 mouse = context.pointer.position;
        if (const UnitDragGhost* ghost = unitDragGhost(context.input); ghost != nullptr) {
            const Unit* unit = context.state.findUnit(ghost->unitId);
            if (unit == nullptr) {
                return;
            }
            const Rectangle rect{mouse.x - 38.0F, mouse.y - 38.0F, 76.0F, 76.0F};
            UnitItem::drawUnit(assets_, *unit, rect, context.animationTimeSeconds);
            return;
        }

        if (const EquipmentDragGhost* ghost = equipmentDragGhost(context.input); ghost != nullptr) {
            const Rectangle rect{mouse.x - 30.0F, mouse.y - 30.0F, 60.0F, 60.0F};
            UnitItem::drawEquipmentIcon(assets_, ghost->equipment, rect);
            DrawRectangleLinesEx(rect, 1.0F, GOLD);
        }
    }

    void drawHoverPanel(const RenderContext& context) {
        if (hasDragGhost(context.input) || !context.pointer.insideVirtualCanvas) {
            return;
        }

        const Vector2 mouse = context.pointer.position;
        if (const Unit* unit = hoveredUnit(context.state, context.layout, mouse); unit != nullptr) {
            std::vector<std::string> lines;
            lines.push_back(unit->name + "  *" + std::to_string(unit->star));
            lines.push_back(unit->owner == Owner::PlayerCtrl ? "Owner: Player" : "Owner: Enemy");
            lines.push_back("HP: " + std::to_string(unit->runtime.hp) + "/" +
                            std::to_string(unit->derivedStats.maxHp));
            lines.push_back("Mana: " + std::to_string(unit->runtime.mana) + "/" +
                            std::to_string(unit->derivedStats.maxMana));
            lines.push_back("ATK: " + std::to_string(unit->derivedStats.atk) +
                            "  Range: " + std::to_string(unit->derivedStats.range));
            lines.push_back("Attack Int: " + std::to_string(unit->derivedStats.attackInterval).substr(0, 4));
            lines.push_back(unit->equipment ? "Equip: " + std::string(equipmentName(*unit->equipment))
                                            : "Equip: None");
            std::string traits = "Traits:";
            for (Trait trait : unit->traits) {
                traits += " ";
                traits += traitName(trait);
            }
            lines.push_back(traits);
            ui::drawPanel(ui::panelNear(mouse, lines), lines, assets_.texture(TextureSlot::Panel));
            return;
        }

        if (const auto trait = context.layout.traitAt(mouse)) {
            const TraitSummary summary = summarizeTrait(context.state, *trait);
            std::vector<std::string> lines{
                std::string(summary.name),
                "Count: " + std::to_string(summary.count),
                summary.activationThreshold > 0 ? "Threshold: " + std::to_string(summary.activationThreshold)
                                                : "Threshold: none",
                std::string(summary.effectDescription),
            };
            ui::drawPanel(ui::panelNear(mouse, lines), lines, assets_.texture(TextureSlot::Panel));
        }
    }

    // Modal overlays.
    void drawOutcomeOverlay(const RenderContext& context) {
        if (context.outcomeMessage.empty()) {
            return;
        }

        DrawRectangle(0, 0, config::WindowWidth, config::WindowHeight, ui::theme::Overlay);
        const Rectangle panel{static_cast<float>(config::WindowWidth) / 2.0F - 320.0F,
                              static_cast<float>(config::WindowHeight) / 2.0F - 120.0F, 640.0F, 240.0F};
        ui::drawTexturedRect(assets_.texture(TextureSlot::Panel), panel, ui::theme::PanelStrong,
                             Color{255, 255, 255, 245});
        DrawRectangleLinesEx(panel, 2.0F, GOLD);
        ui::drawTextInRect(context.outcomeMessage, Rectangle{panel.x, panel.y + 34.0F, panel.width, 58.0F},
                           44, GOLD, ui::HorizontalAlign::Center, ui::VerticalAlign::Middle);
        ui::drawTextInRect("Choose how to continue.",
                           Rectangle{panel.x, panel.y + 102.0F, panel.width, 32.0F}, 24, RAYWHITE,
                           ui::HorizontalAlign::Center, ui::VerticalAlign::Middle);
        ui::drawButton(assets_.texture(TextureSlot::Button), context.layout.outcomeRestartButtonRect(),
                       "New Run", regularButtonStyle(), 24);
        ui::drawButton(assets_.texture(TextureSlot::Button), context.layout.outcomeLoadButtonRect(),
                       "Load Save", regularButtonStyle(), 24);
    }
};

Renderer::Renderer() : impl_(std::make_unique<Impl>()) {}

Renderer::~Renderer() = default;

Renderer::Renderer(Renderer&&) noexcept = default;

Renderer& Renderer::operator=(Renderer&&) noexcept = default;

void Renderer::loadAssets(const std::filesystem::path& root) {
    impl_->loadAssets(root);
}

void Renderer::unloadAssets() noexcept {
    impl_->unloadAssets();
}

void Renderer::draw(const RenderContext& context) {
    impl_->draw(context);
}

}  // namespace synera
