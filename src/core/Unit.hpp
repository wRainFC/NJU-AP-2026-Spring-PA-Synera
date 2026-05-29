#pragma once

#include "core/Ability.hpp"
#include "core/Types.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace synera {

struct UnitStats {
    int maxHp = 300;
    int atk = 30;
    int range = 1;
    int maxMana = 60;
    float attackInterval = 1.0F;
    float moveInterval = 0.25F;
};

struct UnitRuntime {
    int hp = 300;
    int mana = 0;
    UnitState state = UnitState::Idle;
    UnitId targetId = InvalidUnitId;
    float attackTimer = 0.0F;
    float moveTimer = 0.0F;
    float stunTimer = 0.0F;
};

class Unit {
public:
    UnitId id = InvalidUnitId;
    std::string templateId;
    std::string name;
    Owner owner = Owner::PlayerCtrl;

    UnitStats baseStats;
    UnitStats derivedStats;
    UnitRuntime runtime;
    std::vector<Trait> traits;
    int star = 1;

    std::optional<AxialPos> boardPos;
    std::optional<int> benchSlot;
    std::optional<EquipmentType> equipment;

    std::unique_ptr<Ability> ability;

    [[nodiscard]] bool alive() const noexcept;
    [[nodiscard]] bool onBoard() const noexcept;
    [[nodiscard]] bool onBench() const noexcept;
    [[nodiscard]] bool canAttackTarget(const Unit& target) const;

    void receiveDamage(int amount) noexcept;
    void heal(int amount) noexcept;
    void gainMana(int amount) noexcept;
    void resetForCombat() noexcept;
    void checkInvariants() const;
};

}  // namespace synera
