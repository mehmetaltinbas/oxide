#include "sim/node.hpp"

namespace sim {

namespace {

// Carried over from the TypeScript game, where these were tuned by playing.
constexpr NodeDef kDefs[kNodeKindCount] = {
	{NodeKind::Tree, "Tree", 160, 17, Work::Chop, {{ItemId::Wood, 12, 1}}, 1, false},
	{NodeKind::Stone, "Stone Node", 200, 18, Work::Mine, {{ItemId::Stone, 12, 1}}, 1, false},
	// One blow in eight turns up a piece or two of high quality metal, which
	// is the whole supply of it outside a monument.
	{NodeKind::Metal, "Metal Node", 260, 19, Work::Mine,
	 {{ItemId::MetalOre, 6, 1}, {ItemId::Stone, 4, 1}, {ItemId::HqMetalOre, 1, 0.125}}, 3, false},
	{NodeKind::Sulfur, "Sulfur Node", 260, 19, Work::Mine,
	 {{ItemId::SulfurOre, 6, 1}, {ItemId::Stone, 4, 1}}, 2, false},
	{NodeKind::Nettle, "Nettle", 40, 11, Work::Pick, {{ItemId::Cloth, 10, 1}}, 1, false},
	{NodeKind::Barrel, "Barrel", 50, 14, Work::Break, {}, 0, true},
};

}  // namespace

const NodeDef& nodeDef(NodeKind kind) { return kDefs[static_cast<int>(kind)]; }

}  // namespace sim
