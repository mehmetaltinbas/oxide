#pragma once

#include <cstdint>

namespace sim {

/** What lives on the island, and who holds its monuments. */
enum class NpcKind : std::uint8_t { Rabbit, Elk, Kangaroo, Wolf, Bear, Scientist, Soldier };

inline constexpr int kNpcKindCount = 7;

}  // namespace sim
