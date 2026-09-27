#pragma once

namespace client {

/** Where somebody was on one tick, as the server described them. */
struct Pose {
	std::uint16_t id = 0;
	double x = 0;
	double y = 0;
	double aim = 0;
	bool alive = true;
	bool sprinting = false;
	bool swimming = false;
	std::uint8_t team = 0;
};

}  // namespace client
