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

struct SpriteClipData {
    std::string id;
    std::string texturePath;
    int frameCount = 1;
    float framesPerSecond = 8.0F;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(id), CEREAL_NVP(texturePath), CEREAL_NVP(frameCount),
                CEREAL_NVP(framesPerSecond));
    }
};

struct CombatActionProfileData {
    std::string id;
    std::string kind = std::string{actionKindName(CombatActionKind::BasicAttack)};
    std::string attackKind = std::string{attackKindName(AttackVisualKind::Melee)};
    float durationSeconds = 0.36F;
    std::vector<float> hitTimes;
    SpriteClipData unitClip;
    float lungePixels = 0.0F;
    bool projectileEnabled = false;
    SpriteClipData projectileClip;
    float projectilePixelsPerSecond = 520.0F;
    SpriteClipData impactClip;
    float impactDurationSeconds = 0.22F;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(id), CEREAL_NVP(kind), CEREAL_NVP(attackKind),
                CEREAL_NVP(durationSeconds), CEREAL_NVP(hitTimes), CEREAL_NVP(unitClip),
                CEREAL_NVP(lungePixels), CEREAL_NVP(projectileEnabled), CEREAL_NVP(projectileClip),
                CEREAL_NVP(projectilePixelsPerSecond), CEREAL_NVP(impactClip),
                CEREAL_NVP(impactDurationSeconds));
    }
};

struct CombatActionManifestData {
    std::vector<CombatActionProfileData> profiles;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(profiles));
    }
};

[[nodiscard]] SpriteClipSpec clipFromData(SpriteClipData data) {
    return SpriteClipSpec{
        .id = std::move(data.id),
        .texturePath = std::move(data.texturePath),
        .frameCount = std::max(1, data.frameCount),
        .framesPerSecond = data.framesPerSecond <= 0.0F ? 8.0F : data.framesPerSecond,
    };
}

[[nodiscard]] CombatActionProfile profileFromData(CombatActionProfileData data) {
    CombatActionProfile profile{
        .id = std::move(data.id),
        .kind = actionKindFromName(data.kind),
        .attackKind = attackKindFromName(data.attackKind),
        .durationSeconds = std::max(0.01F, data.durationSeconds),
        .hitTimes = std::move(data.hitTimes),
        .unitClip = clipFromData(std::move(data.unitClip)),
        .lungePixels = data.lungePixels,
        .projectileEnabled = data.projectileEnabled,
        .projectileClip = clipFromData(std::move(data.projectileClip)),
        .projectilePixelsPerSecond = std::max(1.0F, data.projectilePixelsPerSecond),
        .impactClip = clipFromData(std::move(data.impactClip)),
        .impactDurationSeconds = std::max(0.01F, data.impactDurationSeconds),
    };
    if (profile.hitTimes.empty()) {
        profile.hitTimes.push_back(std::min(profile.durationSeconds, profile.durationSeconds * 0.5F));
    }
    for (float& hitTime : profile.hitTimes) {
        hitTime = std::clamp(hitTime, 0.0F, profile.durationSeconds);
    }
    std::ranges::sort(profile.hitTimes);
    return profile;
}

[[nodiscard]] CombatActionProfile makeMeleeProfile(std::string id, std::string clipId,
                                                   std::string texturePath) {
    return CombatActionProfile{
        .id = std::move(id),
        .kind = CombatActionKind::BasicAttack,
        .attackKind = AttackVisualKind::Melee,
        .durationSeconds = 0.36F,
        .hitTimes = {0.14F},
        .unitClip = SpriteClipSpec{.id = std::move(clipId), .texturePath = std::move(texturePath),
                                   .frameCount = 4, .framesPerSecond = 10.0F},
        .lungePixels = 16.0F,
        .projectileEnabled = false,
        .projectileClip = SpriteClipSpec{},
        .impactClip = SpriteClipSpec{.id = "impact.hit", .texturePath = "textures/effects/hit.png",
                                     .frameCount = 4, .framesPerSecond = 12.0F},
        .impactDurationSeconds = 0.22F,
    };
}

