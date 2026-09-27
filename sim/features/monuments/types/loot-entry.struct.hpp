#pragma once

#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/** One entry of a monument's loot table: how much of what, when it rolls. */
struct LootEntry {
	ItemId id;
	int low;
	int high;
};

}  // namespace sim
