#pragma once

#include <vector>

#include "client/features/ui/types/wheel-option.struct.hpp"

namespace client {

/**
 * The building plan's ring, in the order of BuildKind.
 *
 * The order matters: the wheel returns the slice's index and the caller reads
 * it straight back as a BuildKind, so the two cannot drift apart.
 */
std::vector<WheelOption> buildPlanOptions();

}  // namespace client
