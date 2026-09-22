import { NodeDef } from 'src/features/world/types/node-def.interface';

export const NODES: Record<NodeDef['kind'], NodeDef> = {
    tree: {
        kind: 'tree',
        name: 'Tree',
        hp: 160,
        radius: 17,
        yield: { wood: 12 },
        prefers: 'chop',
        color: '#3aa24a',
    },
    stone_node: {
        kind: 'stone_node',
        name: 'Stone Node',
        hp: 200,
        radius: 18,
        yield: { stone: 12 },
        prefers: 'mine',
        color: '#9aa6b2',
    },
    metal_node: {
        kind: 'metal_node',
        name: 'Metal Node',
        hp: 260,
        radius: 19,
        yield: { metal_ore: 6, stone: 4 },
        prefers: 'mine',
        color: '#b69269',
    },
    sulfur_node: {
        kind: 'sulfur_node',
        name: 'Sulfur Node',
        hp: 260,
        radius: 19,
        yield: { sulfur_ore: 6, stone: 4 },
        prefers: 'mine',
        color: '#e7dd64',
    },
    nettle: {
        kind: 'nettle',
        name: 'Nettle',
        hp: 40,
        radius: 11,
        yield: { cloth: 10 },
        prefers: 'pick',
        color: '#8cc94a',
    },
    /**
     * A roadside barrel, as Rust leaves along its roads: a few hits with
     * anything but your fists and it gives up its scrap, the one thing the
     * roads are for. Sometimes a little more.
     */
    barrel: {
        kind: 'barrel',
        name: 'Barrel',
        hp: 50,
        radius: 14,
        yield: {},
        prefers: 'break',
        color: '#3a6fa8',
        loot: [
            { id: 'scrap', count: [3, 6], chance: 1 },
            { id: 'metal', count: [10, 25], chance: 0.3 },
            { id: 'lowgrade', count: [5, 12], chance: 0.25 },
            { id: 'pistol_ammo', count: [4, 8], chance: 0.12 },
        ],
    },
};
