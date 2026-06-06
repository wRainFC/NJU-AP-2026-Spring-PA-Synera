#pragma once

#include "core/Types.hpp"

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace synera {

enum class CombatActionKind { BasicAttack, Ability };

struct CombatActionProfile {
    std::string id;
    CombatActionKind kind = CombatActionKind::BasicAttack;
    AttackVisualKind attackKind = AttackVisualKind::Melee;
    float durationSeconds = 0.36F;
    std::vector<float> hitTimes;
    std::string animationProfileId;
};

class CombatActionCatalog {
public:
    CombatActionCatalog();

    void resetToDefaults();
    bool loadFromFile(const std::filesystem::path& path);

    [[nodiscard]] const CombatActionProfile& profile(std::string_view id) const noexcept;
    [[nodiscard]] const CombatActionProfile& defaultBasicAttack(AttackVisualKind attackKind) const noexcept;
    [[nodiscard]] const CombatActionProfile& defaultAbility() const noexcept;
    [[nodiscard]] std::span<const CombatActionProfile> profiles() const noexcept;

private:
    void upsert(CombatActionProfile profile);

    std::vector<CombatActionProfile> profiles_;
    std::unordered_map<std::string, std::size_t> indexById_;
};

}  // namespace synera
