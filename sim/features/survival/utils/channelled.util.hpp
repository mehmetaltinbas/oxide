#pragma once

#include "sim/features/items/systems/inventory.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/survival/types/player.struct.hpp"

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

/**
 * Stops whatever is in progress, and gives back the time it was holding.
 *
 * This is the whole of stopping one, and it is one routine because the part
 * everybody forgets is the last line. A channelled action books `attackTimer`
 * for as long as it expects to take: five seconds for a bandage, four for a
 * magazine. Dropping the action and leaving the booking standing meant that
 * putting the bandage away bought you five seconds in which nothing else
 * worked either, and the slot you switched to did nothing at all.
 *
 * Anything that adds a new channelled action clears it through here rather
 * than by setting its own fields to nought, and gets that for free.
 */
inline void stopChannelling(Player& player) {
	player.applying = ItemId::None;
	player.useLeft = 0;
	player.useTotal = 0;
	player.reloadLeft = 0;
	player.reloadTotal = 0;
	player.bowDraw = 0;
	player.attackTimer = 0;
}

}  // namespace sim
