#pragma once

#include "sim/shared/constants/health-display.constant.hpp"

namespace sim {

/**
 * The three routines every wounded thing goes through.
 *
 * Anything on the island that can be hurt and shows a bar for it, an animal, a
 * tree, a furnace, a wall, obeys one rule about when that bar is on screen,
 * and obeys it from here rather than from a condition written out again at
 * each of the four places that draw one. Writing it out again is how three of
 * them went on showing a bar for ever after the fourth stopped.
 *
 * A thing joins in by carrying `sinceHurt` beside its `hp`, calling
 * `tookDamage` wherever its health drops, and `ageWound` once per tick. The
 * drawing then asks `showsHealth` and never tests anything itself.
 *
 * See docs/systems/health-display.md.
 */

/** Whether a bar belongs on screen for something with this much left. */
inline bool showsHealth(int hp, int maxHp, double sinceHurt) {
	return hp > 0 && hp < maxHp && sinceHurt < kHealthShownFor;
}

/** The same, for anything that carries its own `maxHp`. */
template <class T>
bool showsHealth(const T& thing) {
	return showsHealth(thing.hp, thing.maxHp, thing.sinceHurt);
}

/**
 * Called wherever health comes off, and nowhere else.
 *
 * Not "was attacked": a blow that lands for nothing does not restart the
 * clock, because what the bar reports on is the wound.
 */
template <class T>
void tookDamage(T& thing) {
	thing.sinceHurt = 0;
}

/**
 * One tick of the wound getting older, for every one of them.
 *
 * Every one, wherever the player is and whether the thing is being simulated
 * closely or not. Standing next to something does not keep its bar up: the
 * wound is as old whether you are watching it or not, and a bar that comes
 * back when you walk over is worse than no bar.
 */
template <class T>
void ageWound(T& thing, double dt) {
	thing.sinceHurt += dt;
}

}  // namespace sim
