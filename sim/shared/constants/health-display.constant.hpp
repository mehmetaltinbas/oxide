#pragma once

namespace sim {

/**
 * How long after it was last hurt a thing still shows its health.
 *
 * A bar over everything you have ever hit is a map of your own past, not
 * information about now: a forest of half-chopped trees, every one of them
 * still advertising it, says nothing about what is happening. After this the
 * bar goes and the thing keeps the health it had. Hurt to 65 stays at 65, it
 * simply stops saying so.
 *
 * The clock runs wherever you are. Standing next to something does not keep
 * its bar up: what the bar reports on is the wound, and the wound is as old
 * whether you are watching it or not.
 *
 * One number for animals, resource nodes, deployables and building pieces
 * alike, because they are one rule. Fifteen minutes: long enough that a bar
 * is still up when you come back to finish something off, short enough that
 * an afternoon's work is not still on the screen at dusk.
 */
inline constexpr double kHealthShownFor = 900.0;

}  // namespace sim
