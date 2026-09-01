import { ItemId } from 'src/features/items/types/item-id.type';
import { NpcKind } from 'src/features/npcs/types/npc-kind.type';

export interface NpcDef {
    kind: NpcKind;
    name: string;
    hp: number;
    speed: number;
    radius: number;
    damage: number;
    attackRange: number;
    attackCooldown: number;
    hostile: boolean;
    /**
     * Wildlife rather than people.
     *
     * Wildlife is a resource: it regrows a day after it is killed, like a tree
     * or an ore node. People are not: clans replace their own on their own
     * timer. Set this on any future animal and the island keeps its population
     * with no other change.
     */
    wild?: boolean;
    color: string;
    dark: string;
    gun?: { damage: number; range: number; cooldown: number; speed: number; spread: number };
    loot?: Partial<Record<ItemId, [number, number]>>;
}
