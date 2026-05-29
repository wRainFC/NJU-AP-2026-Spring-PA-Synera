#pragma once

#include "core/Types.hpp"

#include <optional>
#include <vector>

namespace synera {

class Bench {
public:
    explicit Bench(int size);

    [[nodiscard]] int size() const noexcept;
    [[nodiscard]] std::optional<UnitId> occupant(int slot) const;
    [[nodiscard]] bool empty(int slot) const;
    [[nodiscard]] std::optional<int> firstEmptySlot() const;

    bool place(UnitId unitId, int slot);
    void remove(int slot);
    bool swapSlots(int left, int right);

private:
    std::vector<std::optional<UnitId>> slots_;

    [[nodiscard]] bool inBounds(int slot) const noexcept;
};

} // namespace synera
