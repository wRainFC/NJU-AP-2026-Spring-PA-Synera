#pragma once

#include "board/Pathfinder.hpp"
#include "core/GameState.hpp"

namespace synera {

class CombatSystem {
public:
    void update(GameState& state, float dt);

private:
    void updateUnit(GameState& state, Unit& unit, float dt);
    Unit* acquireTarget(GameState& state, const Unit& unit);
    void moveTowardTarget(GameState& state, Unit& unit, const Unit& target, float dt);
    void performAttack(Unit& attacker, Unit& target);

    Pathfinder pathfinder_;
};

}  // namespace synera
