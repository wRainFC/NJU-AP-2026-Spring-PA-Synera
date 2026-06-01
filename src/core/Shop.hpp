#pragma once

#include "config/GameConfig.hpp"
#include "core/ShopOffer.hpp"

#include <array>
#include <functional>
#include <optional>
#include <span>

namespace synera {

// Persistent shop state. Roll rules live in ShopPool/ShopSystem, so saved games only need offers + lock.
class Shop {
public:
    using Offers = std::array<ShopOffer, config::ShopOfferCount>;

    // Offers are exposed as spans so UI/save code can scan the fixed 5 slots without owning them.
    [[nodiscard]] std::span<ShopOffer, config::ShopOfferCount> offers() noexcept;
    [[nodiscard]] std::span<const ShopOffer, config::ShopOfferCount> offers() const noexcept;
    [[nodiscard]] std::optional<std::reference_wrapper<ShopOffer>> offerAt(int index) noexcept;
    [[nodiscard]] std::optional<std::reference_wrapper<const ShopOffer>> offerAt(int index) const noexcept;

    void setOffer(int index, ShopOffer offer);
    void replaceOffers(Offers offers) noexcept;
    void clearOffer(int index);
    void clearOffers() noexcept;

    [[nodiscard]] bool locked() const noexcept;
    void setLocked(bool locked) noexcept;
    bool toggleLocked() noexcept;

private:
    Offers offers_{};
    bool locked_ = false;

    [[nodiscard]] static bool inBounds(int index) noexcept;
};

}  // namespace synera
