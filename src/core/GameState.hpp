#pragma once

#include "app/GameConfig.hpp"
#include "board/Bench.hpp"
#include "board/Board.hpp"
#include "core/Player.hpp"
#include "core/Unit.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
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
    [[nodiscard]] ShopOffers& shopOffers() noexcept;
    [[nodiscard]] const ShopOffers& shopOffers() const noexcept;
    [[nodiscard]] std::vector<EquipmentType>& equipmentPool() noexcept;
    [[nodiscard]] const std::vector<EquipmentType>& equipmentPool() const noexcept;

    [[nodiscard]] Unit* findUnit(UnitId id);
    [[nodiscard]] const Unit* findUnit(UnitId id) const;
    [[nodiscard]] std::vector<Unit*> allUnits();
    [[nodiscard]] std::vector<const Unit*> allUnits() const;
    [[nodiscard]] std::vector<Unit*> aliveUnitsByOwner(Owner owner);
    [[nodiscard]] std::vector<Unit*> playerBoardUnits();
    [[nodiscard]] std::vector<Unit*> enemyBoardUnits();
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
