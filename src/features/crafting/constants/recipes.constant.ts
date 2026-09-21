import { Recipe } from 'src/features/crafting/types/recipe.interface';

export const RECIPES: Recipe[] = [
    { out: 'hatchet', amount: 1, cost: { wood: 60, stone: 40 }, bench: 0, seconds: 4 },
    { out: 'pickaxe', amount: 1, cost: { wood: 60, stone: 60 }, bench: 0, seconds: 4 },
    { out: 'hammer', amount: 1, cost: { wood: 40, stone: 20 }, bench: 0, seconds: 3 },
    // A hunting bow is the first weapon you make, as in Rust: no bench.
    { out: 'bow', amount: 1, cost: { wood: 200, cloth: 50 }, bench: 0, seconds: 6 },
    { out: 'building_plan', amount: 1, cost: { wood: 20 }, bench: 0, seconds: 2 },
    { out: 'spear', amount: 1, cost: { wood: 60, cloth: 10 }, bench: 0, seconds: 4 },
    { out: 'bandage', amount: 2, cost: { cloth: 8 }, bench: 0, seconds: 2 },
    { out: 'lowgrade', amount: 4, cost: { animal_fat: 3, cloth: 1 }, bench: 0, seconds: 2 },
    { out: 'campfire', amount: 1, cost: { wood: 100 }, bench: 0, seconds: 3 },
    { out: 'sleeping_bag', amount: 1, cost: { cloth: 30 }, bench: 0, seconds: 4 },
    { out: 'wooden_box', amount: 1, cost: { wood: 120 }, bench: 0, seconds: 4 },
    { out: 'tool_cupboard', amount: 1, cost: { wood: 250 }, bench: 0, seconds: 6 },
    {
        out: 'furnace',
        amount: 1,
        cost: { stone: 100, wood: 200, lowgrade: 25 },
        bench: 0,
        seconds: 6,
    },
    { out: 'clothing', amount: 1, cost: { leather: 30, cloth: 20 }, bench: 0, seconds: 5 },
    { out: 'workbench1', amount: 1, cost: { wood: 500, scrap: 50 }, bench: 0, seconds: 8 },

    { out: 'arrow', amount: 8, cost: { wood: 40, stone: 15 }, bench: 1, seconds: 3 },
    { out: 'lock', amount: 1, cost: { metal: 100 }, bench: 1, seconds: 4 },
    { out: 'gunpowder', amount: 10, cost: { sulfur: 20, charcoal: 30 }, bench: 1, seconds: 3 },
    { out: 'medkit', amount: 1, cost: { cloth: 15, lowgrade: 10 }, bench: 1, seconds: 4 },
    { out: 'workbench2', amount: 1, cost: { metal: 500, scrap: 250 }, bench: 1, seconds: 10 },

    { out: 'revolver', amount: 1, cost: { metal: 150, scrap: 75 }, bench: 2, seconds: 8 },
    { out: 'pistol_ammo', amount: 12, cost: { gunpowder: 10, metal: 10 }, bench: 2, seconds: 3 },
    {
        out: 'satchel',
        amount: 1,
        cost: { gunpowder: 80, metal: 30, cloth: 10, lowgrade: 15 },
        bench: 2,
        seconds: 8,
    },
    { out: 'hazmat', amount: 1, cost: { cloth: 60, scrap: 100, metal: 40 }, bench: 2, seconds: 10 },
    { out: 'workbench3', amount: 1, cost: { metal: 1000, scrap: 500 }, bench: 2, seconds: 14 },

    { out: 'rifle', amount: 1, cost: { metal: 450, scrap: 300 }, bench: 3, seconds: 12 },
    { out: 'ak47', amount: 1, cost: { metal: 600, scrap: 450, wood: 200 }, bench: 3, seconds: 15 },
    { out: 'rifle_ammo', amount: 12, cost: { gunpowder: 20, metal: 15 }, bench: 3, seconds: 3 },
    {
        out: 'c4',
        amount: 1,
        cost: { gunpowder: 200, metal: 100, cloth: 30, lowgrade: 60 },
        bench: 3,
        seconds: 14,
    },
];
