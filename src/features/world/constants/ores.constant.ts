import { Biome } from 'src/features/world/types/biome.type';
import { OreMix } from 'src/features/world/types/ore-mix.interface';

/**
 * Where the ore is. The island is three regions, and each has a fixed amount
 * rather than a density, scaled by the map's area:
 *
 * - The snow and the desert hold 750 nodes each. They were 1,000 until
 *   2026-09-23; a quarter came off both, because a trip out to either paid for
 *   itself several times over.
 * - The green (grass and forest together) holds 500.
 *
 * The counts are for a map of `referenceArea`, the 20736 square island, and
 * scale with the area: a map twice the size gets twice the ore, so the balance
 * between the regions holds whatever size is running.
 *
 * Each region leans to one ore at 45 / 30 / 25, stone second everywhere as the
 * common filler: stone near where you start, metal up in the cold, sulfur,
 * and so gunpowder, down in the heat.
 */
export const ORES: {
    referenceArea: number;
    spacing: number;
    regions: { biomes: Biome[]; count: number; mix: [OreMix, OreMix, OreMix] }[];
} = {
    referenceArea: 20736 * 20736,
    /** How far an ore node keeps from anything else, as before. */
    spacing: 76,
    regions: [
        {
            biomes: ['grass', 'forest'],
            count: 500,
            mix: [
                { kind: 'stone_node', share: 0.45 },
                { kind: 'metal_node', share: 0.3 },
                { kind: 'sulfur_node', share: 0.25 },
            ],
        },
        {
            biomes: ['snow'],
            count: 750,
            mix: [
                { kind: 'metal_node', share: 0.45 },
                { kind: 'stone_node', share: 0.3 },
                { kind: 'sulfur_node', share: 0.25 },
            ],
        },
        {
            biomes: ['desert'],
            count: 750,
            mix: [
                { kind: 'sulfur_node', share: 0.45 },
                { kind: 'stone_node', share: 0.3 },
                { kind: 'metal_node', share: 0.25 },
            ],
        },
    ],
};
