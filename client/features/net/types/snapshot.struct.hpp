#pragma once

#include "client/features/net/types/pose.struct.hpp"

#include <vector>

namespace client {

/** One tick's worth of everybody near you. */
struct Snapshot {
	std::uint32_t tick = 0;
	std::vector<Pose> poses;
};

}  // namespace client
