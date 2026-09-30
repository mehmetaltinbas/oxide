#include "sim/features/monuments/utils/raise-monuments.util.hpp"

#include <cmath>

#include "sim/features/monuments/constants/monument-rooms.constant.hpp"

namespace sim {

void raiseMonuments(const World& world, BuildSystem& build) {
	MonumentRoom rooms[kMonumentRoomsMax];
	for (const Monument& monument : world.monuments()) {
		const int n = monumentRooms(monument.kind, rooms);
		// The cell the middle of the monument falls in, which everything in
		// the plan is measured from.
		const int cx = static_cast<int>(std::floor(monument.x / kBuildCell));
		const int cy = static_cast<int>(std::floor(monument.y / kBuildCell));
		for (int i = 0; i < n; ++i) {
			const MonumentRoom& room = rooms[i];
			build.raiseMonumentRoom(cx + room.gx, cy + room.gy, room.wide, room.deep, room.roofed,
									room.doorSide, room.doorAt);
		}
	}
}

}  // namespace sim
