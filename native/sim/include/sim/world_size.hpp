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
inline constexpr int kWorldWidth = 20736;
inline constexpr int kWorldHeight = 20736;

/** Terrain is painted from a coarse grid; everything alive lives in world units. */
inline constexpr int kBiomeTile = 96;
inline constexpr int kBiomeCols = kWorldWidth / kBiomeTile;
inline constexpr int kBiomeRows = kWorldHeight / kBiomeTile;

/** Building snaps to this grid. */
inline constexpr int kBuildCell = 64;

}  // namespace sim
