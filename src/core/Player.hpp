#pragma once

#include "app/GameConfig.hpp"

namespace synera {

class Player {
public:
    int hp            = config::InitialPlayerHp;
    int gold          = config::InitialGold;
    int level         = 1;
    int populationCap = config::InitialPopulationCap;
    int currentRound  = 1;

    [[nodiscard]] bool isDead() const noexcept;
    [[nodiscard]] bool canAfford(int cost) const noexcept;
    bool spendGold(int cost) noexcept;
    void addGold(int amount) noexcept;
    bool upgradePopulation() noexcept;
};

}  // namespace synera
