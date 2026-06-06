#include "core/abilities/BasicAbilities.hpp"

#include "board/HexGrid.hpp"
#include "core/AbilityContext.hpp"
#include "core/Unit.hpp"

#include <algorithm>
#include <optional>
#include <ranges>

namespace synera {

namespace {

constexpr int FireLineLength       = 3;
constexpr int FireLineDamage       = 85;
constexpr int HealingAuraRange     = 2;
constexpr int HealingAuraAmount    = 70;
constexpr int StunStrikeDamage     = 55;
constexpr float StunStrikeDuration = 1.0F;

[[nodiscard]] std::optional<AxialPos> targetPosOf(const Unit& caster, AbilityContext& context) {
    Unit* target = context.findUnit(caster.runtime.targetId);
    if (target == nullptr || !target->alive()) {
        return std::nullopt;
    }
    return target->boardPos;
}

[[nodiscard]] AxialPos directionToward(AxialPos source, AxialPos target) noexcept {
    return *std::ranges::min_element(hex::Directions, [&](AxialPos lhs, AxialPos rhs) {
        const AxialPos lhsPos{source.q + lhs.q, source.r + lhs.r};
        const AxialPos rhsPos{source.q + rhs.q, source.r + rhs.r};
        return hex::hexDistance(lhsPos, target) < hex::hexDistance(rhsPos, target);
    });
}

}  // namespace

std::string_view NoopAbility::name() const noexcept {
    return "No Skill";
}

std::string_view NoopAbility::description() const noexcept {
    return "Placeholder ability.";
}

void NoopAbility::cast(Unit&, AbilityContext&) {}

std::string_view FireLineAbility::name() const noexcept {
    return "Fire Line";
}

std::string_view FireLineAbility::description() const noexcept {
    return "Deals damage in a short hex line toward the current target.";
}

std::string_view FireLineAbility::combatActionProfileId() const noexcept {
    return "fire_line.cast";
}

void FireLineAbility::cast(Unit& caster, AbilityContext& context) {
    const auto origin = caster.boardPos;
    const auto target = targetPosOf(caster, context);
    if (origin && target) {
        const AxialPos direction = directionToward(*origin, *target);
        context.forEachEnemyOf(caster, [&](Unit& enemy) {
            if (!enemy.boardPos) {
                return;
            }
            for (int step : std::views::iota(1, FireLineLength + 1)) {
                const AxialPos hitPos{
                    origin->q + direction.q * step,
                    origin->r + direction.r * step,
                };
                if (*enemy.boardPos == hitPos) {
                    context.dealDamage(enemy, FireLineDamage);
                    return;
                }
            }
        });
    }
}

std::string_view HealingAuraAbility::name() const noexcept {
    return "Healing Aura";
}

std::string_view HealingAuraAbility::description() const noexcept {
    return "Heals nearby allies around the caster.";
}

std::string_view HealingAuraAbility::combatActionProfileId() const noexcept {
    return "healing_aura.cast";
}

void HealingAuraAbility::cast(Unit& caster, AbilityContext& context) {
    if (caster.boardPos) {
        context.forEachAllyOf(caster, [&](Unit& ally) {
            if (ally.boardPos && hex::hexDistance(*caster.boardPos, *ally.boardPos) <= HealingAuraRange) {
                context.heal(ally, HealingAuraAmount);
            }
        });
    }
}

std::string_view StunStrikeAbility::name() const noexcept {
    return "Stun Strike";
}

std::string_view StunStrikeAbility::description() const noexcept {
    return "Damages and briefly stuns the current target.";
}

std::string_view StunStrikeAbility::combatActionProfileId() const noexcept {
    return "stun_strike.cast";
}

void StunStrikeAbility::cast(Unit& caster, AbilityContext& context) {
    Unit* target = context.findUnit(caster.runtime.targetId);
    if (target != nullptr && caster.canAttackTarget(*target)) {
        context.dealDamage(*target, StunStrikeDamage);
        if (target->alive()) {
            context.applyStun(*target, StunStrikeDuration);
        }
    }
}

}  // namespace synera
