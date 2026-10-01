#pragma once

#include "sim/features/building/types/deploy-def.struct.hpp"
#include "sim/features/building/types/deploy-kind.enum.hpp"

namespace sim {

/**
 * The one table of deployables, in the order of DeployKind.
 *
 * Everything that wants to know anything about one asks here. Adding a
 * deployable is adding a value to the enum and a row to this table, and the
 * table is checked against the enum at compile time, so a missing row is a
 * build error rather than a thing that silently behaves like a campfire.
 */
const DeployDef& deployDef(DeployKind kind);

}  // namespace sim
