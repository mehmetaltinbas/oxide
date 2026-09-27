#pragma once

namespace client {

/**
 * What the detail pane's buttons do.
 *
 * The pack screen knows what a thing is and where it sits; it does not know how
 * eating or getting dressed works, so the game hands it these and the screen
 * calls whichever one the button says.
 */
struct ItemActions {
	std::function<void(sim::ItemId)> consume;
	std::function<void(sim::ItemId)> wear;
	std::function<void()> takeOff;
	std::function<void(sim::ItemStack)> drop;
};

}  // namespace client
