#pragma once

#include <cstdint>

namespace sim {

/** Everything standing on the island that can be worked or broken. */
enum class NodeKind : std::uint8_t {
	Tree,
	Stone,
	Metal,
	Sulfur,
	Nettle,
	Barrel,
};

inline constexpr int kNodeKindCount = 6;

}  // namespace sim
