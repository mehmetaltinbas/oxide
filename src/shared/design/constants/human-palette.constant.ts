/**
 * The range of people on the island: skin tones and hair colours.
 *
 * A survivor is picked from these by their id, so the same person keeps the
 * same face all game and a camp of them is a mix rather than a row of clones.
 */
export const HUMAN_PALETTE = {
    skin: ['#e8b98f', '#d9a076', '#c08a5e', '#a86f45', '#8a5634', '#6b4128'],
    hair: ['#2a1e16', '#3f2f22', '#5a3a22', '#8a5a2e', '#c9a060', '#1c1c1c', '#7a6a5c'],
} as const;
