#pragma once

#include "sim/features/items/types/gun.struct.hpp"
#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/**
 * Whether a thing is fired rather than swung.
 *
 * Not `damage > 0`, which is what this used to be everywhere. A rocket
 * launcher does no damage of its own: the rocket carries the blast, so the
 * launcher's own damage is nought, and reading that as "not a gun" meant it
 * could not be loaded, could not be fired and showed no ammunition. What makes
 * something ranged is that it takes ammunition and sends it somewhere.
 */
inline bool isRanged(const Gun& gun) {
	return gun.ammo != ItemId::None && gun.speed > 0;
}

}  // namespace sim
