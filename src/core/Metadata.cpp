#include "core/Metadata.hpp"

#include <array>
#include <ranges>

namespace synera {

namespace {

struct TraitInfo {
    Trait trait;
    std::string_view name;
    int activationThreshold;
    std::string_view effectDescription;
};

struct PhaseInfo {
    Phase phase;
    std::string_view name;
};

struct EquipmentInfo {
    EquipmentType equipment;
    std::string_view name;
    EquipmentStatModifier statModifier;
};

inline constexpr std::array PhaseInfos{
    PhaseInfo{.phase = Phase::Prep, .name = "Prep"},
    PhaseInfo{.phase = Phase::Combat, .name = "Combat"},
    PhaseInfo{.phase = Phase::Resolve, .name = "Resolve"},
};

inline constexpr std::array Traits{
    Trait::Warrior, Trait::Mage, Trait::Ranger, Trait::Guardian, Trait::Mystic, Trait::Assassin,
};

inline constexpr std::array TraitInfos{
    TraitInfo{.trait               = Trait::Warrior,
              .name                = "Warrior",
              .activationThreshold = 2,
              .effectDescription   = "2: Warriors gain +15 ATK."},
    TraitInfo{.trait               = Trait::Mage,
              .name                = "Mage",
              .activationThreshold = 0,
              .effectDescription   = "Reserved trait. No active effect yet."},
    TraitInfo{.trait               = Trait::Ranger,
              .name                = "Ranger",
              .activationThreshold = 2,
              .effectDescription   = "2: Rangers deal two basic-attack hits."},
    TraitInfo{.trait               = Trait::Guardian,
              .name                = "Guardian",
              .activationThreshold = 2,
              .effectDescription   = "2: Player board units gain +80 HP."},
    TraitInfo{.trait               = Trait::Mystic,
              .name                = "Mystic",
              .activationThreshold = 2,
              .effectDescription   = "2: Player board units need 10 less max mana."},
    TraitInfo{.trait               = Trait::Assassin,
              .name                = "Assassin",
              .activationThreshold = 0,
              .effectDescription   = "Reserved trait. No active effect yet."},
};

inline constexpr std::array EquipmentInfos{
    EquipmentInfo{
        .equipment    = EquipmentType::IronSword,
        .name         = "Sword",
        .statModifier = EquipmentStatModifier{.atkBonus = 15},
    },
    EquipmentInfo{
        .equipment    = EquipmentType::ChainVest,
        .name         = "Vest",
        .statModifier = EquipmentStatModifier{.maxHpBonus = 150},
    },
    EquipmentInfo{.equipment    = EquipmentType::SwiftGlove,
                  .name         = "Glove",
                  .statModifier = EquipmentStatModifier{.attackIntervalMultiplier = 0.8F}},
    EquipmentInfo{.equipment    = EquipmentType::ManaCrystal,
                  .name         = "Crystal",
                  .statModifier = EquipmentStatModifier{.maxManaDelta = -30, .minMaxMana = 20}},
};

[[nodiscard]] const TraitInfo *findTraitInfo(Trait trait) noexcept {
    const auto iter = std::ranges::find(TraitInfos, trait, &TraitInfo::trait);
    return iter == TraitInfos.end() ? nullptr : &*iter;
}

[[nodiscard]] const PhaseInfo *findPhaseInfo(Phase phase) noexcept {
    const auto iter = std::ranges::find(PhaseInfos, phase, &PhaseInfo::phase);
    return iter == PhaseInfos.end() ? nullptr : &*iter;
}

[[nodiscard]] const EquipmentInfo *findEquipmentInfo(EquipmentType equipment) noexcept {
    const auto iter = std::ranges::find(EquipmentInfos, equipment, &EquipmentInfo::equipment);
    return iter == EquipmentInfos.end() ? nullptr : &*iter;
}

}  // namespace

std::string_view phaseName(Phase phase) noexcept {
    const PhaseInfo *info = findPhaseInfo(phase);
    return info == nullptr ? "Unknown" : info->name;
}

std::span<const Trait> allTraits() noexcept {
    return Traits;
}

std::string_view traitName(Trait trait) noexcept {
    const TraitInfo *info = findTraitInfo(trait);
    return info == nullptr ? "Trait" : info->name;
}

int traitActivationThreshold(Trait trait) noexcept {
    const TraitInfo *info = findTraitInfo(trait);
    return info == nullptr ? 0 : info->activationThreshold;
}

std::string_view traitEffectDescription(Trait trait) noexcept {
    const TraitInfo *info = findTraitInfo(trait);
    return info == nullptr ? "Unknown trait." : info->effectDescription;
}

std::string_view equipmentName(EquipmentType equipment) noexcept {
    const EquipmentInfo *info = findEquipmentInfo(equipment);
    return info == nullptr ? "Equip" : info->name;
}

std::optional<EquipmentStatModifier> equipmentStatModifier(EquipmentType equipment) noexcept {
    const EquipmentInfo *info = findEquipmentInfo(equipment);
    return info == nullptr ? std::nullopt : std::optional<EquipmentStatModifier>{info->statModifier};
}

}  // namespace synera
