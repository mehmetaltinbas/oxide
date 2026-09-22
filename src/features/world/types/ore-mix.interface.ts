import { ResourceNode } from 'src/features/world/types/resource-node.interface';

/** One ore's share of a region's nodes. */
export interface OreMix {
    kind: Extract<ResourceNode['kind'], 'stone_node' | 'metal_node' | 'sulfur_node'>;
    share: number;
}
