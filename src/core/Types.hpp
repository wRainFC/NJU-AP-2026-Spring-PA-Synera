#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace synera {

using UnitId = std::uint32_t;

inline constexpr UnitId InvalidUnitId = 0;

struct GridPos {
    int x = 0;
    int y = 0;

    friend bool operator==(const GridPos&, const GridPos&) = default;
};

enum class Owner { PlayerCtrl, EnemyCtrl };

enum class Phase { Prep, Combat, Resolve };

enum class UnitState { Idle, Moving, Attacking, Casting, Stunned, Dead };

enum class Trait { Warrior, Mage, Ranger, Guardian, Mystic, Assassin };

enum class EquipmentType { IronSword, ChainVest, SwiftGlove, ManaCrystal };

struct ShopOffer {
    std::string unitTemplateId;
    int cost = 1;
};

}  // namespace synera
