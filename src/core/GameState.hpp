#pragma once

#include "app/GameConfig.hpp"
#include "board/Bench.hpp"
#include "board/Board.hpp"
#include "core/Player.hpp"
#include "core/Unit.hpp"

#include <array>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace synera {

class GameState {
public:
    Phase phase = Phase::Prep;
    Player player;
    Board board;
    Bench bench;
    std::unordered_map<UnitId, std::unique_ptr<Unit>> units;
    std::array<ShopOffer, config::ShopOfferCount> shopOffers{};
    std::vector<EquipmentType> equipmentPool;

    GameState();

    [[nodiscard]] Unit* findUnit(UnitId id);
    [[nodiscard]] const Unit* findUnit(UnitId id) const;
    [[nodiscard]] std::vector<Unit*> aliveUnitsByOwner(Owner owner);
    [[nodiscard]] std::vector<Unit*> playerBoardUnits();
    [[nodiscard]] std::vector<Unit*> enemyBoardUnits();
    [[nodiscard]] int playerBoardUnitCount() const;
    [[nodiscard]] bool isCombatFinished() const;
    [[nodiscard]] bool playerWonCombat() const;

    UnitId createUnit(std::string_view templateId, Owner owner);
    bool placeUnitOnBench(UnitId id, int slot);
    bool placeUnitOnBoard(UnitId id, GridPos pos);
    void removeUnitFromBoard(Unit& unit);
    void removeEnemyUnits();

private:
    UnitId nextUnitId_ = 1;
};

} // namespace synera
