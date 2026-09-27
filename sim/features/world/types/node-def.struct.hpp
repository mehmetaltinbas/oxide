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
	/**
	 * How much of a shot it takes, as a fraction of what a swing would take.
	 *
	 * A bullet is not a pickaxe. Wood splinters and a barrel bursts, but a
	 * round off a boulder or an ore seam takes the stone with it and leaves
	 * the seam where it was: nought here means a shot does nothing at all,
	 * which is what stops a rifle being the fastest way to mine.
	 */
	double shotShare;
};

}  // namespace sim
