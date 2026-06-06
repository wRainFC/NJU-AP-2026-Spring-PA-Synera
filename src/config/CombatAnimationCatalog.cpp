#include "config/CombatAnimationCatalog.hpp"

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

[[nodiscard]] std::string_view attackKindName(AttackVisualKind kind) noexcept {
    return kind == AttackVisualKind::Ranged ? "ranged" : "melee";
}

[[nodiscard]] AttackVisualKind attackKindFromName(std::string_view name) noexcept {
    return name == "ranged" ? AttackVisualKind::Ranged : AttackVisualKind::Melee;
}

[[nodiscard]] std::string_view unitStateName(UnitState state) noexcept {
    switch (state) {
        case UnitState::Moving:
            return "moving";
        case UnitState::Attacking:
            return "attacking";
        case UnitState::Casting:
            return "casting";
        case UnitState::Stunned:
            return "stunned";
        case UnitState::Dead:
            return "dead";
        case UnitState::Idle:
            return "idle";
    }
    return "idle";
}

[[nodiscard]] UnitState unitStateFromName(std::string_view name) noexcept {
    if (name == "moving") {
        return UnitState::Moving;
    }
    if (name == "casting") {
        return UnitState::Casting;
    }
    if (name == "stunned") {
        return UnitState::Stunned;
    }
    if (name == "dead") {
        return UnitState::Dead;
    }
    if (name == "idle") {
        return UnitState::Idle;
    }
    return UnitState::Attacking;
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

struct CombatAnimationProfileData {
    std::string id;
    std::string unitState = std::string{unitStateName(UnitState::Attacking)};
    std::string attackKind = std::string{attackKindName(AttackVisualKind::Melee)};
    SpriteClipData unitClip;
    float lungePixels = 0.0F;
    bool projectileEnabled = false;
    SpriteClipData projectileClip;
    float projectilePixelsPerSecond = 520.0F;
    SpriteClipData castImpactClip;
    SpriteClipData damageImpactClip;
    SpriteClipData healImpactClip;
    SpriteClipData statusImpactClip;
    float impactDurationSeconds = 0.22F;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(id), CEREAL_NVP(unitState), CEREAL_NVP(attackKind),
                CEREAL_NVP(unitClip), CEREAL_NVP(lungePixels), CEREAL_NVP(projectileEnabled),
                CEREAL_NVP(projectileClip), CEREAL_NVP(projectilePixelsPerSecond),
                CEREAL_NVP(castImpactClip), CEREAL_NVP(damageImpactClip),
                CEREAL_NVP(healImpactClip), CEREAL_NVP(statusImpactClip),
                CEREAL_NVP(impactDurationSeconds));
    }
};

