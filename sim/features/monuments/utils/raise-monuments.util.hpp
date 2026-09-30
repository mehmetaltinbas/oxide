#pragma once

#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/world/systems/world.hpp"

namespace sim {

/**
 * Puts up the buildings at every monument on the island.
 *
 * Called once, after the world is generated and before anybody walks about,
 * by whoever owns both the world and the building system. The world knows
 * where the monuments are and the building system holds what is built, and
 * neither knows about the other: this is the one place that does.
 *
 * What it raises is ordinary building pieces owned by kIslandOwner, so a
 * monument's walls stop you, its doorways let you in and its roofs lift when
 * you are under them, all through rules a base already has.
 */
void raiseMonuments(const World& world, BuildSystem& build);

}  // namespace sim
