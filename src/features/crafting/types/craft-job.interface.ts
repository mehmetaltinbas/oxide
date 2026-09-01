export interface CraftJob {
    recipeIndex: number;
    remaining: number;
    /** How many are still to come out of this run. */
    left: number;
}
