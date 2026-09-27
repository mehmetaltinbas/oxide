#pragma once

#include "sim/features/monuments/types/monument-def.struct.hpp"
#include "sim/features/monuments/types/monument-kind.enum.hpp"

namespace sim {

const MonumentDef& monumentDef(MonumentKind kind);

}  // namespace sim
