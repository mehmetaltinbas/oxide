import { Biome } from 'src/features/world/types/biome.type';
import { OreMix } from 'src/features/world/types/ore-mix.interface';

/**
 * Where the ore is. The island is three regions, and each has a fixed amount
 * rather than a density, scaled by the map's area:
 *
 * - The snow and the desert each hold `perRegion` nodes.
 * - The green (grass and forest together) holds half that.
 *
 * `perRegion` is 1,000 on a map of `referenceArea`, the 20736 square island.
 * A server with a map twice the area gets 2,000, so the balance between the
 * regions holds whatever size is running.
 *
 * Each region leans to one ore at 45 / 30 / 25, stone second everywhere as the
 * common filler: stone near where you start, metal up in the cold, sulfur,
 * and so gunpowder, down in the heat.
 */
export const ORES: {
    perRegion: number;
    referenceArea: number;
    spacing: number;
    regions: { biomes: Biome[]; share: number; mix: [OreMix, OreMix, OreMix] }[];
} = {
    perRegion: 1000,
    referenceArea: 20736 * 20736,
    /** How far an ore node keeps from anything else, as before. */
    spacing: 76,
    regions: [
        {
            biomes: ['grass', 'forest'],
            share: 0.5,
            mix: [
                { kind: 'stone_node', share: 0.45 },
                { kind: 'metal_node', share: 0.3 },
                { kind: 'sulfur_node', share: 0.25 },
            ],
        },
        {
            biomes: ['snow'],
            share: 1,
            mix: [
                { kind: 'metal_node', share: 0.45 },
                { kind: 'stone_node', share: 0.3 },
                { kind: 'sulfur_node', share: 0.25 },
            ],
        },
        {
            biomes: ['desert'],
            share: 1,
            mix: [
                { kind: 'sulfur_node', share: 0.45 },
                { kind: 'stone_node', share: 0.3 },
                { kind: 'metal_node', share: 0.25 },
            ],
        },
    ],
};
