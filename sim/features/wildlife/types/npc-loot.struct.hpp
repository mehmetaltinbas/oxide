#pragma once

#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/** What one animal drops when it is killed, as a range. */
struct NpcLoot {
	ItemId id;
	int low;
	int high;
};

}  // namespace sim
