#pragma once

#include <cstdint>

namespace sim {

/** What an animal is doing. */
enum class NpcState : std::uint8_t { Wander, Chase, Attack, Return, Flee };

}  // namespace sim
