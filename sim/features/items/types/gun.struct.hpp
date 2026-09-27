#pragma once

#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/** What a gun does when it goes off. */
struct Gun {
	double damage;
	double cooldown;
	/** How fast the round travels, and how far before it is spent. */
	double speed;
	double range;
	/** How far off true a shot can come out, in radians. */
	double spread;
	ItemId ammo;
	/** How many rounds it holds, and how long it takes to fill again. */
	int magazine;
	double reloadSeconds;
	/** More than one means a cone of them: a shotgun. */
	int pellets;
	/**
	 * How far out in front of you the round appears, in world units.
	 *
	 * The end of the barrel as it is actually drawn, not the middle of your
	 * chest: a long gun's muzzle is most of its own length past your hand, and
	 * rounds coming out of your ribs was the giveaway that this was a guess.
	 * Zero falls back to a sensible default.
	 */
	double muzzle;
	/**
	 * Whether it is fed a round at a time rather than a magazine at a time.
	 *
	 * A shotgun is thumbed full shell by shell, so `reloadSeconds` is the time
	 * for one of them, a half-finished reload leaves you with half a tube, and
	 * pulling the trigger stops the loading and fires what is in there. A
	 * magazine gun is all or nothing.
	 */
	bool singly;
};

}  // namespace sim
