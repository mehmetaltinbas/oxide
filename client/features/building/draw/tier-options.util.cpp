#include "client/features/building/draw/tier-options.util.hpp"

#include <SDL3/SDL.h>

#include "sim/features/items/constants/item-defs.constant.hpp"

namespace client {

std::vector<WheelOption> tierOptions(const sim::Structure& piece,
									 const sim::Inventory& inventory) {
	// The notes are kept alive between calls: the wheel holds the pointers it
	// is given, and the ring is rebuilt every time it opens.
	static char notes[sim::kBuildTierCount][32];
	std::vector<WheelOption> out;
	for (int i = 0; i < sim::kBuildTierCount; ++i) {
		const sim::BuildTier tier = static_cast<sim::BuildTier>(i);
		const sim::TierDef& def = sim::tierDef(tier);
		SDL_snprintf(notes[i], sizeof(notes[i]), "%d %s", def.cost.count,
					 sim::itemDef(def.cost.id).name);
		const bool higher = i > static_cast<int>(piece.tier);
		const bool afford = inventory.count(def.cost.id) >= def.cost.count;
		out.push_back(WheelOption{def.name, def.cost.id, notes[i], higher && afford});
	}
	return out;
}

}  // namespace client
