#pragma once

#include <vector>

#include "client/features/render/systems/paint.hpp"
#include "client/features/ui/types/wheel-option.struct.hpp"

namespace client {

/**
 * A radial menu, held open rather than clicked through.
 *
 * You hold a key or a button down, the ring appears where the cursor is, you
 * move towards the slice you want and let go. Nothing is clicked, so nothing
 * can be mis-clicked, and the choice is made by direction rather than by
 * hitting a small target: the further out you push, the more certain it is.
 *
 * One wheel serves everything that needs one, because a second wheel would be
 * a second set of rules about how far out a slice counts and which way round
 * the options run.
 */
class Wheel {
public:
	bool open() const { return open_; }

	/** Opens it at a point on screen, with the slices in order from the top. */
	void show(std::vector<WheelOption> options, float x, float y);
	/** Closes it and says which slice was under the cursor, or -1 for none. */
	int release(float mouseX, float mouseY, float uiScale);
	void close() { open_ = false; }

	/** Which slice the cursor is on now, or -1 while it is still in the middle. */
	int hovered(float mouseX, float mouseY, float uiScale) const;

	void draw(Paint& paint, float mouseX, float mouseY, float uiScale) const;

private:
	bool open_ = false;
	std::vector<WheelOption> options_;
	float x_ = 0;
	float y_ = 0;
};

}  // namespace client
