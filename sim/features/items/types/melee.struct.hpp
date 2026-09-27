#pragma once

namespace sim {

/** What a tool does when it lands. */
struct Melee {
	double damage;
	/** How much comes off a node per blow, before the node's own preference. */
	double gather;
	double cooldown;
	/** How far the head of it reaches, measured off the swing as it is drawn. */
	double reach;
};

}  // namespace sim
