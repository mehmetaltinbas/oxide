import { dist2 } from 'src/shared/utils/dist2.util';

export function dist(ax: number, ay: number, bx: number, by: number): number {
    return Math.sqrt(dist2(ax, ay, bx, by));
}
