/**
 * How far away someone else's gunfire can be heard, in world units, and how
 * loud it is at the shooter's own spot. It falls away to nothing at `range`,
 * faster than linearly, the way a shot across a valley is a pop.
 */
export const GUNFIRE_HEARING = {
    range: 1400,
    nearLoudness: 0.75,
    falloff: 1.6,
} as const;
