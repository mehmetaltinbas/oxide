import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { removeItem } from 'src/features/items/utils/remove-item.util';

export function removeAcross(containers: Container[], id: ItemId, count: number): number {
    let left = count;
    for (const c of containers) {
        if (left <= 0) break;
        left -= removeItem(c, id, left);
    }
    return count - left;
}
