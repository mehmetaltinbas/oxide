import { MeleeStrike } from 'src/features/items/types/melee-strike.type';
/**
 * How an item sits in the hand, as seen from above.
 *
 * The item is drawn from its own icon, so the pose only says how to lay that
 * icon in the hand: how big it is, which point of it the fist closes on, and
 * how far to turn it so its business end faces forward.
 */
export interface HeldPose {
    /** Drawn size of the icon's unit square, in figure units (the player is 13 across). */
    size: number;
    /** The point of the icon the hand grips, in the icon's -0.5..0.5 square. */
    gripX: number;
    gripY: number;
    /** Radians to turn the icon so it points forward, up the figure. */
    angle: number;
    /**
     * How a melee blow is struck, which is also how the thing is carried:
     * a side blow (hatchet, pickaxe, hammer), a smash (a rock in the fist) or
     * a straight thrust (a spear). Absent is a side blow.
     */
    strike?: MeleeStrike;
}
