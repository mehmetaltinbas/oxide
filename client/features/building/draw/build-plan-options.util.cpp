#include "client/features/building/draw/build-plan-options.util.hpp"

#include "sim/features/building/systems/build-system.hpp"
#include "sim/features/items/types/item-id.enum.hpp"

namespace client {

std::vector<WheelOption> buildPlanOptions() {
	// No icons: none of these is an item, and the shapes they would need are
	// the pieces themselves. The words carry it.
	// The value is the BuildKind, said out loud. It used to be left at its
	// default, so every slice on the ring answered "foundation".
	return {
		{"Foundation", sim::ItemId::None, "the floor", true,
		 static_cast<int>(sim::BuildKind::Foundation)},
		{"Wall", sim::ItemId::None, "closes a side", true,
		 static_cast<int>(sim::BuildKind::Wall)},
		{"Doorway", sim::ItemId::None, "a wall with a hole", true,
		 static_cast<int>(sim::BuildKind::Doorway)},
		{"Door", sim::ItemId::None, "goes in a doorway", true,
		 static_cast<int>(sim::BuildKind::Door)},
		{"Ceiling", sim::ItemId::None, "a roof, and the floor above", true,
		 static_cast<int>(sim::BuildKind::Ceiling)},
	};
}

}  // namespace client
