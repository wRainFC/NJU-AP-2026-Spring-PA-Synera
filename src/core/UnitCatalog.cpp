#include "core/UnitCatalog.hpp"

#include "core/abilities/BasicAbilities.hpp"

#include <algorithm>
#include <array>
#include <memory>

namespace synera {

namespace {

using AbilityFactory = std::unique_ptr<Ability> (*)();

struct UnitTemplate {
    std::string_view id;
    std::string_view displayName;
    UnitStats stats;
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

constexpr std::array UnitTemplates{
    UnitTemplate{"iron_guard", "Iron Guard", DefaultStats, makeStunStrikeAbility},
    UnitTemplate{"ember_mage", "Ember Mage", UnitStats{220, 42, 3, 60, 1.2F, 0.25F}, makeFireLineAbility},
    UnitTemplate{"field_medic", "Field Medic", UnitStats{260, 24, 2, 55, 1.1F, 0.25F}, makeHealingAuraAbility},
    UnitTemplate{"training_dummy", "Training Dummy", UnitStats{180, 18, 1, 80, 1.4F, 0.3F}, makeNoopAbility},
};

const UnitTemplate* findTemplate(std::string_view templateId) {
    const auto iter = std::ranges::find_if(UnitTemplates, [&](const UnitTemplate& unitTemplate) {
        return unitTemplate.id == templateId;
    });
    return iter == UnitTemplates.end() ? nullptr : &*iter;
}

std::vector<Trait> traitsFor(Owner owner) {
    return owner == Owner::PlayerCtrl ? std::vector<Trait>{Trait::Warrior}
                                      : std::vector<Trait>{Trait::Guardian};
}

}  // namespace

Unit UnitCatalog::createUnit(UnitId id, std::string_view templateId, Owner owner) {
    const UnitTemplate* unitTemplate = findTemplate(templateId);

    Unit unit;
    unit.id = id;
    unit.templateId = std::string(templateId);
    unit.name = unitTemplate == nullptr ? "Unknown" : std::string(unitTemplate->displayName);
    unit.owner = owner;
    unit.baseStats = unitTemplate == nullptr ? DefaultStats : unitTemplate->stats;
    unit.derivedStats = unit.baseStats;
    unit.runtime.hp = unit.derivedStats.maxHp;
    unit.runtime.mana = 0;
    unit.ability = unitTemplate == nullptr ? makeNoopAbility() : unitTemplate->abilityFactory();
    unit.traits = traitsFor(owner);
    return unit;
}

}  // namespace synera
