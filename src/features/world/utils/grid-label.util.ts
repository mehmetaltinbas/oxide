import { gridColumn } from 'src/features/world/utils/grid-column.util';
import { MAP_GRID_CELLS } from 'src/features/world/constants/map-grid.constant';
import { WORLD_H } from 'src/features/world/constants/world-h.constant';
import { WORLD_W } from 'src/features/world/constants/world-w.constant';

/** The square a world point is in, as a name: "F7". */
export function gridLabel(x: number, y: number): string {
    const col = Math.min(
        MAP_GRID_CELLS - 1,
        Math.max(0, Math.floor((x / WORLD_W) * MAP_GRID_CELLS)),
    );
    const row = Math.min(
        MAP_GRID_CELLS - 1,
        Math.max(0, Math.floor((y / WORLD_H) * MAP_GRID_CELLS)),
    );
    return `${gridColumn(col)}${row}`;
}
