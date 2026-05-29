#pragma once

#include "app/GameConfig.hpp"
#include "board/Bench.hpp"
#include "board/Board.hpp"
#include "core/Player.hpp"
#include "core/Unit.hpp"

#include <array>
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
#include <vector>

namespace synera {

class GameState {
public:
    using UnitMap = std::unordered_map<UnitId, std::unique_ptr<Unit>>;
    using ShopOffers = std::array<ShopOffer, config::ShopOfferCount>;

    GameState();

    [[nodiscard]] Phase phase() const noexcept;
    void setPhase(Phase phase) noexcept;

    [[nodiscard]] Player& player() noexcept;
    [[nodiscard]] const Player& player() const noexcept;
    [[nodiscard]] const Board& board() const noexcept;
    [[nodiscard]] const Bench& bench() const noexcept;
    [[nodiscard]] std::span<ShopOffer, config::ShopOfferCount> shopOffers() noexcept;
    [[nodiscard]] std::span<const ShopOffer, config::ShopOfferCount> shopOffers() const noexcept;
    [[nodiscard]] std::span<EquipmentType> equipmentPool() noexcept;
    [[nodiscard]] std::span<const EquipmentType> equipmentPool() const noexcept;
    void addEquipment(EquipmentType equipment);
    bool removeEquipmentAt(std::size_t index);

    [[nodiscard]] Unit* findUnit(UnitId id);
    [[nodiscard]] const Unit* findUnit(UnitId id) const;
    template <std::invocable<Unit&> Visitor> void forEachUnit(Visitor&& visitor) {
        for (auto& unit : units_ | std::views::values) {
            std::invoke(std::forward<Visitor>(visitor), *unit);
        }
    }

    template <std::invocable<const Unit&> Visitor> void forEachUnit(Visitor&& visitor) const {
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

    template <std::invocable<Unit&> Visitor> void forEachPlayerBoardUnit(Visitor&& visitor) {
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

    template <std::invocable<Unit&> Visitor> void forEachEnemyBoardUnit(Visitor&& visitor) {
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
    [[nodiscard]] std::optional<UnitId> benchOccupant(int slot) const;
    [[nodiscard]] std::optional<int> firstEmptyBenchSlot() const;

    UnitId createUnit(std::string_view templateId, Owner owner);
    bool placeUnitOnBench(UnitId id, int slot);
    bool placeUnitOnBoard(UnitId id, GridPos pos);
    void removeUnitFromBoard(Unit& unit);
    void removeEnemyUnits();

private:
    Phase phase_ = Phase::Prep;
    Player player_;
    Board board_;
    Bench bench_;
    UnitMap units_;
    ShopOffers shopOffers_{};
    std::vector<EquipmentType> equipmentPool_;
    UnitId nextUnitId_ = 1;
};

} // namespace synera
