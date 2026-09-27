#pragma once

#include <cstdint>

namespace sim {

/** The things you put down rather than build: they sit on a cell of their own. */
enum class DeployKind : std::uint8_t {
	Campfire,
	Furnace,
	ToolCupboard,
	WoodenBox,
	SleepingBag,
	Workbench1,
	Workbench2,
	Workbench3,
};

}  // namespace sim
