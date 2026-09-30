#pragma once

namespace sim {

/**
 * One building at a monument, in cells, relative to the monument's middle.
 *
 * A monument used to be a circle with crates scattered in it: the radius was
 * the whole of its shape, so every one of them was the same place with
 * different loot in it. A room gives it a plan. Several rooms give it a shape
 * that is not a circle, which is most of what makes an airfield read as an
 * airfield and a town as a town.
 *
 * The rooms are built out of ordinary building pieces owned by the island, so
 * their walls stop you, their doorways let you in, and their roofs lift when
 * you are under them, all through the rules that already exist for a base.
 */
struct MonumentRoom {
	/** The near corner, in cells from the monument's middle cell. */
	int gx;
	int gy;
	/** How many cells across and down. At least one of each. */
	int wide;
	int deep;
	/** Whether it has a roof on it, or is a yard with a wall round it. */
	bool roofed;
	/**
	 * Which wall the way in is cut through, and where along it.
	 *
	 * 0 north, 1 south, 2 west, 3 east. `doorAt` is the cell along that wall,
	 * counted from the near corner.
	 */
	int doorSide;
	int doorAt;
};

}  // namespace sim
