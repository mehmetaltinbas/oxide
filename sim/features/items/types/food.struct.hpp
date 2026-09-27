#pragma once

namespace sim {

/** What eating, drinking or applying something does. */
struct Food {
	double calories;
	double hydration;
	double health;
	/** Zero means it happens at once; otherwise it is a channel of seconds. */
	double useSeconds;
};

}  // namespace sim
