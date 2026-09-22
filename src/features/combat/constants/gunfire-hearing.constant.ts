/**
 * Someone else's gunfire: how far it carries, in world units, and how loud it
 * is at the shooter's own spot. Further than ordinary sounds, as a shot is.
 * How it fades is SOUND_HEARING's falloff.
 */
export const GUNFIRE_HEARING = {
    range: 1400,
    nearLoudness: 0.75,
} as const;
