#include "sim/features/monuments/constants/monument-rooms.constant.hpp"

namespace sim {

int monumentRooms(MonumentKind kind, MonumentRoom out[kMonumentRoomsMax]) {
	int n = 0;
	const auto room = [&](int gx, int gy, int wide, int deep, bool roofed, int side, int at) {
		if (n >= kMonumentRoomsMax) return;
		out[n++] = MonumentRoom{gx, gy, wide, deep, roofed, side, at};
	};
	switch (kind) {
		case MonumentKind::Cabins:
			// Three huts round a clearing, all doors facing in: somewhere a
			// fresh spawn can loot on day one, so it is small and open.
			room(-3, -2, 2, 2, true, 3, 1);
			room(1, -3, 2, 2, true, 1, 0);
			room(0, 1, 2, 2, true, 0, 1);
			break;
		case MonumentKind::Lighthouse:
			// The tower and the keeper's house beside it, on the shore.
			room(-1, -2, 2, 2, true, 1, 1);
			room(-3, 1, 3, 2, true, 0, 1);
			break;
		case MonumentKind::Airfield:
			// A long hangar with the runway beside it, a tower at one end and
			// a walled yard between them. Nothing square about the whole.
			room(-5, -1, 7, 3, true, 3, 1);
			room(3, -3, 2, 2, true, 2, 1);
			room(3, 1, 3, 3, false, 2, 1);
			room(-5, 3, 4, 2, true, 0, 2);
			break;
		case MonumentKind::PowerPlant:
			// Two halls and the yard between them, and a shed on the corner.
			// The hot one, so what you want is inside and under a roof.
			room(-5, -3, 4, 4, true, 3, 2);
			room(1, -3, 4, 4, true, 2, 2);
			room(-2, 2, 5, 3, false, 0, 2);
			room(3, 2, 2, 2, true, 2, 0);
			break;
		case MonumentKind::Military:
			// A compound: a wall round a yard with two blocks inside it and a
			// gate on one side, which is the shape of the place that fights.
			room(-5, -4, 10, 9, false, 1, 5);
			room(-4, -3, 3, 3, true, 3, 1);
			room(1, -3, 3, 3, true, 2, 1);
			room(-1, 1, 3, 2, true, 0, 1);
			break;
		case MonumentKind::Town:
			// A street: houses down both sides of a gap, which is why it is
			// the one monument you walk through rather than into.
			room(-6, -3, 3, 2, true, 1, 1);
			room(-2, -3, 3, 2, true, 1, 1);
			room(2, -3, 3, 2, true, 1, 1);
			room(-6, 2, 3, 2, true, 0, 1);
			room(-2, 2, 3, 2, true, 0, 1);
			room(2, 2, 3, 2, true, 0, 1);
			break;
	}
	return n;
}

}  // namespace sim
