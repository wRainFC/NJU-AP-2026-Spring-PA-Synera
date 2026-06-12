#pragma once

#include "config/GameConfig.hpp"

#include <array>
#include <string>

namespace synera {

struct ShopOffer {
    std::string unitTemplateId;
    int cost = 0;
    int tier = 1;

    [[nodiscard]] bool empty() const noexcept { return unitTemplateId.empty(); }
};

using ShopOffers = std::array<ShopOffer, config::ShopOfferCount>;

}  // namespace synera
