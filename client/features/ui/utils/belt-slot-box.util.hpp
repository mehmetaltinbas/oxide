#pragma once

#include "client/features/ui/types/belt-slot.struct.hpp"

namespace client {

/**
 * The belt's geometry, worked out in one place.
 *
 * The pack screen drags stacks into and out of the belt while it is open, so it
 * has to know where the belt is; sharing this is what stops the two drifting
 * apart by a few points and leaving a strip that looks like a slot and is not.
 */
BeltSlot beltSlotBox(int index, int width, int height, float uiScale);

/** Which belt slot a point is over, or -1. */
int beltSlotUnder(float px, float py, int width, int height, float uiScale);

}  // namespace client
