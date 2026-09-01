import { ItemCategory } from 'src/features/items/types/item-category.type';

/**
 * Which item categories may sit on the belt. Anything else is carried in the
 * main inventory and never auto-placed into a belt slot. See docs/systems/inventory.md.
 */
export const BELT_CATEGORIES: ItemCategory[] = [
    'tool',
    'weapon',
    'consumable',
    'deployable',
    'explosive',
    'clothing',
];
