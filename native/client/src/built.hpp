#pragma once

#include "paint.hpp"
#include "sim/build.hpp"

namespace client {

/** Where a piece would go, worked out from where the cursor is. */
struct BuildTarget {
    sim::BuildKind kind = sim::BuildKind::Foundation;
    int gx = 0;
    int gy = 0;
    sim::EdgeSide side = sim::EdgeSide::North;
};

/** The cell under a point, and the edge nearest it. */
BuildTarget targetAt(double worldX, double worldY, sim::BuildKind kind);

/**
 * One built piece, seen from above: a floor you can walk over, a wall you
 * cannot, a doorway that is a gap between two posts, and a door in the gap.
 *
 * `x` and `y` are where the world's origin lands on the screen, so a piece
 * draws itself from its own place on the grid.
 */
void drawBuilt(Paint& paint, const sim::Structure& piece, double cameraX, double cameraY,
               double scale, int width, int height);

/** The same shape as a ghost: green where it may go, red where it may not. */
void drawGhost(Paint& paint, const BuildTarget& target, sim::BuildTier tier, bool allowed,
               double cameraX, double cameraY, double scale, int width, int height);

}  // namespace client
