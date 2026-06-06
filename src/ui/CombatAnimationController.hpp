#pragma once

#include "config/AnimationConfig.hpp"
#include "config/CombatAnimationCatalog.hpp"
#include "core/Types.hpp"
#include "systems/CombatEvents.hpp"
#include "raylib.h"

#include <span>
#include <string>
#include <vector>

namespace synera {

class Layout;

enum class CombatImpactVisualKind { Hit, Heal, Status, Cast, Death };

struct UnitVisual {
    UnitId unitId = InvalidUnitId;
    std::string clipId;
    Vector2 offset{};
    UnitState state = UnitState::Idle;
    float animationTimeSeconds = 0.0F;
    bool overridesState = false;
};

struct ProjectileVisual {
    std::string clipId;
    Vector2 position{};
    Vector2 start{};
    Vector2 end{};
};

struct ImpactVisual {
    std::string clipId;
    Vector2 position{};
    float progress = 0.0F;
    CombatImpactVisualKind kind = CombatImpactVisualKind::Hit;
};

struct CombatVisualReadModel {
    std::vector<UnitVisual> units;
    std::vector<ProjectileVisual> projectiles;
    std::vector<ImpactVisual> impacts;
};

class CombatAnimationController {
public:
    void setAnimationCatalog(const CombatAnimationCatalog& catalog) noexcept;
    void reset();
    void addEvents(std::span<const CombatEvent> events, const Layout& layout);
    void update(float dt);
    [[nodiscard]] CombatVisualReadModel readModel() const;

private:
    struct UnitMotion {
        UnitId unitId = InvalidUnitId;
        Vector2 from{};
        Vector2 to{};
        float elapsed = 0.0F;
        float duration = config::UnitMoveVisualDurationSeconds;
    };

    struct UnitAction {
        UnitId unitId = InvalidUnitId;
        std::string clipId;
        UnitState state = UnitState::Attacking;
        Vector2 from{};
        Vector2 to{};
        AttackVisualKind attackKind = AttackVisualKind::Melee;
        float lungePixels = 0.0F;
        float elapsed = 0.0F;
        float duration = config::BasicAttackVisualDurationSeconds;
    };

    struct Projectile {
        std::string clipId;
        Vector2 from{};
        Vector2 to{};
        float elapsed = 0.0F;
        float duration = 0.1F;
    };

    struct Impact {
        std::string clipId;
        Vector2 position{};
        float elapsed = 0.0F;
        float duration = config::ImpactDurationSeconds;
        CombatImpactVisualKind kind = CombatImpactVisualKind::Hit;
    };

    void dispatchEvent(const CombatEvent& event, const Layout& layout);
    void handleUnitMoved(const CombatEvent& event, const Layout& layout);
    void handleAttackStarted(const CombatEvent& event, const Layout& layout);
    void handleAbilityCast(const CombatEvent& event, const Layout& layout);
    void handleDamageDealt(const CombatEvent& event, const Layout& layout);
    void handleHealReceived(const CombatEvent& event, const Layout& layout);
    void handleStatusApplied(const CombatEvent& event, const Layout& layout);
    void handleUnitDied(const CombatEvent& event, const Layout& layout);
    [[nodiscard]] const CombatAnimationCatalog& animationCatalog() const noexcept;
    [[nodiscard]] const CombatAnimationProfile& profileFor(const CombatEvent& event) const noexcept;

    CombatAnimationCatalog defaultAnimationCatalog_;
    const CombatAnimationCatalog* animationCatalog_ = nullptr;
    std::vector<UnitMotion> motions_;
    std::vector<UnitAction> actions_;
    std::vector<Projectile> projectiles_;
    std::vector<Impact> impacts_;
};

}  // namespace synera
