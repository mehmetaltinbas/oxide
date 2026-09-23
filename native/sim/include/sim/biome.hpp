#pragma once

#include <cstdint>

namespace sim {

/** What a tile of ground is. */
enum class Biome : std::uint8_t {
    Water,
    Grass,
    Forest,
    Beach,
    SnowBeach,
    Desert,
    Snow,
    Road,
};

/** How many kinds there are, for anything that wants a table per biome. */
inline constexpr int kBiomeCount = 8;

}  // namespace sim
