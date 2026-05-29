#include "core/GameState.hpp"

#include "core/Ability.hpp"
#include "core/Contract.hpp"

#include <algorithm>
#include <ranges>

namespace synera {

namespace {

std::string displayNameFor(std::string_view templateId) {
    if (templateId == "ember_mage") {
        return "Ember Mage";
    }
    if (templateId == "iron_guard") {
        return "Iron Guard";
    }
    if (templateId == "training_dummy") {
        return "Training Dummy";
    }
    return "Unknown";
}

UnitStats statsFor(std::string_view templateId) {
    if (templateId == "ember_mage") {
        return UnitStats{220, 220, 42, 3, 60, 0, 1.2F, 0.25F};
    }
    if (templateId == "training_dummy") {
        return UnitStats{180, 180, 18, 1, 80, 0, 1.4F, 0.3F};
    }
    return UnitStats{360, 360, 32, 1, 70, 0, 1.0F, 0.25F};
}

std::unique_ptr<Ability> abilityFor(std::string_view templateId) {
    if (templateId == "ember_mage") {
        return std::make_unique<FireLineAbility>();
    }
    return std::make_unique<NoopAbility>();
}

} // namespace

GameState::GameState() : board(config::BoardWidth, config::BoardHeight), bench(config::BenchSize) {}

Unit* GameState::findUnit(UnitId id) {
    const auto iter = units.find(id);
    return iter == units.end() ? nullptr : iter->second.get();
}

const Unit* GameState::findUnit(UnitId id) const {
    const auto iter = units.find(id);
    return iter == units.end() ? nullptr : iter->second.get();
}

std::vector<Unit*> GameState::aliveUnitsByOwner(Owner owner) {
    std::vector<Unit*> result;
    for (auto& [id, unit] : units) {
        (void)id;
        if (unit->owner == owner && unit->alive()) {
            result.push_back(unit.get());
        }
    }
    return result;
}

std::vector<Unit*> GameState::playerBoardUnits() {
    std::vector<Unit*> result;
    for (auto& [id, unit] : units) {
        (void)id;
        if (unit->owner == Owner::PlayerCtrl && unit->onBoard()) {
            result.push_back(unit.get());
        }
    }
    return result;
}

std::vector<Unit*> GameState::enemyBoardUnits() {
    std::vector<Unit*> result;
    for (auto& [id, unit] : units) {
        (void)id;
        if (unit->owner == Owner::EnemyCtrl && unit->onBoard()) {
            result.push_back(unit.get());
        }
    }
    return result;
}

int GameState::playerBoardUnitCount() const {
    return static_cast<int>(std::ranges::count_if(units, [](const auto& entry) {
        const auto& unit = entry.second;
        return unit->owner == Owner::PlayerCtrl && unit->onBoard() && unit->alive();
    }));
}

bool GameState::isCombatFinished() const {
    if (phase != Phase::Combat) {
        return false;
    }

    const bool hasPlayers = std::ranges::any_of(units, [](const auto& entry) {
        const auto& unit = entry.second;
        return unit->owner == Owner::PlayerCtrl && unit->onBoard() && unit->alive();
    });
    const bool hasEnemies = std::ranges::any_of(units, [](const auto& entry) {
        const auto& unit = entry.second;
        return unit->owner == Owner::EnemyCtrl && unit->onBoard() && unit->alive();
    });
    return !hasPlayers || !hasEnemies;
}

bool GameState::playerWonCombat() const {
    const bool hasEnemies = std::ranges::any_of(units, [](const auto& entry) {
        const auto& unit = entry.second;
        return unit->owner == Owner::EnemyCtrl && unit->onBoard() && unit->alive();
    });
    return !hasEnemies;
}

UnitId GameState::createUnit(std::string_view templateId, Owner owner) {
    const UnitId id = nextUnitId_++;
    auto unit = std::make_unique<Unit>();
    unit->id = id;
    unit->templateId = std::string(templateId);
    unit->name = displayNameFor(templateId);
    unit->owner = owner;
    unit->baseStats = statsFor(templateId);
    unit->currentStats = unit->baseStats;
    unit->ability = abilityFor(templateId);
    unit->traits = owner == Owner::PlayerCtrl ? std::vector<Trait>{Trait::Warrior}
                                              : std::vector<Trait>{Trait::Guardian};
    units.emplace(id, std::move(unit));
    return id;
}

bool GameState::placeUnitOnBench(UnitId id, int slot) {
    Unit* unit = findUnit(id);
    if (unit == nullptr || unit->owner != Owner::PlayerCtrl || !bench.empty(slot)) {
        return false;
    }
    if (unit->boardPos) {
        board.remove(*unit->boardPos);
        unit->boardPos.reset();
    }
    if (unit->benchSlot) {
        bench.remove(*unit->benchSlot);
    }
    if (!bench.place(id, slot)) {
        return false;
    }
    unit->benchSlot = slot;
    unit->checkInvariants();
    return true;
}

bool GameState::placeUnitOnBoard(UnitId id, GridPos pos) {
    Unit* unit = findUnit(id);
    if (unit == nullptr || !board.empty(pos)) {
        return false;
    }
    if (unit->owner == Owner::PlayerCtrl && !board.isPlayerHalf(pos)) {
        return false;
    }
    if (unit->owner == Owner::EnemyCtrl && !board.isEnemyHalf(pos)) {
        return false;
    }
    if (unit->owner == Owner::PlayerCtrl && !unit->onBoard() &&
        playerBoardUnitCount() >= player.populationCap) {
        return false;
    }

    if (unit->benchSlot) {
        bench.remove(*unit->benchSlot);
        unit->benchSlot.reset();
    }
    if (unit->boardPos) {
        board.remove(*unit->boardPos);
    }
    if (!board.place(id, pos)) {
        return false;
    }
    unit->boardPos = pos;
    unit->checkInvariants();
    return true;
}

void GameState::removeUnitFromBoard(Unit& unit) {
    if (unit.boardPos) {
        board.remove(*unit.boardPos);
        unit.boardPos.reset();
    }
}

void GameState::removeEnemyUnits() {
    std::vector<UnitId> toErase;
    for (auto& [id, unit] : units) {
        if (unit->owner == Owner::EnemyCtrl) {
            if (unit->boardPos) {
                board.remove(*unit->boardPos);
            }
            toErase.push_back(id);
        }
    }
    for (UnitId id : toErase) {
        units.erase(id);
    }
}

} // namespace synera
