import WORLD_NUMBERS from 'shared/world.json';

/**
 * Building snaps to this grid: foundations on cells, walls on cell edges.
 *
 * Shared with the server, which validates placements against the same grid.
 */
export const CELL = WORLD_NUMBERS.cell;
