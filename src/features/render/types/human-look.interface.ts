/**
 * How one person is drawn: the player, somebody else in the room, or a
 * survivor, a raider or a scientist. See `drawHuman`.
 */
export interface HumanLook {
    /** Body radius the figure is scaled to. 13 is the player. */
    radius: number;
    skin: string;
    hair: string;
    /** The shirt or suit. Null is bare skin, which is how you wash up. */
    shirt: string | null;
    /** Trousers, and the shoes under them. */
    legs: string;
    /** Walk cycle in radians; feet and arms swing off it. */
    phase: number;
    /** 0 standing still, 1 walking at full stride. */
    stride: number;
    swimming: boolean;
    /** The right hand is out in front, holding something. */
    holding: boolean;
    /**
     * A bare-handed jab in progress: `t` runs 0 to 1 over the punch, `side`
     * is which fist throws it. Absent when not punching.
     */
    punch?: { t: number; side: -1 | 1 };
    /** Where the holding hand is, if not the usual spot out to the right. */
    holdAt?: [number, number];
    /** Where the other hand is, if it is doing something: drawing a string. */
    offHand?: [number, number];
    /** Paints what is in that hand, with the hand at the origin. */
    held?: (ctx: CanvasRenderingContext2D) => void;
    /** A full hood over the head, as the hazmat suit has. */
    hood: string | null;
    /** Everything tints to this while the hurt flash is on. */
    hurt: string | null;
}
