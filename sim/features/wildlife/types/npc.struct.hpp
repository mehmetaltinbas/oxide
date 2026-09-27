#pragma once

#include <cstdint>

#include "sim/features/wildlife/types/npc-kind.enum.hpp"
#include "sim/features/wildlife/types/npc-state.enum.hpp"

namespace sim {

/** One animal, somewhere on the island. */
struct Npc {
	int id;
	NpcKind kind;
	double x;
	double y;
	double vx;
	double vy;
	double facing;
	int hp;
	NpcState state;
	double stateTime;
	double attackTimer;
	/** How far through its gait it is, for the legs. */
	double animPhase;
	/** Seconds left of the white flash of being hit. */
	double flash;
	double homeX;
	double homeY;
	double leash;
	/** How long it has stood at the water's edge waiting for you to come out. */
	double shoreWait;
	/** Seconds until it may fire again. */
	double gunTimer;
	/** What a blow knocked into it, shed over the next moment. */
	double knockX;
	double knockY;
	/** Where it was posted, for the ones that hold a monument. */
	bool guard;
	/**
	 * What it is chasing or running from, and for how much longer.
	 *
	 * Zero means it has nothing in mind but you and the grass. Held for a few
	 * seconds so a wolf does not lose interest in a deer the instant the deer
	 * steps behind a tree.
	 */
	int target;
	double mind;
	/**
	 * Seconds left of being frightened.
	 *
	 * Set by anything that hurts it, and the reason an arrow out of nowhere
	 * sends a deer running: once it is fleeing its state is no longer Chase,
	 * so "has it been provoked" could not be read off the state alone.
	 */
	double alarm;
	std::uint32_t seed;
};

}  // namespace sim
