#pragma once

#include "sim/features/world/types/node-kind.enum.hpp"
#include "sim/features/world/types/node-yield.struct.hpp"
#include "sim/features/world/types/work.enum.hpp"

namespace sim {

/** What one kind of them is worth and how tough it is. */
struct NodeDef {
	NodeKind kind;
	const char* name;
	int hp;
	double radius;
	Work prefers;
	NodeYield yields[3];
	int yieldCount;
	/**
	 * Whether it is smashed rather than worked: nothing comes off it until it
	 * breaks, and then what was inside spills on the ground.
	 */
	bool loot;
};

}  // namespace sim
