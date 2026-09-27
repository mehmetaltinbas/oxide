#pragma once

#include <vector>

#include "client/features/render/types/point.struct.hpp"

namespace client {

/** The points of a circle, for when a shape needs them rather than the fill. */
std::vector<Point> circlePoints(float cx, float cy, float radius, int segments = 0);

/** The same, squashed: a shadow on the ground is an ellipse, not a circle. */
std::vector<Point> ellipsePoints(float cx, float cy, float rx, float ry, float turn = 0,
								 int segments = 0);

}  // namespace client
