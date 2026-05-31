#include <catch2/catch_test_macros.hpp>

#include "board/HexGrid.hpp"
#include "core/GameState.hpp"
#include "systems/SaveSystem.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

[[nodiscard]] synera::AxialPos pos(int col, int row) noexcept {
    return synera::hex::oddRToAxial(synera::OffsetPos{col, row});
}

[[nodiscard]] std::filesystem::path savePath(std::string_view name) {
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("synera_" + std::string{name} + "_" + std::to_string(ticks) + ".json");
}

}  // namespace

TEST_CASE("SaveSystem round-trips core game state through JSON", "[save]") {
    synera::GameState state;
    state.player().hp = 73;
    state.player().gold = 11;
    state.player().level = 2;
    state.player().populationCap = 4;
    state.player().currentRound = 5;

    const synera::UnitId guardId = state.createUnit("iron_guard", synera::Owner::PlayerCtrl);
    const synera::UnitId mageId = state.createUnit("ember_mage", synera::Owner::PlayerCtrl);
    REQUIRE(state.placeUnitOnBoard(guardId, pos(0, 4)));
    REQUIRE(state.placeUnitOnBench(mageId, 1));

    auto* guard = state.findUnit(guardId);
    REQUIRE(guard != nullptr);
    guard->star = 2;
    guard->equipment = synera::EquipmentType::IronSword;
    guard->recomputeDerivedStats();
    guard->runtime.hp = 333;
    guard->runtime.mana = 12;
    guard->runtime.state = synera::UnitState::Attacking;
    guard->runtime.attackTimer = 0.25F;
    guard->runtime.combatStartPos = guard->boardPos;

    auto* mage = state.findUnit(mageId);
    REQUIRE(mage != nullptr);
    mage->equipment = synera::EquipmentType::ManaCrystal;
    mage->recomputeDerivedStats();
    mage->runtime.hp = 101;
    mage->runtime.mana = 20;
    mage->runtime.state = synera::UnitState::Stunned;
    mage->runtime.stunTimer = 0.5F;

    state.addEquipment(synera::EquipmentType::ChainVest);
    state.addEquipment(synera::EquipmentType::SwiftGlove);

    synera::Shop::Offers offers{};
    offers[0] = synera::ShopOffer{.unitTemplateId = "iron_guard", .cost = 1, .tier = 1};
    offers[1] = synera::ShopOffer{.unitTemplateId = "ember_mage", .cost = 2, .tier = 2};
    state.shop().replaceOffers(offers);
    state.shop().setLocked(true);

    const synera::SaveSystem saveSystem;
    const auto path = savePath("roundtrip");
    REQUIRE(saveSystem.save(state, path.string()).has_value());

    auto loadedResult = saveSystem.load(path.string());
    REQUIRE(loadedResult.has_value());
    synera::GameState loaded = std::move(*loadedResult);
    std::filesystem::remove(path);

    CHECK(loaded.phase() == synera::Phase::Prep);
    CHECK(loaded.player().hp == 73);
    CHECK(loaded.player().gold == 11);
    CHECK(loaded.player().level == 2);
    CHECK(loaded.player().populationCap == 4);
    CHECK(loaded.player().currentRound == 5);

    CHECK(loaded.equipmentPool().size() == 2);
    CHECK(loaded.equipmentPool()[0] == synera::EquipmentType::ChainVest);
    CHECK(loaded.equipmentPool()[1] == synera::EquipmentType::SwiftGlove);

    CHECK(loaded.shop().locked());
    CHECK(loaded.shop().offers()[0].unitTemplateId == "iron_guard");
    CHECK(loaded.shop().offers()[1].unitTemplateId == "ember_mage");

    CHECK(loaded.boardOccupant(pos(0, 4)) == guardId);
    CHECK(loaded.benchOccupant(1) == mageId);

    const auto* loadedGuard = loaded.findUnit(guardId);
    REQUIRE(loadedGuard != nullptr);
    CHECK(loadedGuard->templateId == "iron_guard");
    CHECK(loadedGuard->star == 2);
    CHECK(loadedGuard->equipment == synera::EquipmentType::IronSword);
    CHECK(loadedGuard->runtime.hp == 333);
    CHECK(loadedGuard->runtime.mana == 12);
    CHECK(loadedGuard->runtime.state == synera::UnitState::Attacking);
    CHECK(loadedGuard->runtime.attackTimer == 0.25F);
    CHECK(loadedGuard->runtime.combatStartPos == pos(0, 4));
    CHECK(loadedGuard->derivedStats.atk == 69);

    const auto* loadedMage = loaded.findUnit(mageId);
    REQUIRE(loadedMage != nullptr);
    CHECK(loadedMage->equipment == synera::EquipmentType::ManaCrystal);
    CHECK(loadedMage->runtime.hp == 101);
    CHECK(loadedMage->runtime.mana == 20);
    CHECK(loadedMage->runtime.state == synera::UnitState::Stunned);
    CHECK(loadedMage->runtime.stunTimer == 0.5F);
    CHECK(loadedMage->derivedStats.maxMana == 30);
}

TEST_CASE("SaveSystem only allows saving during preparation", "[save]") {
    synera::GameState state;
    state.setPhase(synera::Phase::Combat);

    const synera::SaveSystem saveSystem;
    const auto path = savePath("combat_rejected");

    CHECK_FALSE(saveSystem.save(state, path.string()).has_value());
    CHECK_FALSE(std::filesystem::exists(path));
}

TEST_CASE("SaveSystem reports missing and malformed save files", "[save]") {
    const synera::SaveSystem saveSystem;
    const auto missingPath = savePath("missing");
    std::filesystem::remove(missingPath);

    CHECK_FALSE(saveSystem.load(missingPath.string()).has_value());

    const auto malformedPath = savePath("malformed");
    {
        std::ofstream out{malformedPath};
        out << "{ invalid json";
    }

    CHECK_FALSE(saveSystem.load(malformedPath.string()).has_value());
    std::filesystem::remove(malformedPath);
}
