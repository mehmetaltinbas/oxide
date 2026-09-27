#pragma once

#include <vector>

#include "client/features/ui/types/wheel-option.struct.hpp"
#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/items/systems/inventory.hpp"

namespace client {

/** What the hammer's ring gives back when the slice is "take it down". */
inline constexpr int kWheelWreck = -2;

/**
 * The ring of tiers a piece could be made of, in the order of BuildTier.
 *
 * Every tier is on it, including the ones you cannot have: what a thing could
 * become and what it will cost is the decision, and a ring that hides the
 * stone because you are short of it tells you nothing about what to go and
 * fetch. The ones out of reach are drawn dim and cannot be chosen.
 *
 * Taking it down is the last slice, in the warning colour. It belongs here
 * rather than on the left button: unbuilding is the one thing on the hammer
 * you cannot take back, and it should take the same deliberate hold-and-push
 * as choosing to spend two hundred stone.
 */
std::vector<WheelOption> tierOptions(const sim::Structure& piece,
									 const sim::Inventory& inventory);

}  // namespace client
