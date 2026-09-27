#pragma once

namespace sim {

/**
 * How far past a monument's own radius you may not build.
 *
 * Their loot is what a run is for, and letting anyone wall one in would turn a
 * shared landmark into private property. The margin covers the approach as
 * well as the ground, so nobody rings one with walls and charges at the door.
 */
inline constexpr double kMonumentNoBuildMargin = 90;

/** How long an emptied crate takes to fill again. */
inline constexpr double kCrateRespawnSeconds = 300;

/**
 * How far apart the big places are kept: a third of the island's short side,
 * measured off the map rather than written out, so a bigger island spreads them
 * further rather than leaving them huddled in one corner.
 */
inline constexpr double kMonumentRelax = 0.85;
inline constexpr int kMonumentRounds = 12;

}  // namespace sim
