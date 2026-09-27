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
	/**
	 * How many hold it, and nought for the places nobody holds.
	 *
	 * Only two monuments have anybody in them: the power plant has its
	 * scientists and the military camp has its soldiers. The airfield is
	 * irradiated and empty, which makes the suit the way in rather than the
	 * gun, and the rest are neither.
	 */
	int guards;
	/** The military camp is held by soldiers rather than scientists. */
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
