import { WORLD_H } from 'src/features/world/constants/world-h.constant';
import { WORLD_W } from 'src/features/world/constants/world-w.constant';

/**
 * The map grid, Rust's way: columns lettered from A, rows numbered from 0, so
 * a place is called by its square, "F7".
 *
 * `CELLS_ACROSS` is how many squares fit along the map's longest side; the
 * short side gets however many fit at the same size, so a square is always
 * square whatever shape the map is. Fifteen across puts a square at about
 * 1,380 units on today's island.
 */
export const CELLS_ACROSS = 15;

/** The size of one grid square in world units, the same on both axes. */
export const MAP_GRID_SIZE = Math.max(WORLD_W, WORLD_H) / CELLS_ACROSS;

/** How many squares the map is, across and down. */
export const MAP_GRID_COLS = Math.ceil(WORLD_W / MAP_GRID_SIZE);
export const MAP_GRID_ROWS = Math.ceil(WORLD_H / MAP_GRID_SIZE);
