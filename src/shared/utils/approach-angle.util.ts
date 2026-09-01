import { angleDelta } from 'src/shared/utils/angle-delta.util';
import { clamp } from 'src/shared/utils/clamp.util';

export function approachAngle(from: number, to: number, maxStep: number): number {
    const d = angleDelta(from, to);
    return from + clamp(d, -maxStep, maxStep);
}
