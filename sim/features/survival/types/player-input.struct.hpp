#pragma once

namespace sim {

/** What a player is asking to do this frame. */
struct PlayerInput {
	double moveX = 0;
	double moveY = 0;
	bool sprint = false;
	double aim = 0;
};

}  // namespace sim
