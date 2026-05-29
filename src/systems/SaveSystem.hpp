#pragma once

#include "core/GameState.hpp"

#include <expected>
#include <string>

namespace synera {

class SaveSystem {
public:
    [[nodiscard]] std::expected<void, std::string> save(const GameState& state,
                                                        const std::string& path) const;
    [[nodiscard]] std::expected<GameState, std::string> load(const std::string& path) const;
};

} // namespace synera
