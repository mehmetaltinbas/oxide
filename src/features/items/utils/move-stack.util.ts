import { ITEMS } from 'src/features/items/constants/items.constant';
import { Container } from 'src/features/items/types/container.interface';
import { addItem } from 'src/features/items/utils/add-item.util';

export function moveStack(
    from: Container,
    fromIndex: number,
    to: Container,
    toIndex?: number,
): void {
    const stack = from.slots[fromIndex];
    if (!stack) return;
    if (toIndex === undefined) {
        const left = addItem(to, stack.id, stack.count);
        if (left <= 0) from.slots[fromIndex] = null;
        else stack.count = left;
        return;
    }
    const target = to.slots[toIndex];
    if (!target) {
        to.slots[toIndex] = stack;
        from.slots[fromIndex] = null;
        return;
    }
    if (target.id === stack.id) {
        const max = ITEMS[stack.id].stack;
        const put = Math.min(max - target.count, stack.count);
        target.count += put;
        stack.count -= put;
        if (stack.count <= 0) from.slots[fromIndex] = null;
        return;
    }
    // Different items: swap.
    to.slots[toIndex] = stack;
    from.slots[fromIndex] = target;
}
