#pragma once

#include "sim/features/items/systems/inventory.hpp"
#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/**
 * Anything you do that takes time is bound to the thing in your hand.
 *
 * A reload, a bandage, a syringe, and whatever gets added next: all of them
 * are something being done *with* an item, and putting that item away stops
 * it. You cannot finish pushing a magazine into a rifle you have just slung,
 * or keep winding a dressing you have put back in your pack.
 *
 * The rule is enforced by the action itself rather than by whoever changed the
 * slot. Every tick, the action asks whether what it is being done with is
 * still in the hand, and stops the moment it is not. Nothing at the call sites
 * has to remember: a new way of changing slots, a death, a swap from a
 * container, all of them cancel it for free, and a new channelled action gets
 * the behaviour by asking the same question.
 *
 * See docs/systems/channelled-actions.md.
 */
inline bool stillInHand(const Inventory& inventory, ItemId what) {
	return what != ItemId::None && inventory.held() == what;
}

}  // namespace sim
