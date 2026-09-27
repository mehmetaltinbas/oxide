#pragma once

#include "client/design/types/color.struct.hpp"

namespace client {

/** One speck: a chip of wood, a spark, a drop of blood. */
struct Particle {
	double x;
	double y;
	double vx;
	double vy;
	double life;
	double total;
	double size;
	double gravity;
	Color color;
};

}  // namespace client
