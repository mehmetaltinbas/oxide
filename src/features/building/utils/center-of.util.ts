import { CELL } from 'src/features/building/constants/cell.constant';
import { Deployable } from 'src/features/building/types/deployable.interface';
import { Structure } from 'src/features/building/types/structure.interface';
import { edgeCenter } from 'src/features/building/utils/edge-center.util';

export function centerOf(t: Structure | Deployable): { x: number; y: number } {
    if ('kind' in t && 'gx' in t) {
        const s = t as Structure;
        if (s.kind === 'foundation')
            return { x: s.gx * CELL + CELL / 2, y: s.gy * CELL + CELL / 2 };
        return edgeCenter(s.gx, s.gy, s.side ?? 'n');
    }
    const d = t as Deployable;
    return { x: d.x, y: d.y };
}
