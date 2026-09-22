import { Faction } from 'src/features/combat/types/faction.type';
import { ItemId } from 'src/features/items/types/item-id.type';

export interface ThrownExplosive {
    id: number;
    item: ItemId;
    x: number;
    y: number;
    vx: number;
    vy: number;
    fuse: number;
    damage: number;
    radius: number;
    /** Stuck to a structure, the way a satchel sticks to a wall. */
    stuckTo: number | null;
    owner: Faction;
    /** A rocket: flies straight, no drag, and goes off on impact. */
    rocket?: boolean;
}
