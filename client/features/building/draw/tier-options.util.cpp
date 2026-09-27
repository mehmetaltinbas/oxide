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
	// Twig is where everything starts and nothing goes back to: it is not on
	// the ring, because a ring with a slice you can never pick on it is a ring
	// you have to read before you can use.
	for (int i = static_cast<int>(sim::BuildTier::Wood); i < sim::kBuildTierCount; ++i) {
		const sim::BuildTier tier = static_cast<sim::BuildTier>(i);
		const sim::TierDef& def = sim::tierDef(tier);
		SDL_snprintf(notes[i], sizeof(notes[i]), "%d %s", def.cost.count,
					 sim::itemDef(def.cost.id).name);
		const bool higher = i > static_cast<int>(piece.tier);
		const bool afford = inventory.count(def.cost.id) >= def.cost.count;
		out.push_back(WheelOption{def.name, def.cost.id, notes[i], higher && afford, i, false});
	}
	// And the way back down, if it is still young enough and yours.
	static char window[40];
	const double left = sim::BuildSystem::kFreeDemolishSeconds - piece.age;
	const bool canWreck = piece.owner == 0 && left > 0;
	if (canWreck) {
		SDL_snprintf(window, sizeof(window), "%dm%02ds left", static_cast<int>(left) / 60,
					 static_cast<int>(left) % 60);
	} else {
		SDL_snprintf(window, sizeof(window), "too old");
	}
	out.push_back(
		WheelOption{"Take it down", sim::ItemId::Hammer, window, canWreck, kWheelWreck, true});
	return out;
}

}  // namespace client
