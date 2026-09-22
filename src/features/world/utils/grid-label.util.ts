import { gridColumn } from 'src/features/world/utils/grid-column.util';
import {
    MAP_GRID_COLS,
    MAP_GRID_ROWS,
    MAP_GRID_SIZE,
} from 'src/features/world/constants/map-grid.constant';

/** The square a world point is in, as a name: "F7". */
export function gridLabel(x: number, y: number): string {
    const col = Math.min(MAP_GRID_COLS - 1, Math.max(0, Math.floor(x / MAP_GRID_SIZE)));
    const row = Math.min(MAP_GRID_ROWS - 1, Math.max(0, Math.floor(y / MAP_GRID_SIZE)));
    return `${gridColumn(col)}${row}`;
}
