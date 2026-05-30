#pragma once

#include <expected>
#include <string>

namespace synera {

class GameState;

class SaveSystem {
public:
    // Writes a versioned JSON snapshot; callers keep ownership of the live GameState.
    [[nodiscard]] std::expected<void, std::string> save(const GameState& state,
                                                        const std::string& path) const;
    // Rebuilds a fresh GameState through catalog and placement APIs instead of deserializing live objects.
    [[nodiscard]] std::expected<GameState, std::string> load(const std::string& path) const;
};

}  // namespace synera
