#pragma once

#include <cstdint>

#include "sim/features/world/types/node-kind.enum.hpp"

namespace sim {

/** One of them, standing somewhere. */
struct ResourceNode {
	int id;
	NodeKind kind;
	double x;
	double y;
	double radius;
	int hp;
	int maxHp;
	/** Seconds until it grows back, once it has been taken. */
	double respawn;
	/** Seconds of shaking left from the last blow that landed on it. */
	double shake;
	/** Its own number, so its drawn shape never changes between frames. */
	std::uint32_t seed;
};

}  // namespace sim
