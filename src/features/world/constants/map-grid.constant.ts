/**
 * The map grid, Rust's way: columns lettered from A, rows numbered from 0, so
 * a place is called by its square, "F7".
 *
 * Fifteen a side, which makes a square about 1,380 units: seven by seven of the
 * blocks the open map is painted in. A finer grid than eleven, so a place can
 * be called out more precisely, A to O and 0 to 14.
 */
export const MAP_GRID_CELLS = 15;
