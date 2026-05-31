#include "systems/SaveSystem.hpp"

#include "core/GameState.hpp"
#include "core/UnitCatalog.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <array>
#include <expected>
#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace synera {

namespace {

constexpr int SaveVersion = 1;

constexpr auto KnownOwners = std::array{Owner::PlayerCtrl, Owner::EnemyCtrl};
constexpr auto KnownPhases = std::array{Phase::Prep, Phase::Combat, Phase::Resolve};
constexpr auto KnownUnitStates = std::array{UnitState::Idle,    UnitState::Moving,  UnitState::Attacking,
                                            UnitState::Casting, UnitState::Stunned, UnitState::Dead};
constexpr auto KnownEquipmentTypes = std::array{EquipmentType::IronSword, EquipmentType::ChainVest,
                                                EquipmentType::SwiftGlove, EquipmentType::ManaCrystal};

template <typename Enum>
[[nodiscard]] constexpr int enumToInt(Enum value) noexcept {
    return static_cast<int>(value);
}

template <typename Enum, std::size_t Size>
[[nodiscard]] std::expected<Enum, std::string> enumFromInt(int value,
                                                           const std::array<Enum, Size>& knownValues,
                                                           std::string_view fieldName) {
    const auto iter =
        std::ranges::find_if(knownValues, [value](Enum known) { return enumToInt(known) == value; });
    if (iter == knownValues.end()) {
        return std::unexpected(std::string(fieldName) + " has unsupported value " + std::to_string(value));
    }
    return *iter;
}

struct PosData {
    int q = 0;
    int r = 0;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(q), CEREAL_NVP(r));
    }
};

struct UnitData {
    UnitId id = InvalidUnitId;
    std::string templateId;
    int owner = enumToInt(Owner::PlayerCtrl);
    int state = enumToInt(UnitState::Idle);
    int star = 1;
    int hp = 0;
    int mana = 0;
    UnitId targetId = InvalidUnitId;
    float attackTimer = 0.0F;
    float moveTimer = 0.0F;
    float stunTimer = 0.0F;
    bool hasCombatStartPos = false;
    PosData combatStartPos{};
    bool hasBoardPos = false;
    PosData boardPos{};
    bool hasBenchSlot = false;
    int benchSlot = -1;
    bool hasEquipment = false;
    int equipment = enumToInt(EquipmentType::IronSword);

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(id), CEREAL_NVP(templateId), CEREAL_NVP(owner), CEREAL_NVP(state),
                CEREAL_NVP(star), CEREAL_NVP(hp), CEREAL_NVP(mana), CEREAL_NVP(targetId),
                CEREAL_NVP(attackTimer), CEREAL_NVP(moveTimer), CEREAL_NVP(stunTimer),
                CEREAL_NVP(hasCombatStartPos), CEREAL_NVP(combatStartPos), CEREAL_NVP(hasBoardPos),
                CEREAL_NVP(boardPos), CEREAL_NVP(hasBenchSlot), CEREAL_NVP(benchSlot),
                CEREAL_NVP(hasEquipment), CEREAL_NVP(equipment));
    }
};

struct PlayerData {
    int hp = config::InitialPlayerHp;
    int gold = config::InitialGold;
    int level = 1;
    int populationCap = config::InitialPopulationCap;
    int currentRound = 1;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(hp), CEREAL_NVP(gold), CEREAL_NVP(level), CEREAL_NVP(populationCap),
                CEREAL_NVP(currentRound));
    }
};

struct ShopOfferData {
    std::string unitTemplateId;
    int cost = 0;
    int tier = 1;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(unitTemplateId), CEREAL_NVP(cost), CEREAL_NVP(tier));
    }
};

struct ShopData {
    std::array<ShopOfferData, config::ShopOfferCount> offers{};
    bool locked = false;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(offers), CEREAL_NVP(locked));
    }
};

