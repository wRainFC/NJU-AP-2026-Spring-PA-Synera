#pragma once

#include <string_view>

namespace synera {

class AbilityContext;
class Unit;

class Ability {
public:
    virtual ~Ability() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
    virtual void cast(Unit& caster, AbilityContext& context) = 0;
};

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
    void cast(Unit& caster, AbilityContext& context) override;
};

class HealingAuraAbility final : public Ability {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    void cast(Unit& caster, AbilityContext& context) override;
};

}  // namespace synera
