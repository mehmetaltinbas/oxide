#pragma once

#include "paint.hpp"
#include "sim/npc.hpp"

namespace client {

/**
 * One animal, seen from above: a body, a head at the front, four legs stepping
 * under it, and two eyes so you can tell which end is which at a glance.
 *
 * A name tag and a health bar go over it, because a bear and a boar are two
 * brown shapes otherwise, and you want to know which one you are walking
 * towards well before it has taken a scratch.
 */
void drawAnimal(Paint& paint, const sim::Npc& npc, float x, float y, float scale);

/**
 * The name over an animal, and the bar under it once it has taken a scratch.
 *
 * `uiScale` is the display's density: the tag is lettering, so it is sized in
 * points like the rest of the interface rather than in world pixels.
 */
void drawAnimalTag(Paint& paint, const sim::Npc& npc, float x, float y, float scale,
                   float uiScale);

}  // namespace client