struct CombatAnimationManifestData {
    std::vector<CombatAnimationProfileData> profiles;

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

[[nodiscard]] CombatAnimationProfile profileFromData(CombatAnimationProfileData data) {
    return CombatAnimationProfile{
        .id = std::move(data.id),
        .unitState = unitStateFromName(data.unitState),
        .attackKind = attackKindFromName(data.attackKind),
        .unitClip = clipFromData(std::move(data.unitClip)),
        .lungePixels = data.lungePixels,
        .projectileEnabled = data.projectileEnabled,
        .projectileClip = clipFromData(std::move(data.projectileClip)),
        .projectilePixelsPerSecond = std::max(1.0F, data.projectilePixelsPerSecond),
        .castImpactClip = clipFromData(std::move(data.castImpactClip)),
        .damageImpactClip = clipFromData(std::move(data.damageImpactClip)),
        .healImpactClip = clipFromData(std::move(data.healImpactClip)),
        .statusImpactClip = clipFromData(std::move(data.statusImpactClip)),
        .impactDurationSeconds = std::max(0.01F, data.impactDurationSeconds),
    };
}

[[nodiscard]] CombatAnimationProfile makeMeleeProfile(std::string id, std::string clipId,
                                                      std::string texturePath) {
    return CombatAnimationProfile{
        .id = std::move(id),
        .unitState = UnitState::Attacking,
        .attackKind = AttackVisualKind::Melee,
        .unitClip = SpriteClipSpec{.id = std::move(clipId), .texturePath = std::move(texturePath),
                                   .frameCount = 4, .framesPerSecond = 10.0F},
        .lungePixels = 16.0F,
        .projectileEnabled = false,
        .projectileClip = SpriteClipSpec{},
        .castImpactClip = SpriteClipSpec{},
        .damageImpactClip = SpriteClipSpec{.id = "impact.hit", .texturePath = "textures/effects/hit.png",
                                           .frameCount = 4, .framesPerSecond = 12.0F},
        .healImpactClip = SpriteClipSpec{.id = "impact.heal", .texturePath = "textures/effects/heal.png",
                                         .frameCount = 4, .framesPerSecond = 12.0F},
        .statusImpactClip = SpriteClipSpec{.id = "impact.status", .texturePath = "textures/effects/status.png",
                                           .frameCount = 4, .framesPerSecond = 12.0F},
        .impactDurationSeconds = 0.22F,
    };
}

[[nodiscard]] CombatAnimationProfile makeRangedProfile(std::string id, std::string unitClipId,
                                                       std::string unitTexturePath,
                                                       std::string projectileClipId,
                                                       std::string projectileTexturePath) {
    return CombatAnimationProfile{
        .id = std::move(id),
        .unitState = UnitState::Attacking,
        .attackKind = AttackVisualKind::Ranged,
        .unitClip = SpriteClipSpec{.id = std::move(unitClipId), .texturePath = std::move(unitTexturePath),
                                   .frameCount = 4, .framesPerSecond = 10.0F},
        .projectileEnabled = true,
        .projectileClip = SpriteClipSpec{.id = std::move(projectileClipId),
                                         .texturePath = std::move(projectileTexturePath),
                                         .frameCount = 1, .framesPerSecond = 8.0F},
        .projectilePixelsPerSecond = 520.0F,
        .castImpactClip = SpriteClipSpec{},
        .damageImpactClip = SpriteClipSpec{.id = "impact.hit", .texturePath = "textures/effects/hit.png",
                                           .frameCount = 4, .framesPerSecond = 12.0F},
        .healImpactClip = SpriteClipSpec{.id = "impact.heal", .texturePath = "textures/effects/heal.png",
                                         .frameCount = 4, .framesPerSecond = 12.0F},
        .statusImpactClip = SpriteClipSpec{.id = "impact.status", .texturePath = "textures/effects/status.png",
                                           .frameCount = 4, .framesPerSecond = 12.0F},
        .impactDurationSeconds = 0.22F,
    };
}

[[nodiscard]] CombatAnimationProfile makeAbilityProfile(std::string id, std::string clipId,
                                                        std::string texturePath) {
    return CombatAnimationProfile{
        .id = std::move(id),
        .unitState = UnitState::Casting,
        .attackKind = AttackVisualKind::Ranged,
        .unitClip = SpriteClipSpec{.id = std::move(clipId), .texturePath = std::move(texturePath),
                                   .frameCount = 4, .framesPerSecond = 8.0F},
        .projectileEnabled = false,
        .projectileClip = SpriteClipSpec{},
        .castImpactClip = SpriteClipSpec{.id = "impact.cast", .texturePath = "textures/effects/cast.png",
                                         .frameCount = 4, .framesPerSecond = 10.0F},
        .damageImpactClip = SpriteClipSpec{.id = "impact.hit", .texturePath = "textures/effects/hit.png",
                                           .frameCount = 4, .framesPerSecond = 12.0F},
        .healImpactClip = SpriteClipSpec{.id = "impact.heal", .texturePath = "textures/effects/heal.png",
                                         .frameCount = 4, .framesPerSecond = 12.0F},
        .statusImpactClip = SpriteClipSpec{.id = "impact.status", .texturePath = "textures/effects/status.png",
                                           .frameCount = 4, .framesPerSecond = 12.0F},
        .impactDurationSeconds = 0.22F,
    };
}

[[nodiscard]] CombatAnimationProfile makeStunStrikeProfile() {
    CombatAnimationProfile profile =
        makeAbilityProfile("stun_strike.cast", "stun_strike.cast", "textures/actions/stun_strike_cast.png");
    profile.attackKind = AttackVisualKind::Melee;
    profile.lungePixels = 22.0F;
    profile.statusImpactClip = SpriteClipSpec{.id = "impact.stun", .texturePath = "textures/effects/stun.png",
                                              .frameCount = 4, .framesPerSecond = 12.0F};
    profile.impactDurationSeconds = 0.30F;
    return profile;
}

[[nodiscard]] CombatAnimationProfile makeFireLineProfile() {
    CombatAnimationProfile profile =
        makeAbilityProfile("fire_line.cast", "fire_line.cast", "textures/actions/fire_line_cast.png");
    profile.projectileEnabled = true;
    profile.projectileClip = SpriteClipSpec{.id = "projectile.fire_line",
                                            .texturePath = "textures/projectiles/fire_line.png",
                                            .frameCount = 2, .framesPerSecond = 12.0F};
    profile.projectilePixelsPerSecond = 680.0F;
    profile.damageImpactClip = SpriteClipSpec{.id = "impact.fire",
                                              .texturePath = "textures/effects/fire_hit.png",
                                              .frameCount = 4, .framesPerSecond = 12.0F};
    profile.impactDurationSeconds = 0.28F;
    return profile;
}

[[nodiscard]] CombatAnimationProfile makeHealingAuraProfile() {
    CombatAnimationProfile profile =
        makeAbilityProfile("healing_aura.cast", "healing_aura.cast",
                           "textures/actions/healing_aura_cast.png");
    profile.castImpactClip = SpriteClipSpec{.id = "impact.heal_cast",
                                            .texturePath = "textures/effects/heal_cast.png",
                                            .frameCount = 4, .framesPerSecond = 10.0F};
    profile.impactDurationSeconds = 0.32F;
    return profile;
}

}  // namespace

