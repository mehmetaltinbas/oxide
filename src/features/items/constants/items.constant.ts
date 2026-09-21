import { ItemDef } from 'src/features/items/types/item-def.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

/**
 * Every item has a per-slot stack limit. Bulk resources stack deep, food and
 * medicine barely at all, and anything you hold is one to a slot.
 * See docs/systems/inventory.md.
 */
const R = (id: ItemId, name: string, color: string, desc: string, stack: number): ItemDef => ({
    id,
    name,
    category: 'resource',
    stack,
    color,
    desc,
});

/** Same shape, but filed under ammunition so it lists with the guns. */
const A = (id: ItemId, name: string, color: string, desc: string, stack: number): ItemDef => ({
    id,
    name,
    category: 'ammo',
    stack,
    color,
    desc,
});

export const ITEMS: Record<ItemId, ItemDef> = {
    // ---- resources
    wood: R('wood', 'Wood', '#b36e25', 'Chopped from trees. The bottom of every recipe.', 1000),
    stone: R('stone', 'Stones', '#b0b9c1', 'Broken from rock nodes.', 1000),
    metal_ore: R('metal_ore', 'Metal Ore', '#b69269', 'Smelt it in a furnace for fragments.', 1000),
    metal: R(
        'metal',
        'Metal Fragments',
        '#e1e9f0',
        'Smelted ore. The backbone of good gear.',
        1000,
    ),
    sulfur_ore: R('sulfur_ore', 'Sulfur Ore', '#e7dd64', 'Smelt it for sulfur.', 1000),
    sulfur: R(
        'sulfur',
        'Sulfur',
        '#fff382',
        'Half of gunpowder, and therefore half of every raid.',
        1000,
    ),
    cloth: R('cloth', 'Cloth', '#e1d1b3', 'Cut from hemp. Bandages, clothing, bags.', 1000),
    leather: R('leather', 'Leather', '#c87f46', 'Skinned from animals.', 500),
    bone: R('bone', 'Bone Fragments', '#f3efe2', 'From carcasses. Crude tools and arrows.', 500),
    scrap: R('scrap', 'Scrap', '#b1a080', 'Salvaged at monuments. Buys the good blueprints.', 1000),
    charcoal: R('charcoal', 'Charcoal', '#545454', 'Furnace by-product. Half of gunpowder.', 1000),
    gunpowder: R(
        'gunpowder',
        'Gunpowder',
        '#817c62',
        'Sulfur and charcoal. Ammunition and explosives.',
        1000,
    ),
    animal_fat: R(
        'animal_fat',
        'Animal Fat',
        '#efe0b4',
        'Cut off a carcass. Render it into fuel.',
        500,
    ),
    lowgrade: R(
        'lowgrade',
        'Low Grade Fuel',
        '#eac350',
        'Rendered from animal fat. Burns in lamps and satchels.',
        500,
    ),
    meat_raw: R(
        'meat_raw',
        'Raw Meat',
        '#d16565',
        'Eat it raw at your own risk. Cook it on a fire.',
        20,
    ),
    meat_cooked: R('meat_cooked', 'Cooked Meat', '#b36425', 'Proper food.', 20),
    water: R('water', 'Water', '#66c0f7', 'Collected at rivers and lakes.', 250),

    // ---- tools
    rock: {
        id: 'rock',
        name: 'Rock',
        category: 'tool',
        stack: 1,
        color: '#9e9e9e',
        desc: 'What you start with. Slow at everything.',
        melee: { damage: 12, gather: 1, cooldown: 0.62, reach: 46 },
    },
    hatchet: {
        id: 'hatchet',
        name: 'Hatchet',
        category: 'tool',
        stack: 1,
        color: '#d5dae0',
        desc: 'Fast on trees, poor on rock.',
        melee: { damage: 22, gather: 3, cooldown: 0.5, reach: 50 },
    },
    pickaxe: {
        id: 'pickaxe',
        name: 'Pickaxe',
        category: 'tool',
        stack: 1,
        color: '#d7ebfe',
        desc: 'Fast on ore, poor on wood.',
        melee: { damage: 20, gather: 3, cooldown: 0.52, reach: 50 },
    },
    hammer: {
        id: 'hammer',
        name: 'Hammer',
        category: 'tool',
        stack: 1,
        color: '#f0d9a3',
        desc: 'Upgrades and repairs building pieces you own.',
        melee: { damage: 8, gather: 1, cooldown: 0.5, reach: 56 },
    },
    building_plan: {
        id: 'building_plan',
        name: 'Building Plan',
        category: 'tool',
        stack: 1,
        color: '#ffe095',
        desc: 'Places twig foundations, walls and doorways.',
    },

    // ---- weapons
    spear: {
        id: 'spear',
        name: 'Wooden Spear',
        category: 'weapon',
        stack: 1,
        color: '#c89646',
        desc: 'Cheap reach. Better than fists.',
        melee: { damage: 38, gather: 1, cooldown: 0.85, reach: 74 },
    },
    bow: {
        id: 'bow',
        name: 'Hunting Bow',
        category: 'weapon',
        stack: 1,
        color: '#d3e8bc',
        desc: 'Quiet, cheap to feed, punishing to aim.',
        gun: { damage: 48, cooldown: 0.85, speed: 760, range: 620, spread: 0.03, ammo: 'arrow' },
    },
    revolver: {
        id: 'revolver',
        name: 'Revolver',
        category: 'weapon',
        stack: 1,
        color: '#fbf0c4',
        desc: 'First real gun most people build.',
        gun: {
            damage: 42,
            cooldown: 0.34,
            speed: 900,
            range: 640,
            spread: 0.05,
            ammo: 'pistol_ammo',
            magazine: 6,
            reloadSeconds: 3,
        },
    },
    rifle: {
        id: 'rifle',
        name: 'Semi-Automatic Rifle',
        category: 'weapon',
        stack: 1,
        color: '#d7edfe',
        desc: 'The gun that decides most fights.',
        gun: {
            damage: 62,
            cooldown: 0.19,
            speed: 1250,
            range: 950,
            spread: 0.035,
            ammo: 'rifle_ammo',
            magazine: 16,
            reloadSeconds: 4.0,
        },
    },

    /**
     * The AK. Scaled off the semi-auto by Rust's own ratios between the two
     * (Facepunch wiki, rifle.ak against rifle.semiauto): 50 damage to 40, 450
     * rounds a minute to about 343, 30 in the magazine to 16, a 4.4s reload to
     * 4.0. Rust's AK kicks hard, which a top-down game has no camera to show,
     * so the recoil is spent as spread instead: it wins close and loses long.
     */
    ak47: {
        id: 'ak47',
        name: 'Assault Rifle',
        category: 'weapon',
        stack: 1,
        color: '#c98a52',
        desc: 'The AK. Fast, loud and hard to hold on target past close range.',
        gun: {
            damage: 78,
            cooldown: 0.145,
            speed: 1250,
            range: 950,
            spread: 0.06,
            ammo: 'rifle_ammo',
            magazine: 30,
            reloadSeconds: 4.4,
        },
    },

    // ---- ammo
    arrow: A('arrow', 'Arrows', '#d3e8bc', 'For the bow.', 64),
    pistol_ammo: A('pistol_ammo', 'Pistol Ammo', '#fbf0c4', 'For the revolver.', 128),
    rifle_ammo: A('rifle_ammo', 'Rifle Ammo', '#d7edfe', 'For the rifles.', 128),

    // ---- consumables
    bandage: {
        id: 'bandage',
        name: 'Bandage',
        category: 'consumable',
        stack: 12,
        color: '#ebebeb',
        desc: 'Stops bleeding. Heals a little, slowly. Takes five seconds.',
        food: { health: 5 },
        useSeconds: 5,
    },
    medkit: {
        id: 'medkit',
        name: 'Medical Syringe',
        category: 'consumable',
        stack: 10,
        color: '#f95177',
        desc: 'Heals a lot over a few seconds. Quick to apply.',
        food: { health: 60 },
        useSeconds: 1.5,
    },

    // ---- clothing
    clothing: {
        id: 'clothing',
        name: 'Hide Clothing',
        category: 'clothing',
        stack: 1,
        color: '#c87f46',
        desc: 'Keeps the cold off and turns a little damage.',
        wear: { warmth: 14, armor: 0.12 },
    },
    hazmat: {
        id: 'hazmat',
        name: 'Hazmat Suit',
        category: 'clothing',
        stack: 1,
        color: '#64e6b7',
        desc: 'The only way to loot a hot monument and walk out.',
        wear: { warmth: 8, armor: 0.2, radiation: 0.9 },
    },

    // ---- deployables
    campfire: {
        id: 'campfire',
        name: 'Campfire',
        category: 'deployable',
        stack: 5,
        color: '#ffa358',
        desc: 'Warmth, light, and it cooks meat.',
    },
    furnace: {
        id: 'furnace',
        name: 'Furnace',
        category: 'deployable',
        stack: 5,
        color: '#c67545',
        desc: 'Burns wood to smelt ore into fragments and sulfur.',
    },
    tool_cupboard: {
        id: 'tool_cupboard',
        name: 'Tool Cupboard',
        category: 'deployable',
        stack: 2,
        color: '#fcc515',
        desc: 'Claims the ground around it. Nobody else builds inside your radius.',
    },
    wooden_box: {
        id: 'wooden_box',
        name: 'Wooden Box',
        category: 'deployable',
        stack: 10,
        color: '#b36e25',
        desc: 'Storage. What raiders are actually coming for.',
    },
    sleeping_bag: {
        id: 'sleeping_bag',
        name: 'Sleeping Bag',
        category: 'deployable',
        stack: 5,
        color: '#bc6161',
        desc: 'Respawn point. Without one, death puts you back on the beach.',
    },
    workbench1: {
        id: 'workbench1',
        name: 'Workbench Level 1',
        category: 'deployable',
        stack: 2,
        color: '#a88f5c',
        desc: 'Unlocks the first proper tier of recipes while you stand near it.',
    },
    workbench2: {
        id: 'workbench2',
        name: 'Workbench Level 2',
        category: 'deployable',
        stack: 2,
        color: '#b29e76',
        desc: 'Guns and better armour.',
    },
    workbench3: {
        id: 'workbench3',
        name: 'Workbench Level 3',
        category: 'deployable',
        stack: 2,
        color: '#c9b692',
        desc: 'Rifles and C4.',
    },
    lock: {
        id: 'lock',
        name: 'Code Lock',
        category: 'deployable',
        stack: 10,
        color: '#f0d9a3',
        desc: 'Locks a door so raiders have to break it instead of walking in.',
    },

    // ---- explosives
    satchel: {
        id: 'satchel',
        name: 'Satchel Charge',
        category: 'explosive',
        stack: 10,
        color: '#f57d25',
        desc: 'Cheap raiding. Unreliable, and it takes several.',
        boom: { damage: 475, radius: 90, fuse: 3.2 },
    },
    c4: {
        id: 'c4',
        name: 'Timed Explosive',
        category: 'explosive',
        stack: 5,
        color: '#f95151',
        desc: 'Opens anything. Expensive enough that you plan around it.',
        boom: { damage: 550, radius: 120, fuse: 4.5 },
    },
};
