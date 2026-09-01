import { Deployable } from 'src/features/building/types/deployable.interface';
import { Structure } from 'src/features/building/types/structure.interface';
import { ClanState } from 'src/features/clans/types/clan-state.interface';
import { PlayerState } from 'src/features/survival/types/player-state.interface';

export interface SaveShape {
    version: 1;
    seed: number;
    clock: number;
    savedAt: number;
    player: PlayerState;
    structures: Structure[];
    deployables: Deployable[];
    clans: ClanState[];
    /** Only nodes that differ from a fresh world, keyed by id. */
    nodeDeltas: [number, number, number][];
    lootedCrates: number[];
    nextBuildId: number;
}
