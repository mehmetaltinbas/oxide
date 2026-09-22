/**
 * How barrels are laid along the roads. A road tile is 96 across; one edge tile
 * in three gets a cluster of one to three barrels, sitting `verge` in from the road's edge, and none
 * closer than 220 to another barrel, which leaves one every few hundred units.
 */
export const BARRELS = {
    chancePerTile: 0.33,
    spacing: 220,
    verge: 16,
    /** How close anything else may stand to a barrel. */
    clearance: 40,
    /** A cluster is one to this many barrels, as likely each. */
    maxCluster: 3,
    /** Centre to centre within a cluster: two barrels' radii and a hair. */
    gap: 29,
} as const;
