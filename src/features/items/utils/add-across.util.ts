import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { addItem } from 'src/features/items/utils/add-item.util';

/** Add across several containers in order. Returns the remainder. */
export function addAcross(containers: Container[], id: ItemId, count: number): number {
    let left = count;
    for (const c of containers) {
        if (left <= 0) break;
        left = addItem(c, id, left);
    }
    return left;
}
