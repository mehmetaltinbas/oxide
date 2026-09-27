#pragma once

#include "sim/features/building/types/container.struct.hpp"
#include "sim/features/monuments/types/monument-kind.enum.hpp"

namespace sim {

/** A crate of loot at a monument, which fills again a while after it is emptied. */
struct LootCrate {
	int id;
	MonumentKind monument;
	double x;
	double y;
	Container container;
	bool looted;
	double respawn;
};

}  // namespace sim
