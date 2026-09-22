import { DeployableKind } from 'src/features/building/types/deployable-kind.type';
import { EdgeSide } from 'src/features/building/types/edge-side.type';

/** One queued construction step. */
export interface PendingPiece {
    kind: 'foundation' | 'wall' | 'doorway' | 'deploy';
    gx: number;
    gy: number;
    side?: EdgeSide;
    deploy?: DeployableKind;
}
