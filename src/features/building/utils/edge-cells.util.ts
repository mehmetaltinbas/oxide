import { EdgeSide } from 'src/features/building/types/edge-side.type';

/** The two cells an edge separates. */
export function edgeCells(
    gx: number,
    gy: number,
    side: EdgeSide,
): [[number, number], [number, number]] {
    return side === 'n'
        ? [
              [gx, gy - 1],
              [gx, gy],
          ]
        : [
              [gx - 1, gy],
              [gx, gy],
          ];
}
