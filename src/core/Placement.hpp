#pragma once

namespace synera {

enum class PlacementResult {
    Ok,
    InvalidPhase,
    InvalidUnit,
    InvalidOwner,
    InvalidPosition,
    InvalidHalf,
    PopulationFull,
    Occupied,
};

}  // namespace synera
