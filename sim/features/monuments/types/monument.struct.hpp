#pragma once

#include "sim/features/monuments/types/monument-kind.enum.hpp"

namespace sim {

/** One of them, standing somewhere. */
struct Monument {
	int id;
	MonumentKind kind;
	double x;
	double y;
	double radius;
};

}  // namespace sim
