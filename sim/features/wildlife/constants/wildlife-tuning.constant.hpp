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

}  // namespace sim
