#pragma once

#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/**
 * Everything one deployable is, on one row.
 *
 * A deployable used to be described in five places: a footprint here, a slot
 * count there, a bench tier somewhere else, and the item it comes from in two
 * more. Adding one meant finding all five, and a miss fell through to a
 * default rather than failing: a new box with no footprint was silently two
 * squares, and a new bench with no tier was silently not a bench.
 *
 * Now it is a row. See docs/systems/deployables.md.
 */
struct DeployDef {
	/** Which one this is. Rows are in the order of the enum, and checked. */
	DeployKind kind;
	/** The item in your hand that puts it down. */
	ItemId item;
	/** Its floor, in fine squares, unturned: across, then down. */
	int wide;
	int deep;
	/** How much it holds, and nought for the ones that hold nothing. */
	int slots;
	/** Which tier of workbench it is, and nought for everything else. */
	int bench;
};

}  // namespace sim
