#include "systems/SaveSystem.hpp"

namespace synera {

std::expected<void, std::string> SaveSystem::save(const GameState&, const std::string&) const {
    return {};
}

std::expected<void, std::string> SaveSystem::load(GameState&, const std::string&) const {
    return {};
}

} // namespace synera
