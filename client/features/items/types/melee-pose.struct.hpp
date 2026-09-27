#pragma once

#include "client/features/render/types/point.struct.hpp"

namespace client {

/** One frame of a swing: where the hand is and how the tool sits in it. */
struct MeleePose {
	Point hand{12, -6};
	/** The tool's own angle, about the grip. */
	float angle = 1.5707963f;
	/** How long it looks along its length; below 1 is foreshortening. */
	float stretch = 0.55f;
	/** The whole upper body's turn into the blow. */
	float twist = 0;
	/** Whether what sits over the fist is the head rather than the grip. */
	bool overHand = true;
};

}  // namespace client
