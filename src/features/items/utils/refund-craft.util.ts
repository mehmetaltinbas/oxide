import { Recipe } from 'src/features/crafting/types/recipe.interface';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { acquire } from 'src/features/items/utils/acquire.util';

/** Give back what a cancelled run had already paid. Overflow hits the floor. */
export function refundCraft(
    recipe: Recipe,
    runs: number,
    hotbar: Container,
    pack: Container,
): { id: ItemId; count: number }[] {
    const spilled: { id: ItemId; count: number }[] = [];
    for (const [id, need] of Object.entries(recipe.cost) as [ItemId, number][]) {
        const left = acquire(hotbar, pack, id, need * runs);
        if (left > 0) spilled.push({ id, count: left });
    }
    return spilled;
}
