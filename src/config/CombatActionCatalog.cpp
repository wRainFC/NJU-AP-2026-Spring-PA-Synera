#include "config/CombatActionCatalog.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <fstream>
#include <ranges>

namespace synera {

namespace {

constexpr std::string_view DefaultMeleeProfileId = "default.basic_melee";
constexpr std::string_view DefaultRangedProfileId = "default.basic_ranged";
constexpr std::string_view DefaultAbilityProfileId = "default.ability_cast";

[[nodiscard]] std::string_view actionKindName(CombatActionKind kind) noexcept {
    return kind == CombatActionKind::Ability ? "ability" : "basic_attack";
}

[[nodiscard]] std::string_view attackKindName(AttackVisualKind kind) noexcept {
    return kind == AttackVisualKind::Ranged ? "ranged" : "melee";
}

[[nodiscard]] CombatActionKind actionKindFromName(std::string_view name) noexcept {
    return name == "ability" ? CombatActionKind::Ability : CombatActionKind::BasicAttack;
}

[[nodiscard]] AttackVisualKind attackKindFromName(std::string_view name) noexcept {
    return name == "ranged" ? AttackVisualKind::Ranged : AttackVisualKind::Melee;
}

struct CombatActionProfileData {
    std::string id;
    std::string kind = std::string{actionKindName(CombatActionKind::BasicAttack)};
    std::string attackKind = std::string{attackKindName(AttackVisualKind::Melee)};
    float durationSeconds = 0.36F;
    std::vector<float> hitTimes;
    std::string animationProfileId;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(id), CEREAL_NVP(kind), CEREAL_NVP(attackKind),
                CEREAL_NVP(durationSeconds), CEREAL_NVP(hitTimes), CEREAL_NVP(animationProfileId));
    }
};

struct CombatActionManifestData {
    std::vector<CombatActionProfileData> profiles;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(profiles));
    }
};

[[nodiscard]] CombatActionProfile profileFromData(CombatActionProfileData data) {
    CombatActionProfile profile{
        .id = std::move(data.id),
        .kind = actionKindFromName(data.kind),
        .attackKind = attackKindFromName(data.attackKind),
        .durationSeconds = std::max(0.01F, data.durationSeconds),
        .hitTimes = std::move(data.hitTimes),
        .animationProfileId = std::move(data.animationProfileId),
    };
    if (profile.animationProfileId.empty()) {
        profile.animationProfileId = profile.id;
    }
    if (profile.hitTimes.empty()) {
        profile.hitTimes.push_back(std::min(profile.durationSeconds, profile.durationSeconds * 0.5F));
    }
    for (float& hitTime : profile.hitTimes) {
        hitTime = std::clamp(hitTime, 0.0F, profile.durationSeconds);
    }
    std::ranges::sort(profile.hitTimes);
    return profile;
}

[[nodiscard]] CombatActionProfile makeProfile(std::string id, CombatActionKind kind,
                                              AttackVisualKind attackKind, float durationSeconds,
                                              std::vector<float> hitTimes,
                                              std::string animationProfileId = {}) {
    if (animationProfileId.empty()) {
        animationProfileId = id;
    }
    return CombatActionProfile{
        .id = std::move(id),
        .kind = kind,
        .attackKind = attackKind,
        .durationSeconds = durationSeconds,
        .hitTimes = std::move(hitTimes),
        .animationProfileId = std::move(animationProfileId),
    };
}

}  // namespace

CombatActionCatalog::CombatActionCatalog() {
    resetToDefaults();
}

void CombatActionCatalog::resetToDefaults() {
    profiles_.clear();
    indexById_.clear();

    upsert(makeProfile(std::string{DefaultMeleeProfileId}, CombatActionKind::BasicAttack,
                       AttackVisualKind::Melee, 0.36F, {0.14F}));
    upsert(makeProfile(std::string{DefaultRangedProfileId}, CombatActionKind::BasicAttack,
                       AttackVisualKind::Ranged, 0.44F, {0.22F}));
    upsert(makeProfile(std::string{DefaultAbilityProfileId}, CombatActionKind::Ability,
                       AttackVisualKind::Ranged, 0.48F, {0.24F}));

    upsert(makeProfile("iron_guard.basic_slash", CombatActionKind::BasicAttack,
                       AttackVisualKind::Melee, 0.36F, {0.14F}));
    upsert(makeProfile("training_dummy.basic_slam", CombatActionKind::BasicAttack,
                       AttackVisualKind::Melee, 0.44F, {0.20F}));
    upsert(makeProfile("storm_archer.basic_arrow", CombatActionKind::BasicAttack,
                       AttackVisualKind::Ranged, 0.44F, {0.22F}));
    upsert(makeProfile("ember_mage.basic_bolt", CombatActionKind::BasicAttack,
                       AttackVisualKind::Ranged, 0.48F, {0.26F}));
    upsert(makeProfile("field_medic.basic_pulse", CombatActionKind::BasicAttack,
                       AttackVisualKind::Ranged, 0.46F, {0.24F}));
    upsert(makeProfile("stun_strike.cast", CombatActionKind::Ability,
                       AttackVisualKind::Melee, 0.46F, {0.18F}));
    upsert(makeProfile("fire_line.cast", CombatActionKind::Ability,
                       AttackVisualKind::Ranged, 0.58F, {0.30F}));
    upsert(makeProfile("healing_aura.cast", CombatActionKind::Ability,
                       AttackVisualKind::Ranged, 0.60F, {0.32F}));
}

bool CombatActionCatalog::loadFromFile(const std::filesystem::path& path) {
    std::ifstream in{path};
    if (!in) {
        return false;
    }

    try {
        CombatActionManifestData manifest;
        cereal::JSONInputArchive archive{in};
        archive(cereal::make_nvp("combatActions", manifest));
        for (auto& profile : manifest.profiles) {
            if (!profile.id.empty()) {
                upsert(profileFromData(std::move(profile)));
            }
        }
        return true;
    } catch (...) {
        resetToDefaults();
        return false;
    }
}

const CombatActionProfile& CombatActionCatalog::profile(std::string_view id) const noexcept {
    const auto iter = indexById_.find(std::string{id});
    if (iter == indexById_.end()) {
        return defaultAbility();
    }
    return profiles_[iter->second];
}

const CombatActionProfile& CombatActionCatalog::defaultBasicAttack(AttackVisualKind attackKind) const noexcept {
    const std::string_view id = attackKind == AttackVisualKind::Ranged ? DefaultRangedProfileId
                                                                       : DefaultMeleeProfileId;
    const auto iter = indexById_.find(std::string{id});
    return iter == indexById_.end() ? profiles_.front() : profiles_[iter->second];
}

const CombatActionProfile& CombatActionCatalog::defaultAbility() const noexcept {
    const auto iter = indexById_.find(std::string{DefaultAbilityProfileId});
    return iter == indexById_.end() ? profiles_.front() : profiles_[iter->second];
}

std::span<const CombatActionProfile> CombatActionCatalog::profiles() const noexcept {
    return profiles_;
}

void CombatActionCatalog::upsert(CombatActionProfile profile) {
    if (profile.id.empty()) {
        return;
    }
    const auto iter = indexById_.find(profile.id);
    if (iter != indexById_.end()) {
        profiles_[iter->second] = std::move(profile);
        return;
    }
    indexById_.emplace(profile.id, profiles_.size());
    profiles_.push_back(std::move(profile));
}

}  // namespace synera
