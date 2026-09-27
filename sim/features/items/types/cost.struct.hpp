#pragma once

#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/**
 * So much of one thing: what a recipe asks for, what a wall costs.
 *
 * The same shape as a stack, and deliberately a different name: a stack is
 * something that exists somewhere, a cost is something being asked for.
 */
struct Cost {
	ItemId id;
	int count;
};

}  // namespace sim
