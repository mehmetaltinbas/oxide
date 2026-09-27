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
		case ItemCategory::Explosive: return true;
		// Not clothing. A suit is not a thing you hold: it goes in the pack
		// and from there onto your back, and a freshly made one filling a belt
		// slot pushed out the hatchet you actually had in your hand.
		default: return false;
	}
}

}  // namespace sim
