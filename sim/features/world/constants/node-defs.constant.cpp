#include "sim/features/world/constants/node-defs.constant.hpp"
#include "sim/features/world/types/node-def.struct.hpp"
#include "sim/features/world/types/node-kind.enum.hpp"
#include "sim/features/world/types/work.enum.hpp"
#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

namespace {

// Carried over from the TypeScript game, where these were tuned by playing.
constexpr NodeDef kDefs[kNodeKindCount] = {
	// A round splinters wood, at a fifth of what an axe does: you can fell a
	// tree with a rifle and you will spend a magazine doing it.
	{NodeKind::Tree, "Tree", 160, 17, Work::Chop, {{ItemId::Wood, 12, 1}}, 1, false, 0.2},
	// Nothing at all off rock or an ore seam. A bullet off stone takes the
	// stone with it, and a rifle must not be a faster pickaxe.
	{NodeKind::Stone, "Stone Node", 200, 18, Work::Mine, {{ItemId::Stone, 12, 1}}, 1, false, 0},
	// One blow in eight turns up a piece or two of high quality metal, which
	// is the whole supply of it outside a monument.
	{NodeKind::Metal, "Metal Node", 260, 19, Work::Mine,
	 {{ItemId::MetalOre, 6, 1}, {ItemId::Stone, 4, 1}, {ItemId::HqMetalOre, 1, 0.125}}, 3, false,
	 0},
	{NodeKind::Sulfur, "Sulfur Node", 260, 19, Work::Mine,
	 {{ItemId::SulfurOre, 6, 1}, {ItemId::Stone, 4, 1}}, 2, false, 0},
	{NodeKind::Nettle, "Nettle", 40, 11, Work::Pick, {{ItemId::Cloth, 10, 1}}, 1, false, 0.2},
	// A barrel bursts: that is what shooting one is for.
	{NodeKind::Barrel, "Barrel", 50, 14, Work::Break, {}, 0, true, 1.0},
};

}  // namespace

const NodeDef& nodeDef(NodeKind kind) { return kDefs[static_cast<int>(kind)]; }

}  // namespace sim
