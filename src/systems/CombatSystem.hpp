#pragma once

#include "board/Pathfinder.hpp"
#include "core/Types.hpp"

namespace synera {

class GameState;
class Unit;

class CombatSystem {
public:
    void update(GameState& state, float dt);

private:
    void updateUnit(GameState& state, Unit& unit, float dt);
    Unit* acquireTarget(GameState& state, const Unit& unit);
    void updateStun(Unit& unit, float dt);
    bool tryCastAbility(GameState& state, Unit& unit);
    void moveTowardTarget(GameState& state, Unit& unit, const Unit& target, float dt);
    void performAttack(Unit& attacker, Unit& target);
    void cleanupDeadBoardUnits(GameState& state);

    Pathfinder pathfinder_;
};

}  // namespace synera
