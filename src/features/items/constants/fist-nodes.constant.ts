import { ResourceNode } from 'src/features/world/types/resource-node.interface';

/**
 * What bare hands can work: a tree for wood and a plain stone for stone. Metal
 * and sulfur sit in harder rock and want a pick, and a barrel wants a tool.
 */
export const FIST_NODES: ResourceNode['kind'][] = ['tree', 'stone_node'];
