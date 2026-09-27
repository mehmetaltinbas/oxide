#pragma once

#include "sim/features/items/constants/item-defs.constant.hpp"
#include "sim/features/items/types/item-category.enum.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/building/types/deployable.struct.hpp"

namespace sim {

/**
 * Whether a thing belongs on the belt.
 *
 * Anything you can hold goes there first, so a freshly crafted hatchet is in
 * your hand rather than buried in a bag. Wood and ore are not belt items: they
 * would fill it with things you never hold.
 *
 * Raw food is the odd one out. It is a consumable by the rules of eating, but
 * it is not a thing you hold: you carry it to a fire and cook it. Eight kills
 * in a row used to leave the belt full of meat and no room for the hatchet.
 */
inline bool isBeltItem(ItemId id) {
	if (id == ItemId::MeatRaw) return false;
	switch (itemDef(id).category) {
		case ItemCategory::Tool:
		case ItemCategory::Weapon:
		case ItemCategory::Consumable:
		case ItemCategory::Deployable:
		case ItemCategory::Explosive:
		case ItemCategory::Clothing: return true;
		default: return false;
	}
}

}  // namespace sim
