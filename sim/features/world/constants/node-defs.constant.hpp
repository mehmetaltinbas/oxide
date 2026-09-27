#pragma once

#include "sim/features/world/types/node-def.struct.hpp"
#include "sim/features/world/types/node-kind.enum.hpp"

namespace sim {

/** The table, in the order of NodeKind. */
const NodeDef& nodeDef(NodeKind kind);

}  // namespace sim
