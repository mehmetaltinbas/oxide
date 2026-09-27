#pragma once

namespace sim {

/**
 * How long anything lying on the ground lasts before the island takes it back.
 *
 * Thirty minutes, which is half an in-game day: long enough to come back for a
 * kill you could not carry, short enough that a raid does not leave a field of
 * loot lying about for ever. One number for every kind of drop there is, and
 * the only one: see docs/systems/ground-items.md.
 */
inline constexpr double kDropLifetime = 1800;

}  // namespace sim
