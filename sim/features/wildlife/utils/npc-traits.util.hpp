#pragma once

#include "sim/features/world/types/biome.enum.hpp"
#include "sim/features/wildlife/types/npc-def.struct.hpp"
#include "sim/features/wildlife/types/npc-kind.enum.hpp"

namespace sim {

/** Whether that country is one of its homes. */
bool livesIn(const NpcDef& def, Biome biome);
/** Whether it hunts that, unprovoked. */
bool hunts(const NpcDef& def, NpcKind prey);

/**
 * Whether it runs from a player.
 *
 * Only something that does not hunt. `skittish` is about the pecking order
 * between animals, and a wolf is in it: it bolts from a bear. Reading that one
 * flag as "runs away from you" had wolves fleeing the thing they are supposed
 * to be the danger to.
 */
inline bool fleesPlayer(const NpcDef& def) { return def.skittish && !def.hostile; }

}  // namespace sim
