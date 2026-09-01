import { ItemStack } from 'src/features/items/types/item-stack.interface';

export interface GroundItem {
    id: number;
    item: ItemStack;
    x: number;
    y: number;
    vx: number;
    vy: number;
    life: number;
    bob: number;
}
