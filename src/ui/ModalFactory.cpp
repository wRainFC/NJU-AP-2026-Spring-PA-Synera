#include "ui/ModalFactory.hpp"

#include "core/Metadata.hpp"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace synera {

namespace {

enum class RoundModalLineKind { Gold, Hp, Equipment, NextRound };

struct RoundModalLineSpec {
    RoundModalLineKind kind;
};

constexpr std::array RoundSettlementLineSpecs{
    RoundModalLineSpec{.kind = RoundModalLineKind::Gold},
    RoundModalLineSpec{.kind = RoundModalLineKind::Hp},
    RoundModalLineSpec{.kind = RoundModalLineKind::Equipment},
    RoundModalLineSpec{.kind = RoundModalLineKind::NextRound},
};

[[nodiscard]] std::string signedDelta(int value) {
    return (value >= 0 ? "+" : "") + std::to_string(value);
}

[[nodiscard]] std::string changeLine(std::string_view label, int before, int after) {
    return std::string{label} + ": " + std::to_string(before) + " -> " + std::to_string(after) + " (" +
           signedDelta(after - before) + ")";
}

[[nodiscard]] std::string roundModalLine(RoundModalLineSpec spec, const RoundResult& result,
                                         const EquipmentDropResult& drop) {
    switch (spec.kind) {
        case RoundModalLineKind::Gold:
            return changeLine("Gold", result.goldBefore, result.goldAfter);
        case RoundModalLineKind::Hp:
            return changeLine("HP", result.hpBefore, result.hpAfter);
        case RoundModalLineKind::Equipment:
            return "Equipment: " +
                   (drop.dropped && drop.equipment ? std::string{equipmentName(*drop.equipment)}
                                                   : std::string{"None"});
        case RoundModalLineKind::NextRound:
            return std::string{result.advancedRound ? "Next: Round " : "Retry: Round "} +
                   std::to_string(result.nextRound);
    }
    return {};
}

}  // namespace

ModalModel roundSettlementModal(const RoundResult& result, const EquipmentDropResult& drop) {
    std::vector<std::string> lines;
    lines.reserve(RoundSettlementLineSpecs.size());
    for (RoundModalLineSpec spec : RoundSettlementLineSpecs) {
        lines.push_back(roundModalLine(spec, result, drop));
    }

    return ModalModel{
        .title   = "Round " + std::to_string(result.resolvedRound) +
                 (result.playerWon ? " Clear" : " Lost"),
        .lines   = std::move(lines),
        .buttons = {ModalButton{.id = ModalButtonId::ContinueResolve, .label = "Continue"}},
        .accent  = result.playerWon ? GOLD : RED,
    };
}

ModalModel terminalOutcomeModal(std::string_view title, Color accent) {
    return ModalModel{
        .title   = std::string{title},
        .lines   = {"Choose how to continue."},
        .buttons = {
            ModalButton{.id = ModalButtonId::NewRun, .label = "New Run"},
            ModalButton{.id = ModalButtonId::LoadSave, .label = "Load Save"},
        },
        .accent = accent,
    };
}

}  // namespace synera
