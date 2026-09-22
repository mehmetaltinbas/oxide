/**
 * How the trees are laid. The forest gets `forestPerTile` trees per biome tile:
 * 1.09, the density it came out at before this was a number, measured across
 * three islands, so the forest looks as it always has. The grassland then gets
 * `grassShare` of the forest's measured density, and the snow its own thinner
 * stand.
 */
export const TREES = {
    spacing: 58,
    forestPerTile: 1.09,
    grassShare: 0.5,
    /** Chance per sample in the snow, as the snow stand has always been. */
    snowChance: 0.35,
} as const;