struct SaveDataV1 {
    int version = SaveVersion;
    int phase = enumToInt(Phase::Prep);
    PlayerData player{};
    std::vector<UnitData> units;
    ShopData shop{};
    std::vector<int> equipmentPool;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(version), CEREAL_NVP(phase), CEREAL_NVP(player), CEREAL_NVP(units),
                CEREAL_NVP(shop), CEREAL_NVP(equipmentPool));
    }
};

// Save DTOs stay flat and versioned so future migrations do not depend on live object layout.
[[nodiscard]] PosData posDataFrom(AxialPos pos) noexcept {
    return PosData{.q = pos.q, .r = pos.r};
}

[[nodiscard]] AxialPos posFromData(PosData data) noexcept {
    return AxialPos{.q = data.q, .r = data.r};
}

[[nodiscard]] UnitData unitDataFrom(const Unit& unit) {
    UnitData data{
        .id = unit.id,
        .templateId = unit.templateId,
        .owner = enumToInt(unit.owner),
        .state = enumToInt(unit.runtime.state),
        .star = unit.star,
        .hp = unit.runtime.hp,
        .mana = unit.runtime.mana,
        .targetId = unit.runtime.targetId,
        .attackTimer = unit.runtime.attackTimer,
        .moveTimer = unit.runtime.moveTimer,
        .stunTimer = unit.runtime.stunTimer,
        .hasCombatStartPos = unit.runtime.combatStartPos.has_value(),
        .combatStartPos = unit.runtime.combatStartPos.transform(posDataFrom).value_or(PosData{}),
        .hasBoardPos = unit.boardPos.has_value(),
        .boardPos = unit.boardPos.transform(posDataFrom).value_or(PosData{}),
        .hasBenchSlot = unit.benchSlot.has_value(),
        .benchSlot = unit.benchSlot.value_or(-1),
        .hasEquipment = unit.equipment.has_value(),
        .equipment =
            unit.equipment.transform(enumToInt<EquipmentType>).value_or(enumToInt(EquipmentType::IronSword)),
    };
    return data;
}

[[nodiscard]] PlayerData playerDataFrom(const Player& player) noexcept {
    return PlayerData{
        .hp = player.hp,
        .gold = player.gold,
        .level = player.level,
        .populationCap = player.populationCap,
        .currentRound = player.currentRound,
    };
}

[[nodiscard]] ShopData shopDataFrom(const Shop& shop) {
    ShopData data{.locked = shop.locked()};
    const auto offers = shop.offers();
    for (const auto index : std::views::iota(std::size_t{0}, data.offers.size())) {
        data.offers[index] = ShopOfferData{
            .unitTemplateId = offers[index].unitTemplateId,
            .cost = offers[index].cost,
            .tier = offers[index].tier,
        };
    }
    return data;
}

[[nodiscard]] SaveDataV1 saveDataFrom(const GameState& state) {
    SaveDataV1 data{
        .version = SaveVersion,
        .phase = enumToInt(state.phase()),
        .player = playerDataFrom(state.player()),
        .units = {},
        .shop = shopDataFrom(state.shop()),
        .equipmentPool = {},
    };

    state.forEachUnit([&](const Unit& unit) { data.units.push_back(unitDataFrom(unit)); });
    std::ranges::sort(data.units, {}, &UnitData::id);

    const auto equipmentValues = state.equipmentPool() | std::views::transform(enumToInt<EquipmentType>);
    data.equipmentPool.assign(equipmentValues.begin(), equipmentValues.end());
    return data;
}

[[nodiscard]] std::expected<void, std::string> validatePlayerData(const PlayerData& player) {
    if (player.hp < 0) {
        return std::unexpected("Player hp cannot be negative");
    }
    if (player.gold < 0) {
        return std::unexpected("Player gold cannot be negative");
    }
    if (player.level < 1 || player.level > config::MaxPlayerLevel) {
        return std::unexpected("Player level is outside the supported range");
    }
    if (player.populationCap < 0) {
        return std::unexpected("Player population cap cannot be negative");
    }
    if (player.currentRound < 1) {
        return std::unexpected("Player round must be positive");
    }
    return {};
}

