import { angleDelta } from 'src/shared/utils/angle-delta.util';

/** True when the angle from `origin` to `target` falls inside a cone. */
export function inCone(
    ox: number,
    oy: number,
    facing: number,
    halfAngle: number,
    tx: number,
    ty: number,
): boolean {
    const a = Math.atan2(ty - oy, tx - ox);
    return Math.abs(angleDelta(facing, a)) <= halfAngle;
}
