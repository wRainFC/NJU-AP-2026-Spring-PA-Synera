#pragma once

#include "core/Types.hpp"

#include <vector>

namespace synera {

class GameState;
class Unit;

class AbilityContext {
public:
    explicit AbilityContext(GameState& state) noexcept;

    [[nodiscard]] std::vector<Unit*> enemiesOf(const Unit& unit);
    [[nodiscard]] std::vector<Unit*> alliesOf(const Unit& unit);

    void dealDamage(Unit& target, int amount) const noexcept;
    void heal(Unit& target, int amount) const noexcept;

private:
    GameState& state_;
};

} // namespace synera
