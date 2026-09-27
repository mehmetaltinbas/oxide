#pragma once

#include "sim/features/wildlife/types/npc-def.struct.hpp"
#include "sim/features/wildlife/types/npc-kind.enum.hpp"

namespace sim {

const NpcDef& npcDef(NpcKind kind);

}  // namespace sim