[[nodiscard]] std::expected<void, std::string> validateShopData(const ShopData& shop) {
    for (const ShopOfferData& offer : shop.offers) {
        if (offer.cost < 0) {
            return std::unexpected("Shop offer cost cannot be negative");
        }
        if (offer.tier < 1 || offer.tier > config::ShopMaxTier) {
            return std::unexpected("Shop offer tier is outside the supported range");
        }
    }
    return {};
}

[[nodiscard]] std::expected<void, std::string> restorePlayer(GameState& state, const PlayerData& data) {
    if (auto validation = validatePlayerData(data); !validation) {
        return validation;
    }

    Player& player = state.player();
    player.hp = data.hp;
    player.gold = data.gold;
    player.level = data.level;
    player.populationCap = data.populationCap;
    player.currentRound = data.currentRound;
    return {};
}

[[nodiscard]] std::expected<void, std::string> restoreShop(GameState& state, const ShopData& data) {
    if (auto validation = validateShopData(data); !validation) {
        return validation;
    }

    Shop::Offers offers{};
    for (const auto index : std::views::iota(std::size_t{0}, offers.size())) {
        offers[index] = ShopOffer{
            .unitTemplateId = data.offers[index].unitTemplateId,
            .cost = data.offers[index].cost,
            .tier = data.offers[index].tier,
        };
    }
    state.shop().replaceOffers(offers);
    state.shop().setLocked(data.locked);
    return {};
}

[[nodiscard]] std::expected<void, std::string> restoreEquipmentPool(GameState& state,
                                                                    const std::vector<int>& equipmentPool) {
    for (const int equipmentValue : equipmentPool) {
        auto equipment = enumFromInt(equipmentValue, KnownEquipmentTypes, "Equipment type");
        if (!equipment) {
            return std::unexpected(equipment.error());
        }
        state.addEquipment(*equipment);
    }
    return {};
}

[[nodiscard]] std::string_view placementResultName(PlacementResult result) noexcept {
    constexpr auto Names = std::array{
        std::pair{PlacementResult::Ok, std::string_view{"ok"}},
        std::pair{PlacementResult::InvalidPhase, std::string_view{"invalid phase"}},
        std::pair{PlacementResult::InvalidUnit, std::string_view{"invalid unit"}},
        std::pair{PlacementResult::InvalidOwner, std::string_view{"invalid owner"}},
        std::pair{PlacementResult::InvalidPosition, std::string_view{"invalid position"}},
        std::pair{PlacementResult::InvalidHalf, std::string_view{"invalid half"}},
        std::pair{PlacementResult::PopulationFull, std::string_view{"population full"}},
        std::pair{PlacementResult::Occupied, std::string_view{"occupied"}},
    };
    const auto iter =
        std::ranges::find_if(Names, [result](const auto& name) { return name.first == result; });
    return iter == Names.end() ? std::string_view{"unknown placement error"} : iter->second;
}

[[nodiscard]] std::expected<Unit, std::string> restoreUnitIdentity(const UnitData& data) {
    if (data.id == InvalidUnitId) {
        return std::unexpected("Unit id cannot be zero");
    }
    if (data.star < 1) {
        return std::unexpected("Unit star must be positive");
    }
    if (data.hasBoardPos && data.hasBenchSlot) {
        return std::unexpected("Unit cannot be saved on both board and bench");
    }

    auto owner = enumFromInt(data.owner, KnownOwners, "Unit owner");
    if (!owner) {
        return std::unexpected(owner.error());
    }
    auto unitState = enumFromInt(data.state, KnownUnitStates, "Unit state");
    if (!unitState) {
        return std::unexpected(unitState.error());
    }

    Unit unit = UnitCatalog::createUnit(data.id, data.templateId, *owner);
    unit.star = data.star;
    if (data.hasEquipment) {
        auto equipment = enumFromInt(data.equipment, KnownEquipmentTypes, "Unit equipment");
        if (!equipment) {
            return std::unexpected(equipment.error());
        }
        unit.equipment = *equipment;
    }
    unit.recomputeDerivedStats();

    unit.runtime.hp = std::clamp(data.hp, 0, unit.derivedStats.maxHp);
    unit.runtime.mana = std::clamp(data.mana, 0, unit.derivedStats.maxMana);
    unit.runtime.state = *unitState;
    unit.runtime.targetId = data.targetId;
    unit.runtime.attackTimer = std::max(0.0F, data.attackTimer);
    unit.runtime.moveTimer = std::max(0.0F, data.moveTimer);
    unit.runtime.stunTimer = std::max(0.0F, data.stunTimer);
    unit.runtime.combatStartPos =
        data.hasCombatStartPos ? std::optional{posFromData(data.combatStartPos)} : std::nullopt;
    unit.checkInvariants();
    return std::expected<Unit, std::string>{std::in_place, std::move(unit)};
}

