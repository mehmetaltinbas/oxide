import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { countIn } from 'src/features/items/utils/count-in.util';

export function countAcross(containers: Container[], id: ItemId): number {
    let n = 0;
    for (const c of containers) n += countIn(c, id);
    return n;
}
