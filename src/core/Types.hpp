#pragma once

#include <cstdint>
#include <string>

namespace synera {

using UnitId = std::uint32_t;

inline constexpr UnitId InvalidUnitId = 0;

struct AxialPos {
    int q = 0;
    int r = 0;

    friend bool operator==(const AxialPos&, const AxialPos&) = default;
};

struct OffsetPos {
    int col = 0;
    int row = 0;

    friend bool operator==(const OffsetPos&, const OffsetPos&) = default;
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
