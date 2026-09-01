import { ITEMS } from 'src/features/items/constants/items.constant';
import { ItemStack } from 'src/features/items/types/item-stack.interface';

export function stackLabel(stack: ItemStack): string {
    return stack.count > 1 ? `${ITEMS[stack.id].name} x${stack.count}` : ITEMS[stack.id].name;
}
