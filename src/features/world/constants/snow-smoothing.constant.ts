/**
 * Filling in the snowfield after the noise has laid it down: a grass or forest
 * tile with at least `neighbours` of its eight neighbours in snow becomes snow,
 * repeated `passes` times.
 */
export const SNOW_SMOOTHING = {
    passes: 4,
    neighbours: 4,
    /**
     * A green pocket up to this many tiles, whose edge is at least this share
     * snow (water not counted), is snowed over whole.
     */
    pocketTiles: 400,
    pocketSnowShare: 0.6,
} as const;
