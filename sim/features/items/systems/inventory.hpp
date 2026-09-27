#pragma once

#include <array>

#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/item-stack.struct.hpp"

namespace sim {

/**
 * How long a transfer takes.
 *
 * Moving a stack is work: you are lifting things in and out of a box, not
 * clicking a spreadsheet. A single bandage is nearly instant and a full stack
 * of stone is a real pause you would not want to start with a bear behind you.
 */
struct Transfer {
	static constexpr double kBaseSeconds = 0.34;
	static constexpr double kPerItemSeconds = 0.004;
	static constexpr double kMaxSeconds = 1.4;

	static double seconds(int count) {
		const double t = kBaseSeconds + count * kPerItemSeconds;
		return t < kMaxSeconds ? t : kMaxSeconds;
	}
};

/** The belt you carry things on, and the pack behind it. */
inline constexpr int kHotbarSlots = 6;
/**
 * The pack, at its largest.
 *
 * Twenty-four of these are yours from the start; the last twelve only open
 * when a backpack is on your back. The array is always the full size, so
 * nothing has to move when one goes on or comes off, and `packSlots()` is the
 * only thing that says how much of it you may actually use.
 */
inline constexpr int kPackSlots = 36;
/** What you carry with nothing on your back. */
inline constexpr int kBarePackSlots = 24;
/** What a backpack adds. */
inline constexpr int kBackpackSlots = 12;

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

	/** What is worn, which is one thing at a time. */
	ItemStack& worn() { return worn_; }
	const ItemStack& worn() const { return worn_; }

	/** What is on your back, which is a backpack or nothing. */
	ItemStack& back() { return back_; }
	const ItemStack& back() const { return back_; }

	/**
	 * How many pack slots are open to you.
	 *
	 * Asked by everything that fills, empties or draws the pack, so a slot
	 * behind a backpack you are not wearing cannot be reached by any route.
	 */
	int packSlots() const {
		return back_.id == ItemId::None ? kBarePackSlots : kBarePackSlots + kBackpackSlots;
	}

	int activeSlot() const { return active_; }
	void selectSlot(int slot);

	/** What is in your hand, or nothing. */
	ItemId held() const { return hotbar_[active_].id; }

	/** Takes what it can; returns what would not fit. */
	int add(ItemId id, int count);

	/**
	 * Into one run of slots and no other, for the screen's "send it across".
	 * Each returns what would not fit.
	 */
	int addToPack(ItemId id, int count);
	int addToBelt(ItemId id, int count);
	/** How many of something is carried, belt and pack together. */
	int count(ItemId id) const;
	/** Takes some away, and says how many it actually found. */
	int take(ItemId id, int count);

private:
	std::array<ItemStack, kHotbarSlots> hotbar_{};
	std::array<ItemStack, kPackSlots> pack_{};
	ItemStack worn_{};
	ItemStack back_{};
	int active_ = 0;
};

}  // namespace sim
