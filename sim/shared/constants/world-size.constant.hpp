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
// A multiple of both the biome tile and the build cell, which is what the
// assertions below are about: 20640 is 215 tiles and 258 foundations. It was
// 20736, which divides by ninety-six and by sixty-four but not by eighty.
inline constexpr int kWorldWidth = 20640;
inline constexpr int kWorldHeight = 20640;

/** Terrain is painted from a coarse grid; everything alive lives in world units. */
inline constexpr int kBiomeTile = 96;
inline constexpr int kBiomeCols = kWorldWidth / kBiomeTile;
inline constexpr int kBiomeRows = kWorldHeight / kBiomeTile;

/** Building snaps to this grid. */
/**
 * How big one foundation is.
 *
 * Eighty, which is ten fine squares: a foundation is ten dots by ten dots,
 * and every square you build is the same. It was sixty-four, which is eight,
 * and eight was too few to lay anything out on: a furnace and a box took half
 * a room between them.
 */
inline constexpr int kBuildCell = 80;

}  // namespace sim
