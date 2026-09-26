#pragma once

#include "paint.hpp"
#include "sim/deployable.hpp"

namespace client {

/**
 * The things people put down: a fire, a furnace, a box, a cupboard, a bag.
 *
 * Each is drawn from above at the size it actually blocks, so what you walk
 * into is what you can see.
 */
void drawDeployable(Paint& paint, const sim::Deployable& deployable, float x, float y,
                    float scale, float clock = 0);

}  // namespace client
