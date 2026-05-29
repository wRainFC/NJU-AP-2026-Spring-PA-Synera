#include "core/UnitCatalog.hpp"

#include "core/abilities/BasicAbilities.hpp"

#include <memory>

namespace synera {

namespace {

std::string displayNameFor(std::string_view templateId) {
    if (templateId == "ember_mage") {
        return "Ember Mage";
    }
    if (templateId == "iron_guard") {
        return "Iron Guard";
    }
    if (templateId == "training_dummy") {
        return "Training Dummy";
    }
    return "Unknown";
}

UnitStats statsFor(std::string_view templateId) {
    if (templateId == "ember_mage") {
        return UnitStats{220, 42, 3, 60, 1.2F, 0.25F};
    }
    if (templateId == "training_dummy") {
        return UnitStats{180, 18, 1, 80, 1.4F, 0.3F};
    }
    return UnitStats{360, 32, 1, 70, 1.0F, 0.25F};
}

std::unique_ptr<Ability> abilityFor(std::string_view templateId) {
    if (templateId == "ember_mage") {
        return std::make_unique<FireLineAbility>();
    }
    return std::make_unique<NoopAbility>();
}

std::vector<Trait> traitsFor(Owner owner) {
    return owner == Owner::PlayerCtrl ? std::vector<Trait>{Trait::Warrior}
                                      : std::vector<Trait>{Trait::Guardian};
}

}  // namespace

Unit UnitCatalog::createUnit(UnitId id, std::string_view templateId, Owner owner) {
    Unit unit;
    unit.id = id;
    unit.templateId = std::string(templateId);
    unit.name = displayNameFor(templateId);
    unit.owner = owner;
    unit.baseStats = statsFor(templateId);
    unit.derivedStats = unit.baseStats;
    unit.runtime.hp = unit.derivedStats.maxHp;
    unit.runtime.mana = 0;
    unit.ability = abilityFor(templateId);
    unit.traits = traitsFor(owner);
    return unit;
}

}  // namespace synera
