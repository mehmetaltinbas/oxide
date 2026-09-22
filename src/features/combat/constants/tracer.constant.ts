/**
 * How a round is drawn in flight. Bigger rounds hit harder, so the tracer's
 * width and length both grow with the round's damage: an AK's 78 is drawn
 * heavier than a semi-auto's 62, which is heavier than a revolver's.
 */
export const TRACER = {
    /** Width of the tracer core before any damage is counted. */
    baseWidth: 1.4,
    /** Extra width per point of damage. */
    widthPerDamage: 0.03,
    /** Tracer length as a multiple of the round's own, per point of damage. */
    lengthPerDamage: 0.006,
    /** The black line round the core, each side. */
    inkWidth: 1.6,
    /**
     * A round fades out over its last moments in the air, and thins as it
     * goes. Long enough to read at a glance: a rifle round covers about 250
     * units in this time.
     */
    fadeSeconds: 0.2,
} as const;
