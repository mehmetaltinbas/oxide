#pragma once

#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/world/systems/world.hpp"
#include "sim/features/survival/types/player-input.struct.hpp"
#include "sim/features/survival/types/player.struct.hpp"
#include "sim/features/survival/types/player-input.struct.hpp"

namespace sim {

/**
 * One step of a player's movement, on the client and on the server alike.
 *
 * The client runs it to move at once rather than waiting for the network, and
 * the server runs the same code on the same input, so the two agree without
 * anyone keeping two copies of the rules in step by hand.
 */
void stepPlayer(const World& world, const BuildSystem& build, Player& player,
				const PlayerInput& input, double dt);

}  // namespace sim
