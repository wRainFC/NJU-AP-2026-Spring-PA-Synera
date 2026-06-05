#pragma once

#include "board/Pathfinder.hpp"
#include "config/CombatActionCatalog.hpp"
#include "core/Types.hpp"
#include "systems/CombatEvents.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace synera {

class GameState;
class Unit;

class CombatSystem {
public:
    void setActionCatalog(const CombatActionCatalog& catalog) noexcept;
    void update(GameState& state, float dt);
    [[nodiscard]] std::span<const CombatEvent> events() const noexcept;
    [[nodiscard]] std::vector<CombatEvent> drainEvents();
    void clearEvents() noexcept;

private:
    struct PendingCombatHit {
        UnitId targetId = InvalidUnitId;
        int amount = 0;
        float timeSeconds = 0.0F;
        bool resolved = false;
    };

    struct PendingCombatAction {
        std::uint64_t id = 0;
        CombatActionKind kind = CombatActionKind::BasicAttack;
        std::string profileId;
        UnitId sourceId = InvalidUnitId;
        UnitId targetId = InvalidUnitId;
        AxialPos from{};
        AxialPos to{};
        AttackVisualKind attackKind = AttackVisualKind::Melee;
        float elapsedSeconds = 0.0F;
        float durationSeconds = 0.0F;
        bool canceled = false;
        bool abilityResolved = false;
        std::vector<PendingCombatHit> hits;
    };

    void updateUnit(GameState& state, Unit& unit, float dt);
    Unit* acquireTarget(GameState& state, const Unit& unit);
    void updateStun(Unit& unit, float dt);
    bool tryCastAbility(Unit& unit);
    void moveTowardTarget(GameState& state, Unit& unit, const Unit& target, float dt);
    void performAttack(Unit& attacker, Unit& target);
    void updatePendingActions(GameState& state, float dt);
    void resolvePendingHit(GameState& state, PendingCombatAction& action, PendingCombatHit& hit);
    void resolvePendingAbility(GameState& state, PendingCombatAction& action);
    void cleanupDeadBoardUnits(GameState& state);
    void emit(CombatEvent event);
    [[nodiscard]] bool unitHasPendingAction(UnitId unitId) const noexcept;
    [[nodiscard]] const CombatActionCatalog& actionCatalog() const noexcept;
    [[nodiscard]] const CombatActionProfile& basicAttackProfileFor(const Unit& attacker) const noexcept;
    [[nodiscard]] const CombatActionProfile& abilityProfileFor(const Unit& caster) const noexcept;

    Pathfinder pathfinder_;
    CombatActionCatalog defaultActionCatalog_;
    const CombatActionCatalog* actionCatalog_ = nullptr;
    std::vector<CombatEvent> events_;
    std::vector<PendingCombatAction> pendingActions_;
    std::uint64_t nextActionId_ = 1;
};

}  // namespace synera
