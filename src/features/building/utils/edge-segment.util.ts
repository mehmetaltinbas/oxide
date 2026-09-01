import { CELL } from 'src/features/building/constants/cell.constant';
import { EdgeSide } from 'src/features/building/types/edge-side.type';

/** World-space endpoints of an edge piece. */
export function edgeSegment(
    gx: number,
    gy: number,
    side: EdgeSide,
): [number, number, number, number] {
    const x = gx * CELL;
    const y = gy * CELL;
    return side === 'n' ? [x, y, x + CELL, y] : [x, y, x, y + CELL];
}
