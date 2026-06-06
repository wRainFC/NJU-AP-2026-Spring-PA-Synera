#pragma once

#include "core/GameState.hpp"
#include "core/Types.hpp"
#include "core/Unit.hpp"

#include <concepts>
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace synera {

class GameState;
class Unit;

enum class AbilityResultType { Damage, Heal, Stun };

struct AbilityResult {
    AbilityResultType type = AbilityResultType::Damage;
    UnitId targetId = InvalidUnitId;
    AxialPos targetPos{};
    int amount = 0;
    float durationSeconds = 0.0F;
};

class AbilityContext {
public:
    explicit AbilityContext(GameState& state) noexcept;

    template <std::invocable<Unit&> Visitor>
    void forEachEnemyOf(const Unit& unit, Visitor&& visitor) {
        forEachMatchingUnit(unit, std::not_equal_to<Owner>{}, std::forward<Visitor>(visitor));
    }

    template <std::invocable<Unit&> Visitor>
    void forEachAllyOf(const Unit& unit, Visitor&& visitor) {
        forEachMatchingUnit(unit, std::equal_to<Owner>{}, std::forward<Visitor>(visitor));
    }

    void dealDamage(Unit& target, int amount) const noexcept;
    void heal(Unit& target, int amount) const noexcept;
    void applyStun(Unit& target, float durationSeconds) const noexcept;
    [[nodiscard]] Unit* findUnit(UnitId id) const;
    [[nodiscard]] std::span<const AbilityResult> results() const noexcept;

private:
    template <typename OwnerRelation, std::invocable<Unit&> Visitor>
    void forEachMatchingUnit(const Unit& unit, OwnerRelation relation, Visitor&& visitor) {
        state_.forEachUnit([&](Unit& candidate) {
            if (relation(candidate.owner, unit.owner) && candidate.alive()) {
                std::invoke(visitor, candidate);
            }
        });
    }

    GameState& state_;
    mutable std::vector<AbilityResult> results_;
};

}  // namespace synera
