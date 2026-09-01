import { ITEMS } from 'src/features/items/constants/items.constant';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

/**
 * How much more of one thing would fit: what the part-filled stacks will take,
 * plus a full stack for every empty slot.
 *
 * `hasSpace` answers a yes/no; a transfer needs the number, so it can move the
 * part of a stack that fits instead of refusing the whole thing.
 */
export function containerRoom(c: Container, id: ItemId): number {
    const max = ITEMS[id].stack;
    let room = 0;
    for (const s of c.slots) {
        if (!s) room += max;
        else if (s.id === id) room += max - s.count;
    }
    return room;
}
