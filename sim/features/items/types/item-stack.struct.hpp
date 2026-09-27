#pragma once

#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/** A count of one thing, as it sits in a slot or on the ground. */
struct ItemStack {
	ItemId id = ItemId::None;
	int count = 0;
};

}  // namespace sim
