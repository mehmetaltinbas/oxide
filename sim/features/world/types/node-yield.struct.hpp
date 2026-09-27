#pragma once

#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/** What comes off it per blow, before the tool and the node's preference. */
struct NodeYield {
	ItemId id;
	int per;
	/**
	 * How often a blow gives any of it at all.
	 *
	 * One for the things a node is made of, and a small number for the rare
	 * thing hiding in it: high quality metal comes out of a metal node a few
	 * pieces at a time and not every swing, which is what makes it rare
	 * without needing a second kind of node for it.
	 */
	double chance;
};

}  // namespace sim
