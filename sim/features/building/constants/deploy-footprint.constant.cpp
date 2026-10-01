#include "sim/features/building/constants/deploy-footprint.constant.hpp"
#include "sim/features/building/constants/deploy-defs.constant.hpp"

namespace sim {

DeployFootprint deployFootprint(DeployKind kind) {
	const DeployDef& def = deployDef(kind);
	return {def.wide, def.deep};
}

}  // namespace sim
