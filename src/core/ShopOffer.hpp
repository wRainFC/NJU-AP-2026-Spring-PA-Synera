#pragma once

#include <string>

namespace synera {

struct ShopOffer {
    std::string unitTemplateId;
    int cost = 0;
    int tier = 1;

    [[nodiscard]] bool empty() const noexcept { return unitTemplateId.empty(); }
};

}  // namespace synera
