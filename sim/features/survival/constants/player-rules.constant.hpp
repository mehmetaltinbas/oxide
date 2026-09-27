#pragma once

namespace sim {

/** The numbers a player moves by, shared with the server that checks them. */
struct PlayerRules {
	static constexpr double kRadius = 13;
	static constexpr double kSpeed = 132;
	static constexpr double kSprint = 1.4;
	/** Swimming: slow, and no sprinting out of it. */
	static constexpr double kSwim = 0.46;
};

}  // namespace sim
