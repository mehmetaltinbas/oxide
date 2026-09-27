#pragma once

namespace sim {

/** What one of them carries, and nothing for the ones that do not. */
struct NpcGun {
	double damage;
	double range;
	double cooldown;
	double speed;
	double spread;
};

}  // namespace sim
