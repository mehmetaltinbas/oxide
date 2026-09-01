import { WorkbenchLevel } from 'src/features/crafting/types/workbench-level.type';
import { ItemId } from 'src/features/items/types/item-id.type';

export interface Recipe {
    out: ItemId;
    amount: number;
    cost: Partial<Record<ItemId, number>>;
    bench: WorkbenchLevel;
    seconds: number;
}
