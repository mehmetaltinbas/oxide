#pragma once

#include "paint.hpp"
#include "sim/biome.hpp"
#include "sim/node.hpp"

namespace client {

/**
 * The colours of the world, carried over from the TypeScript game's tokens so
 * the rewrite looks like the game it replaces rather than like a new one.
 */

/** The pen. Not pure black: a very dark green-black sits better on foliage. */
constexpr Color kInk = rgb(0x14110d);
constexpr float kInkWidth = 2.6f;
constexpr float kInkMark = 2.2f;
constexpr float kInkFine = 1.5f;
/** Trees and ore are inked half again as heavily as everything else. */
constexpr float kNodeLineScale = 1.5f;

constexpr Color kSnow = rgb(0xf2f7fb);
constexpr Color kSkin = rgb(0xc08a5e);
constexpr Color kVoid = rgb(0x0a0c0a);

inline Color biomeColor(sim::Biome biome) {
    switch (biome) {
        case sim::Biome::Water: return rgb(0x2f80c8);
        case sim::Biome::Grass: return rgb(0x6c9c3b);
        case sim::Biome::Forest: return rgb(0x508a31);
        case sim::Biome::Beach: return rgb(0xe8cf96);
        case sim::Biome::SnowBeach: return rgb(0xece9dd);
        case sim::Biome::Desert: return rgb(0xd8b762);
        case sim::Biome::Snow: return rgb(0xdde9f2);
        case sim::Biome::Road: return rgb(0x726d62);
    }
    return rgb(0x6c9c3b);
}

inline Color nodeColor(sim::NodeKind kind) {
    switch (kind) {
        case sim::NodeKind::Tree: return rgb(0x3aa24a);
        case sim::NodeKind::Stone: return rgb(0x9aa6b2);
        case sim::NodeKind::Metal: return rgb(0xb69269);
        case sim::NodeKind::Sulfur: return rgb(0xe7dd64);
        case sim::NodeKind::Nettle: return rgb(0x8cc94a);
        case sim::NodeKind::Barrel: return rgb(0x7a6a44);
    }
    return rgb(0x9aa6b2);
}

/** The lighter tree of the grassland, in its two greens. */
constexpr Color kBroadleafLight = rgb(0x8fd15a);
constexpr Color kBroadleafDark = rgb(0x62a83c);

}  // namespace client
