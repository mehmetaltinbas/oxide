import { PendingPiece } from 'src/features/building/types/pending-piece.interface';
import { ClanSkill } from 'src/features/clans/types/clan-skill.type';

export interface ClanState {
    /** Owner id used on their structures: always index + 1. */
    index: number;
    name: string;
    tag: string;
    color: string;
    skill: ClanSkill;
    x: number;
    y: number;
    tech: number;
    /** Seconds until this clan can mount another raid. */
    raidCooldown: number;
    wiped: boolean;
    tcId: number | null;
    /**
     * A clan that has just moved in raises its compound piece by piece in real
     * time instead of appearing whole.
     */
    founding: boolean;
    buildQueue: PendingPiece[];
    buildTimer: number;
    /** Counts down while the clan is short-handed and has a bed to spawn on. */
    respawnTimer: number;
}
