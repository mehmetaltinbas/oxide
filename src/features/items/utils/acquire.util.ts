import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { addItem } from 'src/features/items/utils/add-item.util';
import { isBeltItem } from 'src/features/items/utils/is-belt-item.util';

/**
 * Where an item goes when it arrives on its own: a pickup, a craft, a harvest.
 *
 * Anything you can hold goes to the belt first, so a freshly crafted hatchet is
 * in your hand rather than buried in a bag. Everything else, and any overflow,
 * goes to the main inventory. See docs/systems/inventory.md.
 */
export function acquire(belt: Container, inventory: Container, id: ItemId, count: number): number {
    let left = count;
    if (isBeltItem(id)) left = addItem(belt, id, left);
    return addItem(inventory, id, left);
}
