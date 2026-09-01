/**
 * The one progress indicator, drawn beside the player.
 *
 * Every timed action uses it: a bandage, a syringe, a magazine change. **The
 * arc is always the same size**; only how fast it fills changes, so a
 * three-second job visibly runs faster than a five-second one rather than the
 * two looking identical.
 *
 * It sits at the player's right rather than above the belt, because a timer you
 * have to look away from is a timer you do not watch.
 */
export const PROGRESS_ARC = {
    /** Distance from the player's centre. */
    radius: 20,
    /** Middle of the arc, in radians. Zero is screen-right. */
    center: 0,
    /** Half its angular width, so it spans twice this. */
    halfAngle: 0.8,
    width: 4,
} as const;
