import { ItemId } from 'src/features/items/types/item-id.type';

export interface NodeDef {
    kind: 'tree' | 'stone_node' | 'metal_node' | 'sulfur_node' | 'nettle' | 'barrel';
    name: string;
    hp: number;
    radius: number;
    /** Yield per successful gather tick, before tool multipliers. */
    yield: Partial<Record<ItemId, number>>;
    /** Tools that are efficient here. */
    prefers: 'chop' | 'mine' | 'pick' | 'break';
    /**
     * What falls out when it breaks, rolled once, instead of a yield on every
     * hit: a barrel is smashed, not mined. Each entry is a count range; an
     * entry with `chance` below 1 only turns up that often.
     */
    loot?: { id: ItemId; count: [number, number]; chance: number }[];
    color: string;
}
