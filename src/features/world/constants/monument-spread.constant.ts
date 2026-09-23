import { WORLD_H } from 'src/features/world/constants/world-h.constant';
import { WORLD_W } from 'src/features/world/constants/world-w.constant';

/**
 * How far apart the big places are kept: the Airfield, the Power Plant and the
 * Military Base are never neighbours, whatever size the island is.
 *
 * A third of the map's short side, which on a 20736 island is about 6,900: far
 * enough that each is its own trip. Measured off the map rather than written
 * out, so a bigger island spreads them further rather than leaving them
 * huddled in one corner of it.
 *
 * `relax` is how much of that is given up on each failed round of attempts, so
 * an island with little room still gets all three rather than losing one.
 */
export const MONUMENT_SPREAD = {
    apart: Math.min(WORLD_W, WORLD_H) / 3,
    relax: 0.85,
    rounds: 12,
} as const;
