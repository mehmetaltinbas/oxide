#pragma once

#include <cstdint>

namespace client {

/** How a node is dressed: its own look, and whether it stands in the snow. */
struct SpriteKey {
	sim::NodeKind kind;
	int variant;
	bool snowy;
	bool broadleaf;
};

}  // namespace client
