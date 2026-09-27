#pragma once

#include "sim/features/building/types/container.struct.hpp"
#include "sim/features/building/types/deploy-kind.enum.hpp"

namespace sim {

struct Deployable {
	int id;
	DeployKind kind;
	double x;
	double y;
	int hp;
	int maxHp;
	int owner;
	bool lit;
	/** Seconds of fuel left, and how far through the current piece of work. */
	double fuel;
	double progress;
	double flash;
	/** Seconds since anything last took health off it: see kHealthShownFor. */
	double sinceHurt = 0;
	/**
	 * Whether it was put down turned a quarter.
	 *
	 * Its footprint swaps across and down, and so does its drawing. One flag
	 * rather than an angle: a thing on a square grid can face two ways that
	 * matter, and a free angle would mean a free footprint too.
	 */
	bool turned = false;
	Container container;
};

}  // namespace sim
