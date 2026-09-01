/**
 * How far past a monument's own radius you may not build.
 *
 * Monuments are the looting locations: every crate on the island sits inside
 * one, and their loot is what a run is for. Letting anyone wall one in would
 * turn a shared landmark into private property, which is the one thing it must
 * never be. The margin covers the approach as well as the ground, so nobody can
 * ring a monument with walls and charge at the door.
 */
export const MONUMENT_NO_BUILD_MARGIN = 90;
