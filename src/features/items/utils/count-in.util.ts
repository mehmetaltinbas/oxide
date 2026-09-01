import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

export function countIn(c: Container, id: ItemId): number {
    let n = 0;
    for (const s of c.slots) if (s && s.id === id) n += s.count;
    return n;
}
