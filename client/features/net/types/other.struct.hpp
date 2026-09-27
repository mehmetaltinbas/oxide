#pragma once

#include <string>

namespace client {

/** Somebody else on the island, as they are drawn. */
struct Other {
	std::uint16_t id = 0;
	std::string name = "survivor";
	double x = 0;
	double y = 0;
	/** Where they are drawn: a hundred milliseconds behind, and smoothed. */
	double drawX = 0;
	double drawY = 0;
	double aim = 0;
	bool alive = true;
	bool sprinting = false;
	bool swimming = false;
	std::uint8_t team = 0;
	/** How far through a stride they look to be, worked out from how they move. */
	double walkPhase = 0;
};

}  // namespace client
