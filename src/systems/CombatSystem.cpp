#include "systems/CombatSystem.hpp"

#include "board/HexGrid.hpp"
#include "core/AbilityContext.hpp"
#include "core/GameState.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <tuple>
#include <utility>

namespace synera {

void CombatSystem::setActionCatalog(const CombatActionCatalog& catalog) noexcept {
    actionCatalog_ = &catalog;
}

void CombatSystem::update(GameState& state, float dt) {
    clearEvents();
    if (state.phase() != Phase::Combat) {
        pendingActions_.clear();
        return;
    }
    updatePendingActions(state, dt);
    cleanupDeadBoardUnits(state);
    state.forEachUnit([&](Unit& unit) {
        updateUnit(state, unit, dt);
    });
    cleanupDeadBoardUnits(state);
}

std::span<const CombatEvent> CombatSystem::events() const noexcept {
    return events_;
}

std::vector<CombatEvent> CombatSystem::drainEvents() {
    return std::exchange(events_, {});
}

void CombatSystem::clearEvents() noexcept {
    events_.clear();
}

void CombatSystem::updateUnit(GameState& state, Unit& unit, float dt) {
    if (!unit.alive() || !unit.onBoard()) {
        return;
    }
    if (unit.runtime.state == UnitState::Stunned) {
        updateStun(unit, dt);
        return;
    }
    if (unitHasPendingAction(unit.id)) {
        return;
    }

    Unit* target = acquireTarget(state, unit);
    if (target == nullptr) {
        unit.runtime.state    = UnitState::Idle;
        unit.runtime.targetId = InvalidUnitId;
        return;
    }
    unit.runtime.targetId = target->id;

    if (!unit.canAttackTarget(*target)) {
        moveTowardTarget(state, unit, *target, dt);
        return;
    }

    if (tryCastAbility(state, unit)) {
        return;
    }

    unit.runtime.attackTimer += dt;
    if (unit.runtime.attackTimer >= unit.derivedStats.attackInterval) {
        unit.runtime.attackTimer = 0.0F;
        performAttack(unit, *target);
    }
}

Unit* CombatSystem::acquireTarget(GameState& state, const Unit& unit) {
    Unit* best = nullptr;
    auto bestKey =
        std::tuple{std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
                   std::numeric_limits<int>::max(), std::numeric_limits<int>::min(), InvalidUnitId};

    state.forEachUnit([&](Unit& candidate) {
        if (!candidate.alive() || !candidate.onBoard() || candidate.owner == unit.owner) {
            return;
        }

        const OffsetPos offset  = hex::axialToOddR(*candidate.boardPos);
        const auto candidateKey = std::tuple{hex::hexDistance(*unit.boardPos, *candidate.boardPos),
                                             candidate.runtime.hp, offset.col, -offset.row, candidate.id};
        if (candidateKey < bestKey) {
            best    = &candidate;
            bestKey = candidateKey;
        }
    });

    return best;
}

void CombatSystem::updateStun(Unit& unit, float dt) {
    unit.runtime.stunTimer -= dt;
    if (unit.runtime.stunTimer <= 0.0F) {
        unit.runtime.stunTimer = 0.0F;
        unit.runtime.state     = UnitState::Idle;
    }
}

bool CombatSystem::tryCastAbility(GameState& state, Unit& unit) {
    if (!unit.ability || unit.runtime.mana < unit.derivedStats.maxMana) {
        return false;
    }

    const CombatActionProfile& profile = abilityProfileFor(unit);
    const Unit* target = state.findUnit(unit.runtime.targetId);
    const AxialPos from = unit.boardPos.value_or(AxialPos{});
    const AxialPos to = target == nullptr ? from : target->boardPos.value_or(from);
    unit.runtime.state = UnitState::Casting;
    unit.runtime.mana = 0;

    const float hitTime = profile.hitTimes.empty() ? profile.durationSeconds : profile.hitTimes.front();
    const std::uint64_t actionId = nextActionId_++;
    pendingActions_.push_back(PendingCombatAction{
        .id = actionId,
        .kind = CombatActionKind::Ability,
        .profileId = profile.id,
        .animationProfileId = profile.animationProfileId,
        .sourceId = unit.id,
        .targetId = unit.runtime.targetId,
        .from = from,
        .to = to,
        .attackKind = profile.attackKind,
        .durationSeconds = profile.durationSeconds,
        .hits = {PendingCombatHit{.targetId = unit.runtime.targetId, .timeSeconds = hitTime}},
    });
    emit(CombatEvent{
        .type                  = CombatEventType::AbilityCast,
        .actionId              = actionId,
        .actionProfileId       = profile.id,
        .animationProfileId    = profile.animationProfileId,
        .sourceId              = unit.id,
        .targetId              = unit.runtime.targetId,
        .from                  = from,
        .to                    = to,
        .attackKind            = profile.attackKind,
        .actionDurationSeconds = profile.durationSeconds,
        .hitDelaySeconds       = hitTime,
    });
    return true;
}

void CombatSystem::moveTowardTarget(GameState& state, Unit& unit, const Unit& target, float dt) {
    unit.runtime.state = UnitState::Moving;
    unit.runtime.moveTimer += dt;
    if (unit.runtime.moveTimer < unit.derivedStats.moveInterval) {
        return;
    }
    unit.runtime.moveTimer = 0.0F;

    const auto path = pathfinder_.findPathToAttackRange(state.board(), *unit.boardPos, *target.boardPos,
                                                        unit.derivedStats.range);
    if (path.empty()) {
        unit.runtime.state = UnitState::Idle;
        return;
    }
    const AxialPos from = *unit.boardPos;
    const AxialPos to   = path.front();
    if (!state.moveBoardUnit(unit.id, to)) {
        unit.runtime.state = UnitState::Idle;
        return;
    }
    emit(CombatEvent{
        .type     = CombatEventType::UnitMoved,
        .actionId = 0,
        .actionProfileId = {},
        .animationProfileId = {},
        .sourceId = unit.id,
        .from     = from,
        .to       = to,
    });
}

void CombatSystem::performAttack(Unit& attacker, Unit& target) {
    attacker.runtime.state = UnitState::Attacking;
    const CombatActionProfile& profile = basicAttackProfileFor(attacker);
    const AttackVisualKind attackKind = profile.attackKind;
    const std::uint64_t actionId = nextActionId_++;
    std::vector<PendingCombatHit> hits;
    for (float hitTime : profile.hitTimes) {
        hits.push_back(PendingCombatHit{
            .targetId = target.id,
            .amount = attacker.derivedStats.atk,
            .timeSeconds = hitTime,
        });
    }
    if (attacker.mechanics.doubleBasicAttack) {
        const float firstHit = profile.hitTimes.empty() ? profile.durationSeconds * 0.5F
                                                        : profile.hitTimes.front();
        hits.push_back(PendingCombatHit{
            .targetId = target.id,
            .amount = attacker.derivedStats.atk,
            .timeSeconds = std::min(profile.durationSeconds, firstHit + 0.08F),
        });
        std::ranges::sort(hits, {}, &PendingCombatHit::timeSeconds);
    }

    pendingActions_.push_back(PendingCombatAction{
        .id = actionId,
        .kind = CombatActionKind::BasicAttack,
        .profileId = profile.id,
        .animationProfileId = profile.animationProfileId,
        .sourceId = attacker.id,
        .targetId = target.id,
        .from = attacker.boardPos.value_or(AxialPos{}),
        .to = target.boardPos.value_or(AxialPos{}),
        .attackKind = attackKind,
        .durationSeconds = profile.durationSeconds,
        .hits = std::move(hits),
    });
    emit(CombatEvent{
        .type                  = CombatEventType::AttackStarted,
        .actionId              = actionId,
        .actionProfileId       = profile.id,
        .animationProfileId    = profile.animationProfileId,
        .sourceId              = attacker.id,
        .targetId              = target.id,
        .from                  = attacker.boardPos.value_or(AxialPos{}),
        .to                    = target.boardPos.value_or(AxialPos{}),
        .attackKind            = attackKind,
        .actionDurationSeconds = profile.durationSeconds,
        .hitDelaySeconds       = profile.hitTimes.empty() ? profile.durationSeconds : profile.hitTimes.front(),
    });
    attacker.gainMana(10);
}

void CombatSystem::updatePendingActions(GameState& state, float dt) {
    for (PendingCombatAction& action : pendingActions_) {
        Unit* source = state.findUnit(action.sourceId);
        if (source == nullptr || !source->alive() || !source->onBoard()) {
            action.canceled = true;
            continue;
        }

        action.elapsedSeconds += dt;
        for (PendingCombatHit& hit : action.hits) {
            if (hit.resolved || action.elapsedSeconds < hit.timeSeconds) {
                continue;
            }
            if (action.kind == CombatActionKind::Ability) {
                resolvePendingAbility(state, action);
            } else {
                resolvePendingHit(state, action, hit);
            }
            hit.resolved = true;
        }
        if (action.elapsedSeconds >= action.durationSeconds) {
            source->runtime.state = UnitState::Idle;
        }
    }

    std::erase_if(pendingActions_, [](const PendingCombatAction& action) {
        return action.canceled || action.elapsedSeconds >= action.durationSeconds;
    });
}

void CombatSystem::resolvePendingHit(GameState& state, PendingCombatAction& action,
                                     PendingCombatHit& hit) {
    Unit* source = state.findUnit(action.sourceId);
    Unit* target = state.findUnit(hit.targetId);
    if (source == nullptr || target == nullptr || !target->alive() || !target->onBoard()) {
        return;
    }

    target->receiveDamage(hit.amount);
    emit(CombatEvent{
        .type                  = CombatEventType::DamageDealt,
        .actionId              = action.id,
        .actionProfileId       = action.profileId,
        .animationProfileId    = action.animationProfileId,
        .sourceId              = action.sourceId,
        .targetId              = hit.targetId,
        .from                  = source->boardPos.value_or(action.from),
        .to                    = target->boardPos.value_or(action.to),
        .amount                = hit.amount,
        .attackKind            = action.attackKind,
        .actionDurationSeconds = action.durationSeconds,
        .hitDelaySeconds       = hit.timeSeconds,
    });
}

void CombatSystem::resolvePendingAbility(GameState& state, PendingCombatAction& action) {
    if (action.abilityResolved) {
        return;
    }
    Unit* source = state.findUnit(action.sourceId);
    if (source == nullptr || source->ability == nullptr || !source->alive() || !source->onBoard()) {
        action.abilityResolved = true;
        return;
    }

    AbilityContext context{state};
    source->ability->cast(*source, context);
    for (const AbilityResult& result : context.results()) {
        emitAbilityResult(*source, action, result);
    }
    action.abilityResolved = true;
}

void CombatSystem::emitAbilityResult(const Unit& source, const PendingCombatAction& action,
                                     const AbilityResult& result) {
    CombatEventType eventType = CombatEventType::DamageDealt;
    switch (result.type) {
        case AbilityResultType::Damage:
            eventType = CombatEventType::DamageDealt;
            break;
        case AbilityResultType::Heal:
            eventType = CombatEventType::HealReceived;
            break;
        case AbilityResultType::Stun:
            eventType = CombatEventType::StatusApplied;
            break;
    }

    emit(CombatEvent{
        .type                  = eventType,
        .actionId              = action.id,
        .actionProfileId       = action.profileId,
        .animationProfileId    = action.animationProfileId,
        .sourceId              = action.sourceId,
        .targetId              = result.targetId,
        .from                  = source.boardPos.value_or(action.from),
        .to                    = result.targetPos,
        .amount                = result.amount,
        .attackKind            = action.attackKind,
        .actionDurationSeconds = action.durationSeconds,
        .hitDelaySeconds       = action.elapsedSeconds,
        .statusDurationSeconds = result.durationSeconds,
    });
}

void CombatSystem::cleanupDeadBoardUnits(GameState& state) {
    state.forEachUnit([&](Unit& unit) {
        if (!unit.alive() && unit.onBoard()) {
            emit(CombatEvent{
                .type     = CombatEventType::UnitDied,
                .actionId = 0,
                .actionProfileId = {},
                .animationProfileId = {},
                .sourceId = unit.id,
                .from     = *unit.boardPos,
                .to       = *unit.boardPos,
            });
            state.removeUnitFromBoard(unit);
        }
    });
}

void CombatSystem::emit(CombatEvent event) {
    events_.push_back(event);
}

bool CombatSystem::unitHasPendingAction(UnitId unitId) const noexcept {
    return std::ranges::any_of(pendingActions_, [unitId](const PendingCombatAction& action) {
        return !action.canceled && action.sourceId == unitId;
    });
}

const CombatActionCatalog& CombatSystem::actionCatalog() const noexcept {
    return actionCatalog_ == nullptr ? defaultActionCatalog_ : *actionCatalog_;
}

const CombatActionProfile& CombatSystem::basicAttackProfileFor(const Unit& attacker) const noexcept {
    const AttackVisualKind fallbackKind =
        attacker.derivedStats.range > 1 ? AttackVisualKind::Ranged : AttackVisualKind::Melee;
    if (attacker.basicAttackProfileId.empty()) {
        return actionCatalog().defaultBasicAttack(fallbackKind);
    }
    const CombatActionProfile& profile = actionCatalog().profile(attacker.basicAttackProfileId);
    return profile.kind == CombatActionKind::BasicAttack ? profile : actionCatalog().defaultBasicAttack(fallbackKind);
}

const CombatActionProfile& CombatSystem::abilityProfileFor(const Unit& caster) const noexcept {
    if (caster.ability == nullptr) {
        return actionCatalog().defaultAbility();
    }
    const CombatActionProfile& profile = actionCatalog().profile(caster.ability->combatActionProfileId());
    return profile.kind == CombatActionKind::Ability ? profile : actionCatalog().defaultAbility();
}

}  // namespace synera
