#include "sim/features/building/constants/deployable-defs.constant.hpp"
#include "sim/features/building/types/container.struct.hpp"
#include "sim/features/building/types/deploy-kind.enum.hpp"
#include "sim/features/items/constants/item-defs.constant.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/item-stack.struct.hpp"
#include "sim/features/building/constants/deploy-defs.constant.hpp"

#include <algorithm>

namespace sim {

int benchTier(DeployKind kind) { return deployDef(kind).bench; }

int containerSlots(DeployKind kind) { return deployDef(kind).slots; }

int Container::add(ItemId id, int count) {
	if (id == ItemId::None || count <= 0) return 0;
	const int stack = itemDef(id).stack;
	for (ItemStack& slot : slots) {
		if (count <= 0) break;
		if (slot.id != id) continue;
		const int put = std::min(stack - slot.count, count);
		slot.count += put;
		count -= put;
	}
	for (ItemStack& slot : slots) {
		if (count <= 0) break;
		if (slot.id != ItemId::None) continue;
		const int put = std::min(stack, count);
		slot = ItemStack{id, put};
		count -= put;
	}
	return count;
}

int Container::count(ItemId id) const {
	int total = 0;
	for (const ItemStack& slot : slots) {
		if (slot.id == id) total += slot.count;
	}
	return total;
}

int Container::take(ItemId id, int count) {
	int taken = 0;
	for (ItemStack& slot : slots) {
		if (count <= 0) break;
		if (slot.id != id) continue;
		const int off = std::min(slot.count, count);
		slot.count -= off;
		count -= off;
		taken += off;
		if (slot.count <= 0) slot = ItemStack{};
	}
	return taken;
}

bool deployableOf(ItemId id, DeployKind& out) {
	for (int i = 0; i < kDeployKindCount; ++i) {
		const DeployKind kind = static_cast<DeployKind>(i);
		if (deployDef(kind).item != id) continue;
		out = kind;
		return true;
	}
	return false;
}

ItemId itemOf(DeployKind kind) { return deployDef(kind).item; }

}  // namespace sim
