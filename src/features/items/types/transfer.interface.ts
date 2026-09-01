import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

/** A stack being carried from one container to another, with time on it. */
export interface Transfer {
    from: Container;
    to: Container;
    /** Slot it started in. Re-checked on arrival, because slots can move. */
    index: number;
    id: ItemId;
    count: number;
    elapsed: number;
    duration: number;
}
