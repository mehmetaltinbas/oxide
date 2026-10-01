#include "sim/features/building/constants/deploy-defs.constant.hpp"

namespace sim {

namespace {

// In the order of DeployKind, which the check below holds you to.
//
//                       kind                      item                   w  d  slots bench
constexpr DeployDef kDefs[] = {
	{DeployKind::Campfire, ItemId::Campfire, 2, 2, 4, 0},
	{DeployKind::Furnace, ItemId::Furnace, 3, 3, 6, 0},
	// A cupboard holds the upkeep, which is not in yet.
	{DeployKind::ToolCupboard, ItemId::ToolCupboard, 3, 2, 6, 0},
	// Three rows of six.
	{DeployKind::WoodenBox, ItemId::WoodenBox, 2, 2, 18, 0},
	// Seven rows of six: a large box is the reason a base has a loot room
	// rather than a wall of crates.
	{DeployKind::LargeBox, ItemId::LargeBox, 4, 2, 42, 0},
	// A bag holds you, not your things.
	{DeployKind::SleepingBag, ItemId::SleepingBag, 4, 2, 0, 0},
	// All three benches take the same floor and are told apart by what they
	// are made of: see the drawing. A bench you could squeeze in where the
	// next one would not fit made the tier a question of space.
	{DeployKind::Workbench1, ItemId::Workbench1, 3, 2, 0, 1},
	{DeployKind::Workbench2, ItemId::Workbench2, 3, 2, 0, 2},
	{DeployKind::Workbench3, ItemId::Workbench3, 3, 2, 0, 3},
};

static_assert(static_cast<int>(sizeof(kDefs) / sizeof(kDefs[0])) == kDeployKindCount,
			  "every deployable needs a row, and only one");

/** And in the right order, so `kind` can index the table directly. */
constexpr bool inOrder() {
	for (int i = 0; i < kDeployKindCount; ++i) {
		if (static_cast<int>(kDefs[i].kind) != i) return false;
	}
	return true;
}
static_assert(inOrder(), "the table must be in the order of DeployKind");

}  // namespace

const DeployDef& deployDef(DeployKind kind) {
	return kDefs[static_cast<int>(kind)];
}

}  // namespace sim
