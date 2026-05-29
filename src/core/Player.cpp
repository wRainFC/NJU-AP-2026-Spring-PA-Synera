#include "core/Player.hpp"

namespace synera {

bool Player::isDead() const noexcept {
    return hp <= 0;
}

bool Player::canAfford(int cost) const noexcept {
    return cost >= 0 && gold >= cost;
}

bool Player::spendGold(int cost) noexcept {
    if (!canAfford(cost)) {
        return false;
    }
    gold -= cost;
    return true;
}

void Player::addGold(int amount) noexcept {
    if (amount > 0) {
        gold += amount;
    }
}

bool Player::upgradePopulation() noexcept {
    const int cost = 2 + level * 2;
    if (!spendGold(cost)) {
        return false;
    }
    ++level;
    ++populationCap;
    return true;
}

} // namespace synera
