import { BuildKind } from 'src/features/building/types/build-kind.type';
import { BuildTier } from 'src/features/building/types/build-tier.type';
import { EdgeSide } from 'src/features/building/types/edge-side.type';

export interface Structure {
    id: number;
    kind: BuildKind;
    tier: BuildTier;
    gx: number;
    gy: number;
    /** Only set for edge pieces (wall, doorway, door). */
    side?: EdgeSide;
    hp: number;
    maxHp: number;
    /** Owner id: 0 is the player, otherwise a clan index + 1. */
    owner: number;
    /** Doors only. */
    open?: boolean;
    locked?: boolean;
    /** Which way is the soft side, as a unit vector, for melee rules. */
    softX: number;
    softY: number;
    flash: number;
}
