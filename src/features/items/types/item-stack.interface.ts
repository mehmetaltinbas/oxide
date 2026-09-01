import { ItemId } from 'src/features/items/types/item-id.type';

export interface ItemStack {
    id: ItemId;
    count: number;
    /**
     * Rounds currently in this weapon. Lives on the stack rather than the item
     * definition because two revolvers in a crate are not the same revolver:
     * one can be full and the other empty. Absent on anything that is not a gun,
     * and on a gun that has never been loaded (treated as empty).
     */
    loaded?: number;
}
