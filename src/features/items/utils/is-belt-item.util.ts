import { BELT_CATEGORIES } from 'src/features/items/constants/belt-categories.constant';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { ItemId } from 'src/features/items/types/item-id.type';

export function isBeltItem(id: ItemId): boolean {
    return BELT_CATEGORIES.includes(ITEMS[id].category);
}
