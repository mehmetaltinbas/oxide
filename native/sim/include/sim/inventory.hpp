#pragma once

#include <array>

#include "sim/item.hpp"

namespace sim {

/** The belt you carry things on, and the pack behind it. */
inline constexpr int kHotbarSlots = 6;
inline constexpr int kPackSlots = 24;

/**
 * What someone is carrying.
 *
 * The belt fills first and the pack takes the overflow, so what you pick up is
 * to hand rather than buried. Anything that will not fit is returned rather
 * than quietly lost.
 */
class Inventory {
public:
    Inventory();

    std::array<ItemStack, kHotbarSlots>& hotbar() { return hotbar_; }
    const std::array<ItemStack, kHotbarSlots>& hotbar() const { return hotbar_; }
    std::array<ItemStack, kPackSlots>& pack() { return pack_; }
    const std::array<ItemStack, kPackSlots>& pack() const { return pack_; }

    int activeSlot() const { return active_; }
    void selectSlot(int slot);

    /** What is in your hand, or nothing. */
    ItemId held() const { return hotbar_[active_].id; }

    /** Takes what it can; returns what would not fit. */
    int add(ItemId id, int count);
    /** How many of something is carried, belt and pack together. */
    int count(ItemId id) const;
    /** Takes some away, and says how many it actually found. */
    int take(ItemId id, int count);

private:
    std::array<ItemStack, kHotbarSlots> hotbar_{};
    std::array<ItemStack, kPackSlots> pack_{};
    int active_ = 0;
};

}  // namespace sim
