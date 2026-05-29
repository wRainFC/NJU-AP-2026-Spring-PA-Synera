#include "systems/SaveSystem.hpp"

namespace synera {

std::expected<void, std::string> SaveSystem::save(const GameState&, const std::string&) const {
    return {};
}

std::expected<GameState, std::string> SaveSystem::load(const std::string&) const {
    return GameState{};
}

} // namespace synera
