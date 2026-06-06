#include "ui/CombatAnimationController.hpp"

#include "ui/Layout.hpp"

#include <algorithm>
#include <cmath>

namespace synera {

namespace {

[[nodiscard]] Vector2 subtract(Vector2 left, Vector2 right) noexcept {
    return Vector2{left.x - right.x, left.y - right.y};
}

[[nodiscard]] Vector2 scale(Vector2 value, float amount) noexcept {
    return Vector2{value.x * amount, value.y * amount};
}

[[nodiscard]] float length(Vector2 value) noexcept {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

[[nodiscard]] Vector2 normalized(Vector2 value) noexcept {
    const float magnitude = length(value);
    if (magnitude <= 0.001F) {
        return Vector2{};
    }
    return Vector2{value.x / magnitude, value.y / magnitude};
}

[[nodiscard]] Vector2 lerp(Vector2 from, Vector2 to, float t) noexcept {
    return Vector2{from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t};
}

[[nodiscard]] float clamp01(float value) noexcept {
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float progress(float elapsed, float duration) noexcept {
    if (duration <= 0.0F) {
        return 1.0F;
    }
    return clamp01(elapsed / duration);
}

[[nodiscard]] float triangle(float t) noexcept {
    return t < 0.5F ? t * 2.0F : (1.0F - t) * 2.0F;
}

template <class Item>
void eraseForUnit(std::vector<Item>& items, UnitId unitId) {
    std::erase_if(items, [&](const Item& item) { return item.unitId == unitId; });
}

}  // namespace

void CombatAnimationController::setAnimationCatalog(const CombatAnimationCatalog& catalog) noexcept {
    animationCatalog_ = &catalog;
}

void CombatAnimationController::reset() {
    motions_.clear();
    actions_.clear();
    projectiles_.clear();
    impacts_.clear();
}

void CombatAnimationController::addEvents(std::span<const CombatEvent> events, const Layout& layout) {
    for (const CombatEvent& event : events) {
        dispatchEvent(event, layout);
    }
}

void CombatAnimationController::update(float dt) {
    for (UnitMotion& motion : motions_) {
        motion.elapsed += dt;
    }
    for (UnitAction& action : actions_) {
        action.elapsed += dt;
    }
    for (Projectile& projectile : projectiles_) {
        projectile.elapsed += dt;
        if (projectile.elapsed >= projectile.duration) {
            // The authoritative hit impact is emitted from DamageDealt.
        }
    }
    for (Impact& impact : impacts_) {
        impact.elapsed += dt;
    }

    std::erase_if(motions_, [](const UnitMotion& motion) { return motion.elapsed >= motion.duration; });
    std::erase_if(actions_, [](const UnitAction& action) { return action.elapsed >= action.duration; });
    std::erase_if(projectiles_,
                  [](const Projectile& projectile) { return projectile.elapsed >= projectile.duration; });
    std::erase_if(impacts_, [](const Impact& impact) { return impact.elapsed >= impact.duration; });
}

CombatVisualReadModel CombatAnimationController::readModel() const {
    CombatVisualReadModel model;
    for (const UnitMotion& motion : motions_) {
        const float t = progress(motion.elapsed, motion.duration);
        model.units.push_back(UnitVisual{
            .unitId = motion.unitId,
            .clipId = {},
            .offset = subtract(lerp(motion.from, motion.to, t), motion.to),
            .state = UnitState::Moving,
            .animationTimeSeconds = motion.elapsed,
            .overridesState = true,
        });
    }

    for (const UnitAction& action : actions_) {
        Vector2 offset{};
        if (action.attackKind == AttackVisualKind::Melee) {
            const Vector2 direction = normalized(subtract(action.to, action.from));
            offset = scale(direction, action.lungePixels * triangle(progress(action.elapsed, action.duration)));
        }
        model.units.push_back(UnitVisual{
            .unitId = action.unitId,
            .clipId = action.clipId,
            .offset = offset,
            .state = action.state,
            .animationTimeSeconds = action.elapsed,
            .overridesState = true,
        });
    }

    for (const Projectile& projectile : projectiles_) {
        if (projectile.elapsed < 0.0F) {
            continue;
        }
        const float t = progress(projectile.elapsed, projectile.duration);
        model.projectiles.push_back(ProjectileVisual{
            .clipId = projectile.clipId,
            .position = lerp(projectile.from, projectile.to, t),
            .start = projectile.from,
            .end = projectile.to,
        });
    }

    for (const Impact& impact : impacts_) {
        if (impact.elapsed < 0.0F) {
            continue;
        }
        model.impacts.push_back(ImpactVisual{
            .clipId = impact.clipId,
            .position = impact.position,
            .progress = progress(impact.elapsed, impact.duration),
            .kind = impact.kind,
        });
    }
    return model;
}

void CombatAnimationController::dispatchEvent(const CombatEvent& event, const Layout& layout) {
    switch (event.type) {
        case CombatEventType::UnitMoved:
            handleUnitMoved(event, layout);
            break;
        case CombatEventType::AttackStarted:
            handleAttackStarted(event, layout);
            break;
        case CombatEventType::AbilityCast:
            handleAbilityCast(event, layout);
            break;
        case CombatEventType::DamageDealt:
            handleDamageDealt(event, layout);
            break;
        case CombatEventType::HealReceived:
            handleHealReceived(event, layout);
            break;
        case CombatEventType::StatusApplied:
            handleStatusApplied(event, layout);
            break;
        case CombatEventType::UnitDied:
            handleUnitDied(event, layout);
            break;
    }
}

void CombatAnimationController::handleUnitMoved(const CombatEvent& event, const Layout& layout) {
    eraseForUnit(motions_, event.sourceId);
    motions_.push_back(UnitMotion{
        .unitId = event.sourceId,
        .from = layout.boardHexCenter(event.from),
        .to = layout.boardHexCenter(event.to),
    });
}

void CombatAnimationController::handleAttackStarted(const CombatEvent& event, const Layout& layout) {
    const CombatAnimationProfile& profile = profileFor(event);
    const Vector2 from = layout.boardHexCenter(event.from);
    const Vector2 to = layout.boardHexCenter(event.to);
    const float actionDuration = event.actionDurationSeconds > 0.0F
                                     ? event.actionDurationSeconds
                                     : config::BasicAttackVisualDurationSeconds;

    eraseForUnit(actions_, event.sourceId);
    actions_.push_back(UnitAction{
        .unitId = event.sourceId,
        .clipId = profile.unitClip.id,
        .state = profile.unitState,
        .from = from,
        .to = to,
        .attackKind = profile.attackKind,
        .lungePixels = profile.lungePixels,
        .duration = actionDuration,
    });

    if (!profile.projectileEnabled) {
        return;
    }

    const float distance = length(subtract(to, from));
    const float speedDuration = distance / profile.projectilePixelsPerSecond;
    const float hitDelay = event.hitDelaySeconds > 0.0F ? event.hitDelaySeconds : actionDuration * 0.5F;
    const float duration = std::max(0.05F, std::min(speedDuration, hitDelay));
    projectiles_.push_back(Projectile{
        .clipId = profile.projectileClip.id,
        .from = from,
        .to = to,
        .elapsed = -std::max(0.0F, hitDelay - duration),
        .duration = duration,
    });
}

void CombatAnimationController::handleAbilityCast(const CombatEvent& event, const Layout& layout) {
    const CombatAnimationProfile& profile = profileFor(event);
    const Vector2 from = layout.boardHexCenter(event.from);
    const Vector2 to = layout.boardHexCenter(event.to);
    const float actionDuration = event.actionDurationSeconds > 0.0F
                                     ? event.actionDurationSeconds
                                     : config::AbilityCastVisualDurationSeconds;

    eraseForUnit(actions_, event.sourceId);
    actions_.push_back(UnitAction{
        .unitId = event.sourceId,
        .clipId = profile.unitClip.id,
        .state = profile.unitState,
        .from = from,
        .to = to,
        .attackKind = profile.attackKind,
        .lungePixels = profile.lungePixels,
        .duration = actionDuration,
    });

    if (!profile.castImpactClip.id.empty()) {
        impacts_.push_back(Impact{
            .clipId = profile.castImpactClip.id,
            .position = from,
            .duration = profile.impactDurationSeconds,
            .kind = CombatImpactVisualKind::Cast,
        });
    }

    if (!profile.projectileEnabled) {
        return;
    }

    const float distance = length(subtract(to, from));
    const float speedDuration = distance / profile.projectilePixelsPerSecond;
    const float hitDelay = event.hitDelaySeconds > 0.0F ? event.hitDelaySeconds : actionDuration * 0.5F;
    const float duration = std::max(0.05F, std::min(speedDuration, hitDelay));
    projectiles_.push_back(Projectile{
        .clipId = profile.projectileClip.id,
        .from = from,
        .to = to,
        .elapsed = -std::max(0.0F, hitDelay - duration),
        .duration = duration,
    });
}

void CombatAnimationController::handleDamageDealt(const CombatEvent& event, const Layout& layout) {
    const CombatAnimationProfile& profile = profileFor(event);
    impacts_.push_back(Impact{
        .clipId = profile.damageImpactClip.id,
        .position = layout.boardHexCenter(event.to),
        .duration = profile.impactDurationSeconds,
        .kind = CombatImpactVisualKind::Hit,
    });
}

void CombatAnimationController::handleHealReceived(const CombatEvent& event, const Layout& layout) {
    const CombatAnimationProfile& profile = profileFor(event);
    impacts_.push_back(Impact{
        .clipId = profile.healImpactClip.id,
        .position = layout.boardHexCenter(event.to),
        .duration = profile.impactDurationSeconds,
        .kind = CombatImpactVisualKind::Heal,
    });
}

void CombatAnimationController::handleStatusApplied(const CombatEvent& event, const Layout& layout) {
    const CombatAnimationProfile& profile = profileFor(event);
    impacts_.push_back(Impact{
        .clipId = profile.statusImpactClip.id,
        .position = layout.boardHexCenter(event.to),
        .duration = profile.impactDurationSeconds,
        .kind = CombatImpactVisualKind::Status,
    });
}

void CombatAnimationController::handleUnitDied(const CombatEvent& event, const Layout& layout) {
    impacts_.push_back(Impact{
        .clipId = "impact.death",
        .position = layout.boardHexCenter(event.from),
        .duration = config::DeathImpactDurationSeconds,
        .kind = CombatImpactVisualKind::Death,
    });
}

const CombatAnimationCatalog& CombatAnimationController::animationCatalog() const noexcept {
    return animationCatalog_ == nullptr ? defaultAnimationCatalog_ : *animationCatalog_;
}

const CombatAnimationProfile& CombatAnimationController::profileFor(const CombatEvent& event) const noexcept {
    if (!event.animationProfileId.empty()) {
        return animationCatalog().profile(event.animationProfileId);
    }
    if (!event.actionProfileId.empty()) {
        return animationCatalog().profile(event.actionProfileId);
    }
    if (event.type == CombatEventType::AbilityCast) {
        return animationCatalog().defaultAbility();
    }
    return animationCatalog().defaultBasicAttack(event.attackKind);
}

}  // namespace synera
