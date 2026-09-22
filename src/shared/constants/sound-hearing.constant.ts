/**
 * How far a sound carries from where it happens, in world units. Past `range`
 * you hear nothing; closer, it fades in faster than linearly, so a fight on
 * the next hill is faint and one beside you is loud.
 */
export const SOUND_HEARING = {
    range: 1100,
    falloff: 1.5,
} as const;
