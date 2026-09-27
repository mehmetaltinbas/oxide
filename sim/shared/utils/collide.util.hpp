#pragma once

#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/world/systems/world.hpp"
#include "sim/features/world/types/resource-node.struct.hpp"

namespace sim {

/**
 * Standing against things.
 *
 * One call, for everything that walks. Anything solid on the island is solid
 * for a player, for a deer and for a bear alike, and the way to keep it that
 * way is for there to be exactly one routine that knows what solid means. An
 * animal walked straight through a barrel for a month because the player had
 * its own copy of this and the wildlife had none.
 *
 * Pass a position and a radius; both are nudged out of anything they are
 * inside. Call it after moving and before writing the position back. See
 * docs/systems/collision.md.
 */
void keepOutOfSolids(const World& world, const BuildSystem& build, double& x, double& y,
					 double radius);

/**
 * Whether one thing standing on the island stops something walking into it.
 *
 * A new kind of solid is a new case here, never a new check at a call site.
 */
bool blocksMovement(const ResourceNode& node);

/**
 * How much of a node is actually solid.
 *
 * Less than it is drawn: a trunk is thinner than its canopy and a barrel is
 * narrower than the picture of one. Anything that needs to know where the
 * solid part of a thing ends asks here, so the collision and whatever is
 * checking it cannot disagree about where that is.
 */
double solidRadius(const ResourceNode& node);

/**
 * Wires a world to a build system, so everything it drops lands clear.
 *
 * Called once, at startup, by whoever owns both. After that no caller of
 * `World::dropStack` has to think about it, and a new way of dropping
 * something gets the behaviour for free. See docs/systems/dropped-items.md.
 */
void settleDropsAgainst(World& world, const BuildSystem& build);

}  // namespace sim
