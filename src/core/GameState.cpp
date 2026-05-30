#include "core/GameState.hpp"

#include "core/Contract.hpp"
#include "core/UnitCatalog.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

namespace synera {

namespace {

template <class... Visitors>
struct Overloaded : Visitors... {
    using Visitors::operator()...;
};

template <class... Visitors>
Overloaded(Visitors...) -> Overloaded<Visitors...>;

}  // namespace

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

Shop& GameState::shop() noexcept {
    return shop_;
}

const Shop& GameState::shop() const noexcept {
    return shop_;
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

bool GameState::restoreUnit(Unit unit) {
    if (unit.id == InvalidUnitId || units_.contains(unit.id)) {
        return false;
    }

    unit.boardPos.reset();
    unit.benchSlot.reset();
    unit.checkInvariants();
    nextUnitId_ = std::max(nextUnitId_, unit.id + 1);
    units_.emplace(unit.id, std::make_unique<Unit>(std::move(unit)));
    return true;
}

bool GameState::removeUnit(UnitId id) {
    Unit* unit = findUnit(id);
    if (unit == nullptr) {
        return false;
    }
    clearUnitLocation(*unit);
    units_.erase(id);
    return true;
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

    const UnitLocation destination = BenchLocation{.slot = slot};
    if (locationOf(*unit) == destination) {
        return PlacementResult::Ok;
    }

    const auto occupiedBy = occupantAt(destination);
    if (occupiedBy) {
        Unit* other = findUnit(*occupiedBy);
        if (other == nullptr || other->owner != Owner::PlayerCtrl) {
            return PlacementResult::Occupied;
        }
        return swapPlayerUnits(*unit, *other);
    }

    return moveUnitToEmptyLocation(*unit, destination);
}

PlacementResult GameState::placeUnitOnBoardResult(UnitId id, AxialPos pos) {
    Unit* unit = findUnit(id);
    if (unit == nullptr) {
        return PlacementResult::InvalidUnit;
    }
    if (unit->owner == Owner::PlayerCtrl && phase_ != Phase::Prep) {
        return PlacementResult::InvalidPhase;
    }
    const UnitLocation destination = BoardLocation{.pos = pos};
    const auto occupiedBy = occupantAt(destination);
    const PlacementResult validation = validateBoardPlacement(*unit, pos, !unit->onBoard() && !occupiedBy);
    if (validation != PlacementResult::Ok) {
        return validation;
    }
    if (locationOf(*unit) == destination) {
        return PlacementResult::Ok;
    }

    if (occupiedBy) {
        Unit* other = findUnit(*occupiedBy);
        if (other == nullptr || unit->owner != Owner::PlayerCtrl || other->owner != Owner::PlayerCtrl) {
            return PlacementResult::Occupied;
        }
        return swapPlayerUnits(*unit, *other);
    }

    return moveUnitToEmptyLocation(*unit, destination);
}

bool GameState::placeUnitOnBench(UnitId id, int slot) {
    return placeUnitOnBenchResult(id, slot) == PlacementResult::Ok;
}

bool GameState::placeUnitOnBoard(UnitId id, AxialPos pos) {
    return placeUnitOnBoardResult(id, pos) == PlacementResult::Ok;
}

GameState::UnitLocation GameState::locationOf(const Unit& unit) const noexcept {
    if (unit.boardPos) {
        return BoardLocation{.pos = *unit.boardPos};
    }
    if (unit.benchSlot) {
        return BenchLocation{.slot = *unit.benchSlot};
    }
    return std::monostate{};
}

std::optional<UnitId> GameState::occupantAt(const UnitLocation& location) const {
    return std::visit(Overloaded{
                          [](std::monostate) -> std::optional<UnitId> { return std::nullopt; },
                          [&](BoardLocation boardLocation) -> std::optional<UnitId> {
                              return board_.occupant(boardLocation.pos);
                          },
                          [&](BenchLocation benchLocation) -> std::optional<UnitId> {
                              return bench_.occupant(benchLocation.slot);
                          },
                      },
                      location);
}

bool GameState::placeDetachedUnit(Unit& unit, const UnitLocation& location) {
    unit.boardPos.reset();
    unit.benchSlot.reset();

    const bool placed = std::visit(Overloaded{
                                       [](std::monostate) { return true; },
                                       [&](BoardLocation boardLocation) {
                                           if (!board_.place(unit.id, boardLocation.pos)) {
                                               return false;
                                           }
                                           unit.boardPos = boardLocation.pos;
                                           return true;
                                       },
                                       [&](BenchLocation benchLocation) {
                                           if (!bench_.place(unit.id, benchLocation.slot)) {
                                               return false;
                                           }
                                           unit.benchSlot = benchLocation.slot;
                                           return true;
                                       },
                                   },
                                   location);
    if (!placed) {
        return false;
    }

    unit.checkInvariants();
    return true;
}

void GameState::clearUnitLocation(Unit& unit) {
    if (unit.boardPos) {
        board_.remove(*unit.boardPos);
    }
    if (unit.benchSlot) {
        bench_.remove(*unit.benchSlot);
    }
    unit.boardPos.reset();
    unit.benchSlot.reset();
    unit.checkInvariants();
}

PlacementResult GameState::moveUnitToEmptyLocation(Unit& unit, UnitLocation destination) {
    if (locationOf(unit) == destination) {
        return PlacementResult::Ok;
    }

    clearUnitLocation(unit);
    if (!placeDetachedUnit(unit, destination)) {
        return PlacementResult::InvalidPosition;
    }
    return PlacementResult::Ok;
}

PlacementResult GameState::swapPlayerUnits(Unit& left, Unit& right) {
    const UnitLocation leftLocation = locationOf(left);
    const UnitLocation rightLocation = locationOf(right);
    if (std::holds_alternative<std::monostate>(leftLocation) ||
        std::holds_alternative<std::monostate>(rightLocation)) {
        return PlacementResult::InvalidPosition;
    }

    const auto* leftBoard = std::get_if<BoardLocation>(&leftLocation);
    const auto* rightBoard = std::get_if<BoardLocation>(&rightLocation);
    if (leftBoard != nullptr && rightBoard != nullptr) {
        if (!board_.swapCells(leftBoard->pos, rightBoard->pos)) {
            return PlacementResult::InvalidPosition;
        }
        left.boardPos = rightBoard->pos;
        left.benchSlot.reset();
        right.boardPos = leftBoard->pos;
        right.benchSlot.reset();
        left.checkInvariants();
        right.checkInvariants();
        return PlacementResult::Ok;
    }

    const auto* leftBench = std::get_if<BenchLocation>(&leftLocation);
    const auto* rightBench = std::get_if<BenchLocation>(&rightLocation);
    if (leftBench != nullptr && rightBench != nullptr) {
        if (!bench_.swapSlots(leftBench->slot, rightBench->slot)) {
            return PlacementResult::InvalidPosition;
        }
        left.benchSlot = rightBench->slot;
        left.boardPos.reset();
        right.benchSlot = leftBench->slot;
        right.boardPos.reset();
        left.checkInvariants();
        right.checkInvariants();
        return PlacementResult::Ok;
    }

    clearUnitLocation(left);
    clearUnitLocation(right);
    const bool placedLeft = placeDetachedUnit(left, rightLocation);
    const bool placedRight = placeDetachedUnit(right, leftLocation);
    SYNERA_ENSURES(placedLeft && placedRight);
    return placedLeft && placedRight ? PlacementResult::Ok : PlacementResult::Occupied;
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

void GameState::restorePlayerUnitsAfterCombat() {
    std::vector<Unit*> returningUnits;
    forEachUnit([&](Unit& unit) {
        if (unit.owner == Owner::PlayerCtrl && unit.runtime.combatStartPos) {
            clearUnitLocation(unit);
            returningUnits.push_back(&unit);
        }
    });

    for (Unit* unit : returningUnits) {
        const AxialPos returnPos = *unit->runtime.combatStartPos;
        unit->resetForCombat();
        const bool placed = placeDetachedUnit(*unit, BoardLocation{.pos = returnPos});
        SYNERA_ENSURES(placed);
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
