#pragma once

#include "sim/features/items/types/item-stack.struct.hpp"

namespace sim {

/** A stack lying on the ground, waiting for whoever walks past. */
struct Dropped {
	int id;
	ItemStack stack;
	double x;
	double y;
	/** How long it has lain there, so it can rot away in time. */
	double age;
};

}  // namespace sim