[[nodiscard]] std::expected<void, std::string> restoreUnits(GameState& state,
                                                            const std::vector<UnitData>& units) {
    for (const UnitData& data : units) {
        auto unit = restoreUnitIdentity(data);
        if (!unit) {
            return std::unexpected(unit.error());
        }
        if (!state.restoreUnit(std::move(*unit))) {
            return std::unexpected("Failed to restore unit id " + std::to_string(data.id));
        }
    }

    for (const UnitData& data : units) {
        PlacementResult result = PlacementResult::Ok;
        if (data.hasBoardPos) {
            result = state.placeUnitOnBoardResult(data.id, posFromData(data.boardPos));
        } else if (data.hasBenchSlot) {
            result = state.placeUnitOnBenchResult(data.id, data.benchSlot);
        }
        if (result != PlacementResult::Ok) {
            return std::unexpected("Failed to restore unit " + std::to_string(data.id) +
                                   " placement: " + std::string(placementResultName(result)));
        }
    }
    return {};
}

[[nodiscard]] std::expected<GameState, std::string> stateFromSaveData(const SaveDataV1& data) {
    if (data.version != SaveVersion) {
        return std::unexpected("Unsupported save version " + std::to_string(data.version));
    }

    auto phase = enumFromInt(data.phase, KnownPhases, "Game phase");
    if (!phase) {
        return std::unexpected(phase.error());
    }

    GameState state;
    state.setPhase(Phase::Prep);

    if (auto restored = restorePlayer(state, data.player); !restored) {
        return std::unexpected(restored.error());
    }
    if (auto restored = restoreShop(state, data.shop); !restored) {
        return std::unexpected(restored.error());
    }
    if (auto restored = restoreEquipmentPool(state, data.equipmentPool); !restored) {
        return std::unexpected(restored.error());
    }
    if (auto restored = restoreUnits(state, data.units); !restored) {
        return std::unexpected(restored.error());
    }

    state.setPhase(*phase);
    return std::expected<GameState, std::string>{std::in_place, std::move(state)};
}

}  // namespace

std::expected<void, std::string> SaveSystem::save(const GameState& state, const std::string& path) const {
    if (state.phase() != Phase::Prep) {
        return std::unexpected("Saving is only allowed during preparation phase");
    }

    try {
        const std::filesystem::path savePath{path};
        if (const auto parent = savePath.parent_path(); !parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        std::ofstream out{savePath};
        if (!out) {
            return std::unexpected("Failed to open save file for writing: " + path);
        }

        cereal::JSONOutputArchive archive{out};
        auto data = saveDataFrom(state);
        archive(cereal::make_nvp("save", data));
        return {};
    } catch (const std::exception& exception) {
        return std::unexpected(std::string{"Failed to save game: "} + exception.what());
    }
}

std::expected<GameState, std::string> SaveSystem::load(const std::string& path) const {
    try {
        std::ifstream in{std::filesystem::path{path}};
        if (!in) {
            return std::unexpected("Failed to open save file for reading: " + path);
        }

        cereal::JSONInputArchive archive{in};
        SaveDataV1 data;
        archive(cereal::make_nvp("save", data));
        return stateFromSaveData(data);
    } catch (const std::exception& exception) {
        return std::unexpected(std::string{"Failed to load game: "} + exception.what());
    }
}

}  // namespace synera
