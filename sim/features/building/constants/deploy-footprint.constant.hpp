#pragma once

#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/shared/constants/world-size.constant.hpp"

namespace sim {

/**
 * How finely a building cell is divided for putting things down.
 *
 * Twelve by twelve. A deployable used to take a whole cell, so a campfire and
 * a furnace were the same size and two of anything could not share a room's
 * corner. Twelve divides by two, three, four and six, which is every
 * footprint on the island, so nothing ever lands on half a square.
 */
inline constexpr int kDeployGrid = 12;

/** One of those fine squares, in world units. */
inline constexpr double kDeployCell = static_cast<double>(kBuildCell) / kDeployGrid;

/**
 * Where the fine grid sits against the building grid.
 *
 * A fine square is **centred** on a building-grid line, not started at one. A
 * foundation therefore runs from the middle of one fine square to the middle
 * of another, which is how it reads against the squares drawn under it: half
 * a square over the edge at each end and eleven whole ones between. Started
 * at the line instead, a foundation's corner cut a fine square in half and
 * the two grids looked like a mistake.
 */
inline constexpr double kDeployOrigin = -kDeployCell * 0.5;

/**
 * How much floor a thing takes up, in fine squares, unturned.
 *
 * Measured the way it is drawn: `wide` runs across the screen and `deep` runs
 * down it. Turning a thing swaps them, which is the whole of what rotation
 * is.
 */
struct DeployFootprint {
	int wide;
	int deep;
};

/** The table, by kind. A new deployable is a new row here and nowhere else. */
DeployFootprint deployFootprint(DeployKind kind);

/** The same, with the turn applied: turned, wide and deep change places. */
inline DeployFootprint deployFootprint(DeployKind kind, bool turned) {
	const DeployFootprint f = deployFootprint(kind);
	return turned ? DeployFootprint{f.deep, f.wide} : f;
}

}  // namespace sim
