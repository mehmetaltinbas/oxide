import { ItemCategory } from 'src/features/items/types/item-category.type';
import { ItemId } from 'src/features/items/types/item-id.type';

export interface ItemDef {
    id: ItemId;
    name: string;
    category: ItemCategory;
    stack: number;
    color: string;
    desc: string;
    /** Melee/harvest profile for tools and melee weapons. */
    melee?: { damage: number; gather: number; cooldown: number; reach: number };
    /** Ranged profile. */
    gun?: {
        damage: number;
        cooldown: number;
        speed: number;
        range: number;
        spread: number;
        ammo: ItemId;
        /**
         * Rounds the weapon holds before it needs [R]. A bow has no magazine: it
         * draws straight from the quiver, which is what nocking an arrow is.
         */
        magazine?: number;
        reloadSeconds?: number;
        /**
         * Fires a rocket instead of a round: it flies straight at `speed` and
         * explodes on whatever it meets, or at the end of its range.
         */
        rocket?: { damage: number; radius: number };
    };
    /** Food and drink restore these. */
    food?: { calories?: number; hydration?: number; health?: number };
    /** Worn protection. */
    wear?: { warmth?: number; radiation?: number; armor?: number };
    /** Thrown explosive profile. */
    boom?: { damage: number; radius: number; fuse: number };
    /** Seconds of standing still to apply this. Absent means instant. */
    useSeconds?: number;
}
