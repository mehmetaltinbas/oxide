import { RECIPES } from 'src/features/crafting/constants/recipes.constant';
import { Recipe } from 'src/features/crafting/types/recipe.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

export function recipeFor(out: ItemId): Recipe | undefined {
    return RECIPES.find((r) => r.out === out);
}
