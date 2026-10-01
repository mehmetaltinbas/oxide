#pragma once

namespace sim {

/**
 * How close anybody may be when a resource comes back.
 *
 * A hundred units, a foundation and a quarter. A tree that grows through
 * somebody standing beside it is a tree that was not there a moment ago and
 * is now inside them, and a boulder popping out under a player mid-fight is
 * worse. The regrowth waits, keeps its place, and happens the moment the
 * ground is clear.
 *
 * The one number to turn if things come back too close, or make you wait too
 * long before they come back at all.
 */
inline constexpr double kRegrowthClearance = 100;

/**
 * How often a held regrowth asks again.
 *
 * A second. This is a poll, not a timer: the node is ready and is waiting for
 * the ground to clear, so the question has to be asked often enough that
 * walking away from a spot brings the tree back while you are still looking
 * at it.
 */
inline constexpr double kRegrowthRetry = 1.0;

}  // namespace sim
