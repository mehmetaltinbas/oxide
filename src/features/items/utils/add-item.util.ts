import { ITEMS } from 'src/features/items/constants/items.constant';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

/**
 * Add items, filling partial stacks first then empty slots.
 * Returns whatever would not fit.
 */
export function addItem(c: Container, id: ItemId, count: number): number {
    const max = ITEMS[id].stack;
    let left = count;
    for (const s of c.slots) {
        if (left <= 0) break;
        if (s && s.id === id && s.count < max) {
            const room = max - s.count;
            const put = Math.min(room, left);
            s.count += put;
            left -= put;
        }
    }
    for (let i = 0; i < c.slots.length && left > 0; i++) {
        if (c.slots[i]) continue;
        const put = Math.min(max, left);
        c.slots[i] = { id, count: put };
        left -= put;
    }
    return left;
}
