#pragma once

#include <cstdint>

namespace sim {

/** The kind of work a thing wants: the right tool is worth half again. */
enum class Work : std::uint8_t { Chop, Mine, Pick, Break };

}  // namespace sim
