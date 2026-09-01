import { ITEMS } from 'src/features/items/constants/items.constant';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

export function hasSpace(c: Container, id: ItemId, count: number): boolean {
    const max = ITEMS[id].stack;
    let room = 0;
    for (const s of c.slots) {
        if (!s) room += max;
        else if (s.id === id) room += max - s.count;
        if (room >= count) return true;
    }
    return room >= count;
}
