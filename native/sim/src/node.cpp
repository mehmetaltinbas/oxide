#include "sim/node.hpp"

namespace sim {

namespace {

// Carried over from the TypeScript game, where these were tuned by playing.
constexpr NodeDef kDefs[kNodeKindCount] = {
    {NodeKind::Tree, "Tree", 160, 17},
    {NodeKind::Stone, "Stone Node", 200, 18},
    {NodeKind::Metal, "Metal Node", 260, 19},
    {NodeKind::Sulfur, "Sulfur Node", 260, 19},
    {NodeKind::Nettle, "Nettle", 40, 11},
    {NodeKind::Barrel, "Barrel", 50, 14},
};

}  // namespace

const NodeDef& nodeDef(NodeKind kind) { return kDefs[static_cast<int>(kind)]; }

}  // namespace sim
