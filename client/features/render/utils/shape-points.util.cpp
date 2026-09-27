#include "client/features/render/utils/shape-points.util.hpp"
#include "client/features/render/types/point.struct.hpp"

#include <algorithm>
#include <cmath>

namespace client {

namespace {

/** Enough segments that a circle reads as round at the size it is drawn. */
int segmentsFor(float radius) {
	const int n = static_cast<int>(radius * 0.9f) + 8;
	return n > 64 ? 64 : n;
}

}  // namespace

std::vector<Point> circlePoints(float cx, float cy, float radius, int segments) {
	if (segments <= 0) segments = segmentsFor(radius);
	std::vector<Point> out;
	out.reserve(segments);
	for (int i = 0; i < segments; ++i) {
		const float a = static_cast<float>(i) / segments * 6.28318530718f;
		out.push_back({cx + std::cos(a) * radius, cy + std::sin(a) * radius});
	}
	return out;
}

std::vector<Point> ellipsePoints(float cx, float cy, float rx, float ry, float turn,
								 int segments) {
	if (segments <= 0) segments = segmentsFor(std::max(rx, ry));
	const float ca = std::cos(turn);
	const float sa = std::sin(turn);
	std::vector<Point> out;
	out.reserve(segments);
	for (int i = 0; i < segments; ++i) {
		const float a = static_cast<float>(i) / segments * 6.28318530718f;
		const float x = std::cos(a) * rx;
		const float y = std::sin(a) * ry;
		out.push_back({cx + x * ca - y * sa, cy + x * sa + y * ca});
	}
	return out;
}

}  // namespace client
