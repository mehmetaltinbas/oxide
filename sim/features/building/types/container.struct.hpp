#pragma once

#include <vector>

#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/item-stack.struct.hpp"

namespace sim {

/** Somewhere to put things: a box, a fire's grate, a furnace's belly. */
struct Container {
	std::vector<ItemStack> slots;

	/** Takes what it can; returns what would not fit. */
	int add(ItemId id, int count);
	int count(ItemId id) const;
	int take(ItemId id, int count);
};

}  // namespace sim
