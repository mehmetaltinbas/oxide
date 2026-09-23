#include "sim/world_size.hpp"

// The sizes are compile-time constants; this file exists so the library has
// something to compile and the header's arithmetic is checked by the compiler.
namespace sim {

static_assert(kWorldWidth % kBiomeTile == 0, "the island must divide into whole biome tiles");
static_assert(kWorldWidth % kBuildCell == 0, "the island must divide into whole build cells");

}  // namespace sim
