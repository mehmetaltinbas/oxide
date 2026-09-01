import { CELL } from 'src/features/building/constants/cell.constant';
import { EdgeSide } from 'src/features/building/types/edge-side.type';

/** Snap a world point to the closest cell edge, north or west. */
export function nearestEdge(x: number, y: number): { gx: number; gy: number; side: EdgeSide } {
    const gx = Math.floor(x / CELL);
    const gy = Math.floor(y / CELL);
    const fx = x / CELL - gx;
    const fy = y / CELL - gy;
    const candidates: { gx: number; gy: number; side: EdgeSide; d: number }[] = [
        { gx, gy, side: 'n', d: fy },
        { gx, gy: gy + 1, side: 'n', d: 1 - fy },
        { gx, gy, side: 'w', d: fx },
        { gx: gx + 1, gy, side: 'w', d: 1 - fx },
    ];
    candidates.sort((a, b) => a.d - b.d);
    return { gx: candidates[0].gx, gy: candidates[0].gy, side: candidates[0].side };
}
