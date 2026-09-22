import { ItemId } from 'src/features/items/types/item-id.type';

export interface NodeDef {
    kind: 'tree' | 'stone_node' | 'metal_node' | 'sulfur_node' | 'cotton';
    name: string;
    hp: number;
    radius: number;
    /** Yield per successful gather tick, before tool multipliers. */
    yield: Partial<Record<ItemId, number>>;
    /** Tools that are efficient here. */
    prefers: 'chop' | 'mine' | 'pick';
    color: string;
}
