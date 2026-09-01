import { ItemId } from 'src/features/items/types/item-id.type';

export interface MonumentDef {
    id: string;
    name: string;
    radius: number;
    /** Radiation per second at the centre, falling off to the edge. */
    rads: number;
    scientists: number;
    crates: number;
    /** Loot table weight by item. */
    loot: Partial<Record<ItemId, [number, number]>>;
    color: string;
}
