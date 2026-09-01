import { NodeDef } from 'src/features/world/types/node-def.interface';

export interface ResourceNode {
    id: number;
    kind: NodeDef['kind'];
    x: number;
    y: number;
    radius: number;
    hp: number;
    maxHp: number;
    /** Respawn countdown; nodes come back so a persistent world stays playable. */
    respawn: number;
    shake: number;
    seed: number;
}
