#pragma once

#include "config/GameConfig.hpp"
#include "board/Bench.hpp"
#include "board/Board.hpp"
#include "core/Placement.hpp"
#include "core/Player.hpp"
#include "core/Shop.hpp"
#include "core/Unit.hpp"

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace synera {

// Central mutable model for rules and persistence; UI state such as dragging stays outside.
class GameState {
public:
    using UnitMap = std::unordered_map<UnitId, std::unique_ptr<Unit>>;

    GameState();

    [[nodiscard]] Phase phase() const noexcept;
    void setPhase(Phase phase) noexcept;

    [[nodiscard]] Player& player() noexcept;
    [[nodiscard]] const Player& player() const noexcept;
    [[nodiscard]] const Board& board() const noexcept;
    [[nodiscard]] const Bench& bench() const noexcept;
    [[nodiscard]] Shop& shop() noexcept;
    [[nodiscard]] const Shop& shop() const noexcept;
    [[nodiscard]] std::span<EquipmentType> equipmentPool() noexcept;
    [[nodiscard]] std::span<const EquipmentType> equipmentPool() const noexcept;
    void addEquipment(EquipmentType equipment);
    bool removeEquipmentAt(std::size_t index);

    [[nodiscard]] Unit* findUnit(UnitId id);
    [[nodiscard]] const Unit* findUnit(UnitId id) const;
    template <std::invocable<Unit&> Visitor>
    void forEachUnit(Visitor&& visitor) {
        for (auto& unit : units_ | std::views::values) {
            std::invoke(std::forward<Visitor>(visitor), *unit);
        }
    }

    template <std::invocable<const Unit&> Visitor>
    void forEachUnit(Visitor&& visitor) const {
        for (const auto& unit : units_ | std::views::values) {
            std::invoke(std::forward<Visitor>(visitor), *unit);
        }
    }

    template <std::invocable<Unit&> Visitor>
    void forEachAliveUnitByOwner(Owner owner, Visitor&& visitor) {
        forEachUnit([&](Unit& unit) {
            if (unit.owner == owner && unit.alive()) {
                std::invoke(visitor, unit);
            }
        });
    }

    template <std::invocable<const Unit&> Visitor>
    void forEachAliveUnitByOwner(Owner owner, Visitor&& visitor) const {
        forEachUnit([&](const Unit& unit) {
            if (unit.owner == owner && unit.alive()) {
                std::invoke(visitor, unit);
            }
        });
    }

    template <std::invocable<Unit&> Visitor>
    void forEachPlayerBoardUnit(Visitor&& visitor) {
        forEachUnit([&](Unit& unit) {
            if (unit.owner == Owner::PlayerCtrl && unit.onBoard()) {
                std::invoke(visitor, unit);
            }
        });
    }

    template <std::invocable<const Unit&> Visitor>
    void forEachPlayerBoardUnit(Visitor&& visitor) const {
        forEachUnit([&](const Unit& unit) {
            if (unit.owner == Owner::PlayerCtrl && unit.onBoard()) {
                std::invoke(visitor, unit);
            }
        });
    }

    template <std::invocable<Unit&> Visitor>
    void forEachEnemyBoardUnit(Visitor&& visitor) {
        forEachUnit([&](Unit& unit) {
            if (unit.owner == Owner::EnemyCtrl && unit.onBoard()) {
                std::invoke(visitor, unit);
            }
        });
    }

    template <std::invocable<const Unit&> Visitor>
    void forEachEnemyBoardUnit(Visitor&& visitor) const {
        forEachUnit([&](const Unit& unit) {
            if (unit.owner == Owner::EnemyCtrl && unit.onBoard()) {
                std::invoke(visitor, unit);
            }
        });
    }

    [[nodiscard]] int playerBoardUnitCount() const;
    [[nodiscard]] bool isCombatFinished() const;
    [[nodiscard]] bool playerWonCombat() const;
    [[nodiscard]] std::optional<UnitId> boardOccupant(AxialPos pos) const;
    [[nodiscard]] std::optional<UnitId> benchOccupant(int slot) const;
    [[nodiscard]] std::optional<int> firstEmptyBenchSlot() const;

    UnitId createUnit(std::string_view templateId, Owner owner);
    // Used by persistence to reinsert catalog-built units with stable ids before placement is restored.
    bool restoreUnit(Unit unit);
    bool removeUnit(UnitId id);
    // Placement methods are the only supported way to keep board/bench occupancy and Unit location in sync.
    PlacementResult placeUnitOnBenchResult(UnitId id, int slot);
    PlacementResult placeUnitOnBoardResult(UnitId id, AxialPos pos);
    bool placeUnitOnBench(UnitId id, int slot);
    bool placeUnitOnBoard(UnitId id, AxialPos pos);
    bool moveBoardUnit(UnitId id, AxialPos pos);
    void removeUnitFromBoard(Unit& unit);
    void restorePlayerUnitsAfterCombat();
    void removeEnemyUnits();

private:
    struct BoardLocation {
        AxialPos pos{};

        friend bool operator==(const BoardLocation&, const BoardLocation&) = default;
    };

    struct BenchLocation {
        int slot = -1;

        friend bool operator==(const BenchLocation&, const BenchLocation&) = default;
    };

    using UnitLocation = std::variant<std::monostate, BoardLocation, BenchLocation>;

    Phase phase_ = Phase::Prep;
    Player player_;
    Board board_;
    Bench bench_;
    UnitMap units_;
    Shop shop_;
    std::vector<EquipmentType> equipmentPool_;
    UnitId nextUnitId_ = 1;

    [[nodiscard]] PlacementResult validateBoardPlacement(const Unit& unit, AxialPos pos,
                                                         bool enteringEmptyBoard) const;
    [[nodiscard]] UnitLocation locationOf(const Unit& unit) const noexcept;
    [[nodiscard]] std::optional<UnitId> occupantAt(const UnitLocation& location) const;
    [[nodiscard]] bool placeDetachedUnit(Unit& unit, const UnitLocation& location);
    void clearUnitLocation(Unit& unit);
    [[nodiscard]] PlacementResult moveUnitToEmptyLocation(Unit& unit, UnitLocation destination);
    [[nodiscard]] PlacementResult swapPlayerUnits(Unit& left, Unit& right);
};

}  // namespace synera
