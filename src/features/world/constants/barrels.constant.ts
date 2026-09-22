/**
 * How barrels are laid along the roads. A road tile is 96 across; one in five
 * gets a barrel, and none closer than 220 to another thing, which leaves one
 * every few hundred units of road.
 */
export const BARRELS = {
    chancePerTile: 0.2,
    spacing: 220,
} as const;
