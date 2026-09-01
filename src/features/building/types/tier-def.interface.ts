import { BuildTier } from 'src/features/building/types/build-tier.type';
import { ItemId } from 'src/features/items/types/item-id.type';

export interface TierDef {
    tier: BuildTier;
    name: string;
    hp: number;
    /** Cost to upgrade ONE piece to this tier. */
    cost: Partial<Record<ItemId, number>>;
    color: string;
    edge: string;
    /** Explosives needed to break one wall, for the raid-cost readout. */
    satchels: number;
}
