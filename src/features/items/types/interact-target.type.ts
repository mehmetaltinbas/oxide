import { Deployable } from 'src/features/building/types/deployable.interface';
import { Structure } from 'src/features/building/types/structure.interface';
import { GroundItem } from 'src/features/items/types/ground-item.interface';
import { LootCrate } from 'src/features/world/types/loot-crate.interface';
import { ResourceNode } from 'src/features/world/types/resource-node.interface';

/** Whatever pressing E would act on. See `InteractionSystem.target`. */
export type InteractTarget =
    | { kind: 'item'; item: GroundItem }
    | { kind: 'plant'; node: ResourceNode }
    | { kind: 'door'; door: Structure }
    | { kind: 'deployable'; deployable: Deployable }
    | { kind: 'crate'; crate: LootCrate }
    | { kind: 'water' };
