import { BuildKind } from 'src/features/building/types/build-kind.type';
import { EdgeSide } from 'src/features/building/types/edge-side.type';

export interface BuildPreview {
    kind: BuildKind;
    gx: number;
    gy: number;
    side?: EdgeSide;
    valid: boolean;
    reason: string;
}
