#include "sim/shared/utils/collide.util.hpp"
#include "sim/features/monuments/types/loot-crate.struct.hpp"
#include "sim/features/world/types/node-kind.enum.hpp"
#include "sim/features/world/types/resource-node.struct.hpp"

#include <cmath>
#include <vector>

namespace sim {

bool blocksMovement(const ResourceNode& node) {
	// What has been taken is gone until it grows back, and a nettle is a plant
	// you walk through. A tree, a boulder, an ore node and a barrel are not.
	if (node.hp <= 0) return false;
	return node.kind != NodeKind::Nettle;
}

void keepOutOfSolids(const World& world, const BuildSystem& build, double& x, double& y,
					 double radius) {
	// What has been built: walls, doors and the rest.
	build.resolve(x, y, radius);

	static thread_local std::vector<const ResourceNode*> near;
	const double reach = radius + 40;
	world.nodesInRect(x - reach, y - reach, x + reach, y + reach, near);
	for (const ResourceNode* node : near) {
		if (!blocksMovement(*node)) continue;
		const double dx = x - node->x;
		const double dy = y - node->y;
		// Six tenths of the drawn radius: a trunk is thinner than its canopy
		// and a barrel is narrower than the picture of one.
		const double min = radius + node->radius * 0.6;
		const double d2 = dx * dx + dy * dy;
		if (d2 >= min * min) continue;
		if (d2 < 0.0001) {
			x = node->x + min;
			continue;
		}
		const double d = std::sqrt(d2);
		x = node->x + dx / d * min;
		y = node->y + dy / d * min;
	}

	// The crates at a monument, while there is still something in them.
	for (const LootCrate& crate : world.crates()) {
		if (crate.looted) continue;
		const double dx = x - crate.x;
		const double dy = y - crate.y;
		const double min = radius + 14;
		const double d2 = dx * dx + dy * dy;
		if (d2 >= min * min || d2 < 0.0001) continue;
		const double d = std::sqrt(d2);
		x = crate.x + dx / d * min;
		y = crate.y + dy / d * min;
	}
}

}  // namespace sim
