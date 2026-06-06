#pragma once

#include "core/Types.hpp"

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace synera {

struct SpriteClipSpec {
    std::string id;
    std::string texturePath;
    int frameCount = 1;
    float framesPerSecond = 8.0F;
};

struct CombatAnimationProfile {
    std::string id;
    UnitState unitState = UnitState::Attacking;
    AttackVisualKind attackKind = AttackVisualKind::Melee;
    SpriteClipSpec unitClip;
    float lungePixels = 0.0F;
    bool projectileEnabled = false;
    SpriteClipSpec projectileClip;
    float projectilePixelsPerSecond = 520.0F;
    SpriteClipSpec castImpactClip;
    SpriteClipSpec damageImpactClip;
    SpriteClipSpec healImpactClip;
    SpriteClipSpec statusImpactClip;
    float impactDurationSeconds = 0.22F;
};

class CombatAnimationCatalog {
public:
    CombatAnimationCatalog();

    void resetToDefaults();
    bool loadFromFile(const std::filesystem::path& path);

    [[nodiscard]] const CombatAnimationProfile& profile(std::string_view id) const noexcept;
    [[nodiscard]] const CombatAnimationProfile& defaultBasicAttack(AttackVisualKind attackKind) const noexcept;
    [[nodiscard]] const CombatAnimationProfile& defaultAbility() const noexcept;
    [[nodiscard]] std::span<const CombatAnimationProfile> profiles() const noexcept;

private:
    void upsert(CombatAnimationProfile profile);

    std::vector<CombatAnimationProfile> profiles_;
    std::unordered_map<std::string, std::size_t> indexById_;
};

}  // namespace synera
