import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

/** Remove up to `count`; returns how many were actually taken. */
export function removeItem(c: Container, id: ItemId, count: number): number {
    let left = count;
    for (let i = 0; i < c.slots.length && left > 0; i++) {
        const s = c.slots[i];
        if (!s || s.id !== id) continue;
        const take = Math.min(s.count, left);
        s.count -= take;
        left -= take;
        if (s.count <= 0) c.slots[i] = null;
    }
    return count - left;
}
