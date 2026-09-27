#include "client/features/building/draw/build-plan-options.util.hpp"

#include "sim/features/items/types/item-id.enum.hpp"

namespace client {

std::vector<WheelOption> buildPlanOptions() {
	// No icons: none of these is an item, and the shapes they would need are
	// the pieces themselves. The words carry it.
	return {
		{"Foundation", sim::ItemId::None, "the floor"},
		{"Wall", sim::ItemId::None, "closes a side"},
		{"Doorway", sim::ItemId::None, "a wall with a hole"},
		{"Door", sim::ItemId::None, "goes in a doorway"},
		{"Ceiling", sim::ItemId::None, "a roof, and the floor above"},
	};
}

}  // namespace client
