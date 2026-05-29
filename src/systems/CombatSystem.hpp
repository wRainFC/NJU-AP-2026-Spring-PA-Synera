#pragma once

#include "core/GameState.hpp"

namespace synera {

class CombatSystem {
public:
    void update(GameState& state, float dt);

private:
    void updateUnit(GameState& state, Unit& unit, float dt);
    Unit* acquireTarget(GameState& state, const Unit& unit);
    void performAttack(Unit& attacker, Unit& target);
};

}  // namespace synera
