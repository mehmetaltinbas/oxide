#pragma once

#include <cstdint>

#include "sim/item.hpp"

namespace sim {

/** Everything standing on the island that can be worked or broken. */
enum class NodeKind : std::uint8_t {
    Tree,
    Stone,
    Metal,
    Sulfur,
    Nettle,
    Barrel,
};

inline constexpr int kNodeKindCount = 6;

/** The kind of work a thing wants: the right tool is worth half again. */
enum class Work : std::uint8_t { Chop, Mine, Pick, Break };

/** What comes off it per blow, before the tool and the node's preference. */
struct NodeYield {
    ItemId id;
    int per;
};

/** What one kind of them is worth and how tough it is. */
struct NodeDef {
    NodeKind kind;
    const char* name;
    int hp;
    double radius;
    Work prefers;
    NodeYield yields[2];
    int yieldCount;
    /**
     * Whether it is smashed rather than worked: nothing comes off it until it
     * breaks, and then what was inside spills on the ground.
     */
    bool loot;
};

/** The table, in the order of NodeKind. */
const NodeDef& nodeDef(NodeKind kind);

/** One of them, standing somewhere. */
struct ResourceNode {
    int id;
    NodeKind kind;
    double x;
    double y;
    double radius;
    int hp;
    int maxHp;
    /** Seconds until it grows back, once it has been taken. */
    double respawn;
    /** Seconds of shaking left from the last blow that landed on it. */
    double shake;
    /** Its own number, so its drawn shape never changes between frames. */
    std::uint32_t seed;
};

}  // namespace sim
