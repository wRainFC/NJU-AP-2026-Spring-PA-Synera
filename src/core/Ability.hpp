#pragma once

#include <string_view>

namespace synera {

class GameState;
class Unit;

class Ability {
public:
    virtual ~Ability() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
    virtual void cast(Unit& caster, GameState& state) = 0;
};

class NoopAbility final : public Ability {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    void cast(Unit& caster, GameState& state) override;
};

class FireLineAbility final : public Ability {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    void cast(Unit& caster, GameState& state) override;
};

class HealingAuraAbility final : public Ability {
public:
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    void cast(Unit& caster, GameState& state) override;
};

} // namespace synera
