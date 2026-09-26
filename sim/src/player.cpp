#include "sim/player.hpp"

#include "sim/collide.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace sim {

void stepPlayer(const World& world, const BuildSystem& build, Player& player,
				const PlayerInput& input, double dt) {
	player.aim = input.aim;
	if (player.attackTimer > 0) player.attackTimer = std::max(0.0, player.attackTimer - dt);
	if (player.swingAnim > 0) player.swingAnim = std::max(0.0, player.swingAnim - dt);
	player.swimming = world.biomeAt(player.x, player.y) == Biome::Water;

	double mx = input.moveX;
	double my = input.moveY;
	const double len = std::sqrt(mx * mx + my * my);
	const bool moving = len > 0.0001;
	if (moving) {
		mx /= len;
		my /= len;
	}

	player.sprinting = input.sprint && moving && !player.swimming;
	double speed = PlayerRules::kSpeed * (player.sprinting ? PlayerRules::kSprint : 1.0);
	if (player.swimming) speed *= PlayerRules::kSwim;

	player.x += mx * speed * dt;
	player.y += my * speed * dt;

	// The edge of the map is the edge of the world, as in the other game: you
	// swim to it and no further.
	const double edge = PlayerRules::kRadius;
	if (player.x < edge) player.x = edge;
	if (player.y < edge) player.y = edge;
	if (player.x > kWorldWidth - edge) player.x = kWorldWidth - edge;
	if (player.y > kWorldHeight - edge) player.y = kWorldHeight - edge;

	// One routine for everything that walks. See sim/collide.hpp.
	keepOutOfSolids(world, build, player.x, player.y, PlayerRules::kRadius);

	if (moving) {
		player.walkPhase += dt * (player.sprinting ? 14 : 9);
	} else {
		player.walkPhase -= player.walkPhase * dt * 8;
	}
}

}  // namespace sim
