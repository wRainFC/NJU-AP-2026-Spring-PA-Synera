#include "core/GameState.hpp"

#include "core/Contract.hpp"
#include "core/UnitCatalog.hpp"

#include <algorithm>
#include <cstddef>

namespace synera {

GameState::GameState() : board_(config::BoardWidth, config::BoardHeight), bench_(config::BenchSize) {}

Phase GameState::phase() const noexcept {
    return phase_;
}

void GameState::setPhase(Phase phase) noexcept {
    phase_ = phase;
}

Player& GameState::player() noexcept {
    return player_;
}

const Player& GameState::player() const noexcept {
    return player_;
}

const Board& GameState::board() const noexcept {
    return board_;
}

const Bench& GameState::bench() const noexcept {
    return bench_;
}

std::span<ShopOffer, config::ShopOfferCount> GameState::shopOffers() noexcept {
    return shopOffers_;
}

std::span<const ShopOffer, config::ShopOfferCount> GameState::shopOffers() const noexcept {
    return shopOffers_;
}

std::span<EquipmentType> GameState::equipmentPool() noexcept {
    return equipmentPool_;
}

std::span<const EquipmentType> GameState::equipmentPool() const noexcept {
    return equipmentPool_;
}

void GameState::addEquipment(EquipmentType equipment) {
    equipmentPool_.push_back(equipment);
}

bool GameState::removeEquipmentAt(std::size_t index) {
    if (index >= equipmentPool_.size()) {
        return false;
    }
    equipmentPool_.erase(equipmentPool_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

Unit* GameState::findUnit(UnitId id) {
    const auto iter = units_.find(id);
    return iter == units_.end() ? nullptr : iter->second.get();
}

const Unit* GameState::findUnit(UnitId id) const {
    const auto iter = units_.find(id);
    return iter == units_.end() ? nullptr : iter->second.get();
}

int GameState::playerBoardUnitCount() const {
    int count = 0;
    forEachUnit([&](const Unit& unit) {
        if (unit.owner == Owner::PlayerCtrl && unit.onBoard()) {
            ++count;
        }
    });
    return count;
}

bool GameState::isCombatFinished() const {
    if (phase_ != Phase::Combat) {
        return false;
    }

    bool hasPlayers = false;
    bool hasEnemies = false;
    forEachUnit([&](const Unit& unit) {
        hasPlayers = hasPlayers || (unit.owner == Owner::PlayerCtrl && unit.onBoard() && unit.alive());
        hasEnemies = hasEnemies || (unit.owner == Owner::EnemyCtrl && unit.onBoard() && unit.alive());
    });
    return !hasPlayers || !hasEnemies;
}

bool GameState::playerWonCombat() const {
    bool hasEnemies = false;
    forEachUnit([&](const Unit& unit) {
        hasEnemies = hasEnemies || (unit.owner == Owner::EnemyCtrl && unit.onBoard() && unit.alive());
    });
    return !hasEnemies;
}

std::optional<UnitId> GameState::boardOccupant(AxialPos pos) const {
    return board_.occupant(pos);
}

std::optional<UnitId> GameState::benchOccupant(int slot) const {
    return bench_.occupant(slot);
}

std::optional<int> GameState::firstEmptyBenchSlot() const {
    return bench_.firstEmptySlot();
}

UnitId GameState::createUnit(std::string_view templateId, Owner owner) {
    const UnitId id = nextUnitId_++;
    auto unit = std::make_unique<Unit>(UnitCatalog::createUnit(id, templateId, owner));
    units_.emplace(id, std::move(unit));
    return id;
}

PlacementResult GameState::placeUnitOnBenchResult(UnitId id, int slot) {
    Unit* unit = findUnit(id);
    if (unit == nullptr) {
        return PlacementResult::InvalidUnit;
    }
    if (unit->owner != Owner::PlayerCtrl) {
        return PlacementResult::InvalidOwner;
    }
    if (phase_ != Phase::Prep) {
        return PlacementResult::InvalidPhase;
    }
    if (slot < 0 || slot >= bench_.size()) {
        return PlacementResult::InvalidPosition;
    }
    if (unit->benchSlot == slot) {
        return PlacementResult::Ok;
    }

    const auto occupiedBy = bench_.occupant(slot);
    if (occupiedBy) {
        Unit* other = findUnit(*occupiedBy);
        if (other == nullptr || other->owner != Owner::PlayerCtrl) {
            return PlacementResult::Occupied;
        }
        if (unit->benchSlot) {
            const int sourceSlot = *unit->benchSlot;
            if (!bench_.swapSlots(sourceSlot, slot)) {
                return PlacementResult::InvalidPosition;
            }
            unit->benchSlot = slot;
            other->benchSlot = sourceSlot;
            unit->checkInvariants();
            other->checkInvariants();
            return PlacementResult::Ok;
        }
        if (unit->boardPos) {
            const AxialPos sourcePos = *unit->boardPos;
            board_.remove(sourcePos);
            bench_.remove(slot);
            if (!board_.place(other->id, sourcePos) || !bench_.place(unit->id, slot)) {
                return PlacementResult::Occupied;
            }
            unit->boardPos.reset();
            unit->benchSlot = slot;
            other->benchSlot.reset();
            other->boardPos = sourcePos;
            unit->checkInvariants();
            other->checkInvariants();
            return PlacementResult::Ok;
        }
        return PlacementResult::InvalidPosition;
    }

    if (unit->boardPos) {
        board_.remove(*unit->boardPos);
        unit->boardPos.reset();
    }
    if (unit->benchSlot) {
        bench_.remove(*unit->benchSlot);
    }
    if (!bench_.place(id, slot)) {
        return PlacementResult::InvalidPosition;
    }
    unit->benchSlot = slot;
    unit->checkInvariants();
    return PlacementResult::Ok;
}

PlacementResult GameState::placeUnitOnBoardResult(UnitId id, AxialPos pos) {
    Unit* unit = findUnit(id);
    if (unit == nullptr) {
        return PlacementResult::InvalidUnit;
    }
    if (unit->owner == Owner::PlayerCtrl && phase_ != Phase::Prep) {
        return PlacementResult::InvalidPhase;
    }
    const auto occupiedBy = board_.occupant(pos);
    const PlacementResult validation = validateBoardPlacement(*unit, pos, !unit->onBoard() && !occupiedBy);
    if (validation != PlacementResult::Ok) {
        return validation;
    }
    if (unit->boardPos == pos) {
        return PlacementResult::Ok;
    }

    if (occupiedBy) {
        Unit* other = findUnit(*occupiedBy);
        if (other == nullptr || unit->owner != Owner::PlayerCtrl || other->owner != Owner::PlayerCtrl) {
            return PlacementResult::Occupied;
        }
        if (unit->boardPos) {
            const AxialPos sourcePos = *unit->boardPos;
            if (!board_.swapCells(sourcePos, pos)) {
                return PlacementResult::InvalidPosition;
            }
            unit->boardPos = pos;
            other->boardPos = sourcePos;
            unit->checkInvariants();
            other->checkInvariants();
            return PlacementResult::Ok;
        }
        if (unit->benchSlot) {
            const int sourceSlot = *unit->benchSlot;
            bench_.remove(sourceSlot);
            board_.remove(pos);
            if (!board_.place(unit->id, pos) || !bench_.place(other->id, sourceSlot)) {
                return PlacementResult::Occupied;
            }
            unit->benchSlot.reset();
            unit->boardPos = pos;
            other->boardPos.reset();
            other->benchSlot = sourceSlot;
            unit->checkInvariants();
            other->checkInvariants();
            return PlacementResult::Ok;
        }
        return PlacementResult::Occupied;
    }

    if (unit->benchSlot) {
        bench_.remove(*unit->benchSlot);
        unit->benchSlot.reset();
    }
    if (unit->boardPos) {
        board_.remove(*unit->boardPos);
    }
    if (!board_.place(id, pos)) {
        return PlacementResult::InvalidPosition;
    }
    unit->boardPos = pos;
    unit->checkInvariants();
    return PlacementResult::Ok;
}

bool GameState::placeUnitOnBench(UnitId id, int slot) {
    return placeUnitOnBenchResult(id, slot) == PlacementResult::Ok;
}

bool GameState::placeUnitOnBoard(UnitId id, AxialPos pos) {
    return placeUnitOnBoardResult(id, pos) == PlacementResult::Ok;
}

bool GameState::moveBoardUnit(UnitId id, AxialPos pos) {
    Unit* unit = findUnit(id);
    if (unit == nullptr || !unit->boardPos || !board_.empty(pos)) {
        return false;
    }
    const AxialPos source = *unit->boardPos;
    if (!board_.move(source, pos)) {
        return false;
    }
    unit->boardPos = pos;
    unit->runtime.state = UnitState::Moving;
    unit->checkInvariants();
    return true;
}

void GameState::removeUnitFromBoard(Unit& unit) {
    if (unit.boardPos) {
        board_.remove(*unit.boardPos);
        unit.boardPos.reset();
    }
}

void GameState::removeEnemyUnits() {
    std::erase_if(units_, [this](auto& entry) {
        auto& [id, unit] = entry;
        (void)id;
        if (unit->owner == Owner::EnemyCtrl) {
            if (unit->boardPos) {
                board_.remove(*unit->boardPos);
            }
            return true;
        }
        return false;
    });
}

PlacementResult GameState::validateBoardPlacement(const Unit& unit, AxialPos pos,
                                                  bool enteringEmptyBoard) const {
    if (!board_.inBounds(pos)) {
        return PlacementResult::InvalidPosition;
    }
    if (unit.owner == Owner::PlayerCtrl && !board_.isPlayerHalf(pos)) {
        return PlacementResult::InvalidHalf;
    }
    if (unit.owner == Owner::EnemyCtrl && !board_.isEnemyHalf(pos)) {
        return PlacementResult::InvalidHalf;
    }
    if (unit.owner == Owner::PlayerCtrl && enteringEmptyBoard &&
        playerBoardUnitCount() >= player_.populationCap) {
        return PlacementResult::PopulationFull;
    }
    return PlacementResult::Ok;
}

}  // namespace synera
