#include "core/Shop.hpp"

#include <cstddef>
#include <utility>

namespace synera {

std::span<ShopOffer, config::ShopOfferCount> Shop::offers() noexcept {
    return offers_;
}

std::span<const ShopOffer, config::ShopOfferCount> Shop::offers() const noexcept {
    return offers_;
}

std::optional<std::reference_wrapper<ShopOffer>> Shop::offerAt(int index) noexcept {
    if (!inBounds(index)) {
        return std::nullopt;
    }
    return offers_[static_cast<std::size_t>(index)];
}

std::optional<std::reference_wrapper<const ShopOffer>> Shop::offerAt(int index) const noexcept {
    if (!inBounds(index)) {
        return std::nullopt;
    }
    return offers_[static_cast<std::size_t>(index)];
}

void Shop::setOffer(int index, ShopOffer offer) {
    if (!inBounds(index)) {
        return;
    }
    offers_[static_cast<std::size_t>(index)] = std::move(offer);
}

void Shop::replaceOffers(Offers offers) noexcept {
    offers_ = std::move(offers);
}

void Shop::clearOffer(int index) {
    if (!inBounds(index)) {
        return;
    }
    offers_[static_cast<std::size_t>(index)] = ShopOffer{};
}

void Shop::clearOffers() noexcept {
    offers_ = Offers{};
}

bool Shop::locked() const noexcept {
    return locked_;
}

void Shop::setLocked(bool locked) noexcept {
    locked_ = locked;
}

bool Shop::toggleLocked() noexcept {
    locked_ = !locked_;
    return locked_;
}

bool Shop::inBounds(int index) noexcept {
    return index >= 0 && index < config::ShopOfferCount;
}

}  // namespace synera
