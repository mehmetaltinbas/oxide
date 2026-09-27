#pragma once

namespace sim {

/** What a charge does when it goes off. */
struct Boom {
	double damage;
	double radius;
	/** How long between throwing it and it going off. */
	double fuse;
};

}  // namespace sim
