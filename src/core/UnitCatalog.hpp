#pragma once

#include "core/Unit.hpp"

#include <string_view>

namespace synera {

class UnitCatalog {
public:
    [[nodiscard]] static Unit createUnit(UnitId id, std::string_view templateId, Owner owner);
};

} // namespace synera
