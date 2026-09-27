#pragma once

#include "sim/features/monuments/types/loot-entry.struct.hpp"
#include "sim/features/monuments/types/monument-kind.enum.hpp"

namespace sim {

struct MonumentDef {
	MonumentKind kind;
	const char* name;
	double radius;
	/** Radiation a second at the heart of it, and nought for the safe ones. */
	double rads;
	int scientists;
	/** The military base is held by soldiers rather than scientists. */
	bool soldiers;
	int crates;
	LootEntry loot[9];
	int lootCount;
	/** How many of it an island gets. */
	int count;
	/** A lighthouse stands on the shore; cabins are in the trees and the snow. */
	bool coast;
	bool forestOrSnow;
	/** The big three are never neighbours, whatever size the island is. */
	bool major;
};

}  // namespace sim
