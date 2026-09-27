#pragma once

#include <string>

namespace client {

/** One island a server is running, as the lobby lists it. */
struct RoomEntry {
	std::uint16_t id = 0;
	std::string name;
	int players = 0;
	int max = 0;
};

}  // namespace client
