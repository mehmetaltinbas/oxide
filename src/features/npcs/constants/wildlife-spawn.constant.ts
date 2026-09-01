/**
 * Where wildlife is allowed to appear at worldgen. The margin keeps animals off
 * the map rim, and the clearance stops the first thing you see on spawning
 * being a bear already inside your personal space.
 */
export const WILDLIFE_SPAWN = {
    /** Attempts per animal before giving up on a spot. */
    tries: 40,
    /** Keep this far in from the world edge. */
    margin: 120,
    /** Never place one closer than this to the player's spawn. */
    clearOfPlayer: 600,
    /** How far an animal will wander from where it was placed. */
    leash: 520,
} as const;
