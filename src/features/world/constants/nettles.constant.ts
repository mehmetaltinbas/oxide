/**
 * How thick the nettle grows in the grass and forest: `perTile` per biome tile.
 * It was about 0.2 a tile on average (0.25 in the grass, 0.13 in the forest)
 * before this was a number; halved, so cloth is something you look for.
 */
export const NETTLES = {
    perTile: 0.1,
    spacing: 52,
} as const;
