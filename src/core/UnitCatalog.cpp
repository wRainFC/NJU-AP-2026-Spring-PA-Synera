#include "core/UnitCatalog.hpp"

#include "core/abilities/BasicAbilities.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <span>
#include <vector>

namespace synera {

namespace {

using AbilityFactory = std::unique_ptr<Ability> (*)();

struct UnitTemplate {
    std::string_view id;
    std::string_view displayName;
    std::string_view basicAttackProfileId;
    UnitStats stats;
    std::span<const Trait> traits;
    AbilityFactory abilityFactory;
};

std::unique_ptr<Ability> makeNoopAbility() {
    return std::make_unique<NoopAbility>();
}

std::unique_ptr<Ability> makeFireLineAbility() {
    return std::make_unique<FireLineAbility>();
}

std::unique_ptr<Ability> makeHealingAuraAbility() {
    return std::make_unique<HealingAuraAbility>();
}

std::unique_ptr<Ability> makeStunStrikeAbility() {
    return std::make_unique<StunStrikeAbility>();
}

constexpr UnitStats DefaultStats{360, 32, 1, 70, 1.0F, 0.25F};

constexpr std::array IronGuardTraits{Trait::Warrior, Trait::Guardian};
constexpr std::array EmberMageTraits{Trait::Mage, Trait::Mystic};
constexpr std::array FieldMedicTraits{Trait::Mystic, Trait::Guardian};
constexpr std::array StormArcherTraits{Trait::Ranger, Trait::Assassin};
constexpr std::array TrainingDummyTraits{Trait::Guardian};

constexpr std::array UnitTemplates{
    UnitTemplate{"iron_guard", "Iron Guard", "iron_guard.basic_slash", DefaultStats, IronGuardTraits,
                 makeStunStrikeAbility},
    UnitTemplate{"ember_mage", "Ember Mage", "ember_mage.basic_bolt",
                 UnitStats{220, 42, 3, 60, 1.2F, 0.25F}, EmberMageTraits,
                 makeFireLineAbility},
    UnitTemplate{"field_medic", "Field Medic", "field_medic.basic_pulse",
                 UnitStats{260, 24, 2, 55, 1.1F, 0.25F}, FieldMedicTraits,
                 makeHealingAuraAbility},
    UnitTemplate{"storm_archer", "Storm Archer", "storm_archer.basic_arrow",
                 UnitStats{240, 48, 4, 70, 0.9F, 0.25F}, StormArcherTraits,
                 makeStunStrikeAbility},
    UnitTemplate{"training_dummy", "Training Dummy", "training_dummy.basic_slam",
                 UnitStats{180, 18, 1, 80, 1.4F, 0.3F}, TrainingDummyTraits, makeNoopAbility},
};

const UnitTemplate* findTemplate(std::string_view templateId) {
    const auto iter = std::ranges::find_if(
        UnitTemplates, [&](const UnitTemplate& unitTemplate) { return unitTemplate.id == templateId; });
    return iter == UnitTemplates.end() ? nullptr : &*iter;
}

std::vector<Trait> traitsFor(const UnitTemplate* unitTemplate) {
    if (unitTemplate == nullptr) {
        return {Trait::Warrior};
    }
    return {unitTemplate->traits.begin(), unitTemplate->traits.end()};
}

}  // namespace

Unit UnitCatalog::createUnit(UnitId id, std::string_view templateId, Owner owner) {
    const UnitTemplate* unitTemplate = findTemplate(templateId);

    Unit unit;
    unit.id           = id;
    unit.templateId            = std::string(templateId);
    unit.basicAttackProfileId  = unitTemplate == nullptr ? "default.basic_melee"
                                                         : std::string(unitTemplate->basicAttackProfileId);
    unit.name                  = unitTemplate == nullptr ? "Unknown" : std::string(unitTemplate->displayName);
    unit.owner                 = owner;
    unit.baseStats             = unitTemplate == nullptr ? DefaultStats : unitTemplate->stats;
    unit.derivedStats = unit.baseStats;
    unit.runtime.hp   = unit.derivedStats.maxHp;
    unit.runtime.mana = 0;
    unit.ability      = unitTemplate == nullptr ? makeNoopAbility() : unitTemplate->abilityFactory();
    unit.traits       = traitsFor(unitTemplate);
    return unit;
}

}  // namespace synera
