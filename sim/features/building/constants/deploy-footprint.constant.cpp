#include "sim/features/building/constants/deploy-footprint.constant.hpp"

namespace sim {

DeployFootprint deployFootprint(DeployKind kind) {
	switch (kind) {
		// Two by two: a fire you crouch at and a crate you kneel to.
		case DeployKind::Campfire: return {2, 2};
		case DeployKind::WoodenBox: return {2, 2};
		// Long things, twice across what they are deep.
		case DeployKind::LargeBox: return {4, 2};
		case DeployKind::SleepingBag: return {4, 2};
		// A cupboard is a cabinet, wider than it is deep but not by much.
		case DeployKind::ToolCupboard: return {3, 2};
		// The furnace is the one square thing that is not small.
		case DeployKind::Furnace: return {3, 3};
		// All three the same floor. A bench is a bench: what tells them apart
		// is what is on it, not how much room it takes, and a first bench you
		// could squeeze in where a second would not fit made the tier a
		// question of space rather than of materials.
		case DeployKind::Workbench1: return {3, 2};
		case DeployKind::Workbench2: return {3, 2};
		case DeployKind::Workbench3: return {3, 2};
	}
	return {2, 2};
}

}  // namespace sim
