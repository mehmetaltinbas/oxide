#pragma once

#include <cstdint>

namespace sim {

/** The looting places. Four are one to an island; two are scenery. */
enum class MonumentKind : std::uint8_t { Cabins, Lighthouse, Airfield, PowerPlant, Military, Town };

inline constexpr int kMonumentKindCount = 6;

}  // namespace sim
