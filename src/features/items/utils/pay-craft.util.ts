import { Recipe } from 'src/features/crafting/types/recipe.interface';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { removeAcross } from 'src/features/items/utils/remove-across.util';

export function payCraft(recipe: Recipe, containers: Container[]): void {
    for (const [id, need] of Object.entries(recipe.cost) as [ItemId, number][]) {
        removeAcross(containers, id, need);
    }
}
