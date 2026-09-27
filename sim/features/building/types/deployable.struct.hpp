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
	Container container;
};

}  // namespace sim
