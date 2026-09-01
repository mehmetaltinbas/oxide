import { Recipe } from 'src/features/crafting/types/recipe.interface';
import { WorkbenchLevel } from 'src/features/crafting/types/workbench-level.type';
import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';
import { countAcross } from 'src/features/items/utils/count-across.util';

/** Everything the player can pay a recipe from. */
export function canCraft(recipe: Recipe, containers: Container[], bench: WorkbenchLevel): boolean {
    if (recipe.bench > bench) return false;
    for (const [id, need] of Object.entries(recipe.cost) as [ItemId, number][]) {
        if (countAcross(containers, id) < need) return false;
    }
    return true;
}
