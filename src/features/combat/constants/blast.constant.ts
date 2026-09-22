/**
 * How a blast that spreads (a rocket) falls off across what it reaches: full
 * damage at the middle, nothing at the edge, curved so the pieces right beside
 * the hit take most of it and the far side of the blast takes little.
 */
export const BLAST = {
    falloff: 1.5,
} as const;
