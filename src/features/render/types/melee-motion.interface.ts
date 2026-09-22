/**
 * Where a melee weapon is on one frame, in the figure's own units (forward is
 * negative y, the right shoulder at (10, -1)).
 */
export interface MeleeMotion {
    /** The holding hand. */
    hand: [number, number];
    /** How far the weapon is turned from pointing straight ahead, radians. */
    angle: number;
    /** Along-its-length scale, below 1 when it points up out of the page. */
    stretch: number;
    /** How far the upper body is turned into the blow. */
    twist: number;
    /**
     * The path the head of the weapon just swept, for motion lines: an arc
     * about the shoulder from `from` to `to` (canvas angles) at `radius`, or a
     * straight run for a smash. Absent between blows.
     */
    smear?:
        | { kind: 'arc'; radius: number; from: number; to: number }
        | { kind: 'line'; x0: number; y0: number; x1: number; y1: number };
}
