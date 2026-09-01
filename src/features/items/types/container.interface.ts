import { ItemStack } from 'src/features/items/types/item-stack.interface';

/** A fixed-size grid of slots, like every survival game's backpack. */
export interface Container {
    slots: (ItemStack | null)[];
}
