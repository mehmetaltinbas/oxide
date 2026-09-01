import { EdgeSide } from 'src/features/building/types/edge-side.type';
import { edgeSegment } from 'src/features/building/utils/edge-segment.util';

export function edgeCenter(gx: number, gy: number, side: EdgeSide): { x: number; y: number } {
    const [x0, y0, x1, y1] = edgeSegment(gx, gy, side);
    return { x: (x0 + x1) / 2, y: (y0 + y1) / 2 };
}