[[nodiscard]] CombatActionProfile makeRangedProfile(std::string id, std::string unitClipId,
                                                    std::string unitTexturePath,
                                                    std::string projectileClipId,
                                                    std::string projectileTexturePath) {
    return CombatActionProfile{
        .id = std::move(id),
        .kind = CombatActionKind::BasicAttack,
        .attackKind = AttackVisualKind::Ranged,
        .durationSeconds = 0.44F,
        .hitTimes = {0.22F},
        .unitClip = SpriteClipSpec{.id = std::move(unitClipId), .texturePath = std::move(unitTexturePath),
                                   .frameCount = 4, .framesPerSecond = 10.0F},
        .projectileEnabled = true,
        .projectileClip = SpriteClipSpec{.id = std::move(projectileClipId),
                                         .texturePath = std::move(projectileTexturePath),
                                         .frameCount = 1, .framesPerSecond = 8.0F},
        .projectilePixelsPerSecond = 520.0F,
        .impactClip = SpriteClipSpec{.id = "impact.hit", .texturePath = "textures/effects/hit.png",
                                     .frameCount = 4, .framesPerSecond = 12.0F},
        .impactDurationSeconds = 0.22F,
    };
}

[[nodiscard]] CombatActionProfile makeAbilityProfile(std::string id, std::string clipId,
                                                     std::string texturePath) {
    return CombatActionProfile{
        .id = std::move(id),
        .kind = CombatActionKind::Ability,
        .attackKind = AttackVisualKind::Ranged,
        .durationSeconds = 0.48F,
        .hitTimes = {0.24F},
        .unitClip = SpriteClipSpec{.id = std::move(clipId), .texturePath = std::move(texturePath),
                                   .frameCount = 4, .framesPerSecond = 8.0F},
        .projectileEnabled = false,
        .projectileClip = SpriteClipSpec{},
        .impactClip = SpriteClipSpec{.id = "impact.hit", .texturePath = "textures/effects/hit.png",
                                     .frameCount = 4, .framesPerSecond = 12.0F},
        .impactDurationSeconds = 0.22F,
    };
}

}  // namespace

CombatActionCatalog::CombatActionCatalog() {
    resetToDefaults();
}

void CombatActionCatalog::resetToDefaults() {
    profiles_.clear();
    indexById_.clear();

    upsert(makeMeleeProfile(std::string{DefaultMeleeProfileId}, "default.melee_slash",
                            "textures/actions/default_melee_slash.png"));
    upsert(makeRangedProfile(std::string{DefaultRangedProfileId}, "default.ranged_shot",
                             "textures/actions/default_ranged_shot.png", "projectile.basic",
                             "textures/projectiles/basic.png"));
    upsert(makeAbilityProfile(std::string{DefaultAbilityProfileId}, "default.ability_cast",
                              "textures/actions/default_ability_cast.png"));

    upsert(makeMeleeProfile("iron_guard.basic_slash", "iron_guard.basic_slash",
                            "textures/actions/iron_guard_basic_slash.png"));
    upsert(makeMeleeProfile("training_dummy.basic_slam", "training_dummy.basic_slam",
                            "textures/actions/training_dummy_basic_slam.png"));
    upsert(makeRangedProfile("storm_archer.basic_arrow", "storm_archer.basic_arrow",
                             "textures/actions/storm_archer_basic_arrow.png", "projectile.arrow",
                             "textures/projectiles/arrow.png"));
    upsert(makeRangedProfile("ember_mage.basic_bolt", "ember_mage.basic_bolt",
                             "textures/actions/ember_mage_basic_bolt.png", "projectile.fire_bolt",
                             "textures/projectiles/fire_bolt.png"));
    upsert(makeRangedProfile("field_medic.basic_pulse", "field_medic.basic_pulse",
                             "textures/actions/field_medic_basic_pulse.png", "projectile.heal_pulse",
                             "textures/projectiles/heal_pulse.png"));

    upsert(makeAbilityProfile("stun_strike.cast", "stun_strike.cast",
                              "textures/actions/stun_strike_cast.png"));
    upsert(makeAbilityProfile("fire_line.cast", "fire_line.cast", "textures/actions/fire_line_cast.png"));
    upsert(makeAbilityProfile("healing_aura.cast", "healing_aura.cast",
                              "textures/actions/healing_aura_cast.png"));
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
