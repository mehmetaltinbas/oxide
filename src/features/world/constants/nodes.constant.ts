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
};