CombatAnimationCatalog::CombatAnimationCatalog() {
    resetToDefaults();
}

void CombatAnimationCatalog::resetToDefaults() {
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
    upsert(makeStunStrikeProfile());
    upsert(makeFireLineProfile());
    upsert(makeHealingAuraProfile());
}

bool CombatAnimationCatalog::loadFromFile(const std::filesystem::path& path) {
    std::ifstream in{path};
    if (!in) {
        return false;
    }

    try {
        CombatAnimationManifestData manifest;
        cereal::JSONInputArchive archive{in};
        archive(cereal::make_nvp("combatAnimations", manifest));
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

const CombatAnimationProfile& CombatAnimationCatalog::profile(std::string_view id) const noexcept {
    const auto iter = indexById_.find(std::string{id});
    if (iter == indexById_.end()) {
        return defaultAbility();
    }
    return profiles_[iter->second];
}

const CombatAnimationProfile& CombatAnimationCatalog::defaultBasicAttack(
    AttackVisualKind attackKind) const noexcept {
    const std::string_view id = attackKind == AttackVisualKind::Ranged ? DefaultRangedProfileId
                                                                       : DefaultMeleeProfileId;
    const auto iter = indexById_.find(std::string{id});
    return iter == indexById_.end() ? profiles_.front() : profiles_[iter->second];
}

const CombatAnimationProfile& CombatAnimationCatalog::defaultAbility() const noexcept {
    const auto iter = indexById_.find(std::string{DefaultAbilityProfileId});
    return iter == indexById_.end() ? profiles_.front() : profiles_[iter->second];
}

std::span<const CombatAnimationProfile> CombatAnimationCatalog::profiles() const noexcept {
    return profiles_;
}

void CombatAnimationCatalog::upsert(CombatAnimationProfile profile) {
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
