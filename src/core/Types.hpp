#pragma once

#include <cstdint>
#include <string>

namespace synera {

using UnitId = std::uint32_t;

inline constexpr UnitId InvalidUnitId = 0;

// Primary board coordinate for rules, combat distance, and pathfinding.
struct AxialPos {
    int q = 0;
    int r = 0;

    friend bool operator==(const AxialPos &, const AxialPos &) = default;
};

// Odd-r offset coordinate used only when adapting axial positions to storage or screen layout.
struct OffsetPos {
    int col = 0;
    int row = 0;

    friend bool operator==(const OffsetPos &, const OffsetPos &) = default;
};

enum class Owner { PlayerCtrl, EnemyCtrl };

enum class Phase { Prep, Combat, Resolve };

enum class UnitState { Idle, Moving, Attacking, Casting, Stunned, Dead };

enum class Trait { Warrior, Mage, Ranger, Guardian, Mystic, Assassin };

enum class EquipmentType { IronSword, ChainVest, SwiftGlove, ManaCrystal };

struct ShopOffer {
    std::string unitTemplateId;
    int cost = 0;
    int tier = 1;

    [[nodiscard]] bool empty() const noexcept { return unitTemplateId.empty(); }
};

}  // namespace synera
