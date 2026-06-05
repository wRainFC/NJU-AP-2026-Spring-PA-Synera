#pragma once

#include "core/Ability.hpp"

namespace synera {

class NoopAbility final : public Ability {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    void cast(Unit& caster, AbilityContext& context) override;
};

class FireLineAbility final : public Ability {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    [[nodiscard]] std::string_view combatActionProfileId() const noexcept override;
    void cast(Unit& caster, AbilityContext& context) override;
};

class HealingAuraAbility final : public Ability {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    [[nodiscard]] std::string_view combatActionProfileId() const noexcept override;
    void cast(Unit& caster, AbilityContext& context) override;
};

class StunStrikeAbility final : public Ability {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    [[nodiscard]] std::string_view combatActionProfileId() const noexcept override;
    void cast(Unit& caster, AbilityContext& context) override;
};

}  // namespace synera
