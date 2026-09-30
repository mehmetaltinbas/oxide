#pragma once

#include "sim/features/monuments/types/monument-kind.enum.hpp"
#include "sim/features/monuments/types/monument-room.struct.hpp"

namespace sim {

/** The most rooms any one monument has. */
inline constexpr int kMonumentRoomsMax = 6;

/**
 * The plan of a monument, in cells about its middle.
 *
 * Fills `out` and gives back how many it wrote. A monument with none is open
 * ground with crates on it, which is what the cabins and the lighthouse were
 * and still are.
 */
int monumentRooms(MonumentKind kind, MonumentRoom out[kMonumentRoomsMax]);

}  // namespace sim
