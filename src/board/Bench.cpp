#include "board/Bench.hpp"

#include "core/Contract.hpp"

namespace synera {

Bench::Bench(int size) : slots_(size) {
    SYNERA_EXPECTS(size > 0);
}

int Bench::size() const noexcept {
    return static_cast<int>(slots_.size());
}

std::optional<UnitId> Bench::occupant(int slot) const {
    if (!inBounds(slot)) {
        return std::nullopt;
    }
    return slots_[slot];
}

bool Bench::empty(int slot) const {
    return inBounds(slot) && !slots_[slot].has_value();
}

std::optional<int> Bench::firstEmptySlot() const {
    for (int slot = 0; slot < size(); ++slot) {
        if (empty(slot)) {
            return slot;
        }
    }
    return std::nullopt;
}

bool Bench::place(UnitId unitId, int slot) {
    SYNERA_EXPECTS(unitId != InvalidUnitId);
    if (!empty(slot)) {
        return false;
    }
    slots_[slot] = unitId;
    return true;
}

void Bench::remove(int slot) {
    if (!inBounds(slot)) {
        return;
    }
    slots_[slot].reset();
}

bool Bench::swapSlots(int left, int right) {
    if (!inBounds(left) || !inBounds(right)) {
        return false;
    }
    std::swap(slots_[left], slots_[right]);
    return true;
}

bool Bench::inBounds(int slot) const noexcept {
    return slot >= 0 && slot < size();
}

} // namespace synera
