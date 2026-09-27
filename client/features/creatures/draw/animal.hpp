#pragma once

#include "client/features/render/systems/paint.hpp"
#include "sim/features/wildlife/types/npc.struct.hpp"

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
 * Both are part of the animal, not of the interface: they are measured in world
 * units and so grow when you zoom in and shrink when you zoom out, like
 * everything else standing on the island.
 */
void drawAnimalTag(Paint& paint, const sim::Npc& npc, float x, float y, float scale);

}  // namespace client
