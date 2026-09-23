#pragma once

#include <cstdint>

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

/** What one kind of them is worth and how tough it is. */
struct NodeDef {
    NodeKind kind;
    const char* name;
    int hp;
    double radius;
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
    /** Its own number, so its drawn shape never changes between frames. */
    std::uint32_t seed;
};

}  // namespace sim
