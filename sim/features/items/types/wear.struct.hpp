#pragma once

namespace sim {

/** What wearing something does. */
struct Wear {
	double warmth;
	/** The fraction of a blow it turns, and of the radiation it keeps out. */
	double armor;
	double radiation;
};

}  // namespace sim
