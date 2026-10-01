#pragma once

namespace sim {

/**
 * The island's size, in world units, and the grids laid over it.
 *
 * One place for these, because everything else is measured off them: a bigger
 * island gets more ore and more monuments without a number being retyped
 * anywhere. Carried over from the TypeScript game, where 20736 was chosen
 * because it divides evenly by both the biome tile and the build cell.
 */
/**
 * How big the island is.
 *
 * Nineteen thousand two hundred: two hundred and forty foundations, two
 * hundred and forty ground tiles, and twenty lettered map squares of twelve
 * foundations each. Everything on the island is measured in foundations, and
 * anything that is not a whole number of them crosses the building grid at an
 * angle nobody can use.
 */
inline constexpr int kWorldWidth = 19200;
inline constexpr int kWorldHeight = 19200;

/**
 * How big one foundation is.
 *
 * Eighty, which is ten fine squares: a foundation is ten dots by ten dots,
 * and every square you build is the same. It was sixty-four, which is eight,
 * and eight was too few to lay anything out on: a furnace and a box took half
 * a room between them.
 */
inline constexpr int kBuildCell = 80;

/**
 * The square the ground is painted in.
 *
 * The same as a foundation, so where one country ends is somewhere you could
 * put a floor. At ninety-six it was close enough to eighty to look like it
 * ought to line up and never did, which is worse than being obviously
 * different: every shoreline and every treeline stepped across the building
 * grid at an angle.
 */
inline constexpr int kBiomeTile = kBuildCell;
inline constexpr int kBiomeCols = kWorldWidth / kBiomeTile;
inline constexpr int kBiomeRows = kWorldHeight / kBiomeTile;

}  // namespace sim
