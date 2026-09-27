#pragma once

namespace sim {

/** How far an animal will wander from where it was placed. */
inline constexpr double kNpcLeash = 520;
/**
 * Anything further than this from a player stops thinking, which keeps the
 * simulation flat however big the map gets.
 */
inline constexpr double kNpcActiveRadius = 1900;
/** How long an animal waits at the shore before giving up on a swimmer. */
inline constexpr double kShoreGiveUp = 3.0;

/**
 * How long after being hurt an animal still shows its health.
 *
 * A bar over everything you have ever shot at is a map of your own past, not
 * information about now. After this it goes away and the animal keeps the
 * health it had: hurt to 65 stays at 65, it simply stops saying so.
 *
 * A minute while this is being looked at. It is meant to sit at fifteen.
 */
inline constexpr double kHealthShownFor = 60.0;

}  // namespace sim
