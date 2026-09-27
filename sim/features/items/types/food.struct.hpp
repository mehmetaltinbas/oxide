#pragma once

namespace sim {

/** What eating, drinking or applying something does. */
struct Food {
	double calories;
	double hydration;
	double health;
	/** Zero means it happens at once; otherwise it is a channel of seconds. */
	double useSeconds;
	/**
	 * Health that comes back slowly rather than at once, at one a second.
	 *
	 * A syringe does both: a little straight away and the rest over the next
	 * twenty seconds, which is what makes it worth using before a fight
	 * rather than in the middle of one.
	 */
	double overTime;
	/** How much bleeding it stops, in seconds of it. */
	double bleedCure;
	/** How much radiation it clears. */
	double radCure;
};

}  // namespace sim
