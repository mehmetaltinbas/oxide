#pragma once

#include "sim/features/items/types/item-id.enum.hpp"

namespace client {

/** One slice of a radial menu: what it says, what it shows, and whether it may be chosen. */
struct WheelOption {
	const char* label = "";
	/** The picture on the slice, or None for a slice drawn as a shape. */
	sim::ItemId icon = sim::ItemId::None;
	/** A line under the label: what it costs, what it is made of. */
	const char* note = "";
	bool allowed = true;
	/**
	 * What choosing it means, for the caller to read back.
	 *
	 * Not the slice's position. A ring that mixes things of different kinds,
	 * four tiers and a "take it down", cannot have its index read as one of
	 * them, and a ring whose order changes would quietly change what every
	 * slice does.
	 */
	int value = 0;
	/** Drawn in the warning colour: the one on the ring you cannot undo. */
	bool danger = false;
};

}  // namespace client
