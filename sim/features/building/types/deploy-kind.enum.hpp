#pragma once

#include <cstdint>

namespace sim {

/** The things you put down rather than build: they sit on a cell of their own. */
enum class DeployKind : std::uint8_t {
	Campfire,
	Furnace,
	ToolCupboard,
	WoodenBox,
	LargeBox,
	SleepingBag,
	Workbench1,
	Workbench2,
	Workbench3,
};

/** How many there are, which is what the one table of them is checked against. */
inline constexpr int kDeployKindCount = 9;

}  // namespace sim
