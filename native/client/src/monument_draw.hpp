#pragma once

#include "paint.hpp"
#include "sim/world.hpp"

namespace client {

/**
 * A looting place: its ground, the concrete laid on it, its name, and the
 * dashed ring round the hot ones telling you to turn back without a suit.
 */
void drawMonument(Paint& paint, const sim::Monument& monument, double cameraX, double cameraY,
                  double scale, int width, int height, float uiScale);

/** One crate of loot, standing where it was dropped. */
void drawCrate(Paint& paint, const sim::LootCrate& crate, float x, float y, float scale);

}  // namespace client
