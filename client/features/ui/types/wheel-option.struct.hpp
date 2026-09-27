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
};

}  // namespace client
